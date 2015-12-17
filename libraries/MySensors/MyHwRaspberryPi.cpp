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

#ifdef RASPBERRYPI_ARCH

#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include "MyHw.h"
#include "MyHwRaspberryPi.h"

static uint8_t * _config;
static size_t _length;
static unsigned long millis_at_start;

void hw_init()
{
	timeval curTime;

	gettimeofday(&curTime, NULL);
	millis_at_start = curTime.tv_sec;
}

static void hw_initConfigBlock(size_t length = 1024 /*ATMega328 has 1024 bytes*/)
{
	static bool initDone = false;

	if (!initDone) {
		_config = new uint8_t[length];
		_length = length;
		initDone = true;
	}
}

void hw_readConfigBlock(void* buf, void* adr, size_t length)
{
	hw_initConfigBlock();
	int offs = reinterpret_cast<int>(adr);

	if (offs + length < _length) {
		memcpy(buf, _config+offs, length);
	}
}

void hw_writeConfigBlock(void* buf, void* adr, size_t length)
{
	hw_initConfigBlock();
	int offs = reinterpret_cast<int>(adr);

	if (offs + length < _length) {
		memcpy(_config+offs, buf, length);
	}
}

uint8_t hw_readConfig(int adr)
{
	uint8_t value;

	hw_readConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
	return value;
}

void hw_writeConfig(int adr, uint8_t value)
{
	uint8_t curr = hw_readConfig(adr);

	if (curr != value) {
		hw_writeConfigBlock(&value, reinterpret_cast<void*>(adr), 1); 
	}
}

unsigned long hw_millis()
{
	timeval curTime;

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000) + (curTime.tv_usec / 1000);
}

MyHwRaspberryPi::MyHwRaspberryPi() : MyHw()
{
}

void MyHwRaspberryPi::sleep(unsigned long ms)
{
	// TODO: Not supported!
}

bool MyHwRaspberryPi::sleep(uint8_t interrupt, uint8_t mode, unsigned long ms)
{
	// TODO: Not supported!
	return false;
}

inline uint8_t MyHwRaspberryPi::sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms)
{
	// TODO: Not supported!
	return 0;
}

#ifdef DEBUG
void MyHwRaspberryPi::debugPrint(bool isGW, const char *fmt, ... )
{
	va_list arglist;

	printf("Debug: ");
	va_start(arglist, fmt);
	vprintf(fmt, arglist);
	va_end(arglist);
}
#endif

#endif  // #ifdef RASPBERRYPI_ARCH
