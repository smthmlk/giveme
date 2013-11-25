giveme
======

Multi-threaded audio conversion utility with lots of configurability

Installation
------------
Compile:

```bash
./configure
make
make install (as root)
```

You will need ncurses installed, as well as the ncurses-devel or ncurses-dev packages.

Using
-----

You should read the built-in help, first:

```bash
$ giveme --help
```

Example:

    cd dirThatHasMusicFilesInIt
    giveme -f wav --outdir /var/tmp/wavez

The directory 'dirThatHasMusicFilesInIt' should have music
which you have a decoder for. If they are .mp3 files, you
should have Lame installed, for example. 'giveme' then
decodes them to the .wav format and puts them in a new
directory in /var/tmp called 'wavez'. You can omit the 
--outdir option and it would put the .wav files in the same
directory as the .mp3s.
