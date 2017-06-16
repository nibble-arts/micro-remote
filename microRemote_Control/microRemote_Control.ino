/*
* microRemote controller for Blackmagic Micro Cinema Camera
* v0.2
* 2016-12-16 by Thomas H Winkler
* <thomas.winkler@iggmp.net>
* 
* using: 
*   RF24 library by J. Coliz <maniacbug@ymail.com>
*   ADAFRUIT OLED SSD1306 library by Adafruit <http://www.adafruit.com>
*   BMC_SBUS library by Stu Aitken
*/

/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/
    

#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>

/***************************************************
 * RF24 definitions
 */
#include <RF24.h>
//#include <printf.h>

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9,8);

byte addresses[][6] = {"1Node","2Node"};


/***************************************************
 * S-Bus definitions
 */
#include <BMC_SBUS.h>

#define RECchannel 1
#define IRISchannel 2
#define FOCUSchannel 3
#define ZOOMchannel 4
#define ISOchannel 5
#define SECTORchannel 6
#define COLORchannel 7
#define AUDIOchannel 8

#define sbusMID 1024   //neutral val
#define sbusLOW 0      //low switch val
#define sbusHIGH 2000   //high switch val

#define sbusWAIT 4      //frame timing delay in msecs

BMC_SBUS mySBUS;


/***************************************************
 * OLED definitions
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);



/***************************************************
 * navigation type
 *    SELECT = selected parameter
 *    UPDOWN = digital navigation with up and down function
 *    VALUE = analog value
 */
#define NONE 0
#define SELECT 1
#define UPDOWN 2
#define VALUE 3

#define stepWait 100


/***************************************************
 * button press type
 *    PUSH = push for less than 1 second
 *    LONGPUSH = push for more than 1,5 seconds
 */
#define PUSH 1
#define LONGPUSH -1


/***************************************************
 * IO definitions
 *    BUTTON = wheel pushbutton
 *    STEP  = wheel step A
 *    DIR  = wheel step B
 *    REC    = record button
 *    
 *    CABLE  = cable connector (low if connected to camera = no RF)
 */
#define REC 4
#define TALLY 18 /* A0 */

#define BUTTON 5
#define DIR 6
#define STEP 7

#define CABLE 10


/***************************************************
 * wheel delta value
 */
int delta;
double wheelTime;
byte wheelSpeed;
unsigned int wheelCnt;



/***************************************************
 * ICON definitions
 */
const unsigned char PROGMEM focus_48 [] = {
  0x00, 0x38, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x1C, 0x00, 0x01, 0xFF, 0x80, 0x00, 0x66, 0x80, 0x01,
  0xFF, 0x80, 0x00, 0xEE, 0x00, 0x03, 0xFF, 0xC0, 0x03, 0xFF, 0x80, 0x03, 0xFF, 0xC0, 0x0C, 0xFE,
  0xE0, 0x03, 0xFF, 0xC0, 0x0D, 0xFE, 0x60, 0x03, 0xFF, 0xC0, 0x0B, 0xFF, 0xC0, 0x03, 0xFF, 0xC0,
  0x03, 0xFF, 0x80, 0x03, 0xFF, 0xC0, 0x03, 0x7F, 0x80, 0x01, 0xFF, 0x80, 0x03, 0xFF, 0x80, 0x01,
  0xFF, 0x80, 0x01, 0xFE, 0x80, 0x00, 0x7E, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x02, 0xFF, 0x00, 0x01, 0xFF, 0x80,
  0x07, 0xFF, 0x80, 0x03, 0xFF, 0xC0, 0x03, 0xFF, 0xC0, 0x07, 0xFF, 0xE0, 0x03, 0xFF, 0xE0, 0x07,
  0xFF, 0xE0, 0x0B, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xA0, 0x0F, 0xFF, 0xF0, 0x1F, 0xFF,
  0x80, 0x0F, 0xFF, 0xF0, 0x3B, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x33, 0xFF, 0xE8, 0x0F, 0xFF, 0xF0,
  0x2F, 0xFF, 0x80, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xC0, 0x0F, 0xFF, 0xF0, 0x2F, 0xFF, 0xE0, 0x0F,
  0xFF, 0xF0, 0x1F, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x03, 0xFF, 0xE8, 0x0F, 0xFF, 0xF0, 0x1F, 0xFF,
  0x80, 0x0F, 0xFF, 0xF0, 0x0F, 0xFF, 0xA0, 0x0F, 0xFF, 0xF0, 0x0E, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0,
  0x0C, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x05, 0xFC, 0xE0, 0x0F, 0xFF, 0xF0, 0x00, 0xFA, 0x60, 0x00,
  0xFF, 0x00, 0x00, 0x7E, 0x40, 0x00, 0xFF, 0x00, 0x00, 0x7F, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFE,
  0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x02, 0xFD, 0x80, 0x00, 0xFF, 0x00,
  0x03, 0xFE, 0x00, 0x00, 0xFF, 0x00, 0x03, 0x7E, 0x00, 0x00, 0xFF, 0x00, 0x01, 0xFF, 0x80, 0x00,
  0xFF, 0x00, 0x00, 0xFE, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xD9, 0x80, 0x00, 0xFF, 0x00, 0x00, 0x9D,
  0x80, 0x00, 0xFF, 0x00, 0x00, 0x70, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x60, 0x00, 0x00, 0xFF, 0x00
};

