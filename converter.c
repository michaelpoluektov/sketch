#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Data structure holding the drawing state
typedef struct state { int x, y, tx, ty; unsigned char tool; unsigned int data, color;} state;

// Structure containing image width, height and maxval
typedef struct specs {int width, height, maxval;} specs;

// Operations
enum { DX = 0, DY = 1, TOOL = 2, // basic
       DATA = 3 // intermediate
     };

// Tool Types
enum { NONE = 0, LINE = 1, // basic
       BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5, // intermediate
     };

// Check if file name string is of .pgm format
bool isPgm(char *filename) { 
    char first = filename[strlen(filename)-4];
    char second = filename[strlen(filename)-3];
    char third = filename[strlen(filename)-2];
    char fourth = filename[strlen(filename)-1];
    return (first == '.' && second == 'p' && third == 'g' && fourth == 'm');
}

// Check if file name string is of .sk format
bool isSk(char *filename) {
    char first = filename[strlen(filename)-3];
    char second = filename[strlen(filename)-2];
    char third = filename[strlen(filename)-1];
    return (first == '.' && second == 's' && third == 'k');
}

// Check if character is a whitespace
bool isSpace(char c) {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f');
}

// Check if character is a digit
bool isDigit(char c) {
    return (c >= '0' && c <= '9');
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(unsigned char b) {
    int mostSignificantBits = b >> 6;
    if (mostSignificantBits == 0) {
        return DX;
    }
    else if (mostSignificantBits == 1) {
        return DY;
    }
    else if (mostSignificantBits == 2) {
        return TOOL;
    }
    else return DATA;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(unsigned char b) {
    unsigned char fiveOnes = 0x1F;
    int fiveLSBs = b & fiveOnes;
    int sixthLSB = 0;
    if ((b >> 5) & 1) sixthLSB = -32;
    return fiveLSBs + sixthLSB;
}

// Draw a vertical line on input matrix according to x, y, tx, ty and color values from state s
// Only draws vertical lines
void drawLine(unsigned char **pgmMatrix, state *s, int operand) {
    if (s->y <= s->ty) {
        for (int i = s->y; i <= s->ty; i++) {
            pgmMatrix[i][s->x] = (s->color >> 8) & 255;
        }
    } else {
        for (int i = s->y; i >= s->ty; i--) {
            pgmMatrix[i][s->x] = (s->color >> 8) & 255;
        }
    }
}

// Draw a block on input matrix according to values from state s
void drawBlock(unsigned char **pgmMatrix, state *s, int operand) {
    if (s->tx >= s->x) {
        for (int i = s->x; i <= s->tx; i++) {
            if (s->ty >= s->y) {
                for (int j = s->y; j <= s->ty; j++) {
                    pgmMatrix[j][i] = (s->color >> 8) & 255;
                }
            } else {
                for (int j = s->y; j >= s->ty; j--) {
                    pgmMatrix[j][i] = (s->color >> 8) & 255;
                }            
            }
        }
    } else {
        for (int i = s->x; i >= s->tx; i--) {
            if (s->ty >= s->y) {
                for (int j = s->y; j <= s->ty; j++) {
                    pgmMatrix[j][i] = (s->color >> 8) & 255;
                }
            } else {
                for (int j = s->y; j >= s->ty; j--) {
                    pgmMatrix[j][i] = (s->color >> 8) & 255;
                }            
            }
        }
    }
}

// Execute a byte of the command sequence.
void obey(unsigned char **pgmMatrix, state *s, unsigned char op) {
    int opcode = getOpcode(op);
    int operand = getOperand(op);
    int unsignedOperand = op & 63;
    if (opcode == DX) {
        s->tx += operand;
    } else if (opcode == DY) {
        s->ty += operand;
        if (s->tool == LINE) drawLine(pgmMatrix, s, operand);
        else if (s->tool == BLOCK) drawBlock(pgmMatrix, s, operand);
        s->x = s->tx;
        s->y = s->ty;
    } else if (opcode == TOOL) {
        if (operand == COLOUR) s->color = s->data;
        else if (operand == TARGETX) s->tx = s->data;
        else if (operand == TARGETY) s->ty = s->data;
        else s->tool = operand;
        s->data = 0;
    } else if (opcode == DATA) {
        s->data = s->data << 6;
        s->data = s->data | unsignedOperand;
    }
}

// Returns integer converted from string (assumes string contains only digits)
// Number starting with 0 does NOT mean hexadecimal, considered as valid decimal input
int decimalStringToInt(char *inputStr) {
    int outputInt = 0;
    int len = strlen(inputStr);
    int multiplier = 1;
    for (int i = 1; i <= len; i++) {
        if (isDigit(inputStr[len - i])) {
            outputInt += (inputStr[len-i]-'0')*multiplier;
            multiplier = multiplier * 10;
        } else {
            fprintf(stderr, "Error: invalid decimal number fed to decimalStringtoInt()\n");
            exit(1);
        }
    }
    return outputInt;
}

// Convert current word to integer
// Current word must be of size < 6 characters and only contain digits
int posToInt(FILE *pgmFile, char current) {
    // Extract value into string
    char stringValue[6];
    int valueDigit = 0;
    while (!isSpace(current)) {
        stringValue[valueDigit] = current;
        current = fgetc(pgmFile);
        valueDigit++;
    }
    stringValue[valueDigit] = '\0';
    // Convert string value to int
    int intValue = decimalStringToInt(stringValue);
    return intValue;
}

// Function that skips spaces in provided file
char skipSpace(FILE *pgmFile, char current) {
    current = fgetc(pgmFile);
    while (isSpace(current)) current = fgetc(pgmFile);
    return current;
}

// Convert grayscale value (0 to Maxval) to value from 0 to 255
unsigned char convertColor(int color, int maxval) {
    return (color * 255 / maxval);
}

// Writes commands to set the color to a grayscale value (0 to 255), requires 7 commands
void setColor(FILE *skFile, unsigned char color) {
    // All other color channels should be equal to grayscale color value
    unsigned long red = (long) color << 24;
    unsigned long rgbaColor = 255 + (color << 8) + (color << 16) + red;
    // Set opcode to DATA
    unsigned char opcode = 128 + 64;
    unsigned char operand = 0;
    // If color is 0 just set alpha channel to FF
    if (color == 0) {
        fputc(opcode+3, skFile);
        fputc(255, skFile);
    } else {
        fputc(opcode+((rgbaColor >> 30) & 3), skFile);
        for (int i = 0; i < 5; i++) {
            operand = (rgbaColor >> (24-(i*6))) & 63;
            fputc(opcode+operand, skFile);
        }
    }
    fputc(128+3, skFile);   
}

// Writes a DY command with char value (-32 to 31) as operand
void writeDY(FILE *skFile, char operand) {
    char opcode = 64;
    char negative = 0;
    if (operand < 0) negative = 32;
    char lastFive = negative + operand;
    fputc(opcode + negative + lastFive, skFile);
}

// Sets DATA field to provided input int (inout < 4096)
void setData(FILE *skFile, int input) {
    unsigned char opcode = 128 + 64;
    if (input >= 64) fputc(opcode + ((input >> 6) & 63), skFile);
    fputc(opcode + (input & 63), skFile);
}

// Create empty matrix
// This matrix can be accessed with matrix[x][y] and freed without having to call getSpecs()
unsigned char **allocateMatrix(int width, int height) {
    // Allocate memory for data
    unsigned char *data = malloc(sizeof(unsigned char) * width * height);
    // Create array of pointers to first element in each row
    unsigned char **pointerArray = malloc(sizeof(unsigned char*) * width);
    for (int i = 0; i < width; i++) {
        pointerArray[i] = data + (i*height);
    }
    return pointerArray;    
}

// Free data array first, then free array of pointers to rows
void freeMatrix(unsigned char **pointerToMatrix) {
    free(pointerToMatrix[0]);
    free(pointerToMatrix);
}

// Extract specs from file
specs getSpecs(FILE *pgmFile) {
    specs outputSpecs;
    // Check that first characters correspond to the "magic number"
    if (fgetc(pgmFile) != 'P' || fgetc(pgmFile) != '5') {
        fprintf(stderr, "Error: \"magic number\" of file doesn't check out!\n");
        exit(1);
    }
    // Assign width, height and maxval and skip to image data
    char current = '\0';
    current = skipSpace(pgmFile, current);
    outputSpecs.width = posToInt(pgmFile, current);
    current = skipSpace(pgmFile, current);
    outputSpecs.height = posToInt(pgmFile, current);
    current = skipSpace(pgmFile, current);
    outputSpecs.maxval = posToInt(pgmFile, current);
    return outputSpecs;
}

// Take a .pgm file and return a char matrix (width * height) with grayscale color values
unsigned char **pgmToMatrix(char *filename) {
    FILE *pgmFile = fopen(filename, "rb");
    // Extract specs
    specs pgmSpecs = getSpecs(pgmFile);
    char current = '\0';
    current = skipSpace(pgmFile, current);
    // oneByte is true iff the greyscale value is represented with 1 byte
    bool oneByte = true;
    int color = 0;
    if (pgmSpecs.maxval > 255) {
        oneByte = false;
    }
    // Allocate memory to 2 dimensional array of chars
    unsigned char **imageMatrix = allocateMatrix(pgmSpecs.width, pgmSpecs.height);
    // Assign values to matrix (if maxval is not 255 conversion may not be lossless)
    for (int i = 0; i < pgmSpecs.height; i++) {
        for (int j = 0; j < pgmSpecs.width; j++) {
            // Read byte by byte if only 1 byte is used
            if (oneByte) imageMatrix[j][i] = convertColor(current, pgmSpecs.maxval);
            else { // Otherwise read two 2 bytes
                color = current << 8;
                current = fgetc(pgmFile);
                color += current;
                imageMatrix[j][i] = convertColor(color, pgmSpecs.maxval);
            }
            current = fgetc(pgmFile);
        }
    }
    fclose(pgmFile);
    return imageMatrix;
}

// Convert provided file to .sk
void convertPgm(char *filename) {
    // Generate image matrix
    unsigned char **imageMatrix = pgmToMatrix(filename);
    // Get image width and height through getSpecs()
    FILE *pgmFile = fopen(filename, "rb");
    specs imageSpecs = getSpecs(pgmFile);
    fclose(pgmFile);
    // Open renamed file to write
    char *renamed = filename;
    renamed[strlen(filename)-3] = 's';
    renamed[strlen(filename)-2] = 'k';
    renamed[strlen(filename)-1] = '\0';
    FILE *skFile = fopen(renamed, "w+");
    // Write commands
    unsigned char currentColor;
    int x = 0, y = 0;
    while (x < imageSpecs.width) {
        // Execute TARGETY command, since no data commands were executed, this sets TY to 0
        fputc(128+5, skFile);
        y = 0;
        // Set TARGETX to x
        setData(skFile, x);
        fputc(128+4, skFile);
        // Switches tool to NONE, executes DY to set x and y to tx and ty respectively, resets tool to LINE
        fputc(128, skFile);
        fputc(64, skFile);
        fputc(128+1, skFile);
        // Draws x column of sketch file
        while (y < imageSpecs.height) {
            if (currentColor != imageMatrix[x][y]) {
                currentColor = imageMatrix[x][y];
                setColor(skFile, currentColor);
            }
            if (imageSpecs.height-1 == y) writeDY(skFile, 0);
            else writeDY(skFile, 1);
            y++;
        }
        x++;
    }
    // Close files, free memory
    fclose(skFile);
    freeMatrix(imageMatrix);
    return;
}

void writePgm(FILE *pgmFile, unsigned char **pgmMatrix) {
    fprintf(pgmFile, "P5 %d %d %d\n", 200, 200, 255);
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < 200; j++) {
            fputc(pgmMatrix[i][j], pgmFile);        
        }    
    }
}

