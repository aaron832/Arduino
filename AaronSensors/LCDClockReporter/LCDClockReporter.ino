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

#define MAX_CHARS 16
#define CLOCK_CHAR_OFFSET 6
#define LONG_WAIT 500
unsigned long SLEEP_TIME = 1000; // Sleep time between reports (in milliseconds)
unsigned long WAIT_MODE_TIME = 3600000;
#define CHILD_ID_LCDTEXT_TOP 0   // Id of the sensor child
#define CHILD_ID_LCDTEXT_BOTTOM 1   // Id of the sensor child
#define CHILD_ID_LCDBRIGHTNESS 2   // Id of the sensor child
#define CHILD_ID_BUTTONLEFT 3   // Id of the sensor child
#define CHILD_ID_BUTTONMID 4   // Id of the sensor child
#define CHILD_ID_BUTTONRIGHT 5   // Id of the sensor child

//#define TRIGGER_SLEEPS_NO_RESPONSE 10
#define TRIGGER_SLEEPS_NO_RESPONSE 2
#define MAX_SLEEPS_NO_RESPONSE 0xFFFE
unsigned int sleeps_without_response = 0;

// Initialize motion message
MyMessage msg_top(CHILD_ID_LCDTEXT_TOP, V_TEXT);
MyMessage msg_bottom(CHILD_ID_LCDTEXT_BOTTOM, V_TEXT);
MyMessage msg_brightness(CHILD_ID_LCDBRIGHTNESS, V_PERCENTAGE);
MyMessage msg_buttonLeft(CHILD_ID_BUTTONLEFT, V_STATUS);
MyMessage msg_buttonMid(CHILD_ID_BUTTONMID, V_STATUS);
MyMessage msg_buttonRight(CHILD_ID_BUTTONRIGHT, V_STATUS);
const int rs = 8, en = 7, d4 = 3, d5 = 4, d6 = 5, d7 = 6;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int buzzer = A1;
const int input_button_left = A7;
const int input_button_mid = A5;
const int input_button_right = A6;
int left_button_read, left_button, left_button_last = 0;
int mid_button_read, mid_button, mid_button_last = 0;
int right_button_read, right_button, right_button_last = 0;
unsigned long debounceDelay = 50;
unsigned long lastDebounceTime = 0;
bool sendOk = false;

const int lsize = 3;
const int leds[] = {A0, A2, A3};
int setBrightness, tmpBrightness = 0;
int brightnessTimeout = 10000;  //10 seconds

time_t timeReceivedTime = 0;
unsigned long receivedTime = 0;
unsigned int displayindex = 0;

unsigned long lastUpdate=0, lastRequest=0, brightnessTimer=0;
bool displayMsgRec[2] = {false, false};

#define MAX_SPECIAL_CHAR 8
#define MAX_ALERT_CHAR 4
byte specialchars[MAX_SPECIAL_CHAR][8] = {
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 },
  {  B00000, B01010, B11111, B11111, B01110, B00100, B00000 }
};

void setup()
{
  pinMode(buzzer, OUTPUT);
  pinMode(input_button_left, INPUT);
  pinMode(input_button_mid, INPUT);
  pinMode(input_button_right, INPUT);
  
  for(int i=0; i<lsize; i++) {
    pinMode(leds[i], OUTPUT);
  }
  UpdateBrightness(4);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  for(int i=0; i<MAX_SPECIAL_CHAR; i++) {
    lcd.createChar(i, specialchars[i]);
  }

  writeScreen("Starting..", 0);
  char buf[MAX_CHARS];
  strncpy(buf, " I LOVE YOU EMI ", MAX_CHARS);
  buf[0] = buf[MAX_CHARS-1] = 0x01;
  writeScreen(buf, 1);

  bool requestPass = false;

  while(!requestPass) {
    Serial.println("Setup request time..");
    requestPass = requestTime();
    wait(2000, C_INTERNAL, I_TIME);
  }
  requestPass = false;

  while(!requestPass) {
    Serial.println("Setup request top lcd..");
    requestPass = request(CHILD_ID_LCDTEXT_TOP, V_TEXT);
    wait(2000, C_SET, V_TEXT);
  }
  requestPass = false;

  while(!requestPass) {
    Serial.println("Setup request bottom lcd..");
    requestPass = request(CHILD_ID_LCDTEXT_BOTTOM, V_TEXT);
    wait(2000, C_SET, V_TEXT);
  }
  requestPass = false;

  while(!requestPass) {
    Serial.println("Setup request brightness..");
    requestPass = request(CHILD_ID_LCDBRIGHTNESS, V_PERCENTAGE);
    wait(2000, C_SET, V_PERCENTAGE);
  }
  requestPass = false;
}

