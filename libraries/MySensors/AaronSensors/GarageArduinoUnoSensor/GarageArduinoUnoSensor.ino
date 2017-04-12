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

#define MY_NODE_ID 105

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <DHT.h>  
#include <Bounce2.h>

#define SHORT_WAIT 50
#define LONG_WAIT 500
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

//Children message IDs
#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_BAYDOORSENSOR 2
#define CHILD_ID_DOORSENSOR 3
#define CHILD_ID_RELAY1 4
#define CHILD_ID_TEXT1 5

//Arduino pins
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
#define BAYDOORSENSOR_PIN 4
#define DOORSENSOR_PIN 5
#define RELAY1_PIN 8
#define DEBUG_LED_PIN 7

unsigned long FORCE_REFRESH = 60L*1000L;
unsigned long DHT_REFRESH = 30L*1000L;
unsigned long SLEEP_TIME = 1000L; // Sleep time between reads (in milliseconds)

DHT dht(HUMIDITY_SENSOR_DIGITAL_PIN,DHT11);
float lastTemp;
float lastHum;
boolean metric = false; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgBayDoor(CHILD_ID_BAYDOORSENSOR, V_TRIPPED);
MyMessage msgDoor(CHILD_ID_DOORSENSOR, V_TRIPPED);
MyMessage msgBayDoorSwitch(CHILD_ID_RELAY1, V_LIGHT);
MyMessage msgTextStatus(CHILD_ID_TEXT1, V_TEXT);

//Not really necessary as we are only checking every SLEEP_TIME of 1 second.
Bounce debouncerBayDoor = Bounce();
Bounce debouncerDoor = Bounce();
int bayDoorState=-1;
int doorState=-1;
int firstRun=1;
unsigned long time = 0;
unsigned long dhtRefreshTime = 0;
unsigned long forceRefreshTime = 0;
int forceRefresh = 0;
bool sendOk = false;

void setup()  
{
	//Supposed to be configurable from server.  Whatever.
	//metric = getConfig().isMetric;

	// Setup the button
	pinMode(BAYDOORSENSOR_PIN,INPUT);
	pinMode(DOORSENSOR_PIN,INPUT);
	// Activate internal pull-up
	digitalWrite(BAYDOORSENSOR_PIN,HIGH);
	digitalWrite(DOORSENSOR_PIN,HIGH);

	//Setup Relays
	pinMode(RELAY1_PIN, OUTPUT);
	digitalWrite(RELAY1_PIN, RELAY_OFF);
	
	//Start debug led on
	digitalWrite(DEBUG_LED_PIN,HIGH);

	// After setting up the button, setup debouncer
	debouncerBayDoor.attach(BAYDOORSENSOR_PIN);
	debouncerDoor.attach(DOORSENSOR_PIN);
}

void presentation()  
{ 
	// Send the Sketch Version Information to the Gateway
	sendSketchInfo("Garage Sensor", "1.0");
	wait(LONG_WAIT);

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_HUM, S_HUM, "DHT11 Sensor - Humidity");
	wait(LONG_WAIT);
	present(CHILD_ID_TEMP, S_TEMP, "DHT11 Sensor - Temperature");
	wait(LONG_WAIT);
	present(CHILD_ID_BAYDOORSENSOR, S_DOOR, "Garage Door magnet sensor");
	wait(LONG_WAIT);
	present(CHILD_ID_DOORSENSOR, S_DOOR, "Back Door magnet sensor");
	wait(LONG_WAIT);
	present(CHILD_ID_RELAY1, S_LIGHT, "Bay Door Relay");
	wait(LONG_WAIT);
	present(CHILD_ID_TEXT1, S_INFO, "Device Status");
	wait(LONG_WAIT);
}