// Allocate memory for a drawing state and initialise it
state *newState() {
    state *nState = malloc(sizeof(state));
    nState->x = 0;
    nState->y = 0;
    nState->tx = 0;
    nState->ty = 0;
    nState->tool = 1;
    nState->data = 0;
    nState->color = 0xFFFFFFFF;
    return nState;
}

// Release all memory associated with the drawing state
void freeState(state *s) {
    free(s);
}

// Process pgm matrix according to .sk file
void processMatrix(char *skFilename, unsigned char **pgmMatrix) {
    // Initialise all values as 0
    state *imageState = newState();
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < 200; j++) {
            pgmMatrix[i][j] = 0;        
        }    
    }
    FILE *skFile = fopen(skFilename, "rb");
    unsigned char command;
    while (!feof(skFile)) {
        command = fgetc(skFile);
        obey(pgmMatrix, imageState, command);
    }
    freeState(imageState);
    fclose(skFile);
}

// Convert .sk file to .pgm
void convertSk(char *filename) {
    // .sk files don't hold information about dimensions, default to 200x200
    unsigned char **pgmMatrix = allocateMatrix(200, 200);
    int len = strlen(filename);
    char convertedFilename[len+2];
    strcpy(convertedFilename, filename);
    convertedFilename[len-2] = 'p';
    convertedFilename[len-1] = 'g';
    convertedFilename[len] = 'm';
    convertedFilename[len+1] = '\0';
    FILE *pgmFile = fopen(convertedFilename, "w+");
    processMatrix(filename, pgmMatrix);
    writePgm(pgmFile, pgmMatrix);
    fclose(pgmFile);
    freeMatrix(pgmMatrix);
}

