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
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h> 
//Bounce2 2.2.0
#include <Bounce2.h>
//LedControl 1.0.6
#include <LedControl.h>

#define SHORT_WAIT 50
#define LONG_WAIT 500
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

//Children message IDs
#define CHILD_ID_RELAY1 0

//Arduino pins
#define BUTTON_PIN 2
#define RELAY1_PIN 6

unsigned long SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)

MyMessage msgMusicBoxSwitch1(CHILD_ID_RELAY1, V_LIGHT);

Bounce debouncerButton = Bounce();

static bool playingBox = false;
void MusicBoxLogic()
{
	if(playingBox)
		return;
	playingBox = true;
	
	Serial.println("Playing Box...");
	
	playingBox = false;
}

void setup()  
{
	// Setup the buttons
	pinMode(BUTTON_PIN, INPUT);

    // Activate internal pull-ups
    digitalWrite(BUTTON_PIN, HIGH);
	
	debouncerButton.attach(BUTTON_PIN);
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
	
	// Short delay to allow buttons to properly settle
	sleep(5);
	
	debouncerButton.update();
	value = debouncerButton.read();
	if (value != lastValue) {
		lastValue = value;
		if (value==HIGH) {
			MusicBoxLogic();
		}
	}
	
	sleep(BUTTON_PIN-2, CHANGE, SLEEP_TIME);
}

void receive(const MyMessage &message) {
	// We only expect one type of message from controller. But we better check anyway.
	if (message.sensor == CHILD_ID_RELAY1) {
		if (message.type==V_LIGHT) {
			if(message.getInt() == 1) {
				// Turn on the relay briefly.
				Serial.println("We Activated!");

				MusicBoxLogic();
        
				//Reply with turning the value to 0 again.
				send(msgMusicBoxSwitch1.set(0));
			}
		}
	}
}
