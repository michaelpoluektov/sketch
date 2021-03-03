// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allocate memory for a drawing state and initialise it
state *newState() {
    state *nState = malloc(sizeof(state));
    nState->x = 0;
    nState->y = 0;
    nState->tx = 0;
    nState->ty = 0;
    nState->tool = 1;
    nState->start = 0;
    nState->data = 0;
    nState->end = false;
    return nState;
}

// Release all memory associated with the drawing state
void freeState(state *s) {
    free(s);
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
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
int getOperand(byte b) {
    byte fiveOnes = 0x1F;
    int fiveLSBs = b & fiveOnes;
    int sixthLSB = 0;
    if ((b >> 5) & 1) sixthLSB = -32;
    return fiveLSBs + sixthLSB;
}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op) {
    int opcode = getOpcode(op);
    int operand = getOperand(op);
    int unsignedOperand = op & 63;
    if (opcode == DX) {
        s->tx += operand;
    } else if (opcode == DY) {
        s->ty += operand;
        if (s->tool == LINE) line(d, s->x, s->y, s->tx, s->ty);
        else if (s->tool == BLOCK) block(d, s->x, s->y, (s->tx)-(s->x), (s->ty)-(s->y));
        s->x = s->tx;
        s->y = s->ty;
    } else if (opcode == TOOL) {
        if (operand == COLOUR) colour(d, s->data);
        else if (operand == TARGETX) s->tx = s->data;
        else if (operand == TARGETY) s->ty = s->data;
        else if (operand == SHOW) show(d);
        else if (operand == PAUSE) pause(d, s->data);
        else if (operand == NEXTFRAME) show(d);
        else s->tool = operand;
        s->data = 0;
    } else if (opcode == DATA) {
        s->data = s->data << 6;
        s->data = s->data | unsignedOperand;
    }
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.
bool processSketch(display *d, void *data, const char pressedKey) {
    if (data == NULL) return (pressedKey == 27);
    char *filename = getName(d);
    FILE *sketchFile = fopen(filename, "rb");
    byte ch;
    state *s = (state*) data;
    for (int i = 0; i < s->start; i++) {
        ch = fgetc(sketchFile);
    }
    while (!feof(sketchFile)) {
        ch = fgetc(sketchFile);
        obey(d, s, ch);
        if (getOpcode(ch) == TOOL && getOperand(ch) == NEXTFRAME) {
            s->start++;
            s->end = false;
            s->data = 0;
            s->tool = 1;
            s->tx = 0;
            s->ty = 0;
            s->x = 0;
            s->y = 0;
            return (pressedKey == 27);
        }
        s->start++;
    }
    fclose(sketchFile);
    s->data = 0;
    s->x = 0;
    s->y = 0;
    s->tx = 0;
    s->ty = 0;
    s->tool = 1;
    s->start = 0;
    show(d);
    //TO DO: OPEN, PROCESS/DRAW A SKETCH FILE BYTE BY BYTE, THEN CLOSE IT
    //NOTE: CHECK DATA HAS BEEN INITIALISED... if (data == NULL) return (pressedKey == 27);
    //NOTE: TO GET ACCESS TO THE DRAWING STATE USE... state *s = (state*) data;
    //NOTE: TO GET THE FILENAME... char *filename = getName(d);
    //NOTE: DO NOT FORGET TO CALL show(d); AND TO RESET THE DRAWING STATE APART FROM
    //      THE 'START' FIELD AFTER CLOSING THE FILE

    return (pressedKey == 27);
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif
