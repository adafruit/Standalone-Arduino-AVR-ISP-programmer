// Useful message printing definitions
#include "optiLoader.h"
/*
 * Low level support functions
 */

/*
 * flashprint
 * print a text string direct from flash memory to Serial
 */
void flashprint (const char p[])
{
    byte c;
    while (0 != (c = pgm_read_byte(p++))) {
	Serial.write(c);
    }
}

/*
 * hexton
 * Turn a Hex digit (0..9, A..F) into the equivalent binary value (0-16)
 */
byte hexton (byte h)
{
  if (h >= '0' && h <= '9')
    return(h - '0');
  if (h >= 'A' && h <= 'F')
    return((h - 'A') + 10);
  error("Bad hex digit!");
}

/*
 * pulse
 * turn a pin on and off a few times; indicates life via LED
 */
#define PTIME 30
void pulse (int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  } 
  while (times--);
}

