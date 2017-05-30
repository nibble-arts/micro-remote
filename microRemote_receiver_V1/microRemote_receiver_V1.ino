/*
* Follow Focus receiver
* v0.2
* 2016-12-16 by Thomas H Winkler
* <thomas.winkler@iggmp.net>
* 
* using RF24 library by J. Coliz <maniacbug@ymail.com>
*/

/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

/*
* input
* A0 = battery voltage control
* 
* output
* TX = S-Bus (inverted)
* D6 = PWM servo Focus
* D11 = PWM servo FStop
* D10 = PWM servo Zoom
*
* D5 = connection status
* D12 = record

for RF24 pinning refer to the text file
*/

#include <SPI.h>
#include <RF24.h>
#include <printf.h>

#include <Servo.h>

/****************** PIN Config ***************************/
/*                            
 * PWM servo output
 */
#define FOCUS 6
#define FSTOP 11
#define ZOOM 10

/* digital input switch to gnd */
#define SW1 3

/* digital LED output with 1k resistor to gnd */
#define REC 12
#define STATUS 5


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

const int sbusMID         = 1024;   //neutral val
const int sbusLOW         = 0;      //low switch val
const int sbusHIGH        = 2000;   //high switch val

const int sbusWAIT = 4;      //frame timing delay in msecs

BMC_SBUS mySBUS;



/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);

/* Servo definition */
Servo FocusServo;
Servo FstopServo;
Servo ZoomServo;

/**********************************************************/
/* rf24 identification data definition */
byte addresses[][6] = {"1Node","2Node"};


unsigned long timeout;
bool blinkStatus;    

unsigned long lowTime;
bool lowStatus;    

bool online;
float power;

bool recStat;

/* value array */
int value[8];



/**********************************************************/
/* setup routine                                          */
void setup() {

  Serial1.begin(9600);

  /* init S-BUS */
  mySBUS.begin();


/**************************/
/* init radio             */
  radio.begin();

//TODO make channel selectable

  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);

  
  // Start the radio listening for data
  radio.startListening();


/*  if (radio.failureDetected)
    Serial.println("failure");
  else
    Serial.println("no failure");


  if (radio.isValid())
    Serial.println("is valid RF24L01");
  else
    Serial.println("no chip found");
*/

/**************************/
/* init servo             */
  FocusServo.attach(FOCUS);
  FstopServo.attach(FSTOP);
  ZoomServo.attach(ZOOM);

  // init IO
  pinMode(SW1, OUTPUT);

  pinMode(STATUS, OUTPUT);
  digitalWrite(STATUS, LOW);

  pinMode(REC, OUTPUT);
  digitalWrite(REC, LOW);

  online = false;


/*  digitalWrite(STATUS, HIGH);
  delay(200);
  digitalWrite(STATUS, LOW);
  delay(200);
  digitalWrite(STATUS, HIGH);
  delay(200);
  digitalWrite(STATUS, LOW);
  delay(200);*/

  recStat = false;

}


/**********************************************************/
/* main loop                                              */
void loop() {

  int focusVal;
  int fstopVal;
  int zoomVal;


  /* get radio data */
  if(radio.available()){

    radio.read( &value, sizeof(int[8]) );
    timeout = micros();

    digitalWrite(STATUS, HIGH);

    transmit();
  }

  /* if no radio, check for timeout */
  else {

    /* blink LED */
    if (micros()-timeout > 500000) {
      timeout = micros();

      if (blinkStatus == LOW)
        blinkStatus = HIGH;
      else
        blinkStatus = LOW;

      digitalWrite(STATUS, blinkStatus);


      /* stop camera on transmit lost */
//      digitalWrite(SW1, LOW);
    }
  }

  if (recStat)
    digitalWrite(REC, HIGH);
  else
    digitalWrite(REC, LOW);

}


/**********************************************************/
/**********************************************************/
/* transmit data
 *    CABLE: true => send SBUS
 *           false => send RF
 */
void transmit() {


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


  /* reset digital data */
  value[4] = 0;
  value[5] = 0;
  value[6] = 0;
  value[7] = 0;

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

      /* set REC LED to play */
      if (channel = RECchannel)
        recStat = true;
    }

    else {
      mySBUS.Servo(channel, sbusMID);
      mySBUS.Update();
      mySBUS.Send();

      delay(sbusWAIT);
  
      mySBUS.Servo(channel, sbusLOW);
      mySBUS.Update();
      mySBUS.Send();

      /* set REC LED to stop */
      if (channel = RECchannel)
        recStat = false;
    }

    delay(500);
  }
}

