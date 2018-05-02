/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */

#include "MyMatrix.h"
#include "MyMusic.h"
#include "MyServo.h"
 
//In case MySensor network is no longer available.
//#define STANDALONE_MODE
 
// Enable debug prints
#define MY_DEBUG

#ifndef STANDALONE_MODE
// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_NODE_ID 10
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC

//Children message IDs
#define CHILD_ID_RELAY1 0
#define CHILD_ID_SLEEPTIME 1

#include <math.h>
#include <SPI.h>
#include <MySensors.h> 
#endif

//LedControl 1.0.6
#include <LedControl.h>

#define SHORT_WAIT 50
#define LONG_WAIT 500
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

//Arduino pins
#define BUTTON_PIN 2
#define RELAY1_PIN 6

//900 seconds = 15min
//unsigned long SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)
unsigned long SLEEP_TIME = 5000; //Starting time
#define TRIGGER_SLEEPS_NO_RESPONSE 10
#define MAX_SLEEPS_NO_RESPONSE 11
int sleeps_without_response = 0;

MyMessage msgMusicBoxSwitch1(CHILD_ID_RELAY1, V_LIGHT);
MyMessage msgMusicBoxSleepTime(CHILD_ID_SLEEPTIME, V_VAR1);

static bool no_response_recovered = false;
static bool settings_accepted = false;
static bool activateBox = false;
//static bool activateBox = true;  //debug just activate
void ActivateBox()
{
	activateBox = true;
}

void StartActivationRoutine()
{
  MatrixStartLogic();
  MusicStartLogic();
  ServoStartLogic();
}

void EndActivationRoutine()
{
  MatrixStopLogic();
  MusicStopLogic();
  ServoStopLogic();
}

void RunActivationRoutine()
{
  StartActivationRoutine();

  int myrand = random(3);
  switch(myrand) {
    case 0:
      play_pbroutine();
      break;
    case 1:
      play_kokoroutine();
      break;
    case 2:
      play_ponyoroutine();
      break;
  }
  
  EndActivationRoutine();

  wait(1000);
}

//This doesn't seem to work... at least with using MySensors and not being connected.
//I know it works without mysensors.  Possible it only works when Mysensor is connected
//when using mysenesors.
void ButtonInterrupt()
{
	Serial.println("Interrupt...");
	//Disable Button for just now
	//ActivateBox();
}

void setup()  
{
  randomSeed(analogRead(5));
  
	// Setup the buttons
	pinMode(BUTTON_PIN, INPUT);

  // Activate internal pull-ups
  digitalWrite(BUTTON_PIN, HIGH);

  ServoSetupLogic();
	
	//Attach interrupt to the button.
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonInterrupt, RISING);
}

void presentation()  
{ 
	// Send the Sketch Version Information to the Gateway
	sendSketchInfo("Music Box", "1.0");
	wait(LONG_WAIT);

	present(CHILD_ID_RELAY1, S_LIGHT, "Open Box");
  present(CHILD_ID_RELAY1, S_CUSTOM, "Sleep Time");
	wait(LONG_WAIT);
}

void loop()      
{ 
	uint8_t value;
	static uint8_t lastValue=2;
	
	//We are awake.  Now request the status of our switch from the gateway / openhab
	Serial.println("Request from server...");
	request(CHILD_ID_RELAY1, V_LIGHT);

	//mysgw: TSF:MSG:READ,10-10-0,s=0,c=2,t=2,pt=0,l=0,sg=0:
	//mysgw: Sending message on topic: mygateway1-out/10/0/2/0/2
	
	//Wait two seconds for a response as openhab and the gateway might need
	//some time to talk back after our request.
	//This wait specifically loops in mysensor logic for communication.
	//This is where our receive function will get called and activateBox would be set.
	wait(2000);
  sleeps_without_response++;
  if(sleeps_without_response > MAX_SLEEPS_NO_RESPONSE) {
    sleeps_without_response = MAX_SLEEPS_NO_RESPONSE;
  }
  request(CHILD_ID_SLEEPTIME, V_VAR1);
  wait(2000);
	
	//If we got the button interrupt, then play the box.
	if(activateBox)
	{
		RunActivationRoutine();
		activateBox = false;
	}

  if(settings_accepted)
  {
    Serial.println("Settings Accepted.");
    MusicStartLogic();
    pt.tune_playscore (accept_settings_score);
    delay(1000);
    MusicStopLogic();
    settings_accepted = false;
  }

  if(sleeps_without_response == TRIGGER_SLEEPS_NO_RESPONSE)
  {
    ServoStartLogic();
    SetServo(95);
    delay(1000);
    ServoStopNoMoveLogic();
  }
  else if(no_response_recovered)
  {
    ServoStartLogic();
    ServoStopLogic();
    no_response_recovered = false;
  }

	Serial.print("Sleep for ... ");
	Serial.println(SLEEP_TIME);
	//This sleep will will cause the radio to power down.
	//Hopefully powering everything down... with a button wakeup
	sleep(BUTTON_PIN-2, CHANGE, SLEEP_TIME);
	
	//This is if we get interrupt during sleep from the button.
	//Not sure if this works or not.  Haven't tested out sleep wakeup.
	value = digitalRead(BUTTON_PIN);
	if (value==HIGH) {
    //ignore button temporarily
		//RunActivationRoutine();
	}
}

