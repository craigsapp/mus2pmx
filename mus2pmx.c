//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Aug 29 13:50:35 PDT 2012
// Last Modified: Wed Aug 29 16:47:01 PDT 2012
// Filename:      mus2pmx.c
// Syntax:        C
//
// Description:   Convert binary SCORE files (typically ending in the
//                extensions .mus and .pag) into ASCII format (typically
//                ending in .pmx extension).  There can be more than
//                one input file.  If so, then the output will contain
//                multiple pages, with each page separated by a line
//                starting with ##PAGEBREAK.  (Multiple page files cannot be
//                loaded into SCORE, but are useful for converting a 
//                movement from SCORE into another format).
//
// Limitations:   PostScript items are not considered yet (should be treated
//                similar to text items).
//
// Usage:         mus2pmx file.mus [file2.mus] > file.pmx
//
// Compile:       gcc -o mus2pmx mus2pmx.c
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// function declarations:
void     printBinaryPageFileAsAscii  (const char* filename);
int      readLittleShort             (FILE* input);
double   readLittleFloat             (FILE* input);
void     printObjectParameters       (FILE* input, int count);
void     printTextObject             (FILE* input, int count);
void     printNumericObject          (FILE* input, int count);
double   roundFractionDigits         (double number, int digits);

int debugQ   = 0;  // turn on for debugging display
int verboseQ = 0;  // turn on for seeing more info from trailer

