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
 
//In case MySensor network is no longer available.
//#define STANDALONE_MODE
 
// Enable debug prints
//#define MY_DEBUG

#ifndef STANDALONE_MODE
// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_NODE_ID 10
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC

//Children message IDs
#define CHILD_ID_RELAY1 0

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

unsigned long SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)

MyMessage msgMusicBoxSwitch1(CHILD_ID_RELAY1, V_LIGHT);

static bool activateBox = false;
void ActivateBox()
{
	activateBox = true;
}

void MusicBoxLogic()
{
	Serial.println("Playing Box...");
}

//This doesn't seem to work... at least with using MySensors and not being connected.
//I know it works without mysensors.  Possible it only works when Mysensor is connected
//when using mysenesors.
void ButtonInterrupt()
{
	Serial.println("Interrupt...");
	ActivateBox();
}

void setup()  
{
	// Setup the buttons
	pinMode(BUTTON_PIN, INPUT);

    // Activate internal pull-ups
    digitalWrite(BUTTON_PIN, HIGH);
	
	//Attach interrupt to the button.
	attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonInterrupt, RISING);
}

void presentation()  
{ 
	// Send the Sketch Version Information to the Gateway
	sendSketchInfo("Music Box", "1.0");
	wait(LONG_WAIT);

	present(CHILD_ID_RELAY1, S_LIGHT, "Open Box");
	wait(LONG_WAIT);
}

void loop()      
{ 
	uint8_t value;
	static uint8_t lastValue=2;
	
	Serial.println("Loop...");
	
	//We were sleeping... request the status of our input.
	request(CHILD_ID_RELAY1, S_LIGHT);
	
	//Wait two seconds for a response.
	//Might get a button event which hopefully the attached interrupt will get.
	wait(2000);
	
	//If we got the button interrupt, then play the box.
	if(activateBox)
	{
		MusicBoxLogic();
		activateBox = false;
	}
	
	//This sleep will will cause the radio to power down.
	//Cant do that if we actually want to be able to sent it commands.
	sleep(BUTTON_PIN-2, CHANGE, SLEEP_TIME);
	
	//This is if we get interrupt during sleep from the button.
	value = digitalRead(BUTTON_PIN);
	if (value==HIGH) {
		MusicBoxLogic();
	}
}

void receive(const MyMessage &message) {
	// We only expect one type of message from controller. But we better check anyway.
	if (message.sensor == CHILD_ID_RELAY1) {
		if (message.type==V_LIGHT) {
			if(message.getInt() == 1) {
				// Turn on the relay briefly.
				Serial.println("We Activated!");

				ActivateBox();
        
				//Reply with turning the value to 0 again.
				send(msgMusicBoxSwitch1.set(0));
			}
		}
	}
}
