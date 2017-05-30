/*
* Follow Focus sender
* v0.2
* 2016-12-16 by Thomas H Winkler
* <thomas.winkler@iggmp.net>
* 
* using RF24 library by J. Coliz <maniacbug@ymail.com>
* using SoftwareServo library
*/

/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

/*
* input
* A3 = Focus
* A4 = FStop
* A5 = Zoom
* D3 = Switch1
* D4 = Switch2
* D5 = Switch3
* 
* output
* D0 = connection status
* D1 = power low
* D6 = status 1
* D9 = status 2
* D10 = status 3

for RF24 pinning refer to the text file
*/

#include <SPI.h>
#include <RF24.h>
#include <printf.h>
#include <EEPROM.h>

/****************** User Config ***************************/
/* analog inputs */
#define FOCUS A3
#define FSTOP A4
#define ZOOM A5

/* digital input switch to gnd */
#define SW1 3
#define SET 4
#define ENTER 5

/* digital LED output with 1k resistor */
#define STAT1 6
#define STAT2 9
#define STAT3 10

#define RADIO 0
#define PWR 1



/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

struct DATA {
  bool sw1;
  bool sw2;
  bool sw3;
  int focus;
  int fstop;
  int zoom;
  unsigned long time;
};

#define MAX 3

int maxStore = MAX;
DATA autoStore[MAX];
int autoVal;

bool recallStat;
int recallVal;

unsigned long blinkTime;
bool blinkStatus = LOW;

unsigned long noBlinkTime;
bool noBlinkStatus = LOW;

bool recStat;

/* value array */
int value[8];


/**********************************************************/
/**********************************************************/
void setup() {
  
//  Serial.begin(115200);
//  Serial.println(F("RF24 follow focus"));
  
  // init RF24 radio transceiver
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  
  // Open a writing and reading pipe with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();

//******************
//DEBUG show details
//printf_begin();
//radio.printDetails();

  autoVal = 1;
  recallVal = 1;
  recallStat = false;
  

  // init IO
  pinMode(SW1, INPUT_PULLUP);
//  pinMode(SW2, INPUT_PULLUP);
//  pinMode(SW3, INPUT_PULLUP);

  pinMode(SET, INPUT_PULLUP);
  pinMode(ENTER, INPUT_PULLUP);

  pinMode(RADIO, OUTPUT);
  pinMode(PWR, OUTPUT);

  digitalWrite(RADIO, LOW);
  digitalWrite(PWR, LOW);

  pinMode(STAT1, OUTPUT);
  pinMode(STAT2, OUTPUT);
  pinMode(STAT3, OUTPUT);

  digitalWrite(STAT1, LOW);
  digitalWrite(STAT2, LOW);
  digitalWrite(STAT3, LOW);

  recStat = false;

}


/**********************************************************/
/**********************************************************/
void loop() {

  DATA current = scan();
  transmit(current);

  unsigned long setTime = micros();

  /* enter auto mode */
  if (checkSet() == -1) {
    autoMode();
  }
}


/**********************************************************/
/**********************************************************/
/* set auto data */
void autoMode() {
  
  int button;

  digitalWrite(RADIO, LOW);
  statusBlink(0,1,300);

  /* get data from EEPROM */







  //Serial.println(autoStore);
  
  /* wait for setup button release */
  while (checkSet() != 0) {}


  /* setup loop */
  while (true) {

    /* set storeVal LED */
    statusSet(autoVal);

    /* send stored data */
    DATA current = autoStore[autoVal-1];
    transmit(current);
    

    /* enter edit mode */
    if (checkEnter() == -1) {
      edit();
    }

    /* exit or incenent */
    button = checkSet();

    if (button) {
      if (button == -1) {
        statusBlink(0,1,300);
        return;
      }
  
      /* toggle position */
      else {
        incStore();
      }
    }
  }
}


/**********************************************************/
/**********************************************************/
/* save value to auto store */
void edit() {
  
  int button;

  digitalWrite(RADIO, LOW);
  statusClear(0);

  while (checkEnter());
  
  while (true) {

    /* blink LED display */
    if (millis() - blinkTime > 500) {

      blinkTime = millis();

      if (blinkStatus == LOW) {
        blinkStatus = true;
        statusSet(autoVal);
      }
      else {
        blinkStatus = false;
        statusClear(autoVal);
      }
    }


    /* get and send values */
    DATA current = scan();
    transmit(current);
    

    /* save value or exit */
    button = checkEnter();
   
    if (button) {

      digitalWrite(RADIO, LOW);

      /* save value */
      if (button != -1) {
        autoStore[autoVal-1] = current;
        statusBlink(autoVal,2,150);
        /* write data to EEPROM */
      }
      else {
        statusBlink(0,1,300);
        while (checkEnter());
        break;
      }
    }

    /* increment */
    if (checkSet()) {
      incStore();
    }
  }
}


/**********************************************************/
/* check SET button
 *  
 *  true if longer than 1,5 seconds
 */
