// optiLoader.pde
// Jan 2011 by Bill Westfield ("WestfW")
//
// this sketch allows an Arduino to program Optiboot onto any other
// Arduino-like device containing ATmega8, ATmega168, or ATmega328
// microcontroller chips.
//
// It is based on AVRISP
//
// using the following pins:
// 10: slave reset
// 11: MOSI
// 12: MISO
// 13: SCK
//  9: Power to external chip.
//     This is a little questionable, since the power it is legal to draw
//     from a PIC pin is pretty close to the power consumption of an AVR
//     chip being programmed.  But it permits the target to be entirely
//     powered down for safe reconnection of the programmer to additional
//     targets, and it seems to work for most Arduinos.  If the target board
//     contains additional circuitry and is expected to draw more than 40mA,
//     connect the target power to a stronger source of +5V.

// ----------------------------------------------------------------------

// The following credits are from AVRISP.  It turns out that there isn't
// a lot of AVRISP left in this sketch, but probably if AVRISP had never
// existed,  this sketch would not have been written.
//
// October 2009 by David A. Mellis
// - Added support for the read signature command
// 
// February 2009 by Randall Bohn
// - Added support for writing to EEPROM (what took so long?)
// Windows users should consider WinAVR's avrdude instead of the
// avrdude included with Arduino software.
//
// January 2008 by Randall Bohn
// - Thanks to Amplificar for helping me with the STK500 protocol
// - The AVRISP/STK500 (mk I) protocol is used in the arduino bootloader
// - The SPI functions herein were developed for the AVR910_ARD programmer 
// - More information at http://code.google.com/p/mega-isp


#include "optiLoader.h"
#include "SPI.h"

// Global Variables
int pmode=0;
byte pageBuffer[128];		       /* One page of flash */


/*
 * Pins to target
 */
#define SCK 13
#define MISO 12
#define MOSI 11
#define RESET 10
#define CLOCK 9 // self-generate 8mhz clock - handy!

#define BUTTON A1
#define PIEZOPIN 2

void setup () {
  Serial.begin(9600);			/* Initialize serial for status msgs */
  Serial.println("\nAdaBootLoader Bootstrap programmer (originally OptiLoader Bill Westfield (WestfW))");

  pinMode(PIEZOPIN, OUTPUT);
  pinMode(PIEZOPIN2, OUTPUT);

  pinMode(LED_PROGMODE, OUTPUT);
  pulse(LED_PROGMODE,2);
  pinMode(LED_ERR, OUTPUT);
  pulse(LED_ERR, 2);

  pinMode(BUTTON, INPUT);     // button for next programming
  digitalWrite(BUTTON, HIGH); // pullup
  
  pinMode(CLOCK, OUTPUT);
  // setup high freq PWM on pin 9 (timer 1)
  // 50% duty cycle -> 8 MHz
  OCR1A = 0;
  ICR1 = 1;
  // OC1A output, fast PWM
  TCCR1A = _BV(WGM11) | _BV(COM1A1);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // no clock prescale
  
}

void loop (void) {
  Serial.println("\nType 'G' or hit BUTTON for next chip");
  while (1) {
    if  ((! digitalRead(BUTTON)) || (Serial.read() == 'G'))
      break;  
  }
    
  target_poweron();			/* Turn on target power */

  uint16_t signature;
  image_t *targetimage;
        
  if (! (signature = readSignature()))		// Figure out what kind of CPU
    error("Signature fail");
  if (! (targetimage = findImage(signature)))	// look for an image
    error("Image fail");
  
  eraseChip();

  if (! programFuses(targetimage->image_progfuses))	// get fuses ready to program
    error("Programming Fuses fail");
  
  if (! verifyFuses(targetimage->image_progfuses, targetimage->fusemask) ) {
    error("Failed to verify fuses");
  } 

  byte *hextext = targetimage->image_hexcode;  
  uint16_t pageaddr = 0;
  uint8_t pagesize = pgm_read_byte(&targetimage->image_pagesize);
  uint16_t chipsize = pgm_read_word(&targetimage->chipsize);
        
  //Serial.println(chipsize, DEC);
  while (pageaddr < chipsize) {
     byte *hextextpos = readImagePage (hextext, pageaddr, pagesize, pageBuffer);
          
     boolean blankpage = true;
     for (uint8_t i=0; i<pagesize; i++) {
       if (pageBuffer[i] != 0xFF) blankpage = false;
     }          
     if (! blankpage) {
       if (! flashPage(pageBuffer, pageaddr, pagesize))	
	 error("Flash programming failed");
     }
     hextext = hextextpos;
     pageaddr += pagesize;
  }
  
  // Set fuses to 'final' state
  if (! programFuses(targetimage->image_normfuses))
    error("Programming Fuses fail");
    
  end_pmode();
  start_pmode();
  Serial.println("\nVerifing flash...");
  if (! verifyImage(targetimage->image_hexcode) ) {
    error("Failed to verify chip");
  } else {
    Serial.println("\tFlash verified correctly!");
  }

  if (! verifyFuses(targetimage->image_normfuses, targetimage->fusemask) ) {
    error("Failed to verify fuses");
  } else {
    Serial.println("Fuses verified correctly!");
  }
  target_poweroff();			/* turn power off */
  tone(PIEZOPIN, 4000, 200);
}



void error(char *string) { 
  Serial.println(string); 
  digitalWrite(LED_ERR, HIGH);  
  while(1) {
    tone(PIEZOPIN, 4000, 500);
  }
}

void start_pmode () {
  pinMode(13, INPUT); // restore to default

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128); 
  
  debug("...spi_init done");
  // following delays may not work on all targets...
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  debug("...spi_transaction");
  spi_transaction(0xAC, 0x53, 0x00, 0x00);
  debug("...Done");
  pmode = 1;
}

void end_pmode () {
  SPCR = 0;				/* reset SPI */
  digitalWrite(MISO, 0);		/* Make sure pullups are off too */
  pinMode(MISO, INPUT);
  digitalWrite(MOSI, 0);
  pinMode(MOSI, INPUT);
  digitalWrite(SCK, 0);
  pinMode(SCK, INPUT);
  digitalWrite(RESET, 0);
  pinMode(RESET, INPUT);
  pmode = 0;
}


/*
 * target_poweron
 * begin programming
 */
boolean target_poweron ()
{
  pinMode(LED_PROGMODE, OUTPUT);
  digitalWrite(LED_PROGMODE, HIGH);
  digitalWrite(RESET, LOW);  // reset it right away.
  pinMode(RESET, OUTPUT);
  delay(200);
  Serial.print("Starting Program Mode");
  start_pmode();
  Serial.println(" [OK]");
  return true;
}

boolean target_poweroff ()
{
  end_pmode();
  digitalWrite(LED_PROGMODE, LOW);
  delay(200);
  return true;
}





