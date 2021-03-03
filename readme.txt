Readme for converter.c program:
- Converting .pgm to .sk: In theory, converter.c converts any valid .pgm file to .sk, including files with different resolutions and maxvals. The program does not apply a lot of compression to the converted file. Program was only tested on bands.pgm and fractal.pgm.
- Converting .sk to .pgm: converter.c converts .sk files with intermediate sketch viewer functions. Non-vertical lines will not show up properly. Program converts blue channel to greyscale value and ignores alpha channel. 
