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
 */

#ifndef MyTransportNRF24_h
#define MyTransportNRF24_h

#include "MyConfig.h"
#include "MyTransport.h"
#include <stdint.h>
#if defined(ARDUINO)
	#include "utility/RF24.h"
	#include "utility/RF24_config.h"
#else
	typedef bool boolean;
	#include "RF24.h"
	#include "RF24_config.h"
#endif

#define TO_ADDR(x) (RF24_BASE_RADIO_ID + x)

#define WRITE_PIPE ((uint8_t)0)
#define CURRENT_NODE_PIPE ((uint8_t)1)
#define BROADCAST_PIPE ((uint8_t)2)

class MyTransportNRF24 : public MyTransport
{ 
public:
	MyTransportNRF24(uint8_t ce=RF24_CE_PIN, uint8_t cs=RF24_CS_PIN, uint8_t paLevel=RF24_PA_LEVEL);
#ifdef RF24_RPi
	MyTransportNRF24(uint8_t ce, uint8_t cs, uint8_t paLevel, uint32_t spispeed);
#endif
	bool init();
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data, uint8_t len);
	bool available(uint8_t *to);
	uint8_t receive(void* data);
	void powerDown();
	void maskIRQ(bool tx_ok, bool tx_fail, bool rx_ready);
private:
	RF24 rf24;
	uint8_t _address;
	uint8_t _paLevel;
};

#endif