const unsigned char PROGMEM iris_48 [] = {
  0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF,
  0xE0, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x3F, 0xF1, 0x0F, 0xFC, 0x00, 0x00, 0x7F,
  0x01, 0x00, 0xFE, 0x00, 0x00, 0xFC, 0x02, 0x00, 0x3F, 0x00, 0x01, 0xF8, 0x02, 0x00, 0x1F, 0x80,
  0x03, 0xE0, 0x04, 0x00, 0x07, 0xC0, 0x07, 0xC0, 0x04, 0x00, 0x07, 0xE0, 0x0F, 0xA0, 0x04, 0x00,
  0x19, 0xF0, 0x0F, 0x20, 0x04, 0x00, 0x60, 0xF0, 0x1F, 0x10, 0x04, 0x00, 0x80, 0xF8, 0x3E, 0x10,
  0x04, 0x01, 0x00, 0x7C, 0x3C, 0x08, 0x02, 0x02, 0x00, 0x3C, 0x3C, 0x04, 0x02, 0x02, 0x00, 0x3C,
  0x78, 0x03, 0x01, 0x04, 0x00, 0x1E, 0x78, 0x00, 0xC1, 0x04, 0x00, 0x1E, 0x78, 0x00, 0x23, 0xC8,
  0x00, 0x1E, 0xF8, 0x00, 0x1F, 0xF0, 0x00, 0x1F, 0xF0, 0x00, 0x1C, 0x38, 0x00, 0x0F, 0xF0, 0x00,
  0x18, 0x18, 0x3F, 0x0F, 0xF0, 0x00, 0x30, 0x0C, 0xC0, 0xCF, 0xF0, 0x00, 0x30, 0x0F, 0x00, 0x3F,
  0xFC, 0x00, 0xF0, 0x0C, 0x00, 0x0F, 0xF3, 0x03, 0x30, 0x0C, 0x00, 0x0F, 0xF0, 0xFC, 0x18, 0x18,
  0x00, 0x0F, 0xF0, 0x00, 0x1C, 0x38, 0x00, 0x0F, 0xF8, 0x00, 0x1F, 0xF8, 0x00, 0x1F, 0x78, 0x00,
  0x23, 0xC4, 0x00, 0x1E, 0x78, 0x00, 0x40, 0x83, 0x00, 0x1E, 0x78, 0x00, 0x40, 0x80, 0xC0, 0x1E,
  0x3C, 0x00, 0x80, 0x40, 0x20, 0x3C, 0x3C, 0x00, 0x80, 0x40, 0x10, 0x3C, 0x3E, 0x01, 0x00, 0x20,
  0x08, 0x7C, 0x1F, 0x02, 0x00, 0x20, 0x08, 0xF8, 0x0F, 0x0C, 0x00, 0x20, 0x04, 0xF0, 0x0F, 0xB0,
  0x00, 0x20, 0x05, 0xF0, 0x07, 0xC0, 0x00, 0x20, 0x03, 0xE0, 0x03, 0xE0, 0x00, 0x20, 0x07, 0xC0,
  0x01, 0xF8, 0x00, 0x40, 0x1F, 0x80, 0x00, 0xFC, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x7F, 0x00, 0x80,
  0xFE, 0x00, 0x00, 0x3F, 0xF0, 0x8F, 0xFC, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x07,
  0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00
};

const unsigned char PROGMEM zoom_48 [] = {
  0x00, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1F,
  0xF8, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00,
  0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00,
  0x00, 0x07, 0x80, 0x3F, 0xFC, 0x00, 0x00, 0x0F, 0xC0, 0x1F, 0xF8, 0x00, 0x00, 0x0F, 0xC0, 0x0F,
  0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x03, 0xC0, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x07,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00,
  0x00, 0x0F, 0xC0, 0x3F, 0xFC, 0x00, 0x00, 0x1F, 0xE0, 0x7F, 0xFE, 0x00, 0x00, 0x1F, 0xE0, 0x7F,
  0xFE, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F,
  0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00,
  0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF,
  0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F, 0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x3F,
  0xF0, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC0, 0xFF, 0xFF, 0x00,
  0x00, 0x0F, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC0, 0x0F,
  0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xF0, 0x00, 0x00, 0x0F,
  0xC0, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
  0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00
};

const unsigned char PROGMEM audio_48 [] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0x00, 0xC0, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xC0, 0x00, 0x0F, 0xF0, 0x00, 0xC0, 0xC0,
  0xC0, 0x0F, 0xF0, 0x00, 0xC0, 0xC0, 0xC0, 0x0F, 0xF0, 0x00, 0x60, 0xC1, 0x80, 0x0F, 0xF0, 0x00,
  0x60, 0xC1, 0x81, 0x8F, 0xF0, 0xC0, 0x30, 0x03, 0x03, 0x8F, 0xF1, 0xE0, 0x30, 0x03, 0x07, 0x0F,
  0xF0, 0xF0, 0x00, 0x00, 0x0E, 0x0F, 0xF0, 0x78, 0x00, 0x00, 0x0C, 0x0F, 0xF0, 0x3C, 0x00, 0x00,
  0x00, 0x0F, 0xF0, 0x1E, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x07,
  0x80, 0x00, 0x00, 0x0F, 0xF0, 0x03, 0xC0, 0x00, 0x00, 0x0F, 0xF0, 0x01, 0xE0, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0xF0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x78, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x3C, 0x00,
  0x00, 0x0F, 0xF0, 0x00, 0x1E, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
  0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00,
  0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
  0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

