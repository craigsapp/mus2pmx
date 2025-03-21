//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Aug 29 13:50:35 PDT 2012
// Last Modified: Fri Feb 22 03:54:54 PST 2013 added EPS graphic item
// Last Modified: Mon Mar 15 18:50:16 PDT 2021 added SCORE v3 file parsing
// Filename:      mus2pmx.c
// Syntax:        C; Emscripten
// vim:           ts=3:nowrap
//
// Description:   Emscripten version of the mus2pmx program for JavaScript that converts
//                binary SCORE files into ASCII (PMX) files.  Binary SCORE data files
//                typically end in the extensions .mus or .pag.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <emscripten.h>

// Global buffer for storing the PMX output data:
#define PMX_BUFFER_SIZE 65536
char pmx_output[PMX_BUFFER_SIZE];
size_t pmx_index = 0;

int debugQ = 0;

// Function declarations:
void   processMusFile         (const unsigned char* data, size_t length);
void   appendToPmxOutput      (const char* format, ...);
void   resetPmxOutput         (void);
int    printTextItem          (const unsigned char** input, int count);
void   printNumericItem       (const unsigned char** input, int count);
int    printItemParameters    (const unsigned char** data, int count);
int    readLittleShort        (const unsigned char** data);
int    readLittleInt          (const unsigned char** data);
double readLittleFloat        (const unsigned char** data);
double roundFractionDigits    (double number, int digits);


///////////////////////////////////////////////////////////////////////////
//
// WASM-exported functions
//
// EMSCRIPTEN_KEEPALIVE is a macro used in Emscripten to prevent
// functions or variables from being removed during dead code elimination
// (tree-shaking) during optimization.
//
// When you compile C/C++ to WebAssembly using Emscripten, the compiler
// aggressively removes code that it thinks is unused. This is good
// for size—but if you're calling a function from JavaScript (i.e.,
// outside C/C++), Emscripten can't see that, and might remove the
// function.
//
// * Tells the compiler not to discard the symbol, even if it appears unused.
// * Ensures the symbol is visible outside the shared object (WebAssembly module).
//

EMSCRIPTEN_KEEPALIVE void convertMusToPmx(const unsigned char* data, size_t length);
EMSCRIPTEN_KEEPALIVE const char* getPmxOutput();


//////////////////////////////
//
// convertMusToPmx -- Input MUS binary data, and then store the
//      ASCII output to the pmx_output array, with the size of the
//      used region of the buffer stored in pmx_index.  This is the
//      function to call from JavaScript to load the intput MUS
//      data.
//
//   Input parameters:
//      const unsigned char* data: bytes from the input mus file.
//      size_t length: length of the data array in bytes.
//
//

void convertMusToPmx(const unsigned char* data, size_t length) {
	processMusFile(data, length);
}


//////////////////////////////
//
// getPmxOutput -- Function to call after loading MUS file in order
//     to extract C string of the ASCII otput text.  This is the function
//     to call to receive back to JavaScript the converted PMX data.
//

const char* getPmxOutput() {
	return pmx_output;
}

//
// WASM-exported functions
//
//////////////////////////////


//////////////////////////////
//
// resetPmxOutput -- Clear the pmx_output buffer of old data.
//

void resetPmxOutput() {
	pmx_index = 0;
	memset(pmx_output, 0, PMX_BUFFER_SIZE);
}



//////////////////////////////
//
// appendToPmxOutput -- Add more text to the output buffer.
//

void appendToPmxOutput(const char* format, ...) {
	va_list args;
	va_start(args, format);
	pmx_index += vsnprintf(pmx_output + pmx_index, PMX_BUFFER_SIZE - pmx_index, format, args);
	va_end(args);
}



