# µCNC
µCNC - A universal CNC firmware for microcontrollers

## µCNC for AVR
µCNC for AVR can be built in a couple of ways

## Method one - Arduino IDE
Copy the boardmap_YOUR_BOARD.h , mcumap_avr.h, mcu_avr.c and uCNC.ino file to the parent µCNC folder is (where all µCNC core code is) and open the ino file with Arduino IDE.

The file [mcudefs.h](https://github.com/Paciente8159/uCNC/blob/master/uCNC/mcudefs.h) should be updated to reflect the changes in the path of the files
Example:
```
#include "mcus\avr\mcumap_avr.h"
```
should be changed to
```
#include "mcumap_avr.h"
```

Choose your target board and just compile and load. You're done.

## Method two - Using the makefile
First you must have AVR GCC tools installed on your computer.
You can download the latest version from [here](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers)
Then edit the [config.h](https://github.com/Paciente8159/uCNC/blob/master/uCNC/config.h) to match your board
```
#define BOARD BOARD_<YOUR_BOARD>
```
and the makefile to the correspondent MCU on your board
```
#define MCU = (choose the avr mcu name that matches the mcu on your board)
```
Run the makefile
```
make -f makefile clean all
```

In the build folder you will have your hex file to upload to your board.