///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   int i;
   for (i=1; i<argc; i++) {
      // If there are multiple input files print an information line
      // showing the original filename for each page.
      if (argc > 2) {
         printf("##FILE:\t%s\n", argv[i]);
      }

      printBinaryPageFileAsAscii(argv[i]);

      // Print "##PAGEBREAK" after every page execept the last one
      // when there are multiple input files.
      if ((i < argc-1) && (argc > 2)) {
         printf("##PAGEBREAK\n");
      }
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// printBinaryPageFileAsAscii -- convert a binary SCORE file into its
//    ASCII PMX representation.  Binary SCORE files can be read into 
//    the SCORE editor with the command "G file[.mus]", ASCII parameter
//    matrix files can be read into SCORE with the command
//    "RE file.pmx".  The default file extension for binary files is ".mus",
//    so it is optional to specify.  Binary files may have other extensions.
//    ".pag" files are binary data files with the intention that they 
//    represent a page of music rather than a system line of music.
//

void printBinaryPageFileAsAscii(const char* filename) {
   FILE *input = fopen(filename, "r");
   if (input == NULL) {
      printf("Error: cannot open file %s for reading.\n", filename);
      exit(1);
   }

   int countFieldByteSize = 2;
   int numberCount = -1;
   // First read the number of 4-byte numbers (or 4-character groupings) 
   // which are found in the file after this number.  The size of this 
   // number is 2 bytes for all files created with DOS versions of SCORE.
   // For Windows versions of SCORE, it is possible that this number can
   // be 4 bytes wide if the number of 4-byte values in the file exceeds
   // 0xffff.
   if (countFieldByteSize == 2) {
      numberCount = readLittleShort(input);
   }
   if (debugQ) {
      printf("#number count is %d\n", numberCount);
   }
   
   // Process the trailer of the file.   First verify that this is 
   // a SCORE file since all SCORE binary files must end in the
   // hex bytes "00 3c 1c c6" which represents the floating point
   // number -9999.0.
   fseek(input, -4, SEEK_END);
   double lastNumber = readLittleFloat(input);
   if (debugQ) {
      printf("#trailer end number is %.1lf\n", lastNumber);
   }
   if (lastNumber != -9999.0) {
      printf("Error: last number is not -9999.0: %.1lf\n", lastNumber);
      exit(1);
   }

   // Now read the trailer of the file.  First go to 8 bytes before the
   // end of the file and read how many bytes are in the trailer 
   // There should be at least 5 numbers. In reverse order from the
   // end of the file, these are:
   //    number 1: The number of floats in the trailer (including
   //              this value.  Standard value is "5.0"  For future
   //              versions of SCORE, this value might be larger, but
   //              it will never be smaller.  Numbers 2, 3, and 4 will
   //              always have a fixed meaning in any future versions
   //              of SCORE, and extra parameters will be added before
   //              them in the file if this value is larger than 5.0.
   //    number 2: The measurement code: 0.0 = inches, 1.0 = centimeters.
   //              This is needed for certain length measurements for
   //              certain objects (not often used).
   //    number 3: Program version number which created the file.
   //    number 4: Program serial number which created the file.
   //    number 5: The last number in the trailer (i.e., the first
   //              trailer byte within the file) must be set to 0.0;
   //              This is an alternate way of identifying the trailer
   //              after a list of objects.  No object should have a
   //              parameter size of 0.0, so a parameter size of 0.0
   //              would indicate the end of the data and the start
   //              of the trailer.

   // Read number 1 (trailer size):
   fseek(input, -8, SEEK_END);
   double trailerSize = readLittleFloat(input);
   if (debugQ) {
      printf("#trailer size is %.1lf\n", trailerSize);
   }
   if (trailerSize < 5.0) {
      printf("Error: trailer size is too small: %.1lf\n", trailerSize);
      exit(1);
   }
 
   // Read number 2 (measurement units):
   fseek(input, -12, SEEK_END);
   double unitType = readLittleFloat(input);
   if (debugQ) {
      printf("#unit type is %.1lf\n", unitType);
   }
   if (verboseQ) {
      if (unitType == 0.0) {
         printf("##UNITS:\tinches\n");
      } else if (unitType == 0.0) {
         printf("##UNITS:\tcentimeters\n");
      }
   }
   
   // Read number 3 (program version number):
   fseek(input, -16, SEEK_END);
   double versionNumber = readLittleFloat(input);
   if (verboseQ) {
      printf("##VERSION:\t%.2lf\n", versionNumber);
   }

   // Read number 4 (program serial number):
   fseek(input, -20, SEEK_END);
   double serialNumber = readLittleFloat(input);
   if (verboseQ) {
      printf("##SERIAL:\t%lf\n", serialNumber);
   }

   // Now that the file trailer has been processed, return to the start
   // of the file (after the first number):
   fseek(input, countFieldByteSize, SEEK_SET);

   // start reading objects one at a time
   double number = 0.0;
   int readCount = 0;
   while (!feof(input)) {
      if (numberCount - readCount - (int)trailerSize - 1 == 0) {
         // all expected numbers have been read from the file, so stop 
         // reading musical objects.
         break;
      } else if (numberCount - readCount - (int)trailerSize - 1 < 0) {
         printf("Error: object data overlaps with trailer contents\n");
         exit(1);
      }
        
      number = readLittleFloat(input);
      number = roundFractionDigits(number, 3);
      readCount++;
      if (number == 0.0) {
         printf("Error: parameter size of next object is zero.\n");
         exit(1);
      }
      if (debugQ) {
         printf("# next object has %d parameters\n", (int)number);
      }
      readCount += (int)number;
      printObjectParameters(input, (int)number);
   }

   if (fclose(input)) {
      printf("Strange error when trying to close file %s.\n", filename);
      exit(1);
   }
}



//////////////////////////////
//
// printObjectParameters -- print the given number of parameters
//    in the musical object at the current point in the file.
//

void printObjectParameters(FILE* input, int count) {
   // P1 == parameter 1, which is the object type
   double P1 = readLittleFloat(input);  
   if (P1 <= 0.0) {
      printf("Strange error: P1 is non-positive: %lf\n", P1);
      exit(1);
   }
   if (P1 >= 100.0) {
      printf("Strange error: P1 is way too large: %lf\n", P1);
      exit(1);
   }
   if (P1 == 16.0) {
      // text objects use "t" instead of "16.0" for the first parameter
      // in the object when displaying as ASCII PMX data.
      printf("t     ");
      printTextObject(input, count-1);
   } else {
      // Print non-text objects.  PostScript objects probably also
      // need special parsing similar to text objects (I have never
      // encountered or used a PostScript object).  PostScript objects
      // will store the filename of the PostScript file which should
      // be inserted into the output printout for the page.
      if (P1 < 10) {
         // The first character on the line for a PMX should not be a space.
         // The fractional value is usually not used (always .0000).  But
         // Walter's data and the newest Windows SCORE files may contain
         // non-zero fraction digits which describe the layer number of
         // the object on the staff.
         printf("%1.4lf", P1);
      } else {
         printf("%2.3lf", P1);
      }
      printNumericObject(input, count-1);
   }
}



//////////////////////////////
//
// printTextObject -- print a P1=16 object, starting with P2 value.
//

void printTextObject(FILE* input, int count) {
   int i;
   double number;
   if (count < 12) {
      printf("Error reading binary text object: there must be 13 fixed ");
      printf("parameters, but there are instead %d.\n", count);
      exit(1);
   }

   // First print the fixed parameters for the text object:
   int characterCount = -1;
   for (i=0; i<12; i++) {
      number = readLittleFloat(input); 
      number = roundFractionDigits(number, 3);
      printf(" %8.3lf", number);
      if (i == 10) {
         // P12 is the number of characters in the string which follows.
         characterCount = (int)number;
      }
   }
   printf("\n");

   if (debugQ) {
      printf("# String length is %d\n", characterCount);
   }

   char buffer[1024] = {0};
   if (fread((void*)buffer, sizeof(char), characterCount, input) == EOF) {
      printf("Error while trying to read text string.\n");
      exit(1);
   }
   buffer[characterCount] = '\0';
   printf("%s\n", buffer);
   
   // Next, read extra padding bytes, since the length of the string
   // field must be a multiple of 4.  These padding bytes should be
   // spaces, but can occasionally be non-zero, so just ignore the 
   // padding bytes.
   int extraBytes = characterCount % 4;
   if (debugQ) {
      printf("#Extra padding bytes after string is %d\n", 4-extraBytes);
   }
   if ((extraBytes > 0) && (extraBytes < 4)) {
      fread((void*)buffer, sizeof(char), 4-extraBytes, input);
      buffer[4-extraBytes] = '\0';
   }
}



//////////////////////////////
//
// printNumericObject -- print a P1!=16 object, starting with P2 value.
//    Also, PostScript objects should probably not be printed with this
//    function (but currently are).
//

void printNumericObject(FILE* input, int count) {
   int i;
   double number;
   for (i=0; i<count; i++) {
      number = readLittleFloat(input); 
      number = roundFractionDigits(number, 3);
      printf(" %8.3lf", number);
   }
   printf("\n");
}



//////////////////////////////
//
// readLittleShort -- Read a (two-byte) unsigned short at the current
//   read position in a file, and which is stored in the file in
//   little-endian ordering.
//

int readLittleShort(FILE* input) {
   unsigned char byteinfo[2];
   if (fread((void*)byteinfo, sizeof(unsigned char), 2, input) == EOF) {
      printf("Error reading little-endian float: unexpected end of file\n");
      exit(1);
   }
   int output = 0;
   output = byteinfo[1];
   output = (output << 8) | byteinfo[0];
   return output;
}



//////////////////////////////
//
// readLittleFloat -- Read a (four-byte) signed float at the
//     current position in a file, and which is stored in the file
//     in little-endian ordering.
//

double readLittleFloat(FILE* input) {
   unsigned char byteinfo[4];
   if (fread((void*)byteinfo, sizeof(unsigned char), 4, input) == EOF) {
      printf("Error reading little-endian float: unexpected end of file\n");
      exit(1);
   }
   union { float f; unsigned int i; } num;
   num.i = 0;
   num.i = byteinfo[3];
   num.i = (num.i << 8) | byteinfo[2];
   num.i = (num.i << 8) | byteinfo[1];
   num.i = (num.i << 8) | byteinfo[0];
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



