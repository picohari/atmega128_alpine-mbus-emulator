# atmega128_alpine-mbus-emulator

This is my working copy of collected sources, mainly the only well known and copied many times: http://www.hohensohn.info/mbus/

Still some bugs to come...

# Alpine M-Bus CD Changer Emulator for ATmega128 with LCD and UART

This firmware emulates a ALPINE M-BUS cd changer like the one I have in my car. As the AUDIO IN from the cd changer is only un-muted if the changer has responsed
 to the commands coming from the head unit, it is NOT possible to apply an external MP3 player or other signal from outside the radio :(

Thus the need for a little emulator project with our little friend, the ATmega128 from ATMEL and GCC! The sources for the decoding and encoding routines originates 
from the well known and ever copied since: http://www.hohensohn.info/mbus/

Some minor changes have been made to the state-machine based communication control and ISR receiving code.

## Installation

My avr-gcc -v gives:
Using built-in specs.
COLLECT_GCC=avr-gcc
COLLECT_LTO_WRAPPER=/usr/local/avr/libexec/gcc/avr/4.8.5/lto-wrapper
Target: avr
Configured with: ../configure --target=avr --prefix=/usr/local/avr --disable-nsl --enable-languages=c,c++ --disable-libssp
Thread model: single
gcc version 4.8.5 (GCC) 

Simply run the makefile with 'make'

## Usage

TODO: Write usage instructions and upload schematics

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
TODO: Write more credits

## License

See LICENCE.md file