// Standalone AVR ISP programmer
// August 2011 by Limor Fried / Ladyada / Adafruit
// Jan 2011 by Bill Westfield ("WestfW")
//
// this sketch allows an Arduino to program a flash program
// into any AVR if you can fit the HEX file into program memory
// No computer is necessary. Two LEDs for status notification
// Press button to program a new chip. Piezo beeper for error/success 
// This is ideal for very fast mass-programming of chips!
//
// It is based on AVRISP
//
// using the following pins:
// 10: slave reset
// 11: MOSI
// 12: MISO
// 13: SCK
//  9: 8 MHz clock output - connect this to the XTAL1 pin of the AVR
//     if you want to program a chip that requires a crystal without
//     soldering a crystal in
// ----------------------------------------------------------------------


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
#define CLOCK 9     // self-generate 8mhz clock - handy!

#define BUTTON A1
#define PIEZOPIN A3

// Set to 1 to enable autostart. This automatically starts programming
// when a chip is inserted. How this detection happens is determined by
// the DETECT_* constants below. This is disabled by default, since the
// default settings require adding a pulldown (see below).
#define AUTOSTART 0

// Pin to use for detecting chip presence. This defaults to the
// (target) RESET pin, which detects the (internal) reset pullup on the
// target chip or board. For this to work, there should be a pulldown on
// the programmer side (which is big enough to not interfere with the
// target pullup). Alternatively, for self-powered boards, this could be
// set to an I/O pin that is connected to the targets VCC (still
// requires a pulldown on the programmer side).
#define DETECT RESET

// The DETECT pin level that indicates a chip is present.
#define DETECT_ACTIVE HIGH

// Debounce delay: The DETECT pin must be stable for this long before it
// is seen as changed. This applies both after removing and after
// re-inserting the chip.
#define DETECT_DEBOUNCE_MS 500

void setup () {
  Serial.begin(57600);			/* Initialize serial for status msgs */
  Serial.println("\nAdaBootLoader Bootstrap programmer (originally OptiLoader Bill Westfield (WestfW))");

  pinMode(PIEZOPIN, OUTPUT);

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

#if AUTOSTART != 0
bool detect_chip() {
  // The most recently read detection state (true is chip present, false is no
  // chip present). Default is to assume a chip is present, to prevent
  // autostarting on powerup, only after *inserting* a chip.
  static bool detected = true;
  // The most recent detection state that has been stable.
  static bool debounced = detected;
  // The timestamp of the most recent change, or 0 when the pin is
  // stable.
  static unsigned long last_change = 0;

  // See if the pin changed
  unsigned long now = millis();
  bool prev_detected = detected;
  detected = (digitalRead(DETECT) == DETECT_ACTIVE);

  if (detected != prev_detected)
    last_change = now;

  if (detected != debounced && (now - last_change) > DETECT_DEBOUNCE_MS) {
    // If stable for long enough, update the debounced state and clear
    // the last_change value. Keep the previous value, to detect changes
    last_change = 0;
    debounced = detected;

    // Detect when a chip is inserted
    if (debounced)
      return true;
  }

  // No change, or chip removed
  return false;
}
#endif

void loop (void) {
  #if AUTOSTART != 0
  Serial.println("\nInsert next chip to autostart or type 'G' or hit BUTTON to force start");
  #else
  Serial.println("\nType 'G' or hit BUTTON for next chip");
  #endif
  while (1) {
    #if AUTOSTART != 0
    if (detect_chip()) {
      Serial.println("Chip detected, autostarting..");
      break;
    }
    #endif
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

  Serial.println();
  Serial.println("Erasing chip");
  eraseChip();
  
  if (! programFuses(targetimage->image_progfuses))	// get fuses ready to program
    error("Programming Fuses fail");
  
  if (! verifyFuses(targetimage->image_progfuses, targetimage->fusemask) ) {
    error("Failed to verify fuses");
  } 

  Serial.println("Fuses set & verified");
  end_pmode();
  start_pmode();

  byte *hextext = targetimage->image_hexcode;  
  uint16_t pageaddr = 0;
  uint8_t pagesize = pgm_read_byte(&targetimage->image_pagesize);
  Serial.print("Page size: "); Serial.println(pagesize, DEC);
  uint16_t chipsize = pgm_read_word(&targetimage->chipsize);
  Serial.print("Chip size: "); Serial.println(chipsize, DEC);
  
  while (pageaddr < chipsize && hextext) {
     Serial.print("Writing address $"); Serial.println(pageaddr, HEX);
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

  delay(100);
  end_pmode();
  delay(100);
  start_pmode();
  delay(100);
  
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



void error(const char *string) {
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
  SPI.end();
  digitalWrite(MISO, LOW);		/* Make sure pullups are off too */
  pinMode(MISO, INPUT);
  digitalWrite(MOSI, LOW);
  pinMode(MOSI, INPUT);
  digitalWrite(SCK, LOW);
  pinMode(SCK, INPUT);
  digitalWrite(RESET, LOW);
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
  delay(100);
  Serial.print("Starting Program Mode");
  start_pmode();
  Serial.println(" [OK]");
  return true;
}

boolean target_poweroff ()
{
  end_pmode();
  digitalWrite(LED_PROGMODE, LOW);
  return true;
}





