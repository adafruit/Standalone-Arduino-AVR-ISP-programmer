#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include <avr/pgmspace.h>
#include "SPI.h"

#ifndef _OPTILOADER_H
#define _OPTILOADER_H

#define VERBOSE 0       // Debugging output?

#define FUSE_PROT 0			/* memory protection */
#define FUSE_LOW 1			/* Low fuse */
#define FUSE_HIGH 2			/* High fuse */
#define FUSE_EXT 3			/* Extended fuse */

#define LED_ERR 8
#define LED_PROGMODE A0

typedef struct image {
    char image_name[30];	       /* Ie "optiboot_diecimila.hex" */
    char image_chipname[12];	       /* ie "atmega168" */
    uint16_t image_chipsig;	       /* Low two bytes of signature */
    byte image_progfuses[5];	       /* fuses to set during programming */
    byte image_normfuses[5];	       /* fuses to set after programming */
    byte fusemask[4];
    uint16_t chipsize;
    byte image_pagesize;	       /* page size for flash programming */
    byte image_hexcode[19000];	       /* intel hex format image (text) */
} image_t;

typedef struct alias {
  char image_chipname[12];
  uint16_t image_chipsig;
  image_t * alias_image;
} alias_t;

// Useful message printing definitions

#define debug(string) // flashprint(PSTR(string));


void pulse (int pin, int times);
void flashprint (const char p[]);


uint16_t spi_transaction (uint8_t a, uint8_t b, uint8_t c, uint8_t d);
image_t *findImage (uint16_t signature);


uint16_t readSignature (void);
boolean programFuses (const byte *fuses);
void eraseChip(void);
boolean verifyImage (byte *hextext);
void busyWait(void);
boolean flashPage (byte *pagebuff, uint16_t pageaddr, uint8_t pagesize);
byte hexton (byte h);
byte * readImagePage (byte *hextext, uint16_t pageaddr, uint8_t pagesize, byte *page);
boolean verifyFuses (const byte *fuses, const byte *fusemask);
void error(const char *string);

#endif
