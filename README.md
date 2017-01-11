# Alpine M-Bus CD Changer Emulator for ATmega128 with LCD and UART

The ALPINE M-BUS is a protocol for the remote control of CD changers used in car audio systems. The purpose of this project is to emulate or pretend the presence of a CD changer to the head-unit to enable the external audio input, tweaking the buttons for other applications or showing data on the display.  

As the audio signal coming from the CD changer is only turned on (un-muted) by the amplifier if the CD changer has replied to the commands coming from the head-unit, it is NOT possible to connect an MP3 player or other auxiliary audio signal from outside the radio without interfacing the M-BUS controller. This is where our project comes into play here: The software decodes the M-BUS commands and emulates the functions of the CD changer/player by sending processed messages (frames) back to the head-unit.


## Features

* All major commands supported: playing, skipping, resuming, disk changing, repeat, scan, etc..  
* M-BUS protocol timings generated/measured by accurate 16bit timer
* Current status and information display on HD44780 LCD
* Debug output on UART/serial console
* Commented & cleaned code, hardware tested on 7525R head-unit
* For own mp3 players or just for fun...


## Installation

Get a local copy of the repository with `git clone https://github.com/picohari/atmega128_alpine-mbus-emulator.git` or simply download the [zip archive](https://github.com/picohari/atmega128_alpine-mbus-emulator/archive/master.zip).

Check your `avr-gcc -v`, it should be similar to mine:
```
Using built-in specs.
COLLECT_GCC=avr-gcc
COLLECT_LTO_WRAPPER=/usr/local/avr/libexec/gcc/avr/4.8.5/lto-wrapper
Target: avr
Configured with: ../configure --target=avr --prefix=/usr/local/avr --disable-nsl
--enable-languages=c,c++ --disable-libssp
Thread model: single
gcc version 4.8.5 (GCC)
```
Compile the code by running the makefile with `make`

On my board the external crystal oscillator has 16Mhz, the timing parameters in the code have been adjusted to match this value. Final tuning was made with logic analyzer.

To program the AVR and set fuses I prefer the [USBasp](http://www.fischl.de/usbasp/).

Run `avrdude -cusbasp -pm128 -Uflash:w:main.hex` to flash the firmware onto the controller.


# Implementation

The code was written for our little friend, the ATmega128 microprocessor from ATMEL and compiled with GCC! The sources for the decoding and encoding routines originates from the well known and ever copied since: http://www.hohensohn.info/mbus/

Some minor changes have been made to the state-machine based communication control routine and to the ISR receiving code, but the software is still in development...

Due to the different voltages between head-unit and the AVR controller, a very small piece of hardware is required to perform level adaptation. With my gear, the original schematic from hohensohn.info didn't work for me, so I took a different level shifting circuit.

## Protocol

The M-BUS is kind of pulse-width modulation: A logical "0" is transmitted as a 0.6ms long pulse and a logical "1" is about 1.8ms

![alt tag](https://raw.githubusercontent.com/picohari/atmega128_alpine-mbus-emulator/master/M-BUS_Adapter/m-bus_timing.png)

Here we can see the transmission of the PING command on the left side and the reply on the right side. The signals have been already inverted and converted to 5V for the controller pins.

![alt tag](https://raw.githubusercontent.com/picohari/atmega128_alpine-mbus-emulator/master/M-BUS_Adapter/logic.png)

We read:

|Address  |Command  |Checksum |
| :------:|:-------:|:-------:|
| 0 0 0 1 | 1 0 0 0 | 1 0 1 0 |
|0x01     | 0x08    |0x0A     |

and later the reply (self-decoding on the input line):

|Address  |Command  |Checksum |
| :------:|:-------:|:-------:|
| 1 0 0 1 | 1 0 0 0 | 0 0 1 0 |
|0x09     | 0x08    |0x02     |

Ping - Pong!


## Hardware

The voltage on the M-Bus is about 10V and drops off very quickly if the load is too high. During my tests, the solution with the Z-Diode didn't work, so I took two small signal NPN transistors to do the level shifting:

![alt tag](https://raw.githubusercontent.com/picohari/atmega128_alpine-mbus-emulator/master/M-BUS_Adapter/adapter.png)

The signals coming from the microcontroller are inverted: a logic "1" (5V) on the output pin pulls down LOW the line on the bus through Q1. A logic "0" will keep it HIGH by the internal pull-up resistor in the head unit.
If the bus line goes LOW and the beginning of a logic "1" is signalled by the falling edge, R1 pulls the level on the receiving pin to HIGH and the ISR is triggered.

On the AVR ATmeag128 pin PD4 is used as input for receiving packets (Input Capture Pin Timer 1: ICP1) and PD5 is configured as output pin to drive the bus line LOW.

![alt tag](https://raw.githubusercontent.com/picohari/atmega128_alpine-mbus-emulator/master/M-BUS_Adapter/board.png)

A simple breadboard setup is done in less than 10 minutes. I like very much the ET-BASE boards from ETT (http://www.ett.co.th/product/03000AVR.html) for development and this is how looks like:


## Software

Not much to say, look at the source code. The playing of a CD is emulated by Timer2. Timer1 capture reads the incoming packets and Timer0 is used for sending. I've refactored the original sources and removed this ugly-looking hungarian notation to get some cleaner plain C - it's still not yet completed and leaks further commenting, ...

I will do some more measurements on the timing and include screenshots of the logic analyzer. More to come.


## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D


## History

TODO: Write history


## Credits

http://www.hohensohn.info/mbus/


## License

See LICENCE.md file
