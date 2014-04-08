#!/usr/bin/ruby
require 'logger'
require 'set'
require 'taglib'
require 'thread'

# MusicFile
# - filepath
# - temp file (wav) path
# - extension
# - method to get wav filename
# - method to get final filepath with proper extension: @arg path, @arg extension
#
class MusicFile
    attr_accessor :extension, :filename, :filepath, :wavpath, :convpath

    def initialize(filename)
        @filename = File.basename(filename)
        @filepath = File.absolute_path(@filename, Dir.getwd)
        @wavpath = nil
        @convpath = nil
        @extension = File.extname(@filename)
        if @extension.empty? then
            raise StandardError, "could not extract extension from file: %s" % @filename
        else
            @extension = @extension.downcase
        end
    end

    def getExtension
        # We do not want to disturb our copy of the extension, but we can't have the leading dot
        x = @extension.dup
        x[0] = ''
        return x
    end

    def make_dest_filepath(dir, extension="wav")
        # to get either a temp file path or a dest file path
        # get just the filename without the extension
        newFilename = @filename.slice(0, @filename.rindex(@extension))
        # add the new extension and then get a path to the new filename in the given directory
        newFilename << "." + extension
        File.absolute_path(newFilename, dir)
    end
end

# Tool
# - path to exe: try to find automatically using env $PATH on start
# - verifies path to exe is correct, and perms to execute are good
# - encode/decode arguments from config file vs. built-in defaults
# - method to encode: @arg MusicFile instance
# - method to decode: @arg 
class Tool
    def initialize(fileType, progName, exePath, encArgs, decArgs)
        @logger = Logger.new(STDOUT)
        @logger.progname = "Tool:#{progName}"
        @fileType = fileType
        @progName = progName
        @exePath = exePath

        if exePath.nil? then
            begin
                @exePath = find_exe(@progName)
            rescue StandardError => e
                @logger.warn("Could not find path to program #{@progName}: #{e.to_s}")
            end
        end
        @decCommandTemplate = "#{@exePath} #{decArgs}"
        @encCommandTemplate = "#{@exePath} #{encArgs}"
    end

    def find_exe(progName)
        binDirL = ENV['PATH'].split(':')
        binDirL.each do |path|
            s = Set.new(Dir.entries(path))
            if s.include?(progName) then
                return File.absolute_path(progName, path)
            end
        end

        raise StandardError "could not find program in $PATH env var"
    end

    def to_s
        "<Tool progName=\033[31m#{@progName}\033[0m exePath=\033[31m#{@exePath}\033[0m encArgs='\033[32m#{@encArgs}\033[0m' decArgs='\033[32m#{@decArgs}\033[0m'>"
    end

    def decode(musicFile, tempDir)
        # tries to launch the exe with the decode parameters
        # decode operations always have a temp directory destination
        musicFile.wavpath = musicFile.make_dest_filepath(tempDir)
        @logger.debug("Decoding \033[31m#{musicFile.filename}\033[0m to temp path \033[32;1m#{musicFile.wavpath}\033[0m")
        run_command(musicFile.filepath, musicFile.wavpath, "decode")
    end

    def encode(musicFile, destDir)
        # tries to launch the exe with the encode parameters
        musicFile.convpath = musicFile.make_dest_filepath(destDir, @fileType)
        @logger.debug("Encoding \033[33m#{musicFile.wavpath}\033[0m to \033[32;1m#{musicFile.convpath}\033[0m")
        run_command(musicFile.wavpath, musicFile.convpath, "encode")
    end

    private
    def run_command(infile, outfile, opName)
        # build encode command
        if opName.eql?('decode')
            command = @decCommandTemplate.sub("$INFILE", "\"#{infile}\"").sub("$OUTFILE", "\"#{outfile}\"")
        elsif opName.eql?('encode')
            command = @encCommandTemplate.sub("$INFILE", "\"#{infile}\"").sub("$OUTFILE", "\"#{outfile}\"")
        else
            raise ArgumentError, "invalid opName given: must be decode or encode"
        end
        @logger.debug("Command=#{command}")

        begin
            retVal = system command
        rescue Exception => e
            @logger.error("Failed to #{opName} file due to exception while running tool: #{e.to_s}")
            return nil
        end

        # check that everything went fine
        return true unless retVal.eql?(false)
            @logger.error("Failed to #{opName} file: command returned false, retcode=#{$?}")
            return nil
    end
