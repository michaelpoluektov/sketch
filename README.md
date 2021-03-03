# Sketch Challenge
My submission to imperative programming coursework "Sketch Challenge"
# sketch.c (closed task)
Displays image encoded as a sketch file (.sk extension), supports all sketch files (basic to advanced)
# converter.c (open task, readme.txt written with word limit)
- Converting .pgm to .sk: In theory, converter.c converts any valid .pgm file to .sk, including files with different resolutions and maxvals. The program does not apply a lot of compression to the converted file. Program was only tested on bands.pgm and fractal.pgm.
- Converting .sk to .pgm: converter.c converts .sk files with intermediate sketch viewer functions. Non-vertical lines will not show up properly. Program converts blue channel to greyscale value and ignores alpha channel. 
# Sketch file description:

## Basic Sketch File

A Basic Sketch File contains a picture made up of white lines drawn on a black background. For simplicity we will stick with a fixed image size of exactly 200x200 pixels for this assignment. Sketch files are encoded as a sequence of single-byte commands, which your program will have to translate into calls to the display module. During drawing a basic sketch file, your program will have to keep track of a current drawing state, which is a data structure defined in sketch.h. This contains the current pixel location (x,y) in the window (which must be initialised as location (0,0) at the beginning of reading a sketch file), a pixel target location (tx,ty) (which must be initialised as (0,0) at the beginning of reading a sketch file), and the currently set tool type (which is initialised as LINE at the beginning of reading a sketch file). The other fields are not used by basic sketch files and should just be initialised to 0. For basic sketch files there are three possible single-byte commands: TOOL to switch active line drawing on or off, DX to move horizontally, and DY to move vertically and draw a line from current to target position if the tool is switched on. Two most significant bits of every command byte determine the opcode (i.e. which of the three commands to do) and the remaining six bits determine the operand (i.e. what data to do it with):

- TOOL: If the two most significant bits (or opcode) of a command byte are 1 (most significant bit) and 0, respectively, then the command is interpreted as the TOOL command. This command selects a tool and uses the remaining 6 bits as an unsigned integer operand, which indicates the type of tool. For a Basic Sketch File the tool type can either be NONE = 0 (switching all drawing off) or LINE = 1 (switching line drawing on).
- DX: If the two most significant bits of a command byte are both 0 then the command is interpreted as the DX command, that shifts the position of the target location (tx,ty) along the x direction by dx pixels. The operand dx is specified between -32 and 31 encoded as a two-complement integer via the remaining 6 bits of the command byte.
- DY: If the two most significant bits of a command byte are 0 (most significant bit) and 1, respectively, then the command is interpreted as the DY opcode, that is shifting the position of the target location (tx,ty) along the y direction by dy pixels. Then, and only if line drawing is switched on, a line is drawn from the current location (x,y) to the target location (tx,ty). Finally, the current location is set to the target location in any case. (If line drawing is switched off then nothing is drawn, but the current location is still changed to the target location). Note that dy is a value between -32 and 31 encoded as a two-complement integer via the remaining 6 bits of the byte.

## Intermediate Sketch File

One major limitation of the basic sketch file format is that each operand can only be 6bits long. As a consequence it would be difficult to stick to single-byte commands as required for all sketch files and yet provide commands for setting absolute locations for target positions (even when working with 200x200 pixel display windows), or for changing RGBA colour values that are 32bits long. The Intermediate Sketch File Format addresses this problem by starting to use the 'data' field in the drawing state and by introducing one more command, that is the opcode DATA:

- DATA: If the two most significant bits (or opcode) of a command byte are both 1, then the command is interpreted as DATA. This command shifts the bits of the 'data' field in the drawing state six positions to the left and replaces the six now empty bits with the six (rightmost) operand bits of the command.
In addition, after executing any TOOL command the 'data' field of the drawing state is set to 0. Together with the DATA command this allows to build up the 'data' value of the drawing state by reading an entire sequence of DATA commands. The Intermediate Sketch File Format also introduces new tool types for using the TOOL command:

- First, if the operand of a TOOL command is BLOCK=2 then the 'tool' field of the drawing state is set to BLOCK which indicates that the DY command should draw a filled box with the main diagonal ranging from (x,y) to (tx,ty). Thus, the 'tool' field of the drawing state can now have the values NONE=0, LINE=1, and BLOCK=2. Pay attention when implementing the behaviour because the block and line functions in the display module have different argument semantics.
- Secondly, if the operand of a TOOL command is COLOUR=3 then the 'tool' field is not changed, but the current drawing colour is updated by interpreting the 'data' field of the drawing state as the new unsigned 32bit RGBA integer value. Thus, this is a one-off tool that just changes the drawing colour using a built-up data value.
- Thirdly, if the operand of a TOOL command is TARGETX=4 then the 'tool' field is not changed, but the current 'tx' field in the drawing state is updated by interpreting the 'data' field of the drawing state as an absolute location in x direction. Thus, this is a one-off tool that just changes the target x location using a built-up data value.
Fourthly, if the operand of a TOOL command is TARGETY=5 then the 'tool' field is not changed, but the current 'ty' field in the drawing state is updated by interpreting the 'data' field of the drawing state as an absolute location in y direction. Thus, this is a one-off tool that just changes the target y location using a built-up data value.

## Advanced Sketch File

The final advanced version introduces three further tools:

- First, if the operand of a TOOL command is SHOW=6 then the 'tool' field of the drawing state is not changed and a single call to the show(...) function of the display module is made.
- Secondly, if the operand of a TOOL command is PAUSE=7 then the 'tool' field is not changed, but the pause(...) function of the display module is called using the 'data' field of the drawing state as representing the number of milliseconds to wait. Thus, this is a one-off tool that just causes the system to wait for a bit.
- Thirdly, if the operand of a TOOL command is NEXTFRAME=8 then the 'tool' field is not changed, but the current position in the sketch file (i.e. the number of the command starting with 0 for the first command in the file) is stored in the 'start' field and the processing of the file is immediately stopped (for instance by utilising the 'end' field of the drawing state as a sentinel variable that prevents reading the next byte). The so far drawn content is then shown on the display, and the processSketch(...) function returns. Note that the start field is the only field of the state that is not reset after closing the sketch file. It must be used to resume processing with the command following the NEXTFRAME TOOL command once the processSketch(...) function is called again. So instead of processing the sketch file from the start every time processSketch(...) is called, processing is started once in the middle of the file as indicated by the start field of the state. This allows to process the content of a sketch file frame-by-frame, animating its content. Use the last two tests in test.c to understand the exact call sequences to the display module that your code should produce.