giveme
======

Multi-threaded audio conversion utility with lots of configurability, written in C over 2007-2009. It lets you use existing audio encoder/decoder builds, and set any parameters you want for each independently. It does not handle tagging as that was a feature I never got around to dealing with.

The primary problem I solved in writing giveme many years ago was to decrease the time it took to convert a lot of flac files to mp3s: my system had two physical CPU cores, but lame, flac, etc are all single-threaded. I wrote this as an exercise to toy with pthreads and to parallelize the decode/encode process for n audio files.

Installation
------------
Compile:

```bash
$ ./configure
$ make
$ sudo make install
```

You will need ncurses installed, as well as the ncurses-devel or ncurses-dev packages. These were needed for an experimental ncurses-based user interface.

Features
--------

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
should have Lame installed, for example. 'giveme' then
decodes them to the .wav format and puts them in a new
directory in /var/tmp called 'wavez'. You can omit the 
--outdir option and it would put the .wav files in the same
directory as the .mp3s.
