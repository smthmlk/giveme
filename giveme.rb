#!/usr/bin/ruby
require 'logger'
require 'optparse'
require 'set'
require 'taglib'
require 'thread'
require 'yaml'

# Default log level used by all classes, overridden by option
$logLevel = Logger::WARN

# Represents a file that can be converted from one audio format to another
# Also holds references (paths) to temporary wav files (+wavpath+), and the
# converted file (+convpath+), including any specified destination directory
private
class MusicFile
  attr_accessor :extension, :filename, :filepath, :wavpath, :convpath

  # +filename+ - the path to a file
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
    extDupe = @extension.dup
    extDupe[0] = ''
    return extDupe
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

# Represents a decoder/encoder for a specific audio format
private
class Tool
  def initialize(fileType, progName, exePath, toolArgs)
    @logger = Logger.new(STDOUT)
    @logger.progname = "Tool->#{progName}"
    @logger.level = $logLevel
    @fileType = fileType
    @progName = progName
    @exePath = exePath
    @toolArgs = toolArgs

    if exePath.nil?
      begin
        @exePath = find_exe(@progName)
      rescue StandardError => e
        @logger.warn("Could not find path to Tool program #{@progName}: #{e.to_s}")
      end
    end
    @commandTemplate = "#{@exePath} #{toolArgs}"
  end

  def to_s
    "<Tool progName=\033[31m#{@progName}\033[0m exePath=\033[31m#{@exePath}\033[0m args='\033[32m#{@toolArgs}\033[0m'>"
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

  def run_command(infile, outfile, opName)
    # build encode command
    command = @commandTemplate.sub("$INFILE", "\"#{infile}\"").sub("$OUTFILE", "\"#{outfile}\"")
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

class Encoder < Tool
  def encode(musicFile, destDir)
    # tries to launch the exe with the encode parameters
    musicFile.convpath = musicFile.make_dest_filepath(destDir, @fileType)
    @logger.debug("Encoding \033[33m#{musicFile.wavpath}\033[0m to \033[32;1m#{musicFile.convpath}\033[0m")
    run_command(musicFile.wavpath, musicFile.convpath, "encode")
  end
end

class Decoder < Tool
  def decode(musicFile, tempDir)
    # tries to launch the exe with the decode parameters
    # decode operations always have a temp directory destination, and we need to ensure no old wav file there exists with the same name
    musicFile.wavpath = musicFile.make_dest_filepath(tempDir)
    if File.exists?(musicFile.wavpath)
      File.delete(musicFile.wavpath)
    end

    @logger.debug("Decoding \033[31m#{musicFile.filename}\033[0m to temp path \033[32;1m#{musicFile.wavpath}\033[0m")
    run_command(musicFile.filepath, musicFile.wavpath, "decode")
  end
end

# Models a batch of files to be converted and the properties of the requested conversion, and contains
# the logic to perform the conversion.
private
class ConversionJob
  attr_accessor :outputFormat, :outputFileL

  def initialize(fileL, outputPath, outputFormat, numThreads, toolH, tempDir)
    @logger = Logger.new(STDOUT)
    @logger.progname = "ConversionJob"
    @logger.level = $logLevel
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
    # task counts used for status updates
    @totalTasks = nil
    @completedTasks = 0.0
    @taskStatusMutex = Mutex.new
  end

  def start_conversion
    encTool = @toolH[@outputFormat]['encode']
    if encTool.nil?
      @logger.error("No encoder available for desired output format #{@outputFormat}")
      abort("")
    end
    threadL = Array.new
    mutex = Mutex.new
    @totalTasks = 2.0 * @fileL.size

    1.upto([@numThreads, @fileL.size].min) do
      # create a thread
      threadL << Thread.new {
        @logger.debug("Thread started")
        done = false

        while done.eql?(false)
          # get a file, safely
          show_status
          musicFile = mutex.synchronize { @fileL.pop }
          if musicFile.nil?
            done = true
            break
          end

          # find appropriate tool to decode the file
          decTool = @toolH[musicFile.getExtension]['decode']
          decTool.decode(musicFile, @tempDir)
          @taskStatusMutex.synchronize { @completedTasks += 1 }
          show_status

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
          @taskStatusMutex.synchronize { @completedTasks += 1 }
          show_status
        end
      }
    end

    # wait for threads to finish
    @logger.debug("#{threadL.size} running")
    threadL.each do |thr|
      thr.join
      @logger.debug("thread #{thr.to_s} joined")
    end
    puts "\r\n"
  end

  private
  def show_status
    @taskStatusMutex.synchronize {
      printf("\r%0.1f%%", 100 * @completedTasks / @totalTasks)
      STDOUT.flush
    }
  end
end


