//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jun 10 17:10:59 PDT 2015
// Last Modified: Wed Jun 10 19:33:49 PDT 2015
// Filename:      drw2aton.c
// Syntax:        C
//
// Description:   Convert binary SCORE DRAW files (typically ending in the
//                extension .drw) into an ASCII format
//                ending in .pmx extension).
//
// Usage:         mus2pmx file.drw > file.aton
//
// $Smake:        gcc -O3 -o drw2aton -lm drw2aton.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// function declarations:
void     printBinaryDrawFileAsAscii  (const char* filename);
int      readChar                    (FILE* input);
int      readLittleShort             (FILE* input);
int      getSymbolOffset             (const char* filename);
int      readChunk                   (int* vectors, int* index, FILE* input);
void     printDrawData               (const char* filename, char* fontNames,
                                      int* vectorOffsets, int* vector);
int      printSymbol                 (int index, int symbolOffset,
                                      char* fontNames, int* vectorOffsets,
                                      int* vectors);

int debugQ   = 0;  // turn on for debugging display

///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   int i;
   printf("@@BEGIN: FONT_LIBRARY\n");
   for (i=1; i<argc; i++) {
      printBinaryDrawFileAsAscii(argv[i]);
   }
   printf("@@END: FONT_LIBRARY\n");
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// printBinaryDrawFileAsAscii -- convert a binary .DRW file into an
//    ASCII representation.
//

void printBinaryDrawFileAsAscii(const char* filename) {
   FILE *input = fopen(filename, "r");
   if (debugQ) {
      printf("@ FILENAME:\t%s\n", filename);
   }
   if (input == NULL) {
      printf("Error: cannot open file %s for reading.\n", filename);
      exit(1);
   }

   int firstNum = readChar(input);
   if (firstNum != 0x4b) {
      printf("Error: expected 180, but got %d at start of file\n", firstNum);
      exit(1);
   }

   int headerBytes = readChar(input);

   int vectorOffsets[11] = {0};
   int i;
   for (i=0; i<11; i++) {
      vectorOffsets[i] = readLittleShort(input);
      if (debugQ) {
         printf("@ OFFSET[%d]:\t%d\n", i, vectorOffsets[i]);
      }
   }

   char fontNames[50] = {0};
   for (i=0; i<50; i++) {
      fontNames[i] = readChar(input);
   }

   // read the repeated header byte count:
   int headerBytes2 = readChar(input);

   if (headerBytes != headerBytes2) {
      printf("Error: header byte count does not match: %d, %d\n",
         headerBytes, headerBytes2);
   }

   // Now read the vector information which are a set of short int triples.
   int numCount = vectorOffsets[10];
   int index = 0;
   int* vectors = (int*)malloc(numCount * sizeof(int));

   while (readChunk(vectors, &index, input) && !feof(input)) {
      // do nothing;
   }

   printDrawData(filename, fontNames, vectorOffsets, vectors);
}



//////////////////////////////
//
// printDrawData --
//

void printDrawData(const char* filename, char* fontNames, int* vectorOffsets,
      int* vectors) {

   int symbolOffset = getSymbolOffset(filename);

   // print 10 entries (may be smaller)
   int i;
   int flag;
   for (i=0; i<10; i++) {
      flag = printSymbol(i, symbolOffset, fontNames, vectorOffsets, vectors);
      if (!flag) {
         break;
      }
   }
}



//////////////////////////////
//
// printSymbol --
//

