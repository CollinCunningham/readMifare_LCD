/**************************************************************************/
/*! 
    @file     readMifare_LCD.pde
    @author   Adafruit Industries
    @license  BSD (see license.txt)

    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:
   
    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)
    - Data in block 4 is displayed on 16x2 LCD Shiel
   
    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


    This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
    This library works with the Adafruit NFC breakout 
      ----> https://www.adafruit.com/products/364
 
    Check out the links above for our tutorials and wiring diagrams 
    These chips use I2C to communicate

    Adafruit invests time and resources providing this open source code, 
    please support Adafruit and open-source hardware by purchasing 
    products from Adafruit!
*/
/**************************************************************************/
#include <Wire.h>
#include <Adafruit_NFCShield_I2C.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

//For NFC Shield
#define IRQ   (6)
#define RESET (3)  // Not connected by default on the NFC Shield

//For LCD Shield
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
#define SENDSERIAL true
#define DEFAULTSTRING "awaiting tag ..."

Adafruit_NFCShield_I2C nfc(IRQ, RESET);
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

void setup(void) {
  
  if (SENDSERIAL){
    Serial.begin(115200);
    Serial.println("Hello!");
  }

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    if (SENDSERIAL) Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  if (SENDSERIAL){
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  }
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  if (SENDSERIAL) Serial.println("Waiting for an ISO14443A Card ...");
  //initialize LCD
  lcd.begin(16, 2);
  lcd.print(DEFAULTSTRING);
  lcd.setBacklight(WHITE);
}


void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  
  if (success) {
    // Display some basic information about the card
    
    if (SENDSERIAL){
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
      Serial.print("  UID Value: "); 
    }
    
    nfc.PrintHex(uid, uidLength);
    
    if (SENDSERIAL){
      Serial.println("");
    }
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
      if (SENDSERIAL){
        Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      }
      
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
    
      if (success)
      {
        if (SENDSERIAL) Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];
    
        // If you want to write something to block 4 to test with, uncomment
        // the following 2 lines and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'W', 'I', 'C', 'K', 'E', 'D', ' ', 'C', 'O', 'O', 'L', 0, 0, 0, 0, 0 }, sizeof data);
        //success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
    
        if (success)
        {
          // Data seems to have been read ... spit it out
          if (SENDSERIAL) Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          if (SENDSERIAL) Serial.println("");
          
          printLCD((const char*)data);
          
          // Wait a bit before reading the card again
          delay(1000);
          
        }
        else
        {
          if (SENDSERIAL) Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        if (SENDSERIAL) Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
    
    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      if (SENDSERIAL) Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
    
      // Try to read the first general-purpose user page (#4)
      if (SENDSERIAL) Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        
        printLCD((const char*)data);
        
        if (SENDSERIAL) Serial.println("");
    
        // Wait a bit before reading the card again
        delay(1000);
        
      }
      else
      {
        if (SENDSERIAL) Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }
  
  //wait, then reset the LCD
  delay(5000);
  printLCD(DEFAULTSTRING);
  
}


void printLCD(const char* text){
  
  clearLCD();
  lcd.print(text);
}


void clearLCD(){
 
  lcd.clear();
  lcd.setCursor(0,0);
}


void checkButtons(){
  
  if (SENDSERIAL) Serial.println("ARE THERE BUTTONS?");
 
  uint8_t buttons = lcd.readButtons();

  if (buttons) {
    
    if (SENDSERIAL) Serial.println("THERE ARE BUTTONS.");
    
    lcd.clear();
    lcd.setCursor(0,0);
    if (buttons & BUTTON_UP) {
      lcd.print("UP ");
      lcd.setBacklight(RED);
    }
    if (buttons & BUTTON_DOWN) {
      lcd.print("DOWN ");
      lcd.setBacklight(YELLOW);
    }
    if (buttons & BUTTON_LEFT) {
      lcd.print("LEFT ");
      lcd.setBacklight(GREEN);
    }
    if (buttons & BUTTON_RIGHT) {
      lcd.print("RIGHT ");
      lcd.setBacklight(TEAL);
    }
    if (buttons & BUTTON_SELECT) {
      lcd.print("SELECT ");
      lcd.setBacklight(VIOLET);
    }
  }
  
}

