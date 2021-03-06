giveme
======

Multi-threaded audio conversion utility with lots of configurability, written in Ruby in 2014. It lets you use existing audio encoder/decoder builds, and set any parameters you want for each independently. It automatically preserves tags from the source audio file (if present) and sets them in the converted files using taglib.

The primary problem I solved in writing giveme many years ago was to decrease the time it took to convert a lot of flac files to mp3s: my system had two physical CPU cores, but lame, flac, etc are all single-threaded. Originally, this began in 1.x and 2.x as a rather involved bash script; later, with 3.x, I rewrote it in C as an exercise to toy with pthreads and to parallelize the decode/encode process for n audio files with more control. The 4.x incarnation is an excuse to play with Ruby, and to fix the tagging issue never addressed in any previous versions.

Installation
------------

Eventually there will be a simple Makefile to install the ruby script in /usr/bin or so. For now, manually drop it in somewhere and chmod +x it.

You will need Ruby >= 2.0 and taglib-ruby >= 0.6, which can be installed using `gem install taglib-ruby`. To install tablib on OSX, you will first need Xcode command line tools installed (`xcode-select --install`) and approve the license (`sudo xcodebuild -license`) before gcc will work.

Finally, `giveme` uses a configuration file (written in YAML) to learn where individual tools are, and how to use them to encode/decode. You will need to copy giveme4.yaml to your home directory as follows:

```bash
cp giveme4.yaml ~/.giveme4.yaml
```


Features
--------
NOTE: this has yet to be updated from 3.x

You should read the built-in help, first:

```bash
$ giveme --help
```

giveme will seek out any audio files in the current working directory using file extensions (.mp3, .flac, .ogg, ..). It will then convert them to the destination file type you specify with the -f option. 

giveme simply instruments existing executables on your system to do the job of decoding and encoding files. This lets you change out versions of encoders/decoders easily and without recompiling giveme or worrying about breaking RPM dependencies (if you build an RPM of giveme using the included .spec file). This is a very nice feature because some of the audio file format enthusiast community like to use standard "approved" builds of encoders: giveme will work flawlessly with them. 

You can specify all the parameters you want to pass to your encoder and decoder executables using giveme's conf file located at ~/.giveme3.conf. Giveme will also use this file to determine what audio files it can decode when searching the CWD for files to decode.

One paramter you will want to set is -t, for the number of threads to use. By default it uses 2, but you set this to however many you like. Keep in mind that it will make sense to limit this to only the number of CPUs or cores your system has, or the number of audio files in the CWD, whichever is smallest.

Example usage
-------------

```bash
$ cd dirThatHasMusicFilesInIt
$ giveme -f wav --outdir /var/tmp/wavez
```

The directory 'dirThatHasMusicFilesInIt' should have music which you have a decoder for. If they are .mp3 files, you
should have Lame installed, for example. 'giveme' then decodes them to the .wav format and puts them in a new
directory in /var/tmp called 'wavez'. You can omit the `-o` option and it would put the .wav files in the same
directory as the .mp3s.

You can also specify individual files to convert by using the `-i` option. This can be used multiple times to convert more than one file. As always, the input files can be in different formats.

```bash
$ giveme.rb -f mp3 -o MyOutputDir -i BeautifulLife.flac -i NeverGonnaSayImSorry.m4a -i WhispersInBlindess.wav
```