void DisplayClock()
{
  time_t currentTime = receivedTime + (now() - timeReceivedTime);

  char buf[CLOCK_CHAR_OFFSET+1];
  lcd.setCursor ( 0, 0 );
  snprintf(buf, CLOCK_CHAR_OFFSET+1, "%02d:%02d ", hour(currentTime), minute(currentTime), second(currentTime));
  lcd.print(buf);
}

void TempIncreaseBrightness()
{
  if(setBrightness < 2)
    UpdateBrightness(2);
  else
    UpdateBrightness(min(setBrightness+1, 4));
}

void UpdateBrightness(int level)
{
  if(level < 0)
    level = setBrightness;
  if(level > 4)
    level = 4;

  tmpBrightness = level;
  
  Serial.print("Brightness:");
  Serial.print(level & (1 << 0));
  Serial.print(",");
  Serial.print(level & (1 << 1));
  Serial.print(",");
  Serial.println(level & (1 << 2));
  
  digitalWrite(leds[2], level & (1 << 0));
  digitalWrite(leds[1], level & (1 << 1));
  digitalWrite(leds[0], level & (1 << 2));

  brightnessTimer = millis();
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
}

void PerformAlert(char alertChar) {
  //nothing yet
  int alertLevel = (int)alertChar - MAX_SPECIAL_CHAR;
  Serial.print("Alert Char: ");
  Serial.print((int)alertChar);
  Serial.print(" Alert Level: ");
  Serial.println(alertLevel);
}

void writeScreen(const char* string, int sensorid)
{
  bool hasAlertChar = false;
  char buf[MAX_CHARS+1];
  int firstChar = 0;
  int lastChar = MAX_CHARS;
  if(sensorid == 0)
    lastChar = (MAX_CHARS - CLOCK_CHAR_OFFSET);
  if(string && string[0] >= MAX_SPECIAL_CHAR+1 && string[0] < MAX_SPECIAL_CHAR+1+MAX_ALERT_CHAR) {
    PerformAlert(string[0]);
    firstChar++;
  }

  //Clear out buffer and put null terminator at end
  for(int i=0; i<=MAX_CHARS; i++) {
    buf[i] = ' ';
  }
  buf[lastChar] = '\0';

  //Do not want to copy over the null terminator.
  strncpy(buf,&string[firstChar],min(lastChar,strlen(string)));
  //Serial.print("Display: ");
  //Serial.print(sensorid);
  //Serial.print(" S: ");
  //Serial.println(string);
  
  lcd.home();
  if(sensorid == 0)
    lcd.setCursor ( CLOCK_CHAR_OFFSET, sensorid );
  else
    lcd.setCursor ( 0, sensorid );
    
  //lcd.print(buf);
  for(int i=0; i<lastChar; i++) {
    if(buf[i] < MAX_SPECIAL_CHAR+1 && buf[i] > 0) {
      lcd.write(byte(buf[i]-1));
    }
    else {
      lcd.write(buf[i]);
    }
  }
}