// A replacement for the library assert function.
void assert(int line, bool b) {
    if (b) return;
    printf("The test on line %d fails.\n", line);
    exit(1);
}

// Test isPgm()
void testIsPgm() {
    assert(__LINE__, isPgm("somefile.pgm") == true);
    assert(__LINE__, isPgm("anotherfile.docx") == false);
    assert(__LINE__, isPgm("i am annoying and i use spaces in my filenames.pgm") == true);
    assert(__LINE__, isPgm("uncompressMeFirstPlease.zip") == false);
}

// Test isSk()
void testIsSk() {
    assert(__LINE__, isSk("somefile.sk") == true);
    assert(__LINE__, isSk("anotherfile.docx") == false);
    assert(__LINE__, isSk("i am annoying and i use spaces in my filenames.sk") == true);
    assert(__LINE__, isSk("ive_done_enough_copy_pasting_for_now.melon") == false);
}

// Test decimalStringToInt(), if string contains non digits the function outputs it's own error.
void testDecimalStringToInt() {
    assert(__LINE__, decimalStringToInt("00123") == 123);
    assert(__LINE__, decimalStringToInt("666") == 666);
    assert(__LINE__, decimalStringToInt("6660") == 6660);
}

// Test convertColor()
void testConvertColor() {
    assert(__LINE__, convertColor(1, 1) == 255);
    assert(__LINE__, convertColor(6, 255) == 6);
    assert(__LINE__, convertColor(1, 3) == 85);
}

