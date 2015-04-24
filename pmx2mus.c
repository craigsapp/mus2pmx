//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb 20 14:45:23 PST 2013
// Last Modified: Fri Feb 22 02:11:42 PST 2013 added EPS graphic items
// Filename:      pmx2mus.c
// Syntax:        C
//
// Description:   Convert SCORE PMX data into binary SCORE files.
//
// Limitations:   Cannot handle very large WinScore files (which uses 
//                a 4-byte count at the start of the file).
//
// Usage:         pmx2mus file.pmx file.mus
//
// $Smake:        gcc -O3 -o pmx2mus pmx2mus.c
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

// function declarations:
void     printAsciiFileAsBinary  (const char* inputfile, 
                                  const char* outputfile);
int      processInputLine        (FILE* input, FILE* output);
int      readAsciiNumberLine     (float* param, int index, 
                                  const char* string);
char*    removeNewline           (char* buffer);
void     writeLittleShort        (FILE* output, int value);
void     writeLittleFloat        (FILE* output, float value);
void     writeLittleInt          (FILE* output, int value);

///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   if (argc != 3) {
      printf("Usage: %s input.pmx output.mus\n", argv[0]);
      exit(1);
   }

   printAsciiFileAsBinary(argv[1], argv[2]);

   return 0;
}

///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// printAsciiFileAsBinary -- convert PMX data from a text file into a binary
//    SCORE .mus file.  Currently input will be converted to a single output.
//    In the future, the fuction may be expanded so allow multiple page 
//    input and then save to enumerated output filenames.
//

void printAsciiFileAsBinary(const char* inputfile, const char* outputfile) {
   FILE *input = fopen(inputfile, "r");
   if (input == NULL) {
      printf("Error: cannot open file %s for reading.\n", inputfile);
   }

   FILE *output = fopen(outputfile, "w");
   if (output == NULL) {
      printf("Error: cannot open file %s for writing.\n", outputfile);
   }

   // store a place holder for the number of parameters stored in the file.
   writeLittleShort(output, 0);

   int count = 0;  // number of 4-byte words stored in file (excluding initial
                   // short in word counter.
   while (!feof(input)) {
      count += processInputLine(input, output);
   }
   fclose(input);

   // write the trailer
   writeLittleFloat(output, 0.0);     // start of trailer marker (size of 
                                      //  trailer is second-to-last # in file)
   writeLittleInt(output, 4000000);   // serial number
   writeLittleFloat(output, 4.0);     // program version
   writeLittleFloat(output, 0.0);     // measurement code 0.0=in; 1.0=cm
   writeLittleFloat(output, 5.0);     // previous numbers in trailer (inclusive)
   writeLittleFloat(output, -9999.0); // end of trailer marker
   count += 6;

   // return to the start of the output file and store the real item count
   rewind(output);
   writeLittleShort(output, count);
   fclose(output);
}



//////////////////////////////
//
// processInputLine -- Extract and write one SCORE item from input.
//