const unsigned char PROGMEM iso_48 [] = {
  0x3C, 0x00, 0xF0, 0x00, 0x3F, 0xC0, 0x3C, 0x03, 0xFC, 0x00, 0x7F, 0xE0, 0x3C, 0x07, 0xFE, 0x00,
  0xFF, 0xF0, 0x3C, 0x0F, 0xFF, 0x01, 0xFF, 0xF8, 0x3C, 0x1F, 0x0F, 0x03, 0xF0, 0xFC, 0x3C, 0x1E,
  0x07, 0x83, 0xE0, 0x7C, 0x3C, 0x3E, 0x07, 0x83, 0xC0, 0x3C, 0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C,
  0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x3C, 0x00, 0x03,
  0xC0, 0x3C, 0x3C, 0x3C, 0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x3C, 0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x3C,
  0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x3E, 0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x3E, 0x00, 0x03, 0xC0, 0x3C,
  0x3C, 0x3F, 0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x1F, 0x00, 0x03, 0xC0, 0x3C, 0x3C, 0x1F, 0x80, 0x03,
  0xC0, 0x3C, 0x3C, 0x0F, 0x80, 0x03, 0xC0, 0x3C, 0x3C, 0x0F, 0xC0, 0x03, 0xC0, 0x3C, 0x3C, 0x07,
  0xE0, 0x03, 0xC0, 0x3C, 0x3C, 0x03, 0xF0, 0x03, 0xC0, 0x3C, 0x3C, 0x01, 0xF8, 0x03, 0xC0, 0x3C,
  0x3C, 0x00, 0xFC, 0x03, 0xC0, 0x3C, 0x3C, 0x00, 0x7E, 0x03, 0xC0, 0x3C, 0x3C, 0x00, 0x3F, 0x03,
  0xC0, 0x3C, 0x3C, 0x00, 0x1F, 0x03, 0xC0, 0x3C, 0x3C, 0x00, 0x1F, 0x83, 0xC0, 0x3C, 0x3C, 0x00,
  0x0F, 0x83, 0xC0, 0x3C, 0x3C, 0x00, 0x07, 0xC3, 0xC0, 0x3C, 0x3C, 0x00, 0x07, 0xC3, 0xC0, 0x3C,
  0x3C, 0x00, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x00, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x00, 0x03, 0xC3,
  0xC0, 0x3C, 0x3C, 0x00, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x00, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x00,
  0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x00, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C,
  0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x3C, 0x03, 0xC3, 0xC0, 0x3C, 0x3C, 0x1E, 0x07, 0x83,
  0xE0, 0x7C, 0x3C, 0x1F, 0x0F, 0x83, 0xF0, 0xFC, 0x3C, 0x0F, 0xFF, 0x01, 0xFF, 0xF8, 0x3C, 0x07,
  0xFE, 0x00, 0xFF, 0xF0, 0x3C, 0x03, 0xFC, 0x00, 0x7F, 0xE0, 0x3C, 0x00, 0xF0, 0x00, 0x3F, 0xC0
};

const unsigned char PROGMEM sector_48 [] = {
  0x00, 0x00, 0x1F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xC0,
  0x00, 0x00, 0x00, 0x0F, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x3F, 0xF3, 0xC0, 0x00, 0x00, 0x00, 0x7F,
  0x03, 0xC0, 0x00, 0x00, 0x00, 0xFC, 0x03, 0xC0, 0x00, 0x00, 0x01, 0xF8, 0x03, 0xC0, 0x00, 0x00,
  0x03, 0xE0, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xC0, 0x03, 0xC0, 0x00, 0x00, 0x0F, 0x80, 0x03, 0xC0,
  0x00, 0x00, 0x0F, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x1F, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x3E, 0x00,
  0x03, 0xC0, 0x00, 0x00, 0x3C, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x3C, 0x00, 0x03, 0xC0, 0x00, 0x00,
  0x78, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x78, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x78, 0x00, 0x03, 0xC0,
  0x00, 0x00, 0xF8, 0x00, 0x03, 0xC0, 0x00, 0x00, 0xF0, 0x00, 0x03, 0xC0, 0x00, 0x00, 0xF0, 0x00,
  0x07, 0xE0, 0x00, 0x00, 0xF0, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x0F, 0xF8, 0x00, 0x00,
  0xF0, 0x00, 0x0F, 0xFC, 0x00, 0x00, 0xF0, 0x00, 0x0F, 0xFE, 0x00, 0x00, 0xF0, 0x00, 0x07, 0xFF,
  0x00, 0x00, 0xF0, 0x00, 0x01, 0x8F, 0x80, 0x00, 0xF8, 0x00, 0x01, 0x87, 0xC0, 0x00, 0x78, 0x00,
  0x01, 0x83, 0xE0, 0x00, 0x78, 0x00, 0x01, 0x81, 0xF0, 0x00, 0x78, 0x00, 0x01, 0x80, 0xF8, 0x00,
  0x3C, 0x00, 0x01, 0x80, 0x7C, 0x00, 0x3C, 0x00, 0x01, 0x80, 0x3E, 0x00, 0x3E, 0x00, 0x01, 0x80,
  0x1F, 0x00, 0x1F, 0x00, 0x01, 0x80, 0x0F, 0x80, 0x0F, 0x00, 0x01, 0x80, 0x07, 0xC0, 0x0F, 0x80,
  0x01, 0x80, 0x03, 0xE0, 0x07, 0xC0, 0x01, 0x80, 0x03, 0xE0, 0x03, 0xE0, 0x01, 0x80, 0x07, 0xC0,
  0x01, 0xF8, 0x01, 0x80, 0x1F, 0x80, 0x00, 0xFC, 0x01, 0x80, 0x3F, 0x00, 0x00, 0x7F, 0x01, 0x80,
  0xFE, 0x00, 0x00, 0x3F, 0xF1, 0x8F, 0xFC, 0x00, 0x00, 0x0F, 0xF9, 0xFF, 0xF0, 0x00, 0x00, 0x07,
  0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00
};

