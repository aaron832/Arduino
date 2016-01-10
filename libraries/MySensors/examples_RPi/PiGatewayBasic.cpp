/*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created  by OUJABER Mohamed <m.oujaber@gmail.com>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <MyTransportNRF24.h>
#include <MySigningNone.h>
#include <MySigningAtsha204Soft.h>
#include <MySensor.h>
#include <RF24.h>

using namespace std;

void msgCallback(const MyMessage &message);

int main()
{
	MySensor *gw;
	
	cout << "Starting Gateway..." << endl;
	
	// NRFRF24L01 radio driver
#ifdef __PI_BPLUS
	MyTransportNRF24 transport(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#else
	MyTransportNRF24 transport(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#endif

	// Hardware profile
	MyHwDriver hw;
	
	// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
#ifdef MY_SIGNING_FEATURE
	//MySigningNone signer;
	MySigningAtsha204Soft signer;
#endif
	
	// Construct MySensors library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
	gw = new MySensor(transport, hw
#ifdef MY_SIGNING_FEATURE
	, signer
#endif
	);

	if (gw == NULL)
	{
		cout << "Failed to initialize MySensors library" << endl;
	}

	/* initialize the Gateway */
	gw->begin(&msgCallback, 0, true, 0);

	while(1) {
		/* process radio msgs */
		gw->process();
		usleep(10000);	// 10ms
	}

	return 0;
}

void msgCallback(const MyMessage &message)
{
	char convBuf[MAX_PAYLOAD*2+1];

	printf("[CALLBACK]%d;%d;%d;%d;%d;%s\n", message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type, message.getString(convBuf));
}
