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

#ifndef MyHwRaspberryPi_h
#define MyHwRaspberryPi_h

#include "MyHw.h"
#include "MyConfig.h"
#include "MyMessage.h"

// Define these as macros (do nothing)
#define hw_digitalWrite(__pin, __value)
#define hw_watchdogReset()
#define hw_reboot()

void hw_init();
void hw_readConfigBlock(void* buf, void* adr, size_t length);
void hw_writeConfigBlock(void* buf, void* adr, size_t length);
void hw_writeConfig(int adr, uint8_t value);
uint8_t hw_readConfig(int adr);
unsigned long hw_millis();

class MyHwRaspberryPi : public MyHw
{ 
public:
	MyHwRaspberryPi();

/*	void init();
	void watchdogReset();
	void reboot();
	unsigned long millis();
	uint8_t readConfig(uint8_t pos);
	void writeConfig(uint8_t pos, uint8_t value);
	void readConfigBlock(void* buf, void * pos, size_t length);
	void writeConfigBlock(void* pos, void* buf, size_t length); */

	void sleep(unsigned long ms);
	bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms);
	uint8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms);
#ifdef DEBUG
	void debugPrint(bool isGW, const char *fmt, ... );
#endif
};
#endif

#endif // #ifdef RASPBERRYPI_ARCH
