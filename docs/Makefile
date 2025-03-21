##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Thu Mar 20 13:00:27 PDT 2025
## Last Modified: Thu Mar 20 13:17:40 PDT 2025
## Filename:      Makefile
## Syntax:        GNU Makefile
##
## Description: This Makefile creates the emscripten version of mus2pmx
##              for use in Javascript.  In this cofiguration, the
##              emscripten-compiled code is in the form of a library
##              (with no main() function).
##
## Targets:
##    all:      First compile the code with emcc into mus2pmx.wasm
##              Then convert to a base64 string (47KB) for inserting
##              into the demo webpage.
##

OS := $(shell uname -s)
BASE64_CMD := base64

ifeq ($(OS), Darwin)
	BASE64_FLAGS := -i
else
	BASE64_FLAGS :=
endif



##############################
##
## all: first compile mus2pms.wasm and then create mus2pmx.wasm.b64
##

all: compile base64



###############################
##
## compile -- See https://github.com/emscripten-core/emscripten
##     Compiles to WASM (WebAssembly) using Emscripten for use in JavaScript.
## Options:
##    -s EXPORTED_FUNCTIONS: Tells Emscripten which C functions should
##    be accessible from Javascript.  The functions need to be prefixed
##    with an underscore (_), but you do not add that underscore when
##    using in Javascript.  See the example HTML file for usage of these
##    functions.
##       * _convertMusToPmx: Custom C function that convers binary MUS data into ASCII equivalent PMX form.
##       * _getPmxOutput:    Function which is called to access the output PMX data.
##       * _malloc:          Function to dynamically allocate memory for input data before sending
##                           to convertMuseToPmx.
##       * _free:            Function to release memory created by malloc.
##                 
##    -s EXPORTED_RUNTIME_METHODS: This exports additional runtime 
##    Emscripten methods that allow JavaScript to interact with
##    the compiled WASM module.
##       * UTF8ToString: Converts UTF-8 encoded C strings to JavaScript strings.
##       * ccall:        Allows calling exported C functions from JavaScript.
##       * cwrap:        Similar to ccall, but returns a JavaScript function that can be called repeatedly.
##       * getValue:     Reads a value from WebAssembly memory (useful for fetching integers, floats, etc.).
##
##    --no-entry: This removes the main function requirement,
##   making the WASM module run in a WebAssembly environment without a main() function.
##   Necessary for WebAssembly modules that act as libraries instead of standalone executables.
##

compile:
	emcc mus2pmx.c -o mus2pmx.wasm \
	   -s EXPORTED_FUNCTIONS="['_convertMusToPmx', '_getPmxOutput', '_malloc', '_free']" \
	   -s EXPORTED_RUNTIME_METHODS="['UTF8ToString', 'ccall', 'cwrap', 'getValue']" \
	   --no-entry -s ENVIRONMENT='web'



##############################
##
## base64 -- convert the mus2pmx WASM executable to base64 text string to insert
##    into the demo HTML webpage for stand-alone use of the program (Don't need to
##    load via a separate script file).  Different OSes have different versions of the
##    base64 program, so this make target tries to automatically detects the conversion syntax.
##

base64:
	$(BASE64_CMD) $(BASE64_FLAGS) mus2pmx.wasm -o mus2pmx.wasm.b64



