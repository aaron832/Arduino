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
#include "Time.h"

#define MAX_CHARS 17
#define LONG_WAIT 500
unsigned long SLEEP_TIME = 1000; // Sleep time between reports (in milliseconds)
unsigned long WAIT_MODE_TIME = 3600000;
#define CHILD_ID_NOTIFICATION 0   // Id of the sensor child
#define INPUT_BUTTON 2

//#define TRIGGER_SLEEPS_NO_RESPONSE 10
#define TRIGGER_SLEEPS_NO_RESPONSE 2
#define MAX_SLEEPS_NO_RESPONSE 0xFFFE
unsigned int sleeps_without_response = 0;

// Initialize sensor messages
MyMessage msg_notification(CHILD_ID_NOTIFICATION, V_TEXT);
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int buzzer = A2;
const int redled = A0;
const int greenled = A1;

time_t timeReceivedTime = 0;
unsigned long receivedTime = 0;
unsigned int displayindex = 0;

unsigned long lastUpdate=0, lastRequest=0;

struct Notification {
  time_t timeRec;
  char message[MAX_CHARS];
}

#define maxNotifications 10
Notification gNotifications[maxNotifications];
unsigned int nPointer = 0;

void setup()
{
  pinMode(buzzer, OUTPUT);
  pinMode(redled, OUTPUT);
  pinMode(greenled, OUTPUT);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.print("Starting up...");
  lcd.setCursor ( 0, 1 );
  lcd.print("Request Time...");

  requestTime();
  wait(2000, C_INTERNAL, I_TIME);
}

time_t GetCurrentTime() 
{
  return receivedTime + (now() - timeReceivedTime);
}

void DisplayClock()
{
  time_t currentTime = GetCurrentTime();

  char buf[MAX_CHARS];
  snprintf(buf, MAX_CHARS, "     %02d:%02d:%02d ", hour(currentTime), minute(currentTime), second(currentTime));
  writeScreen(buf, 0);
  writeScreen("", 1);
}

void UpdateDisplay()
{
  if(displayindex == 0)
  {
    DisplayClock();
  }
  else
  {
    //DisplayEvent(displayindex);
  }

  static bool ledfun = false;

  if(ledfun) {
    digitalWrite(redled, HIGH);
    digitalWrite(greenled, LOW);
  }
  else {
    digitalWrite(greenled, HIGH);
    digitalWrite(redled, LOW);
  }
  ledfun = !ledfun;
}

void writeScreen(const char* string, int sensorid)
{
  char buf[MAX_CHARS];
  for(int i=0; i<MAX_CHARS; i++) {
    buf[i] = ' ';
  }
  buf[MAX_CHARS-1] = '\0';
  int copylength = strlen(string);
  if(copylength > MAX_CHARS)
    copylength = MAX_CHARS;
  else if(copylength > 0)
    copylength--;  // do not copy null terminator
  strncpy(buf,string,copylength);
  //Serial.print("Display: ");
  //Serial.print(sensorid);
  //Serial.print(" S: ");
  //Serial.println(string);
  
  lcd.home();
  lcd.setCursor ( 0, sensorid );
  lcd.print(buf);
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("LCD Clock Display", "1.0");

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_NOTIFICATION, S_INFO);
}

void loop()
{
  unsigned long now = millis();

  // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update every hour
  if ((receivedTime == 0 && (now-lastRequest) > (10UL*1000UL))
    || (receivedTime != 0 && (now-lastRequest) > (60UL*1000UL*60UL))) {
    // Request time from controller. 
    Serial.println("requesting time");
    requestTime();
    wait(2000, C_INTERNAL, I_TIME);
    lastRequest = now;
  }

  // Update display every second
  if (now-lastUpdate > 1000) {
    UpdateDisplay();  
    lastUpdate = now;
  }

  static bool playSound = true;

  if(playSound) {
      tone(buzzer, 1000); // Send 1KHz sound signal...
      delay(500);        // ...for 1 sec
      tone(buzzer, 1200); // Send 1KHz sound signal...
      delay(500);        // ...for 1 sec
      noTone(buzzer);     // Stop sound...
      playSound = false;
  }
  
  //sleeps_without_response++;
  //if(sleeps_without_response > MAX_SLEEPS_NO_RESPONSE) {
  //  sleeps_without_response = MAX_SLEEPS_NO_RESPONSE;
  //}
  
	// Sleep until interrupt comes in on motion sensor. Send update every two minute.
  //request(CHILD_ID_LCDTEXT_BOTTOM, V_TEXT);
  //wait(LONG_WAIT);

  //if(sleeps_without_response >= TRIGGER_SLEEPS_NO_RESPONSE)
  //{
  //  char buf[MAX_CHARS];
  //  snprintf(buf,MAX_CHARS,"No Communication");
  //  writeScreen(buf,0);
  //  snprintf(buf,MAX_CHARS,"%i", sleeps_without_response);
  //  writeScreen(buf,1);
  //}

	//sleep(digitalPinToInterrupt(INPUT_BUTTON), CHANGE, SLEEP_TIME);

  //wait(WAIT_MODE_TIME);
}

// This is called when a message is received
void receive(const MyMessage &message) {
  if (message.sensor == CHILD_ID_NOTIFICATION && message.type == V_TEXT) 
  { // Text messages only
    sleeps_without_response = 0; 
    Serial.print("Notification Rec: "); 
    Serial.println(message.getString());     // Write some debug info
    //writeScreen(message.getString(), message.sensor);

    gNotifications[nPointer].timeRec = GetCurrentTime();
    strncpy(gNotifications[nPointer].message, message.getString(), MAX_CHARS);
    nPointer++;
    if(nPointer >= maxNotifications)
      nPointer = 0;
  }
}

// This is called when a new time value was received
void receiveTime(unsigned long rectime) {
  // Ok, set incoming time 
  Serial.print("Time value received: ");
  Serial.println(rectime);
  receivedTime = rectime;
  timeReceivedTime = now();
}