void CheckButtons()
{
  left_button_read = analogRead(input_button_left) > 512 ? HIGH : LOW;
  mid_button_read = analogRead(input_button_mid) > 512 ? HIGH : LOW;
  right_button_read = analogRead(input_button_right) > 512 ? HIGH : LOW;

  if (left_button_read != left_button_last ||
      mid_button_read != mid_button_last ||
      right_button_read != right_button_last) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    Serial.print(left_button_read);
    Serial.print(",");
    Serial.print(mid_button_read);
    Serial.print(",");
    Serial.println(right_button_read);
  }
  if((millis() - lastDebounceTime) > debounceDelay)
  {
    if(left_button != left_button_read) {
      TempIncreaseBrightness();
      if(left_button_read == HIGH) {
        Serial.println("Sending left button.");
        sendOk = send(msg_buttonLeft.set(1));
      }
      left_button = left_button_read;
    }
    if(mid_button != mid_button_read) {
       TempIncreaseBrightness();
       if(mid_button_read == HIGH) {
         Serial.println("Sending mid button.");
         sendOk = send(msg_buttonMid.set(1));
       }
       mid_button = mid_button_read;
    }
    if(right_button != right_button_read) {
      TempIncreaseBrightness();
      if(right_button_read == HIGH) {
        Serial.println("Sending right button.");
        sendOk = send(msg_buttonRight.set(1));
      }
      right_button = right_button_read;
    }
  }

  left_button_last = left_button_read;
  mid_button_last = mid_button_read;
  right_button_last = right_button_read;
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("LCD Clock Display", "1.0");

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_LCDTEXT_TOP, S_INFO);
  present(CHILD_ID_LCDTEXT_BOTTOM, S_INFO);
}

void loop()
{
  unsigned long now_ms = millis();

  bool receivedAllInfo = (receivedTime != 0) && 
    displayMsgRec[CHILD_ID_LCDTEXT_TOP] && displayMsgRec[CHILD_ID_LCDTEXT_BOTTOM];
  
  //If we have not received information from controller in setup, then keep
  //requesting every 10 seconds.  Request regardless in hour increments.
  if ( (!receivedAllInfo && (now_ms-lastRequest) > (10UL*1000UL)) ||
       (receivedAllInfo == true && (now_ms-lastRequest) > (60UL*1000UL*60UL)) )
  {
    if(receivedTime == 0) {
      requestTime();
      wait(2000, C_INTERNAL, I_TIME);
    }
    if(!displayMsgRec[CHILD_ID_LCDTEXT_TOP]) {
      request(CHILD_ID_LCDTEXT_TOP, V_TEXT);
      wait(2000, C_INTERNAL, V_TEXT);
    }
    //if(!displayMsgRec[CHILD_ID_LCDTEXT_BOTTOM]) {
    //  request(CHILD_ID_LCDTEXT_BOTTOM, V_TEXT);
    //  wait(2000, C_INTERNAL, V_TEXT);
    //}

    lastRequest = now_ms;
  }

  // Update display every second
  if (now_ms-lastUpdate > 1000) {
    UpdateDisplay();  
    lastUpdate = now_ms;
  }

  if(brightnessTimer != 0 && tmpBrightness != setBrightness &&
  (brightnessTimer + brightnessTimeout) < now_ms)
  {
    UpdateBrightness(-1);
  }
  
  static bool startupComplete = true;
  if(startupComplete) {
      tone(buzzer, 1000); // Send 1KHz sound signal...
      delay(50);        // ...for 1 sec
      tone(buzzer, 1200); // Send 1KHz sound signal...
      delay(50);        // ...for 1 sec
      noTone(buzzer);     // Stop sound...
      startupComplete = false;
  }

  CheckButtons();
}

// This is called when a message is received
void receive(const MyMessage &message) {
  Serial.println("Got a message");
  if (message.type == V_TEXT) {                       // Text messages only
    sleeps_without_response = 0;
    Serial.print("Sensor: "); 
    Serial.print(message.sensor); 
    Serial.print(", Message: "); 
    Serial.println(message.getString());     // Write some debug info
    if (message.sensor == CHILD_ID_LCDTEXT_TOP ||
        message.sensor == CHILD_ID_LCDTEXT_BOTTOM) {
      displayMsgRec[message.sensor] = true;
      writeScreen(message.getString(), message.sensor);
    }
  }
  else if(message.sensor == CHILD_ID_LCDBRIGHTNESS && message.type == V_PERCENTAGE) {
    int percent = message.getInt();

    if(percent == 0)
      setBrightness = 0;
    else if(percent > 0 && percent <= 10)
      setBrightness = 1;
    else if(percent > 10 && percent <= 20)
      setBrightness = 2;
    else if(percent > 20 && percent <= 30)
      setBrightness = 3;
    else
      setBrightness = 4;

    Serial.print("Got Percent:");
    Serial.print(percent);
    Serial.print("  Update brightness:");
    Serial.println(setBrightness);
    UpdateBrightness(-1);
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
