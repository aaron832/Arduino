Arduino
=======

MySensors Arduino Library v1.5

Please visit www.mysensors.org for more information

Current build status of master branch: [![Build Status](http://ci.mysensors.org/job/MySensorsArduino/branch/master/badge/icon)](http://ci.mysensors.org/job/MySensorsArduino/branch/master/)

Current build status of development branch: [![Build Status](http://ci.mysensors.org/job/MySensorsArduino/branch/development/badge/icon)](http://ci.mysensors.org/job/MySensorsArduino/branch/development/)


RaspberryPi
=======

#Wiring the NRF	24L01+ radio

|NRF24l01+|Rpi Header Pin|
|---|---|
|GND|25|
|VCC|17|
|CE|22|
|CSN|24|
|SCK|23|
|MOSI|19|
|MISO|21|
|IRQ|23|

#Building & Installing

##RF24 library
* Clone the current version `git clone https://github.com/TMRh20/RF24`
* Change to the library directory (RF24)
* Run `make all` followed by `sudo make install`

##Build the Gateway
* Clone this repository
* Change to Arduino/libraries/MySensors directory
* Run `make all` followed by `sudo make install`

###Serial Gateway

If you want to start daemon at boot `sudo make enable-gwserial`

The standard configuration will build the Serial Gateway with a tty name of
'/dev/ttyMySensorsGateway' and PTS group ownership of 'tty' the PTS will be group read
and write. The default install location will be /usr/local/sbin. If you want to change
that edit the variables in the head of the Makefile.

For some controllers a more recognisable name needs to be used: e.g. /dev/ttyUSB020 (check if this is free).

`sudo ln -s /dev/ttyMySensorsGateway /dev/ttyUSB20`

To automatically create the link on startup, add `ln -s /dev/ttyMySensorsGateway /dev/ttyUSB20` just before `exit0` in `/etc/rc.local`

###Ethernet Gateway

If you want to start daemon at boot `sudo make enable-gwethernet`

To use interrupt:
* Make sure you are using the latest rf24 library version
* Install http://wiringpi.com/
* Connect the nRF24L01 IRQ pin to a free raspberry gpio pin
* Uncomment `#define USE_INTERRUPT` in libraries/MySensors/PiGatewayEthernet.cpp
* Re-run `make all` and `sudo make install` to apply the changes

#Uninstalling

* Change to Arduino/libraries/MySensors directory
* Run `sudo make uninstall`

Support: http://forum.mysensors.org
