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
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * Motion Sensor example using HC-SR501
 * http://www.mysensors.org/build/motion
 *
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_NODE_ID 11

#include <MySensors.h>
#include <LiquidCrystal.h>

unsigned long SLEEP_TIME = 1000; // Sleep time between reports (in milliseconds)
#define CHILD_ID_LCDTEXT 1   // Id of the sensor child
#define INPUT_BUTTON 2

// Initialize motion message
MyMessage msg(CHILD_ID_LCDTEXT, V_TEXT);
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.print("Starting up...");
}

void writeScreen(const char* string)
{
  lcd.print(string);
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("LCD Clock Display", "1.0");

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_LCDTEXT, S_INFO);
}

void loop()
{
	// Sleep until interrupt comes in on motion sensor. Send update every two minute.
	sleep(digitalPinToInterrupt(INPUT_BUTTON), CHANGE, SLEEP_TIME);
}

// This is called when a message is received
void receive(const MyMessage &message) {
  if (message.type == V_TEXT) {                       // Text messages only
    Serial.print("Message: "); 
    Serial.print(message.sensor); 
    Serial.print(", Message: "); 
    Serial.println(message.getString());     // Write some debug info
    if (message.sensor == CHILD_ID_LCDTEXT) {
      writeScreen(message.getString());
    }
  }
}