# Manages tools, identifying suitable music files to convert, and the conversion process.
class Manager
  def initialize
    @logger = Logger.new(STDOUT)
    @logger.progname = "Manager"
    @logger.level = $logLevel
    @fileL = Array.new

    # load tools
    #  fileExt => Tool instance
    @toolH = Hash.new
    load_user_defined_tools
    if @toolH.empty?
      @logger.error("Found no encoders/decoders, so we cannot do anything. Exiting")
      abort("")
    else
      @validExtensionsS = Set.new(@toolH.each_key)
      # We know what tools are available to us, so we can look for files with extensions that match
      @logger.debug("FileTypes that can be converted: #{@validExtensionsS.inspect}")
      @logger.debug("Found #{@toolH.size} tools:")
      @toolH.each_pair do |fileType, toolStruct|
        @logger.debug("  [#{fileType}]")
        @logger.debug("     + \033[33;1mencoder:\033[0m #{toolStruct['encode'].to_s}")
        @logger.debug("     + \033[33;1mdecoder:\033[0m #{toolStruct['decode'].to_s}")
      end
    end
  end

  def find_files_from_cwd
    # find music files in the CWD
    @logger.debug("Examining #{Dir.entries('.').size} files in #{File.absolute_path(".", ".")}")
    Dir.entries(".").each do |filePath|
      unless filePath.start_with?('.') || File.directory?(filePath)
        musicFile = check_file(File.absolute_path(filePath, "."))
        @fileL << musicFile unless musicFile.nil?
      end
    end
  end

  def find_files_from_cli(inputFiles)
    @logger.debug("converting #{inputFiles.size} files from command line")
    inputFiles.each do |filePath|
      unless File.directory?(filePath)
        musicFile = check_file(File.absolute_path(filePath, "."))
        @fileL << musicFile unless musicFile.nil?
      end
    end
  end

  def convert(outputFormat, outputDir, numThreads)
    @logger.debug("Convertity #{@fileL.size} files:")
    @fileL.each do |musicFile|
      @logger.debug(" + [#{musicFile.getExtension.upcase}] #{musicFile.filename}")
    end

    # builds a job and begins the conversion
    #def initialize(fileL, outputPath, outputFormat, numThreads, toolH, tempDir)
    job = ConversionJob.new(@fileL, outputDir, outputFormat, numThreads, @toolH, "/tmp")
    job.start_conversion

    # try to copy over tags from the original file
    Tagger.new.tag(job)
  end

  private
  def check_file(filepath)
    begin
      musicFile = MusicFile.new(filepath)
      return @validExtensionsS.include?(musicFile.getExtension) ? musicFile : nil
    rescue StandardError => e
      @logger.debug("File #{filepath} is not a valid file for conversion: #{e}")
      return nil
    end
  end

  private
  def load_user_defined_tools
    # look for the giveme4.conf file in the user's home-dir
    confPath = "#{ENV['HOME']}/.giveme4.yaml"
    @logger.debug("loading user-defined tool definitions from #{confPath}")
    tools = YAML.load(File.open(confPath))
    toolPair = Struct.new(:encode, :decode)

    tools.each do |rawExtension, toolHash|
      ext = rawExtension.strip.downcase
      #def initialize(fileType, progName, exePath, toolArgs)
      encoder = Encoder.new(ext,
                            File.basename(toolHash.fetch('encode').fetch('exePath').strip),
                            toolHash.fetch('encode').fetch('exePath').strip,
                            toolHash.fetch('encode').fetch('args').strip)
      decoder = Decoder.new(ext,
                            File.basename(toolHash.fetch('decode').fetch('exePath').strip),
                            toolHash.fetch('decode').fetch('exePath').strip,
                            toolHash.fetch('decode').fetch('args').strip)
      @toolH[ext] = toolPair.new(encoder, decoder)
    end
  end
end


# Handles reading and writing tags on music files.
class Tagger
  def initialize(cli_album=nil, cli_artist=nil, cli_year=nil)
    @logger = Logger.new(STDOUT)
    @logger.progname = "Tagger"
    @logger.level = $logLevel
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
              dstFileRef.save
            end
          else
            @logger.info("Source file has no tag; must source tag from directory/filenames")
          end
        end
      end
    end
  end
end


############################################################################
# Parse options
# setting some required defaults first
options = {numThreads: 2,
           outputDir: '.'}

# This isn't great, because a user is likely to define an extension and tool in their conf. TODO: remove this.
FORMATS = ["mp3", "wav", "flac", "ogg", "ape", "m4a"]
VERSION = "4.0.1"
YEAR = 2017

OptionParser.new do |opts|
  opts.banner = "Usage: giveme -f DESIRED_AUDIO_FORMAT [options]"

  opts.on("-v", "--version", "Show version information and exit") do |version|
    options[:version] = version
  end

  opts.on("-t N", "--threads N", Integer, "Set the number of threads to use to N. Default 2") do |numThreads|
    options[:numThreads] = numThreads
  end

  opts.on("-f FORMAT", "--format FORMAT", FORMATS, :REQUIRED, "Select the desired output audio format; must be one of #{FORMATS.join(" ")}") do |format|
    options[:format] = format
  end

  opts.on("-o DIR", "Select the output directory the converted files should be placed in. Defaults to '.'") do |outputDir|
    options[:outputDir] = outputDir
  end

  opts.on("-V", "--verbose", "Turn on verbose logging") do |verbose|
    options[:verbose] = true
  end

  opts.on("-i FILENAME", "Specify a single file to convert. May be specified multiple times") do |input|
    options[:inputFiles] ||= []
    options[:inputFiles] << input
  end
end.parse!

# Validate some options and generally prepare for the work to be done
if options[:version]
  puts "giveme #{VERSION}, #{YEAR}"
  exit
end

if options[:numThreads] <= 0 || options[:numThreads] > 24
  puts "Error: invalid value given for -t/--threads; must be between 1 and 24"
  exit
end

unless options[:verbose].nil?
  $logLevel = Logger::DEBUG
end

puts options

unless File.exists?(options[:outputDir])
  begin
    puts "Output directory \"#{options[:outputDir]}\" does not exist; creating"
    Dir.mkdir(options[:outputDir])
  rescue Exception => e
    puts "Error: could not create directory (#{e.to_s})"
    exit
  end
end

mgr = Manager.new

# create a list of files to convert
if options[:inputFiles].nil?
  # user did not supply a -i arg, so assume they want to convert every applicable file in CWD
  mgr.find_files_from_cwd
else
  mgr.find_files_from_cli(options[:inputFiles])
end

mgr.convert(options[:format], options[:outputDir], options[:numThreads])