const unsigned char PROGMEM color_48 [] = {
  0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFC, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x03, 0xF0, 0x1F, 0x80, 0x00, 0x00, 0x07, 0xC0, 0x07, 0xC0, 0x00, 0x00, 0x0F,
  0x00, 0x01, 0xE0, 0x00, 0x00, 0x1E, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x78, 0x00,
  0x00, 0x38, 0x00, 0x00, 0x38, 0x00, 0x00, 0x78, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x70, 0x00, 0x00,
  0x1C, 0x00, 0x00, 0x70, 0x00, 0x00, 0x1C, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x0E, 0x00, 0x00, 0xE0,
  0x00, 0x00, 0x0E, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x0E, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x0E, 0x00,
  0x00, 0xEF, 0xE0, 0x07, 0xFE, 0x00, 0x00, 0xFF, 0xFC, 0x3F, 0xFE, 0x00, 0x01, 0xFF, 0xFF, 0xFF,
  0xFF, 0x80, 0x03, 0xF0, 0x1F, 0xF8, 0x0F, 0xC0, 0x07, 0xF0, 0x07, 0xE0, 0x1F, 0xE0, 0x0F, 0x70,
  0x07, 0xE0, 0x1C, 0xF0, 0x1E, 0x78, 0x0F, 0xF0, 0x3C, 0x78, 0x3C, 0x38, 0x1E, 0x78, 0x38, 0x3C,
  0x38, 0x3C, 0x1C, 0x38, 0x78, 0x1C, 0x78, 0x1E, 0x3C, 0x3C, 0xF0, 0x1E, 0x70, 0x0F, 0x38, 0x1D,
  0xE0, 0x0E, 0x70, 0x07, 0xF8, 0x1F, 0xC0, 0x0E, 0xE0, 0x03, 0xF0, 0x1F, 0x80, 0x07, 0xE0, 0x01,
  0xFF, 0xFF, 0x00, 0x07, 0xE0, 0x00, 0x7F, 0xFE, 0x00, 0x07, 0xE0, 0x00, 0x7F, 0xEE, 0x00, 0x07,
  0xE0, 0x00, 0x70, 0x0E, 0x00, 0x07, 0xE0, 0x00, 0x70, 0x0E, 0x00, 0x07, 0xE0, 0x00, 0x70, 0x0E,
  0x00, 0x07, 0xE0, 0x00, 0x70, 0x0E, 0x00, 0x07, 0x70, 0x00, 0x38, 0x1C, 0x00, 0x0E, 0x70, 0x00,
  0x38, 0x1C, 0x00, 0x0E, 0x78, 0x00, 0x3C, 0x3C, 0x00, 0x1E, 0x38, 0x00, 0x1C, 0x38, 0x00, 0x1C,
  0x3C, 0x00, 0x1E, 0x78, 0x00, 0x3C, 0x1E, 0x00, 0x0F, 0xF0, 0x00, 0x78, 0x0F, 0x00, 0x07, 0xE0,
  0x00, 0xF0, 0x07, 0xC0, 0x07, 0xE0, 0x03, 0xE0, 0x03, 0xF0, 0x1F, 0xF8, 0x0F, 0xC0, 0x01, 0xFF,
  0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x7F, 0xFC, 0x3F, 0xFE, 0x00, 0x00, 0x0F, 0xE0, 0x07, 0xF0, 0x00
};


/***************************************************
 * mode = select/value/off
 *    false: select mode
 *    true: value mode
 *    
 * select = parameter selection
 *    0: focus
 *    1: iris
 *    2: zoom
 *    3: audio (level)
 *    4: iso
 *    5: sector (shutter angle)
 *    6: color (white balance)
 *    
 * edit = edit modus
 * recort = record modus
 * rf = wireless status
 *    0: no RF
 *    1: RF mode but not connected
 *    2: RF connected
  */
bool mode = false;
byte select = 0;
bool edit = false;
bool record = false;
byte rf = 0;

byte stat;
byte store;

byte lastMode = mode;
byte lastSelect = select;

#define SELMODE 0
#define VALMODE 1
#define RECSELMODE 2
#define RECVALMODE 3
#define RECRUNMODE 4
#define SAVEMODE 5


/***************************************************
 * nr[] = number of saved values for each analog parameter
 *    0: no value
 *    1-3: count of values
 *    
 * save[parameter][nr] = stored values for each analog parameter
 *    each parameter can hold up to 3 values
 *    the maximal count of valid values is stored in nr[]
 *    
 * current = position of used stored parameter
 *    1-3
 */
int nr[4]; // saved values for the analog parameters
int save[4][3]; // saved values
byte current;
byte menuCount;
String menuItems[7];


/***************************************************
 * value = values of the parameters
 */