// Test getOpcode() (copied from test.c because it tests an identical function)
void testGetOpcode() {
    assert(__LINE__, getOpcode(0x80) == TOOL);
    assert(__LINE__, getOpcode(0x81) == TOOL);
    assert(__LINE__, getOpcode(0x40) == DY);
    assert(__LINE__, getOpcode(0x5F) == DY);
    assert(__LINE__, getOpcode(0x60) == DY);
    assert(__LINE__, getOpcode(0x7F) == DY);
    assert(__LINE__, getOpcode(0x00) == DX);
    assert(__LINE__, getOpcode(0x1F) == DX);
    assert(__LINE__, getOpcode(0x20) == DX);
    assert(__LINE__, getOpcode(0x3F) == DX);
}

// Test getOperand() (copied from test.c because it tests an identical function)
void testGetOperand() {
    assert(__LINE__, getOperand(0x00) == 0);
    assert(__LINE__, getOperand(0x1F) == 31);
    assert(__LINE__, getOperand(0x40) == 0);
    assert(__LINE__, getOperand(0x5F) == 31);
    assert(__LINE__, getOperand(0x20) == -32);
    assert(__LINE__, getOperand(0x3F) == -1);
    assert(__LINE__, getOperand(0x60) == -32);
    assert(__LINE__, getOperand(0x7F) == -1);
    assert(__LINE__, getOperand(0x80) == NONE);
    assert(__LINE__, getOperand(0x81) == LINE);
}

// Run tests
void test() { 
    testIsPgm();
    testIsSk();
    testDecimalStringToInt();
    testConvertColor();
    testGetOperand();
    testGetOpcode();
    printf("All tests passed.\n");
}

// Run program if 1 argument, test program if no arguments
int main(int n, char *args[n]) { 
    if (n == 1) test();
    else if (n == 2) {
        if (isPgm(args[1])) {
            convertPgm(args[1]);
            printf("File converted.");
        } else if (isSk(args[1])) {
            convertSk(args[1]);
            printf("File converted: Warning .sk to .pgm convertion is a work in progress:\n");
            printf("- Files containing non vertical lines will not render properly.\n");
            printf("- Only blue color channel will be used for RGBA to grayscale conversion.\n");
            printf("- Advanced sketch file fucntions will be ignored.\n");
        } else {
            fprintf(stderr, "Invalid file type, this program only supports .pgm and .sk files.\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Usage: ./converter filename\n");
        exit(1);
    }
}
