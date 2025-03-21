JavaScript version of mus2pmx
==============================

This directory contains an example of using the mus2pmx converter online.

Files:

* index.html &mdash; Stand-alone webpages where you can drag-and-drop MUS files to convert to PMX data.
* mus2pmx-wasm.c &mdash; Emscripten adapted C program for converting MUS to PMX.
* mus2pmx.wasm &mdash; Enscripten compiled version of mus2pmx-wasm.c
* mus2pmx.wasm.b64 &mdash; Base-64 string containing mus2pmx.wasm program to insert into index.html
* Makefile &mdash; Compiles mus2pmx-wasm.c and creates mus2pmx.wasm.b64 file.



