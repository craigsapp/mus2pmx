The [_mus2pmx_](https://github.com/craigsapp/mus2pmx/blob/master/mus2pmx.c) 
program converts binary SCORE files (usually ending in the extension .mus 
or .pag) into ASCII text files containing SCORE parameter matrix data (usually
ending in .pmx or .txt).

Converted data is sent to standard output.  So to save converted PMX data into
a file, redirect the standard output to a file:
<pre>
   mus2pmx input.mus > output.pmx
</pre>

More than one input file can be given as an argument to the _mus2pmx_ program.
When multiple input files are given, a line starting with ##PAGEBREAK will be
inserted between the output contents of the two files.  To convert multiple
files at once to separate PMX files, you will need to call the program
separately.  This can be done with a bash for-loop like this:
<pre>
   for i in *.mus
   do
      mus2pmx $i > `basename $i .mus`.pmx
   done
</pre>

The [_prettypmx_](https://github.com/craigsapp/prettypmx) program can be used
to compactly format the PMX output from _mus2pmx_:
<pre>
   mus2pmx input.mus | prettypmx > output.pmx
</pre>

Note that PostScript items are not yet handled by _mus2pmx_.  Also the program
will currently not read very large WinSCORE binary files which contain a
four-byte parameter count.

The [bin directory](https://github.com/craigsapp/mus2pmx/blob/master/bin)
contains compiled versions of the program for MS-DOS, Windows, OS X and
linux so that non-geek users can gain access to the program.

The [tests directory](https://github.com/craigsapp/mus2pmx/blob/master/tests)
contains an example .mus file along with its [.pmx](https://github.com/craigsapp/mus2pmx/blob/master/tests/ex1.pmx) equivalent converted with
_mus2pmx_.  Here is what the graphical notation stored in
tests/ex1.mus and tests/ex1.pmx should look like:

![Test notation](tests/ex1.png?raw=true)