int value[8];






/*************************************************************************************
/*************************************************************************************
 * setup routine
 *************************************************************************************
 ************************************************************************************/
void setup()   {                
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  display.display();
  // Clear the buffer.
  display.clearDisplay();

  /* display start screen */
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(28,6);
  display.print("micro");

  display.setCursor(28,20);
  display.print("remote");
  
  display.setTextSize(1);
  display.setCursor(28,44);
  display.print("version 1.1");

  display.display();

  delay (2000);
  display.clearDisplay();


  /* init stati */
  mode = false; // select mode
  select = 0; // selected value
  record = false; // recording stopped
  rf = 0; // 0 = no RF, 1 = RF but no signal, 2 = RF connected
  current = 1; // current save position


  /* fetch last values from EEPROM */
  read_EEPROM();

  store = 0;
  
/***************************************************
 * IO setup
 */
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(STEP, INPUT_PULLUP);
  pinMode(DIR, INPUT_PULLUP);
  pinMode(REC, INPUT_PULLUP);
  pinMode(CABLE, INPUT_PULLUP);

  pinMode(TALLY, OUTPUT);
  digitalWrite(TALLY, LOW);

/***************************************************
 * RF begin
 *    only if not connected to the camera with a cable
 */

/* check for cable connector */

  /* init RF */
  radio.begin();

  /* Set the PA Level low to prevent power supply related issues since this is a
   * getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.*/
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  
  /* Open a writing and reading pipe with opposite addresses */
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();


  /* init S-BUS */
  mySBUS.begin();

  /* stop recording */
  value[7] = -1;


  /* start on select screen */
  set_select();

  /* start interrupt */
  attachInterrupt(digitalPinToInterrupt(STEP), wheel_interrupt, CHANGE);
}



/*************************************************************************************
/*************************************************************************************
 * main loop
 */
void loop(void) {

  int button;
  int wheel;
  int oldWheel;
  int oldValue;

/***********************************************/
/* button and wheel action only when connected */


  if (connected()) {

  





/***********************************************/
/* button action */

    button = checkButton();

    /**********************/
    /* toggle recording */
    if (record_button()) {
      if (record) {
        set_stop();
        while (record_button());
      }
      else {
        set_record();
        while (record_button());
      }
    }

    switch (stat) {

      /* SELECT PARAMETER */
      case SELMODE:
        if (button == PUSH)
          set_value();
          
        if (button == LONGPUSH && analog()) {
          /* remember mode settings */
          lastMode = mode;
          lastSelect = select;

          set_recSelect();

          while (checkButton());
        }
        break;

      /* CHANGE VALUE */
      case VALMODE:
        if (button == PUSH)
          set_select();
          
        if (button == LONGPUSH)
          set_recValue();
        break;

      /* SELECT SAVE POSITION */
      case RECSELMODE:
        if (button == PUSH)
          set_recValue();
          
        if (button == LONGPUSH) {
          /* restore mode settings */
          mode = lastMode;
          select = lastSelect;

          set_select();
          while (checkButton());
        }
        break;

      /* CHANGE REC VALUE */
      case RECVALMODE:
        if (button == PUSH) {
          save_save();
          set_recSelect();
        }
          
        if (button == LONGPUSH)
//          mode = lastMode;
//          select = lastSelect;

          set_value();
          while (checkButton());
        break;

      /* RECORDING */
      case RECRUNMODE:

        if (button == PUSH)
          inc_save();
        break;
    }

    checkWheel();
  }



  /* send data when changed */
//  if (value[select] != oldValue)
    transmit();

//  oldValue = value[select];


/*    display.fillRect(10,0,100,20,BLACK);
    display.setCursor(10,0);
    display.print(rf);
    display.display();*/
}



/*************************************************************************************
/*************************************************************************************
 * functions 
 */

/**********************************************************/
/* check button
 *  
 *  0 = not pushed
 *  1 = short push
 *  -1 = long push longer than 1,5 seconds
 */
int checkButton() {
  
  /* wait for button release */
  if (!digitalRead(BUTTON)) {
    unsigned long curr = micros();

    while (!digitalRead(BUTTON)) { 
 
      /* timeout => long push */
      if (micros() - curr > 1000000) {
        return -1;
      }
    }
  }

  /* button not pushed */
  else {
    return 0;
  }

  /* short push */
  return 1;
}


void checkWheel(void) {

  if (delta != 0) {

    if (delta > 0) {
      increment();
    }
    else {
      decrement();
    }

    delta = 0;
  }

}


/**********************************************************
 * check if analog value selected
 */
bool analog() {
  if (select < 4)
    return true;
  else
    return false;
}


/**********************************************************
 * 
 */
void wheel_interrupt(void) {

  int a = digitalRead(STEP);
  int b = digitalRead(DIR);

  /* CW */
  if ((a && !b) || (!a && b)) {
    delta = 1;
  }

  /* CCW */
  else {
    delta = -1;
  }

  /* get speed */
  if ((millis() - wheelTime) > 75) {
    wheelSpeed = wheelCnt;
    wheelCnt = 0;
    wheelTime = millis();
  }

  if (wheelCnt < 16000)
    wheelCnt++;
}


/**********************************************************
 * increment
 */
