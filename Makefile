##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Tue Feb 19 21:16:26 PST 2013
## Last Modified: Tue Feb 19 21:16:28 PST 2013
## Filename:      ...mus2pmx/Makefile
## Syntax:        GNU Makefile
##
## Description: This Makefile creates the mus2pmx and pmx2mus programs for 
##              multiple operating systems.  Also, gives basic guidelines 
##              on how to compile for Windows using MinGW in linux.
##

# Set the environmental variable $MACOSX_DEPLOYMENT_TARGET to
# "10.4" in Apple OS X to compile for OS X 10.4 and later (for example,
# you can compile for OS X 10.4 computers even if you are using a 10.7
# computer to compile the code).
ENV = 

# Set the ARCH variable to compile 32-bit executable on 64-bit computer
# (if you have 32-bit libraries installed on your 64-bit computer).
# See the examples for ARCH on OS X and linux given in comments below.
ARCH =  

# Select options based on the Operating system type
ifeq ($(origin OSTYPE), undefined)
   ifeq ($(shell uname),Darwin)
      OSTYPE = OSX
      ENV = MACOSX_DEPLOYMENT_TARGET=10.4
      # use the following to compile for 32-bit architecture on 64-bit comps:
      # ARCH = -m32 -arch i386
   else
      OSTYPE = LINUX
      # use the following to compile for 32-bit architecture on 64-bit comps:
      # (you will need 32-bit libraries in order to do this)
      # ARCH = -m32 
   endif
endif

COMPILER = LANG=C gcc
PREFLAGS = -O3 -lm

# MinGW compiling setup (used to compile for Microsoft Windows but actual
# compiling can be done in Linux). You have to install MinGW and this
# variable will probably have to be changed to the correct path to the
# MinGW compiler:
COMPILER = /usr/bin/i686-pc-mingw32-gcc

.PHONY: mus2pmx pmx2mus
all: mus2pmx pmx2mus

mus2pmx:
	$(ENV) $(COMPILER) $(ARCH) $(PREFLAGS) -o mus2pmx mus2pmx.c

pmx2mus:
	$(ENV) $(COMPILER) $(ARCH) $(PREFLAGS) -o pmx2mus pmx2mus.c

clean:
	-rm mus2pmx
	-rm pmx2mus

