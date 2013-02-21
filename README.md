The [SCORE](http://en.wikipedia.org/wiki/SCORE_%28software%29)
notation editor can save and load data files in both binary and
ASCII formats.  Each file format contains a list of graphical items
which are each in turn a list of floating-point numbers.  Typically
binary SCORE files will end with the extension .mus or
.pag.  ASCII PMX files typically end in the extension .pmx.

Binary data files are the primary format used in SCORE.  They are
read from the file structure with the **G**et command and written
with the **SA**ve and **SM** commands.  The **NX** and **NB** comands
navigate alphabetically through binary SCORE files.  ASCII parameter
matrix (PMX) files can be written from the SCORE editor by running
the **PMX** command.  They can be read back into SCORE by using the
**RE**ad command.

The [_mus2pmx_](https://github.com/craigsapp/mus2pmx/blob/master/mus2pmx.c)
program converts binary SCORE files into ASCII files, while the
[_pmx2mus_](https://github.com/craigsapp/mus2pmx/blob/master/pmx2mus.c)
program converts ASCII PMX files into binary SCORE files.

# mus2pmx (binary to ASCII)

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


# pmx2mus (ASCII to binary)

The [_pmx2mus_](https://github.com/craigsapp/mus2pmx/blob/master/pmx2mus.c)
program converts ASCII PMX files into binary SCORE files.  This program reverse
the process of _mus2pmx_, so going from a PMX file to a MUS file and then 
back again to a PMX file should result in an identical file to the original.
MUS files contain trailer information which is lost in PMX data, so going from
an original MUS to PMX to MUS again should have exactly the same data, but 
numbers in the file trailer will be different.

The _pmx2mus_ program takes two arguments: (1) the name of an input PMX file, 
and (2) the name of an output MUS file:
<pre>
   pmx2mus input.pmx output.mus
</pre>

To convert multiple PMX files into their binary forms from the bash (unix) 
terminal:
<pre>
   for i in *.pmx
   do 
      pmx2mus $i `basename $i .pmx`.mus
   done
</pre>


# Limitations

Note that Imported EPS graphics (code 15) items are not yet handled
by _mus2pmx_ or _pmx2mus_.  Also the program will currently not
read very large WinSCORE binary files which contain a four-byte
word count at the start of the file instead of a two-byte count.


# Downloads

The [bin directory](https://github.com/craigsapp/mus2pmx/blob/master/bin)
contains compiled versions of the program for Windows, OS X and
linux so that non-geek users can gain access to the program.


# Texts files

The [tests directory](https://github.com/craigsapp/mus2pmx/blob/master/tests)
contains an example .mus file along with its [.pmx](https://github.com/craigsapp/mus2pmx/blob/master/tests/ex1.pmx) equivalent converted with
_mus2pmx_.  Here is what the graphical notation stored in
tests/ex1.mus and tests/ex1.pmx should look like:

![Test notation](tests/ex1.png?raw=true)