void increment() {

  if (stat == SAVEMODE) {
    dec_menu();
  }

  /* increment select */
  if (!mode) {
    if (stat == SELMODE)
      dec_select();
    else
      dec_save();
  }

  /* increment value */
  else {

    /* analog */
    if (analog())
      inc_value();

    /* digital */
    else {
      value[select] = 1;
      delay (stepWait);
    }
  }
}

/**********************************************************
 * decrement
 */
void decrement() {

  if (stat == SAVEMODE) {
    inc_menu();
  }

  /* increment select */
  if (!mode) {
    if (stat == SELMODE)
      inc_select();
    else
      inc_save();
  }

  /* increment value */
  else {

    /* analog */
    if (analog())
      dec_value();
    else {
      value[select] = -1;
      delay (stepWait);
    }
  }
}


/**********************************************************
 * connected via cable or rf
 */
int connected (void) {

  if (cable() || rf == 2)
    return true;
  else
    return false;

}


/* cable connected ??? */
int cable(void) {

/* DEBUG */
//  return (true);
  return (!digitalRead(CABLE));
}


/**********************************************************
 * record button active
 */
bool record_button() {
  /* DEBUG */
//  return (false);
  return (!digitalRead(REC));  
}
 
/*************************************************************************************
 * modi changes
 ************************************************************************************/

/*************************************************************************************
 * set select mode
 */
void set_select() {
  stat = SELMODE;
  
  edit = false;
  mode = false;
  record = false;

  save_EEPROM();

  update_screen();
}

void inc_select() {
  if (select < 6) select++;
  else select = 0;

  draw_nav();
  draw_icon();
  draw_edit();
}

void dec_select() {
  if (select > 0) select--;
  else select = 6;

  draw_nav();
  draw_icon();
  draw_edit();
}


/***********************************************
 * set value mode
 */
void set_value() {
  stat = VALMODE;
  
  edit = false;
  mode = true;
  record = false;

  save_EEPROM();

  update_screen();
}

void inc_value(void) {
  if (wheelSpeed > 10)
    value[select] += 5;
  else
    value[select]++;
  
  if (value[select] > 100)
    value[select] = 100;

  draw_nav();
}

void dec_value() {
  if (wheelSpeed > 10)
    value[select] -= 5;
  else
    value[select]--;

  if (value[select] < 0)
    value[select] = 0;

  draw_nav();
}

/***********************************************
 * set rec select mode
 */
 void set_recSelect() {

  if (analog()) {
    stat = RECSELMODE;
    
    edit = true;
    mode = false;
    record = true;

    save_EEPROM();
  
    update_screen();
  }
}


/***********************************************
 * set rec select mode
 */
 void set_recValue() {

  if (analog()) {
    stat = RECVALMODE;
    
    edit = true;
    mode = true;
    record = true;

    save_EEPROM();

    update_screen();
  }
}


void inc_save() {

  /* analog parameter */
  if (analog()) {

    /* more values => increment */
    if (nr[select] > current) {
      current++;
    }

    /* jump to first value while recording */
    else {
      current = 1;
    }

    draw_nr();
  }
}

void dec_save() {

  if (analog() && current > 1) {
    current--;
    draw_nr();
  }
  else {
    current = nr[select];
    draw_nr();
  }
}


/* save value */
void save_save() {

  int oldstat = stat;
  stat = SAVEMODE;

  /* no value => add */
/*  if (!nr[select]) {
    current = 1;
    nr[select]++;
  }*/

  draw_save();

  /* select option */
  while (checkButton() == 0) {
    checkWheel();
    draw_save();
    delay(100);
  }

  /* save value */
  if (menuItems[store] == "save") {
    save[select][current-1] = value[select];
  }

  /* add value */
  if (menuItems[store] == "add") {

    if (nr[select] < 3) {
      nr[select]++;
      current = nr[select];

      save[select][current-1] = value[select];
    }
  }

  /* delete value */
  if (menuItems[store] == "delete") {
    remove_save();
  }


  save_EEPROM();

  draw_nav();
  draw_nr();
  
  stat = oldstat;
}


void save_EEPROM() {
  
  int address = 0;
  int i;
  int j;
  

  /*********************/
  /* write save values */
  /* iterate select */
  for (i=0; i<4; i++) {

    /* iterate store */
    for (j=0; j<3; j++) {
      EEPROM.put(address, save[i][j]);
      address++;
    }
  }


  /***********************/
  /* write number values */
  for (i=0; i<4; i++) {
    EEPROM.put(address, nr[i]);
    address++;
  }


  /**********************/
  /* writemanual values */
  for (i=0; i<7; i++) {
    EEPROM.put(address, value[i]);
    address++;
  }

}


void read_EEPROM() {

  int address = 0;
  int i;
  int j;

  /********************/
  /* read save values */
  /* iterate select */
  for (i=0; i<4; i++) {

    /* iterate store */
    for (j=0; j<3; j++) {
      
      save[i][j] = EEPROM.read(address);
      address++;
    }
  }

  /**********************/
  /* read number values */
  for (i=0; i<4; i++) {
    nr[i] = EEPROM.read(address);
    address++;
  }


  /**********************/
  /* read manual values */
  for (i=0; i<7; i++) {
    value[i] = EEPROM.read(address);
    address++;
  }

}


/* remove saved value */
void remove_save() {
  int i;
  
  if (analog()) {
    
    /* remove last in list */
    if (nr[select] == current) {
      nr[select]--;
      current--;
    }

    /* remove entry in middle */
    else {
      /* loop from current to end
       *  move values forward
       */
      for (i = current; i <= nr[select]; i++) {
        save[select][i-1] = save[select][i];
      }

      nr[select]--;

    }

    draw_nr();
  }
}