int printSymbol(int index, int symbolOffset, char* fontNames,
      int* vectorOffsets, int* vectors) {
   int vectorStart = vectorOffsets[index] - 1;
   int vectorByteCount = vectorOffsets[index+1] - vectorOffsets[index];
   if (debugQ) {
      printf("@ VECTOR_BYTE_COUNT:\t%d = %d - %d\n", vectorByteCount,
         vectorOffsets[index+1], vectorOffsets[index]
      );
   }

   if (vectorByteCount == 0) {
      // ignore any symbols in the rest of the file
      return 0;
   }
   if (vectorByteCount < 0) {
      // ignore any symbols in the rest of the file
      return 0;
   }

   int symbolIndex = symbolOffset * 10 + index;
   char symbolName  [6] = {0};
   int i;
   strncpy(symbolName, &fontNames[index * 5], 5);
   for (i=4; i>0; i--) {
      if (symbolName[i] == ' ') {
         symbolName[i] = '\0';
      } else {
         break;
      }
   }

   if (vectorByteCount % 3 != 0) {
      printf("Error: vector byte count is not a multiple of 3: %d\n",
         vectorByteCount);
      printf("\tsymbol name: %s\n", symbolName);
      printf("\tfile index: %d\n", index);
      printf("\tsymbol index: %d\n", symbolIndex);
      exit(1);
   }

   printf("\n@@BEGIN: SYMBOL\n");
   printf("@LABEL:\t\t%s\n", symbolName);
   printf("@LIBINDEX:\t%d\n", symbolIndex);
   printf("@DEFINITION:\n");
   for (i=0; i<vectorByteCount; i+=3) {
      printf("\t%d %d %d\n",
            vectors[vectorStart + i],
            vectors[vectorStart + i + 1],
            vectors[vectorStart + i + 2]);
   }
   printf("@@END: SYMBOL\n\n");
   return 1;
}



//////////////////////////////
//
// getSymbolOffset -- read the last two characters of the filename to
//     determine the starting index of the symbol in the font library.
//     LIBRA.DRW starts with 0, where "RA" = 0
//     RB = 1 * 10;
//     RC = 2 * 10;
//     RD = 3 * 10;
//     RE = 4 * 10;
//     ...
//     RZ = 25 * 10;
//     SA = 26 * 10;
//     etc.
//

int getSymbolOffset(const char* filename) {
   int length = strlen(filename);
   char* extension = strrchr(filename, '.');
   int xlen = 0;
   if (extension != NULL) {
      xlen = strlen(extension);
   }
   int char1 = tolower(filename[length-2-xlen]) - 'r';
   int char2 = tolower(filename[length-1-xlen]) - 'a';
   return char1 * 26 + char2;
}



//////////////////////////////
//
// readChunk --
//

int readChunk(int* vectors, int* index, FILE* input) {
   int byteCount = readChar(input);
   int shortCount = byteCount / 2;
   int i;
   for (i=0; i<byteCount >> 1; i++) {
      vectors[*index] = readLittleShort(input);
      *index = *index + 1;
   }

   if (byteCount % 2) {
      int byteCount2 = readChar(input);
      if (byteCount != byteCount2) {
         printf("Error: byte counts for chunk do not match: %d, %d\n",
               byteCount, byteCount2);
      }
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// readChar -- Read a (two-byte) unsigned short at the current
//   read position in a file that is stored in the file in little-endian
//   ordering.
//

int readChar(FILE* input) {
   unsigned char byteinfo[1];
   if (fread((void*)byteinfo, sizeof(unsigned char), 1, input) == EOF) {
      printf("Error reading char: unexpected end of file\n");
      exit(1);
   }
   return byteinfo[0];
}



//////////////////////////////
//
// readLittleShort -- Read a (two-byte) unsigned short at the current
//   read position in a file that is stored in the file in little-endian
//   ordering.
//

int readLittleShort(FILE* input) {
   unsigned char byteinfo[2];
   if (fread((void*)byteinfo, sizeof(unsigned char), 2, input) == EOF) {
      printf("Error reading little-endian float: unexpected end of file\n");
      exit(1);
   }
   int output = 0;
   if (byteinfo[1] >= 0x80) {
      output = -1;
   }
   output = (output << 8) | byteinfo[1];
   output = (output << 8) | byteinfo[0];
   return output;
}