void receive(const MyMessage &message) {
  if(sleeps_without_response >= TRIGGER_SLEEPS_NO_RESPONSE) {
    no_response_recovered = true;
  }
  sleeps_without_response = 0;
  Serial.print("Got a message! s:");
  Serial.print(message.sensor);
  Serial.print(" t:");
  Serial.println(message.type);
	// We only expect one type of message from controller. But we better check anyway.
	if (message.sensor == CHILD_ID_RELAY1) {
		if (message.type==V_LIGHT) {
			if(message.getInt() == 1) {
				// Turn on the relay briefly.
				Serial.println("We Activated!");
				ActivateBox();
			}
		}
	}
  else if (message.sensor == CHILD_ID_SLEEPTIME) {
    if (message.type==V_VAR1) {
      Serial.print("It's a sleep message of:");
      Serial.println(message.getLong());
      unsigned long newsleeptime = message.getLong();
      if(SLEEP_TIME != newsleeptime &&
         newsleeptime > 1000 && newsleeptime < 3600000) {
          SLEEP_TIME = newsleeptime;
          settings_accepted = true;
      }
    }
  }
}

#define G_STEP_MS 30

#define PBROUTINE_FRAMES 560
#define PBROUTINE_STEP_MS 30
#define PBROUTINE_SERVO_SHORT_ANGLE 30
#define PBROUTINE_SERVO_ANGLE_WIDTH 140
void play_pbroutine() {
  //Song is 18 seconds long.
  int matrixFrame = 0;
  int lastMatrixFrame = -1;
  double matrixStep = PBROUTINE_FRAMES / beatingHeart1_len;
  double sinInput;
  double angle;
  int xangle;

  //Sart Music
  pt.tune_playscore (pbscore); /* start playing 18 seconds */
  //Routine loop
  for(int i=0; i < PBROUTINE_FRAMES; i++)
  {
    //Matrix
    matrixFrame = (int)((double)i / matrixStep);
    if(matrixFrame != lastMatrixFrame && matrixFrame < beatingHeart1_len) {
      loadMatrixDataHexFrame(beatingHeart1, matrixFrame);
      lastMatrixFrame = matrixFrame;
    }
    
    //Servo
    sinInput = ( ((double)(i%PBROUTINE_SERVO_ANGLE_WIDTH))/PBROUTINE_SERVO_ANGLE_WIDTH ) * (2*PI);
    sinInput -= (PI/2); //offset for moving up sin wave
    angle = (sin(sinInput) + 1) / 2;
    xangle = (int)((angle * PBROUTINE_SERVO_ANGLE_WIDTH) + PBROUTINE_SERVO_SHORT_ANGLE);
    //Serial.println(angle);
    SetServo(xangle);
    //SetServo((i % 140) + 20);
    delay(PBROUTINE_STEP_MS);
  }
}