/***********************************************
 * set record mode
 */
void set_record(void) {

  stat = RECRUNMODE;
  current = 1;
  digitalWrite(TALLY, HIGH);
  
  edit = false;
  mode = true;
  record = true;

  value[7] = -1;

  update_screen();
}


void set_stop(void) {
  stat = VALMODE;
  digitalWrite(TALLY, LOW);

  value[7] = 1;
  
  set_select();
}




/***********************************************
 * init the screen
 */
void update_screen(void) {
       
  draw_nav();
  draw_icon();
  draw_edit();
  draw_nr();
  draw_rec();
  draw_rf();
}


/***********************************************
 * draw navigation bar
 * 
 * type:  SELECT => show dots (val 0 to 7)
 *        UPDOWN => show +/- with arrows
 *        VALUE => show bar (val 0 to 100)
 */
void draw_nav() {

  int val = value[select];
  int type = "";
  int proc;


  /* value mode */
  if (mode == true) {

    if (analog())
      type = VALUE;
    else
      type = UPDOWN;
  }

  /* select mode */
  else {
    if (stat != RECSELMODE)
      type = SELECT;
    else
      type = NONE;
  }

  display.fillRect(0, 0, 9, 64, BLACK);

  /* show navigation only when connected */

  proc = 56 - val*56/100;
  switch (type) {

    case NONE:
      display.drawRect(0, 4+proc, 1, 58-proc, WHITE);
      break;

    case SELECT:
      for (int i=0; i<7; i++) {
        if (i == select)
          display.fillCircle(4, 8 + (i*8), 3, WHITE);
        else
          display.fillCircle(4, 8 + (i*8), 3, BLACK);
          display.drawCircle(4, 8 + (i*8), 1, WHITE);
      }

      if (analog())
        display.drawRect(0, 4+proc, 1, 58-proc, WHITE);
      break;

    case UPDOWN:
      display.fillRect(0, 4, 7, 3, WHITE);
      display.fillRect(2, 2, 3, 7, WHITE);
      
      display.fillRect(0, 58, 7, 3, WHITE);
      
      display.fillTriangle(4, 18, 7, 28, 0, 28, WHITE);
      display.fillTriangle(0, 36, 7, 36, 4, 46, WHITE);
      break;

    case VALUE:
      display.drawRect(1, 4, 6, 56, WHITE);
      display.fillRect(1, 4+proc, 6, 58-proc, WHITE);
      break;
  }
 
  display.display();
  delay(1);
}


/***********************************************
 * draw save screen
 */
void draw_save() {
  String items[4];
  
  display.fillRect(8, 0, 60, 64, BLACK);

  if (!nr[select]) {
    menuItems[0] = "add";
    menuItems[1] = "abort";

    menuCount = 2;
  }
  
  else if (nr[select] < 3) {
    menuItems[0] = "save";
    menuItems[1] = "add";
    menuItems[2] = "abort";
    menuItems[3] = "delete";

    menuCount = 4;
  }

  else {
    menuItems[0] = "save";
    menuItems[1] = "abort";
    menuItems[2] = "delete";

    menuCount = 3;
  }
  
  menu(20);
}


/***********************************************
 * show menu
 *    items: pointer to item texts
 *    count: number of items
 *    x: x coordinate
 */
void menu(int x) {

  display.setTextSize(1);

  int y = (64 - ((menuCount - 1) * 15)) / 2;

  for (int i=0; i<menuCount; i++) {
    display.setCursor(x, y + (i*15));
    display.print(menuItems[i]);
  }

  display.drawRect(x - 7, y + 2 + store*15, 3, 3, WHITE);

  display.display();
  delay(1);
}


void inc_menu() {
  if (store < (menuCount - 1))
    store++;
  else
    store = 0;
}


void dec_menu() {
  if (store > 0)
    store--;
  else
    store = 2;  
}


/***********************************************
 * draw the icon corseponding with the select value (0-7)
 */
void draw_icon(void) {
  
  display.fillRect(8, 0, 60, 64, BLACK);
//  display.drawRect(8, 0, 60, 64, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(22, 55);

  switch (select) {
    case 0:
      display.drawBitmap(14, 0, focus_48, 48, 48, 1);

      display.setCursor(24, 57);
      display.print("Focus");
      break;

    case 1:
      display.drawBitmap(14, 0, iris_48, 48, 48, 1);

      display.setCursor(28, 57);
      display.print("Iris");
      break;

    case 2:
      display.drawBitmap(14, 0, zoom_48, 48, 48, 1);

      display.setCursor(28, 57);
      display.print("Zoom");
      break;

    case 3:
      display.drawBitmap(14, 0, audio_48, 48, 48, 1);

      display.setCursor(24, 57);
      display.print("Audio");
      break;

    case 4:
      display.drawBitmap(14, 0, iso_48, 48, 48, 1);

      display.setCursor(30, 57);
      display.print("ISO");
      break;

    case 5:
      display.drawBitmap(14, 0, sector_48, 48, 48, 1);

      display.setCursor(22, 57);
      display.print("Sector");
      break;

    case 6:
      display.drawBitmap(14, 0, color_48, 48, 48, 1);
    
      display.setCursor(24, 57);
      display.print("Color");
      break;
  }

  display.display();
  delay(1);
}