end

#
# ConversionJob
# - takes input file/path
# - takes output path
# - takes dest format
# - takes number of threads (optional; otherwise, counts num cpus and uses that)
# - matches tools to files found vs. dest format
# - kicks off threads to do actual conversion (simple method)
# - runs Tagger on origin and dest files to preserve tags
class ConversionJob
    attr_accessor :outputFormat, :outputFileL

    def initialize(fileL, outputPath, outputFormat, numThreads, toolH, tempDir)
        @logger = Logger.new(STDOUT)
        @logger.progname = "ConversionJob"
        # a list of MusicFile instances to convert
        @fileL = fileL
        # a list of MusicFile's that have been converted
        @outputFileL = Array.new(@fileL.size)
        # a path to a directory where converted file(s) should go
        @outputPath = outputPath
        # what audio format should the output files be
        @outputFormat = outputFormat
        # how many threads to launch concurrently
        @numThreads = numThreads
        # a hash of tools (fileType => Tool instance)
        @toolH = toolH
        # where all wav intermediary files must go
        @tempDir = tempDir
    end

    def start_conversion
        encTool = @toolH[@outputFormat]
        threadL = Array.new
        mutex = Mutex.new

        1.upto([@numThreads, @fileL.size].min) do
            # create a thread
            threadL << Thread.new {
                @logger.debug("Thread started")
                done = false

                while done.eql?(false)
                    # get a file, safely
                    musicFile = mutex.synchronize { @fileL.pop }
                    if musicFile.nil?
                        done = true
                        break
                    end

                    # find appropriate tool to decode the file
                    decTool = @toolH[musicFile.getExtension()]
                    decTool.decode(musicFile, @tempDir)

                    # encode to the desired output dir
                    encTool.encode(musicFile, @outputPath)

                    # remove wav intermediary
                    unless @outputFormat.eql?("wav")
                        begin
                            File.delete(musicFile.wavpath)
                        rescue Exception => e
                            @logger.error("Failed to delete intermediary wav file, #{musicFile.wavpath} due to exception: #{e.to_s}")
                        end
                    end

                    # add to completed file list, safely
                    mutex.synchronize { @outputFileL << musicFile }
                end
            }
        end

        # wait for threads to finish
        @logger.info("#{threadL.size} running")
        threadL.each do |thr|
            thr.join()
            @logger.info("thread #{thr.to_s} joined")
        end
        @logger.info("finished")
    end
end