//////////////////////////////
//
// processMusFile -- Do the conversion from MUS to PMX.
//    Binary SCORE files can be read into the SCORE editor with the command
//    "G file[.mus]", ASCII Parameter MatriX files can be read into SCORE
//    with the command "RE file.pmx".  The default file extension for
//    binary files is ".mus", but also ".pag" is common when the MUS file
//    contains a complete page (but this is optional).
//
//   Input parameters:
//      const unsigned char* data: bytes from the input mus file.
//      size_t length: length of the data array in bytes.
//
//    Javascript summary:
//          const reader = new FileReader();
//          const uint8Array = new Uint8Array(reader.result);
//          const ptr = wasmModule.malloc(uint8Array.length);
//          wasmModule.convertMusToPmx(ptr, uint8Array.length);
//

void processMusFile(const unsigned char* data, size_t length) {
	resetPmxOutput();

	if (length < 4) {
		appendToPmxOutput("Error: File too small\n");
		return;
	}

	const unsigned char* ptr = data;

	// If the file size mod 4 has a remainder of 0, then the item
	// count field at the start of the file is four bytes (a big MUS
	// file created by later versions of SCORE when there are more than
	// 2^16-1 numbers or four-byte chuncks of text.  If the remainder
	// is 2 instead of 0, then it is a small MUS file with less data.
	int countFieldByteSize = (length % 4 == 0) ? 4 : 2;

	int numberCount = (countFieldByteSize == 2) ? readLittleShort(&ptr) : readLittleInt(&ptr);

	// Verify that the length of the data matches the length of the input data:
	if (length < (size_t)(4 * numberCount)) {
		appendToPmxOutput("Error: Invalid file length\n");
		return;
	}

	// Verify that the last four bytes in the file represent the
	// floating-point number -9999.0:
	ptr = data + length - 4;
	double lastNumber = readLittleFloat(&ptr);
	if (lastNumber != -9999.0) {
		appendToPmxOutput("Error: last number is not -9999.0 but instead is %.1lf\n", lastNumber);
		return;
	}

	// Now read the trailer of the file.  First go to 8 bytes before the
	// end of the file and read how many bytes are in the trailer
	// There should be at least 5 numbers (or maybe 4). In reverse order
	// from the end of the file, these are:
	//    number 1: The number of floats in the trailer (including
	//              this value.  Standard value is "5.0"  For future
	//              versions of SCORE, this value might be larger, but
	//              it will never be smaller.  Numbers 2, 3, and 4 will
	//              always have a fixed meaning in any future versions
	//              of SCORE, and extra parameters will be added before
	//              them in the file if this value is larger than 5.0.
	//    number 2: The measurement code: 0.0 = inches, 1.0 = centimeters.
	//              This is needed for certain length measurements for
	//              certain items (not often used).
	//    number 3: Program version number which created the file.
	//    number 4: Program serial number which created the file.
	//    number 5: The last number in the trailer (i.e., the first
	//              trailer byte within the file) must be set to 0.0;
	//              This is an alternate way of identifying the trailer
	//              after a list of items.  No item should have a
	//              parameter size of 0.0, so a parameter size of 0.0
	//              would indicate the end of the data and the start
	//              of the trailer.

	// (1) The number of parameters in the trailer (including this number):
	ptr = data + length - 8;
	double trailerSize = readLittleFloat(&ptr);

	if (trailerSize < 4.0) {
		appendToPmxOutput("Error: trailer size is less than 3 values: %d\n", trailerSize);
		return;
	}

	// (2) The unit type (inches or centimeters):
	// Possible values:
	//    0.0 = inches
	//    1.0 = centimeters
	ptr = data + length - 12;
	double unitType = readLittleFloat(&ptr);

	// (3) The SCORE version number which created the data
	// (or the target SCORE version for the data).
	ptr = data + length - 16;
	double versionNumber = readLittleFloat(&ptr);

	// (4) The serial number of the SCORE program which created
	// the data file (or zero otherwise).  SCORE version 4 (and higher)
	// contains a serial number of the program used to create the
	// data file.
	double serialNumber = 0.0;
	if (trailerSize > 4.0) {
		ptr = data + length - 20;
		serialNumber = readLittleFloat(&ptr);
	}

	// (5) The last parameter in the trailer should be 0.0,
	// so this would be good to check in case the trailer is
	// corrupted.  Ignoring for now (and its position could be
	// later if the trailer parameter count is larger than 5.

	// Print the PMX header extracted from the MUS trailer:
	if (unitType == 0.0) {
		appendToPmxOutput("##UNITS:\tinches\n");
	} else if (unitType == 1.0) {
		appendToPmxOutput("##UNITS:\tcentimeters\n");
	} else {
		// appendToPmxOutput("##UNITS:\tunknown\n");
	}
	appendToPmxOutput("##VERSION:\t%.2lf\n", versionNumber);
	if ((trailerSize > 4.0) && (serialNumber > 0)) {
		appendToPmxOutput("##SERIAL:\t%lf\n", serialNumber);
	}

	// Go back to the start of the list of parameters after the
	// initial 2- or 4-byte count of floats until the trailer.
	ptr = data + countFieldByteSize;

	int trailerBytes = 4 * ((int)trailerSize + 1);
	while (ptr < data + length - trailerBytes) {
		// Number of numbers (4-byte chunks of data) to follow this number:
		double parameterCount = readLittleFloat(&ptr);
		parameterCount = roundFractionDigits(parameterCount, 3);

		if (parameterCount <= 0.0) {
			appendToPmxOutput("Error: parameter size item is zero.\n");
			return;
		}

		int status = printItemParameters(&ptr, parameterCount);
		if (status != 0) {
			break;
		}
	}
}



