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
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  
#include <Bounce2.h>

//Children message IDs
#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_BAYDOORSENSOR 2
#define CHILD_ID_DOORSENSOR 3

//Arduino pins
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
#define BAYDOORSENSOR_PIN 4
#define DOORSENSOR_PIN 5

unsigned long SLEEP_TIME = 1000; // Sleep time between reads (in milliseconds)

DHT dht;
float lastTemp;
float lastHum;
boolean metric = false; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgBayDoor(CHILD_ID_BAYDOORSENSOR, V_TRIPPED);
MyMessage msgDoor(CHILD_ID_DOORSENSOR, V_TRIPPED);

//Not really necessary as we are only checking every SLEEP_TIME of 1 second.
Bounce debouncerBayDoor = Bounce();
Bounce debouncerDoor = Bounce();
int bayDoorState=-1;
int doorState=-1;

void setup()  
{
  //Temp Humid sensor setup
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  //Supposed to be configurable from server.  Whatever.
  //metric = getConfig().isMetric;
  
  // Setup the button
  pinMode(BAYDOORSENSOR_PIN,INPUT);
  pinMode(DOORSENSOR_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BAYDOORSENSOR_PIN,HIGH);
  digitalWrite(DOORSENSOR_PIN,HIGH);
    
  // After setting up the button, setup debouncer
  debouncerBayDoor.attach(BAYDOORSENSOR_PIN);
  debouncerDoor.attach(DOORSENSOR_PIN);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
}

void loop()      
{ 
  Serial.println("Starting...");
  delay(dht.getMinimumSamplingPeriod());
 
  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  }
  
  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      #ifdef MY_DEBUG
      Serial.print("H: ");
      Serial.println(humidity);
      #endif
  }
  
  //Check the door sensors.
  debouncerBayDoor.update();
  debouncerDoor.update();
  
  int value = 0;
  value = debouncerBayDoor.read();
  if (value != bayDoorState) {
     // Send in the new value
     send(msgBayDoor.set(value==HIGH ? 1 : 0));
     bayDoorState = value;
  }
  
  value = debouncerDoor.read();
  if (value != doorState) {
     // Send in the new value
     send(msgDoor.set(value==HIGH ? 1 : 0));
     doorState = value;
  }
  
  sleep(SLEEP_TIME); //sleep a bit
}