void loop()      
{ 
	//Time and refresh sanity
	time = millis();
	if(time < dhtRefreshTime)
		dhtRefreshTime = time;
	if(time < forceRefreshTime)
		forceRefreshTime = time;
		
	if(forceRefreshTime + FORCE_REFRESH < time)
		forceRefresh = 1;
		
	//delay(dht.getMinimumSamplingPeriod());

	if(firstRun)
	{
		//Clear buttons
		sendOk = send(msgBayDoorSwitch.set(0));

		Serial.println("Setting up subscriptions...");
		wait(LONG_WAIT);
		request(CHILD_ID_RELAY1, V_LIGHT);
		wait(LONG_WAIT);
		firstRun = 0;
		
		//Set Status
		sendOk = send(msgTextStatus.set("Started"));
		wait(SHORT_WAIT);
	}
 
	if(dhtRefreshTime + DHT_REFRESH < time)
	{
		// Fetch temperatures from DHT sensor
		float temperature = dht.readTemperature();
		if (isnan(temperature)) {
			#ifdef MY_DEBUG
			Serial.println("Failed reading temperature from DHT");
			#endif
			sendOk = send(msgTextStatus.set("DHT Fail Temp"));
			wait(SHORT_WAIT);
		} else if (temperature != lastTemp) {
			lastTemp = temperature;
			if (!metric) {
				temperature = dht.convertCtoF(temperature);
			}
			sendOk = send(msgTemp.set(temperature,1));
			wait(SHORT_WAIT);
			#ifdef MY_DEBUG
			Serial.print("T: ");
			Serial.println(temperature);
			#endif
		}
  
		// Fetch humidity from DHT sensor
		float humidity = dht.readHumidity();
		if (isnan(humidity)) {
			#ifdef MY_DEBUG
			Serial.println("Failed reading humidity from DHT");
			#endif
			sendOk = send(msgTextStatus.set("DHT Fail Humid"));
			wait(SHORT_WAIT);
		} else if (humidity != lastHum) {
			lastHum = humidity;
			sendOk = send(msgHum.set(humidity,1));
			wait(SHORT_WAIT);
			#ifdef MY_DEBUG
			Serial.print("H: ");
			Serial.println(humidity);
			#endif
		}
		
		dhtRefreshTime = time;
	}
  
	//Check the door sensors.
	debouncerBayDoor.update();
	debouncerDoor.update();

	int value = 0;
	value = debouncerBayDoor.read();
	if (value != bayDoorState || forceRefresh) {
		// Send in the new value
		sendOk = send(msgBayDoor.set(value==HIGH ? 1 : 0));
		wait(SHORT_WAIT);
		bayDoorState = value;
	}

	value = debouncerDoor.read();
	if (value != doorState || forceRefresh) {
		// Send in the new value
		sendOk = send(msgDoor.set(value==HIGH ? 1 : 0));
		wait(SHORT_WAIT);
		doorState = value;
	}
	
	//Reset force refresh
	if(forceRefresh)
	{
		forceRefreshTime = time;
		forceRefresh = 0;
	}

	digitalWrite(DEBUG_LED_PIN,sendOk ? LOW : HIGH);
	//This sleep will prevent messages from being received.  Meant for sensors that only send data.
	//sleep(SLEEP_TIME); //sleep a bit
	Serial.print(".");
	delay(SLEEP_TIME);
}

void receive(const MyMessage &message) {
	// We only expect one type of message from controller. But we better check anyway.
	if (message.sensor == CHILD_ID_RELAY1) {
		if (message.type==V_LIGHT) {
			if(message.getInt() == 1) {
				// Turn on the relay briefly.
				#ifdef MY_DEBUG
				Serial.println("Relay on.");
				#endif
				digitalWrite(RELAY1_PIN, RELAY_ON);
				wait(LONG_WAIT);
				#ifdef MY_DEBUG
				Serial.println("Relay off.");
				#endif
				digitalWrite(RELAY1_PIN, RELAY_OFF);
				
				//Reply with turning the value to 0 again.
				sendOk = send(msgBayDoorSwitch.set(0));
			}
		}
	}
}