#
# Manager
# - builds tool instances from default values vs. config file
# - examines options/CWD for music files
# - contains one or more ConversionJob instances that it creates from parsed options
class Manager
    def initialize()
        @logger = Logger.new(STDOUT)
        @logger.progname = "Manager"
        @fileL = Array.new

        # load tools
        #  fileExt => Tool instance
        @toolH = Hash.new
        load_default_tools()
        load_user_defined_tools()
        if @toolH.empty?
            @logger.error("Found no encoders/decoders, so we cannot do anything. Exiting")
            abort("")
        else
            @validExtensionsS = Set.new(@toolH.each_key)
            # We know what tools are available to us, so we can look for files with extensions that match
            @logger.debug("FileTypes that can be converted: #{@validExtensionsS.inspect()}")
            @logger.debug("Found #{@toolH.size} tools:")
            @toolH.each_pair do |fileType, tool|
                @logger.debug("  [#{fileType}] => #{tool.to_s}")
            end
        end
    end

    def check_file(filepath)
        begin
            musicFile = MusicFile.new(filepath)
            return @validExtensionsS.include?(musicFile.getExtension()) ? musicFile : nil
        rescue StandardError => e
            @logger.debug("File #{filepath} is not a valid file for conversion: #{e}")
            return nil
        end
    end

    def find_files_from_cwd
        # find music files in the CWD
        @logger.debug("Examining #{Dir.entries('.').size} files in #{File.absolute_path(".", ".")}")
        Dir.entries(".").each do |i|
            unless i.start_with?('.')
                musicFile = check_file(File.absolute_path(i, "."))
                @fileL << musicFile unless musicFile.nil?
            end
        end

        @logger.debug("Found #{@fileL.size} files")
        @fileL.each do |i|
            @logger.debug(" + [#{i.getExtension().upcase}] #{i.filename}")
        end
    end

    def find_files_from_cli
        # use names/paths from CLI args 
        # TODO
    end

    def convert
        # TODO: add parameters so jobs can be built
        # builds a job and begins the conversion
        #def initialize(fileL, outputPath, outputFormat, numThreads, toolH, tempDir)
        job = ConversionJob.new(@fileL, ".", "mp3", 2, @toolH, "/tmp")
        job.start_conversion()
        Tagger.new().tag(job)
    end

    private
    def load_default_tools
        @logger.debug("looking for default tools")
        @toolH['mp3'] = Tool.new("mp3", "lame", nil, "-V0 --vbr-new --quiet $INFILE $OUTFILE", "--quiet --decode $INFILE $OUTFILE")
        @toolH['flac'] = Tool.new("flac", "flac", nil, "-V -8 --silent -o $OUTFILE $INFILE", "--decode -s -o $OUTFILE $INFILE")
    end

    def load_user_defined_tools
        @logger.debug("not loading user-defined tools yet")
    end
end

# Tagger
# - method to tag a single job, which calls one or more other methods:
#   1) get CLI tags
#   2) if any tags missing, try to harvest from source file
#   3) if required tags missing, try to guess from directory name
#   if required tags still missing, fail
class Tagger
    def initialize(cli_album=nil, cli_artist=nil, cli_year=nil)
        @logger = Logger.new(STDOUT)
        @logger.progname = "Tagger"
        # these may be nil, but they override all others
        @cliAlbum = cli_album
        @cliArtist = cli_artist
        @cliYear = cli_year
    end

    def tag(job)
        # the provided ConversionJob has a list of all finished MusicFile instances, 
        # which have paths to the destination files. Iterate through them and try to tag

        # avoid trying to tag wav files
        if job.outputFormat.eql?("wav")
            return
        end

        job.outputFileL.each do |musicFile|
            unless musicFile.nil?
                # get a ref to the original file which is a source of tags, perhaps
                TagLib::FileRef.open(musicFile.filepath) do |srcFileRef|
                    unless srcFileRef.null?
                        TagLib::FileRef.open(musicFile.convpath) do |dstFileRef|
                            dstFileRef.tag.title = srcFileRef.tag.title
                            @logger.debug(" => title:\033[33m#{srcFileRef.tag.title}\033[0m")
                            dstFileRef.tag.artist = srcFileRef.tag.artist
                            @logger.debug(" => artist:\033[33m#{srcFileRef.tag.artist}\033[0m")
                            dstFileRef.tag.album = srcFileRef.tag.album
                            @logger.debug(" => album:\033[33m#{srcFileRef.tag.album}\033[0m")
                            dstFileRef.tag.year = srcFileRef.tag.year
                            @logger.debug(" => year:\033[33m#{srcFileRef.tag.year}\033[0m")
                            dstFileRef.tag.track = srcFileRef.tag.track
                            @logger.debug(" => track:\033[33m#{srcFileRef.tag.track}\033[0m")
                            @logger.debug("Tagged #{musicFile.convpath} successfully")
                            dstFileRef.save()
                        end
                    else
                        @logger.info("Source file has no tag; must source tag from directory/filenames")
                    end
                end
            end
        end
    end
end

mgr = Manager.new
mgr.find_files_from_cwd()
mgr.convert()
