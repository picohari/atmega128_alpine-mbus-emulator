# Alpine M-Bus CD Changer Emulator for ATmega128 with LCD and UART

This firmware emulates an ALPINE M-BUS cd changer, like the one I have in my car. As the AUDIO IN from the cd changer is only un-muted if the changer has responsed
 to the commands coming from the head unit, it is NOT possible to apply an external MP3 player or other signal from outside the radio :(

Thus the need for a little emulator project with our little friend, the ATmega128 from ATMEL and GCC! The sources for the decoding and encoding routines originates 
from the well known and ever copied since: http://www.hohensohn.info/mbus/

Some minor changes have been made to the state-machine based communication control and ISR receiving code.

## Installation

My `avr-gcc -v` gives:
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
Simply run the makefile with `make`

After this, run `avrdude -cusbasp -pm128 -Uflash:w:main.hex` to flash the firmware onto the controller. On my board the external oscillator has 16Mhz, the timing parameters in the code have been adjusted to this value. To program the AVR and set fuses I prefer the [USBasp](http://www.fischl.de/usbasp/).

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