//////////////////////////////
//
// printItemParameters -- print the given number of parameters
//    in the musical item at the current point in the file.
//    Paramters with some non-numeric data are:
//      P1 = 16: text
//      P1 = 15: EPS graphic item
//      P1 = 13: Not used
//      P1 = 100+: (or 99+) not possible to use.
//

int printItemParameters(const unsigned char** input, int count) {
	// P1 == parameter 1, which is the item type
	double P1           = readLittleFloat(input);
	char   buffer[1024] = {0};
	int    i;
	if (P1 <= 0.0) {
		appendToPmxOutput("Strange error: P1 is non-positive: %lf\n", P1);
		return -1;
	}
	if (P1 >= 99.0) {
		appendToPmxOutput("Strange error: P1 is way too large: %lf\n", P1);
		return -1;
	}
	if (P1 == 16.0) {
		// text items use "t" instead of "16.0" for the first parameter
		// in the item when displaying as ASCII PMX data.
		appendToPmxOutput("t     ");
		printTextItem(input, count-1);
	} else if (P1 == 15.0) {
		// print EPS graphic item
		// The first 13 4-byte words are numeric parameters
		// Parameter 13 should probably not be printed in the PMX data
		// since it is only used to edit the filename in the SCORE editor.
		if (count < 13) {
			appendToPmxOutput("Error: EPS graphic item has too few parameters\n");
			return -1;
		}
		appendToPmxOutput("%2.3lf", P1);
		printNumericItem(input, 12);
		// Read the remaining bytes into a string to print as the EPS filename.
		count = count - 13;
		if (count <= 0) {
			appendToPmxOutput("Error: expecting non-zero count for P1=15 filename\n");
			return -1;
		}
		if (count > 200) {
			appendToPmxOutput("Error: P1=15 filename too long.\n");
			return -1;
		}

		// Copy the data from the buffer
		memcpy(buffer, *input, count * 4);
		buffer[count * 4] = '\0';  // Ensure null termination
		*input += count * 4; // Advance the input pointer

		// remove any trailing spaces in filename
		for (i=count*4-1; i>=0; i--) {
			if (buffer[i] == 0x20) {
				buffer[i] = '\0';
			} else {
				break;
			}
		}
		appendToPmxOutput("%s\n", buffer);
	} else {
		// Print non-text items.
		if (P1 < 10) {
			// The first character on the line for a PMX should not be a space.
			// The fractional value is usually not used (always .0000).  But
			// Walter's data and the newest Windows SCORE files may contain
			// non-zero fraction digits which describe the layer number of
			// the items on the staff.
			appendToPmxOutput("%1.4lf", P1);
		} else {
			appendToPmxOutput("%2.3lf", P1);
		}
		printNumericItem(input, count-1);
	}

	return 0;
}



