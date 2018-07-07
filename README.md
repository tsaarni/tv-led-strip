# Remote control for TV background light

## Overview

This is a small project implementing remote controllable background light for a TV.

The software has a set of themes that can be set at compile time. A theme consist of four colors, one per each corner of the TV. The software calculates linear gradient between the corners.

![Imgur](https://i.imgur.com/UBVnCq4l.jpg)


[SK6812](https://cdn-shop.adafruit.com/product-files/1138/SK6812+LED+datasheet+.pdf) programmable LEDs are used as a light source. These chips consist of red, green, blue and white LEDs.

![Imgur](https://i.imgur.com/1c0YrRfl.jpg?1)


[TSOP34838](https://www.vishay.com/docs/82489/tsop322.pdf) IR receiver is used to remotely control the selection of color theme and to set the overall brightness of the theme. The settings are written to EEPROM on Atmega328 microcontroller to persist the selection even when the power is off.

Here is a prototype of the project using Arduino Nano, connected to a small breadboard

![Imgur](https://i.imgur.com/LPQrCSql.jpg)

and here is second prototype using minimal ATmega328P configuration

![Imgur](https://i.imgur.com/YPat3FHl.jpg)


## Schematic

[![tv-led-strip by tero.saarni@gmail.com 4fbfcfbe96263ceb - Upverter](https://upverter.com/tero.saarni@gmail.com/4fbfcfbe96263ceb/tv-led-strip/embed_img/15309497480000/)](https://upverter.com/tero.saarni@gmail.com/4fbfcfbe96263ceb/tv-led-strip/#/)

## Dependencies

[PlatformIO](https://platformio.org/) is used to build the project.

Following libraries are used:

* [light_ws2812](https://github.com/cpldcpu/light_ws2812) to control the LEDs
* [IRLib2](https://github.com/cyborg5/IRLib2) to receive remote control codes


## Compiling and testing on Arduino development board


Execute following command to compile the program and upload it to Arduino Nano:

    platformio run -e nanoatmega328

Execute following command to follow the debug logs:

    platformio serialports monitor -b 115200


## Compiling for standalone ATmega328P

This example uses USBasp programmer over ICSP (in-circuit serial programming).

First check the connectivity by reading default fuse values from the microcontroller.  It should respond with `Fuses OK (E:FF, H:D9, L:62)`.

    avrdude -patmega328p -c usbasp


Next program the microcontroller to use external 16MHz oscillator:

    avrdude -patmega328p -c usbasp -U lfuse:w:0xFF:m -U hfuse:w:0xDE:m -U efuse:w:0xFD:m


Compile and flash the program by runnig command:

    platformio run -e 328p16m