#define KOKOROUTINE_FRAMES 683 //20.5 seconds @ 30ms
#define KOKOROUTINE_STEP_MS 30
#define KOKOROUTINE_SERVO_SHORT_ANGLE 30
#define KOKOROUTINE_SERVO_ANGLE_WIDTH 140
void play_kokoroutine() {
  int matrixFrame = 0;
  int lastMatrixFrame = -1;
  double matrixStep = KOKOROUTINE_FRAMES / sunkissheart_len;
  double sinInput;
  double angle;
  int xangle;
  
  pt.tune_playscore (kokoscore); /* start playing 19.5 seconds */
  //pt.tune_playscore (pbscore);

  for(int i=0; i < KOKOROUTINE_FRAMES; i++)
  {
    //Matrix
    matrixFrame = (int)((double)i / matrixStep);
    if(matrixFrame != lastMatrixFrame && matrixFrame < sunkissheart_len) {
      loadMatrixDataHexFrame(sunkissheart, matrixFrame);
      lastMatrixFrame = matrixFrame;
    }

    //Servo up to 85 in 4 seconds
    int introFrames = 4000 / KOKOROUTINE_STEP_MS;
    if(i < introFrames)
    {
      SetServo( (int)(85 * ((double)i/(double)introFrames)) + SERVO_BASE );
    }
    if(i >= introFrames)
    {
    }

    delay(KOKOROUTINE_STEP_MS);
  }
}

#define PONYOROUTINE_FRAMES 683 //20.5 seconds @ 30ms
#define PONYOROUTINE_STEP_MS 30
#define PONYOROUTINE_SERVO_SHORT_ANGLE 47
#define PONYOROUTINE_SERVO_ANGLE_WIDTH 100 //120
#define PONYOROUTINE_1_MS 4000
#define PONYOROUTINE_1_FRAMES (PONYOROUTINE_1_MS / G_STEP_MS)
#define PONYOROUTINE_2_MS 8000
#define PONYOROUTINE_2_FRAMES (PONYOROUTINE_2_MS / G_STEP_MS)
void play_ponyoroutine() {
  double sinInput;
  double yvalue;
  int xvalue;
  int matrixFrame = 0;
  int lastMatrixFrame = -1;  
  
  pt.tune_playscore (ponyoscore); /* start playing 20 seconds */

  int i=0;
  double matrixStep = PONYOROUTINE_1_FRAMES / ((double)fishswim_len*2);
  //4 sconds intro
  for(i=0; i < PONYOROUTINE_1_FRAMES; i++)
  {
    //2 seconds
    if(i<PONYOROUTINE_1_FRAMES/2) {
      SetServo( (int)(85 * ((double)i/(PONYOROUTINE_1_FRAMES/2))) + SERVO_BASE );
    }

    matrixFrame = (int)((double)i / matrixStep) % fishswim_len;
    if(matrixFrame != lastMatrixFrame) {
      loadMatrixDataHexFrame(fishswim, matrixFrame);
      lastMatrixFrame = matrixFrame;
    }
    
    delay(G_STEP_MS);
  }
  
  //8 second, 2 second sway
  int swayStep = 0;
  matrixFrame = 0;
  lastMatrixFrame = -1;
  matrixStep = (PONYOROUTINE_2_FRAMES / (double)fishtojelly_len);
  for(i=0; i < PONYOROUTINE_2_FRAMES; i++)
  {
    matrixFrame = (int)((double)i / matrixStep) % fishtojelly_len;
    if(matrixFrame != lastMatrixFrame) {
      loadMatrixDataHexFrame(fishtojelly, matrixFrame);
      lastMatrixFrame = matrixFrame;
    }
    
    //Sway left to right 4 seconds cycle
    //2PI over 4 seconds.
    sinInput = ((double)swayStep / (PONYOROUTINE_2_FRAMES/2)) * (2*PI);
    //sinInput -= (PI/2); //offset for moving up sin wave
    yvalue = (sin(sinInput) + 1) / 2;
    xvalue = (int)((yvalue * PONYOROUTINE_SERVO_ANGLE_WIDTH) + PONYOROUTINE_SERVO_SHORT_ANGLE);
    SetServo(xvalue);
    //Take a break every second.
    if( ( (int)( (double)i / (1000/G_STEP_MS)) % 2) == 0 ) {
      swayStep++;
    }
    delay(G_STEP_MS);
  }

  matrixFrame = 0;
  lastMatrixFrame = -1;
  matrixStep = PONYOROUTINE_2_FRAMES / ((double)jellydance_len*2);
  for(i=0; i < PONYOROUTINE_2_FRAMES; i++)
  { 
    matrixFrame = (int)((double)i / matrixStep) % jellydance_len;
    if(matrixFrame != lastMatrixFrame) {
      if((matrixFrame/2) % 2 == 0) {
        SetServo(100);
      } else {
        SetServo(90);
      }
      loadMatrixDataHexFrame(jellydance, matrixFrame);
      lastMatrixFrame = matrixFrame;
    }
    
    delay(G_STEP_MS);
  }
}