//////////////////////////////
//
// printTextItem -- print a P1=16 item, starting with P2 value.
//

int printTextItem(const unsigned char** input, int count) {
   if (count < 12) {
      appendToPmxOutput("# Error reading binary text item: there must be 13 fixed ");
      appendToPmxOutput("parameters, but there are instead %d.\n", count);
      return -1;
   }

   // First print the fixed parameters for the text item:
   int characterCount = -1;
   for (int i=0; i<12; i++) {
      double number = readLittleFloat(input);
      number = roundFractionDigits(number, 3);
      appendToPmxOutput(" %8.3lf", number);
      if (i == 10) {
         // P12 is the number of characters in the string which follows.
         characterCount = (int)number;
      }
   }
   appendToPmxOutput("\n");

   if (debugQ) {
      appendToPmxOutput("# String length is %d\n", characterCount);
   }

   // Read text string:
   char buffer[1024] = {0};
	if (characterCount < 1024) {
   	memcpy(buffer, *input, characterCount);
	}
   buffer[characterCount] = '\0';
   *input += characterCount; // Advance pointer

   appendToPmxOutput("%s\n", buffer);

   // Next, read extra padding bytes (ensuring string field is a multiple of 4 bytes)
   int extraBytes = characterCount % 4;
   if (debugQ) {
      appendToPmxOutput("# Extra padding bytes after string is %d\n", 4 - extraBytes);
   }
   if (extraBytes > 0 && extraBytes < 4) {
      *input += (4 - extraBytes); // Skip padding
   }

	return 0;
}



//////////////////////////////
//
// printNumericItem -- print a P1!=16 item, starting with P2 value.
//    Also, PostScript items (P1==15) should probably not be printed
//    with this function (but currently are).
//

void printNumericItem(const unsigned char** input, int count) {
   for (int i=0; i<count; i++) {
      double number = readLittleFloat(input);
      number = roundFractionDigits(number, 3);
      appendToPmxOutput(" %8.3lf", number);
   }
   appendToPmxOutput("\n");
}



//////////////////////////////
//
// readLittleShort -- Read a (two-byte) unsigned short at the current
//   read position in a file that is stored in the file in little-endian
//   ordering.
//

int readLittleShort(const unsigned char** data) {
	int value = (*data)[1] << 8 | (*data)[0];
	*data += 2;
	return value;
}



//////////////////////////////
//
// readLittleInt -- Read a (four-byte) unsigned int at the current
//   read position in a file that is stored in the file in little-endian
//   ordering.
//

int readLittleInt(const unsigned char** data) {
	int value = (*data)[3] << 24 | (*data)[2] << 16 | (*data)[1] << 8 | (*data)[0];
	*data += 4;
	return value;
}



//////////////////////////////
//
// readLittleFloat -- Read a (four-byte) signed float at the
//     current position in a file, and which is stored in the file
//     in little-endian ordering.
//

double readLittleFloat(const unsigned char** data) {
	union { float f; unsigned int i; } num;
	num.i = (*data)[3] << 24 | (*data)[2] << 16 | (*data)[1] << 8 | (*data)[0];
	*data += 4;
	return num.f;
}



//////////////////////////////
//
// roundFractionDigits -- Round a floating-point number to the speicified
//    number of siginificant digits after the decimal point.  SCORE binary
//    values are floats, and they usually contain (roundoff?) junk after
//    the third digit after the decimal point.
//

double roundFractionDigits(double number, int digits) {
	double dshift = pow(10.0, digits);
	if (number < 0.0) {
		return ((int)(number * dshift - 0.5))/dshift;
	} else {
		return ((int)(number * dshift + 0.5))/dshift;
	}
}