int checkSet() {
  
  if (!digitalRead(SET)) {
    unsigned long current = micros();

    while (!digitalRead(SET)) { 
 
      if (micros() - current > 1500000) {
        return -1;
      }

    }
  }
  else {
    return 0;
  }

  return 1;
}


/**********************************************************/
/* check SET button
 *  
 *  true if longer than 1,5 seconds
 */
int checkEnter() {
  
  if (!digitalRead(ENTER)) {
    unsigned long current = micros();

    while (!digitalRead(ENTER)) { 
 
      if (micros() - current > 1500000) {
        return -1;
      }

    }
  }
  else {
    return 0;
  }

  return 1;
}


/**********************************************************/
/**********************************************************/
/* transmit data */
void transmit(DATA data) {

  radio.stopListening();

  /* get send time for connection test */
  unsigned long start_time = micros();
  data.time = start_time;


  value[0] = map(data.focus, 0, 1024, 0, 100);
  value[1] = map(data.fstop, 0, 1024, 0, 100);
  value[2] = map(data.zoom, 0, 1024, 0, 100);
  value[3] = 1024;

  value[4] = 0;
  value[5] = 0;
  value[6] = 0;

  /* play */
  if (data.sw1 and !recStat) {
    recStat = true;
    value[7] = 1;
  }

  /* stop */
  if (!data.sw1 and recStat) {
    recStat = false;
    value[7] = -1;
  }

  if (!recStat) {
/*    radio.startListening();

    while (!radio.available()) {  
      if (micros() - start_time > 10000) {
//        digitalWrite(PWR, LOW);
        break;
      }
    }
    
    if (radio.available()) {
      float power;
      radio.read( &power, sizeof(float) );

      digitalWrite(PWR, HIGH);
    }*/

    digitalWrite(PWR, LOW);
  }
  else {
    digitalWrite(PWR, HIGH);
  }

  /* send data */
  if (!radio.write( &value, sizeof(int[8]) )){



//  if (!radio.write( &data, sizeof(DATA) )){
//    Serial.println(F("*** send failed"));

    /* blink RADIO LED */
//    digitalWrite(RADIO, LOW);

   /* blink LED display */
    if (millis() - noBlinkTime > 500) {

      noBlinkTime = millis();

      if (noBlinkStatus == LOW) {
        noBlinkStatus = true;
      }
      else {
        noBlinkStatus = false;
      }

      digitalWrite(RADIO,noBlinkStatus);
    }

  }
  else
    digitalWrite(RADIO, HIGH);

  value[7] = 0;
}


/**********************************************************/
/**********************************************************/
/* scan input and return DATA struct */
DATA scan () {
  DATA io;

  io.focus = analogRead(FOCUS);
  io.fstop = analogRead(FSTOP);
  io.zoom = analogRead(ZOOM);

  io.sw1 = !digitalRead(SW1);
//  io.sw2 = !digitalRead(SW2);
//  io.sw3 = !digitalRead(SW3);

  return io;
}


/**********************************************************/
/* iterate storeVal */
void incStore() {
  if (autoVal == maxStore)
    autoVal = 1;
  else
    autoVal++;
}


/**********************************************************/
/* iterate storeVal */
void incRecall() {
  if (recallVal == maxStore)
    recallVal = 1;
  else
    recallVal++;
}


/**********************************************************/
/**********************************************************/
/* set status LED 1-3
 *  0 => set all LEDs
 */
void statusSet(int nr) {

  statusClear(0);
  
  switch(nr) {
    case 1:
      digitalWrite(STAT1,HIGH);
      break;
    case 2:
      digitalWrite(STAT2,HIGH);
      break;
    case 3:
      digitalWrite(STAT3,HIGH);
      break;
    default:
      digitalWrite(STAT1,HIGH);
      digitalWrite(STAT2,HIGH);
      digitalWrite(STAT3,HIGH);
      break;
  }
}


/**********************************************************/
/**********************************************************/
/* clear status LED 1-3
 *  0 => clear all LEDs
 */
void statusClear(int nr) {

  switch(nr) {
    case 1:
      digitalWrite(STAT1,LOW);
      break;
    case 2:
      digitalWrite(STAT2,LOW);
      break;
    case 3:
      digitalWrite(STAT3,LOW);
      break;
    default:
      digitalWrite(STAT1,LOW);
      digitalWrite(STAT2,LOW);
      digitalWrite(STAT3,LOW);
      break;
  }
}


/**********************************************************/
/**********************************************************/
/* blink LED nr, cnt times with speed
 *  0 => blink all LEDs
 */
void statusBlink(int nr, int cnt, int speed) {
  
  for (int i=0; i < cnt; i++) {
    statusSet(nr);
    delay(speed);

    statusClear(nr);
    delay(speed);
  }
}


/**********************************************************/
/**********************************************************/
/* read data from EEPROM
 *   return DATA struct
 */

DATA getData(void) {

  DATA data;

  return data;
}


/**********************************************************/
/**********************************************************/
/* write data to EEPROM
 *   DATA struct
 */

void writeData(void) {
  
}