int processInputLine(FILE* input, FILE* output) {
   int    count        =  0;
   int    wcount       =  0;
   int    pcount       =  0;
   char   buffer[1024] = {0};
   float  param[100]   = {0};
   int    textcount    =  0;
   int    extrapad     =  0;
   int    textblocks   =  0;
   float  number       =  0.0;
   int    i;

   if (feof(input)) {
      // end of file, so return current count (which should be 0):
      return count;
   }

   char* ptr = fgets(buffer, sizeof(buffer), input);
   if (strlen(buffer) > 1000) {
      printf("Error: text line is too long: %s\n", buffer);
      exit(1);
   } else if (strcmp(buffer, "") == 0) {
      // empty line
      return count;
   }

   if (isdigit(buffer[0])) {
      pcount = readAsciiNumberLine(param, 0, buffer);
   } else if ((tolower(buffer[0]) == 't') && isspace(buffer[1])) {
      param[0] = 16.0;
      pcount = readAsciiNumberLine(param, 1, &(buffer[2]));
      // read the text line for the text item
      buffer[0] = '\0';
      ptr = fgets(buffer, sizeof(buffer), input);
      if (strlen(buffer) > 1000) {
         printf("Error: text line is too long: %s\n", buffer);
         exit(1);
      }
      removeNewline(buffer);
      textcount = strlen(buffer);
      extrapad  = textcount % 4;
      if (extrapad > 0) {
         extrapad = 4 - extrapad;
      }
      // store spaces in dummy charater spots to fill out a block of four
      // bytes at the end of the text item string.
      for (i=0; i<extrapad; i++) {
         buffer[textcount+i] = (char)0x20;
      }
      textblocks = (textcount + extrapad  ) / 4;
      // P12 of a text object must match the number of characters
      // in the text string.  In other words "textcount" should match
      // param[11].  So to enforce this requirement, just store textcount
      // in param[11].
      param[11] = textcount;
      if (pcount < 13) {
         // Also need to include P13 which is the width of the text.
         // Set to zero if it does not exist in PMX data.
         pcount = 13;
      }
   } else {
      // Not a list of numbers. Something else, so just ignore line
      return count;
   }

   // Write the parameters to the output file.
   if ((int)param[0] == 15) {
      // write an EPS graphic file item
      if (pcount != 13) {
         // must have 13 numeric parameters before filename
         pcount = 13;
      }
      // read the next line in the file which contains the filename
      // the filename may have trailing spaces on its line and may
      // be padded with spaces to make the length of the filename
      // be a multiple of 4.
      ptr = fgets(buffer, sizeof(buffer), input);
      if (strlen(buffer) > 1000) {
         printf("Error: text line is too long: %s\n", buffer);
         exit(1);
      }
      removeNewline(buffer);
      textcount = strlen(buffer);
      extrapad  = textcount % 4;
      if (extrapad > 0) {
         extrapad = 4 - extrapad;
      }
      // store spaces in dummy charater spots to fill out a block of four
      // bytes at the end of the EPS filename item string.
      for (i=0; i<extrapad; i++) {
         buffer[textcount+i] = (char)0x20;
      }
      textblocks = (textcount + extrapad  ) / 4;
      number = pcount + textblocks;
      writeLittleFloat(output, number);
      count++;
      for (i=0; i<pcount; i++) {
         writeLittleFloat(output, param[i]);
         count++;
      }

      wcount = fwrite(buffer, sizeof(char), textcount + extrapad, output);
      count += textblocks;
   } else if ((int)param[0] == 16) {
      // write a text item
      number = pcount + textblocks;
      writeLittleFloat(output, number);
      count++;
      for (i=0; i<pcount; i++) {
         writeLittleFloat(output, param[i]);
         count++;
      }

      wcount = fwrite(buffer, sizeof(char), textcount + extrapad, output);
      count += textblocks;
   } else {
      number = pcount;
      writeLittleFloat(output, number);
      count++;
      for (i=0; i<pcount; i++) {
         writeLittleFloat(output, param[i]);
         count++;
      }
   }

   return count;
}



//////////////////////////////
//
// removeNewline -- Get rid of any 0x0a or 0x0d characters that may
//    be hanging around at the end of the line.
//

char* removeNewline(char* buffer) {
   int len = strlen(buffer);
   int i;
   for (i=len-1; i>=0; i--) {
      if ((buffer[i] == 0x0a) || (buffer[i] == 0x0d)) {
         buffer[i] = '\0';
      } else {
         return buffer;
      }
   }
   return buffer;
}



//////////////////////////////
//
// readAsciiNumberLine -- Read a list of numbers on a line of text.
//      Should check for unexpected text on line after first number.
//

int readAsciiNumberLine(float* param, int index, const char* string) {
   char buffer[1024] = {0};
   strcpy(buffer, string);
   char* ptr = strtok(buffer, "\n\t ");
   double number = 0.0;
   int counter = index;

   while (ptr != NULL) {
      number = strtod(ptr, NULL);
      if (counter > 100) {
         printf("Error: item parameter count is too large\n");
         exit(1);
      }
      param[counter++] = number;
      ptr = strtok(NULL, "\n\t ");
   }

   return counter;
}



//////////////////////////////
//
// writeLittleShort -- Write a two-byte integer to a file with smallest
//   byte written first.
//

void writeLittleShort(FILE* output, int value) {
   char bytes[2];
   bytes[0] = (char)(value & 0xff);
   bytes[1] = (char)((value >> 8) & 0xff);
   int count = fwrite(bytes, sizeof(char), 2, output);
}



//////////////////////////////
//
// writeLittleFloat -- Write a four-byte float to a file with smallest
//   byte written first.
//

void writeLittleFloat(FILE* output, float value) {
   union {int i; float f; } data;
   data.f = value;
   writeLittleInt(output, data.i);
}



//////////////////////////////
//
// writeLittleInt -- Write a four-byte int to a file with smallest
//   byte written first.
//

void writeLittleInt(FILE* output, int value) {
   char bytes[4];
   bytes[0] = (char)(value & 0xff);
   bytes[1] = (char)((value >> 8)  & 0xff);
   bytes[2] = (char)((value >> 16) & 0xff);
   bytes[3] = (char)((value >> 24) & 0xff);
   int count = fwrite(bytes, sizeof(char), 4, output);
}


