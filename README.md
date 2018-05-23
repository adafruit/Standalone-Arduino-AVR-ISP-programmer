Standalone AVR programmer
=========================
This sketch can be used to let one Arduino program a sketch or
bootloader into another one, using the ISP programming pins.

This sketch is based on [OptiLoader][], but modified for use with
Adafruit's adaloader and to be used without a serial connection. It is
intended to be used with [Adafruit's standalone programmer kit][kit] (a
protoshield with ZIF socket, buttons, leds and a buzzer), but it should
be usable with any Arduino wired up to any AVR chip (respecting the
pinout, see below).

[OptiLoader]: https://github.com/WestfW/OptiLoader/
[kit]: https://www.adafruit.com/product/462

The sketch to be flashed is stored in the `images.cpp` file inside this
sketch. By default, it contains the adaLoader bootloader, but you can
replace it with your own bootloader or sketch as well. To do so,
compile that sketch into a .hex file (using the Arduino IDE, you can
use "Sketch" -> "Export compiled Binary" to get the compiled .hex file)
and paste its contents (it's just a text file) into `images.cpp` (see
that file for details). There you can also configure the target chip
signature and fuse settings.

For more info on setting this up, see [Adafruit's tutorial][tutorial].

[tutorial]: https://learn.adafruit.com/standalone-avr-chip-programmer/overview

Pinout
------
To use this sketch, you can use the following pinout. Here, "programmer"
means the Arduino running this particular sketch, "target" means the
chip being programmed. For reference, this also lists pin numbers for an
Arduino Uno programmer and bare atmega328p target chip, adapt these if
you use a different setup.

| Programmer pin | Uno pin number | Connects to
| -------------- | -------------- | --------------
| Digital pin 10 | 10             | Target RESET (atmega328p pin 1)
| MOSI           | 11             | Target MOSI (atmega328p pin 17)
| MISO           | 12             | Target MISO (atmega328p pin 18)
| SCK            | 13             | Target SCK (atmega328p pin 19)
| Digital pin 9  | 9              | Target XTAL1 (optional, atmega328p pin 9)
| Digital pin 8  | 8              | Error LED (active high)
| Analog pin 0   | A0             | Activity LED (active high)
| Analog pin 1   | A1             | Button (active low, internal pullup enabled)
| Analog pin 3   | A1             | Piezo (outputs 4kHz square wave)

Leds & buzzer
-------------
On startup, both leds blink twice and then turn off.

When the target is being programmed the activity led turns on. Once it
is done, the led turns off and the piezo makes a short beep.

When an error occurs during programming or verification, the error led
turns on and stays on and the the piezo makes a continuous beep.

Clock output
------------
This sketch generates an 8Mhz clock as an extra utility. This can be
used if the target chip has fuse settings that expect an external clock
(or crystal, that also seems to work). If you are only configuring the
fuses to use the internal oscillator, this is not needed.

Button
------
A single button can be connected, which can be used to start programming
the next chip (including the first). Alternatively, you can send a
command (uppercase `'G'`) through serial to start programming.

Autostart
---------
This sketch also supports autostarting the programming when a chip is
connected. This works by detecting the (internal or external) pullup on
the reset line. To make sure that the pin reads LOW when nothing is
connected, this needs a big pulldown on the ISP reset line (so between
digital pin 10 on the programmer and GND). It should be big enough to
not interfere with the target's reset pullup, so something like 1M
should be ok.

To enable autostart, add the pulldown resistor and set `AUTOSTART` to
`1` in the .ino file.

Now, when you insert a chip and the reset line is stable for a short
while, the programming will automatically start. If the programmer is
powered up or reset while a chip is inserted, it must be removed and
re-inserted to start programming (of course, you can still use the
button or serial as well).
