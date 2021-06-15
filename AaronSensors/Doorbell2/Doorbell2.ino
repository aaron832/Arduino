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
 
// Enable debug prints
#define MY_DEBUG
#define MY_NODE_ID 109

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h> 

//Bounce2 2.2.0
#include <Bounce2.h>

#define SHORT_WAIT 1000
#define LONG_WAIT 500

//Children message IDs
#define CHILD_ID_BUTTON 0

//Arduino pins
#define LEDLIGHT_PIN LED_BUILTIN //13
#define SENSOR_PIN A0

MyMessage msgButton(CHILD_ID_BUTTON, V_TRIPPED);
unsigned long SLEEP_TIME = 10L; // Sleep time between reads (in milliseconds)
unsigned long MAX_HOLD_COUNT = (60L * 1000L) / SLEEP_TIME;  //1 minute
unsigned long hold_count = 0;

//Not really necessary as we are only checking every SLEEP_TIME of 1 second.
Bounce debouncerButton = Bounce();
int olddoorbellstate = -1;
int doorbellstate = -1;
int sensorValue = 0;
int lastSenseValue=0;
int sensNeededDelta = 50;
int sanityLevel = 700;

void setup()  
{
	// Setup the button
	pinMode(LEDLIGHT_PIN,OUTPUT);
	// Activate internal pull-up
	digitalWrite(LEDLIGHT_PIN,HIGH);

  lastSenseValue = analogRead(SENSOR_PIN);
}

void presentation()  
{ 
	// Send the Sketch Version Information to the Gateway
	sendSketchInfo("Doorbell Sensor", "2.0");
	wait(LONG_WAIT);
	present(CHILD_ID_BUTTON, S_DOOR, "Doorbell button sensor");
}

void loop()      
{ 
	sensorValue = analogRead(SENSOR_PIN);

  if((sensorValue > sanityLevel)) {
    if(sensorValue < (lastSenseValue - sensNeededDelta)) {
      doorbellstate = 1;
      hold_count++;
      if(hold_count >= MAX_HOLD_COUNT) 
      {
        lastSenseValue = sensorValue; //re-calibrate as someone likely isn't holding it down for a minute
      }
    }
    else {
      doorbellstate = 0;
      lastSenseValue = sensorValue; //only update lastSenseValue if its not pressed.
      hold_count = 0;
    }
  }
  else
  {
    doorbellstate = -1;
  }

  if(doorbellstate != olddoorbellstate)
  {
    // Send in the new value
    send(msgButton.set(doorbellstate));
    olddoorbellstate = doorbellstate;
  }

	//Serial.print(".");
	delay(SLEEP_TIME);
}