/***********************************************
 * show edit symbol
 */
void draw_edit(void) {
  display.fillRect(66, 0, 30, 8, BLACK);

  if ((edit || nr[select]) && analog()) {

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(70, 0);
  
    display.print("save");
  }

  display.display();
  delay(1);
}


/***********************************************
 * show memory number
 * 1-3 
 */
void draw_nr() {

  display.fillRect(68, 8, 29, 56, BLACK);

  /* only for analog values */
  if (analog() and record) {

    if (nr[select] > 0 && nr[select] <= 3) {
      display.setTextSize(2);
      display.setTextColor(WHITE);
  
      for (int i=0; i<nr[select]; i++) {
        display.setCursor(76, 10 + 20*i);
        display.write(49+i);

        /* show current value */
        if (current == (i + 1)) {
          display.drawRect(73, 8 + 20*i, 16, 18, WHITE);
          
          value[select] = save[select][current-1];
          draw_nav();
        }
      }
    }
  }

  display.display();
  delay(1);
}


/***********************************************
 * draw RF symbol
 */
 void draw_rf(void) {
  
  display.fillRect(96, 0, 30, 32, BLACK);

  display.setTextSize(1);
  display.setTextColor(WHITE);

  switch (rf) {

    /* RF connected */
    case 2:
      display.drawCircle(111, 10, 6, WHITE);
      display.drawCircle(111, 10, 8, WHITE);
      display.drawCircle(111, 10, 10, WHITE);

      display.fillTriangle(111, 10, 101, 0, 121, 0, BLACK);
      display.fillTriangle(111, 10, 101, 20, 121, 20, BLACK);

    /* not connected */
    case 1:
      display.fillCircle(112, 10, 2, WHITE);
      display.fillRect(111, 10, 2, 10, WHITE);

      display.setCursor(107, 22);
    
      display.print("RF");
      break;

    /* cable */
    case 0:
      display.fillRect(106, 5, 10, 8, WHITE);
      display.fillRect(108, 1, 1, 4, WHITE);
      display.fillRect(113, 1, 1, 4, WHITE);
      display.fillRect(107, 13, 8, 1, WHITE);
      display.fillRect(110, 14, 2, 5, WHITE);

      display.setCursor(97, 22);
      display.print("S-BUS");
      break;
  }


  /* not connected message */
/*  if (!connected()) {
    display.setTextSize(2);
    display.setCursor(11,27);
    display.print("not");
    display.setCursor(11,45);
    display.print("connected");
  }*/

  display.display();
  delay(1);
}


/***********************************************
 * draw record symbol
 */
void draw_rec() {
  display.fillRect(96, 40, 30, 32, BLACK);
//  display.drawRect(96, 40, 30, 32, WHITE);

  if (connected()) {

    if (record > 0) {
      if (edit) {
        display.setTextSize(2);
        display.setCursor(106,47);
        display.print("S");      
      }

      else
        display.fillCircle(110, 50, 9, WHITE);
    }
  }

  display.display();
  delay(1);
}



/**********************************************************/
/**********************************************************/
/* transmit data
 *    CABLE: true => send SBUS
 *           false => send RF
 */
void transmit() {

  int newRf;

  /* cable S_BUS transmisssion */
  if (cable()) {

    newRf = 0;
    radio.powerDown();

    /* analog channels */
    mySBUS.Servo(FOCUSchannel, map(value[0],0,100,0,2047)); /* focus */
    mySBUS.Servo(IRISchannel, map(value[1],0,100,0,2047)); /* iris */
    mySBUS.Servo(ZOOMchannel, map(value[2],0,100,0,2047)); /* zoom */
    mySBUS.Servo(AUDIOchannel, map(value[3],0,100,0,2047)); /* audio */

    mySBUS.Update();
    mySBUS.Send();
    delay(sbusWAIT);

    /* digital channels */
    stepChannel(4,ISOchannel);
    stepChannel(5,SECTORchannel);
    stepChannel(6,COLORchannel);
    stepChannel(7,RECchannel);

  }

  /* RF transmission */
  else {

    radio.powerUp();
    radio.stopListening();
  
    /* get send time for connection test */
    unsigned long start_time = micros();
  //  data.time = start_time;
  
  
    /* send data */
    /* set rf status */
    if (!radio.write( &value, sizeof(int[8]) )) {
      newRf = 1;
    }
    else {
      newRf = 2;
    }
  }
  
  if (newRf != rf) {
    rf = newRf;
    update_screen();
  }

  radio.startListening();
}


/***********************************************
 * send step sequence
 */
void stepChannel (int val, int channel) {

  if (value[val] != 0) {

    if (value[val] > 0) {

      mySBUS.Servo(channel, sbusMID);
      mySBUS.Update();
      mySBUS.Send();

      delay(sbusWAIT);
  
      mySBUS.Servo(channel, sbusHIGH);
      mySBUS.Update();
      mySBUS.Send();
    }

    else {
      mySBUS.Servo(channel, sbusMID);
      mySBUS.Update();
      mySBUS.Send();

      delay(sbusWAIT);
  
      mySBUS.Servo(channel, sbusLOW);
      mySBUS.Update();
      mySBUS.Send();
    }

    delay(sbusWAIT);

    mySBUS.Servo(channel, sbusMID);
    mySBUS.Update();
    mySBUS.Send();

    /* reset digital data */
    value[val] = 0;

    delay(200);
  }
}


