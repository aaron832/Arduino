##########################################################################
# Configurable options                                                   #
##########################################################################
# Install Base location
PREFIX=/usr/local
# Bin Dir
BINDIR=$(PREFIX)/sbin
# Set the name of predictable tty
TTY_NAME := /dev/ttyMySensorsGateway
# Set the group name for the raw tty
TTY_GROUPNAME := tty
##########################################################################
# Please do not change anything below this line                          #
##########################################################################
CC=g++

ARCH=armv6zk
ifeq "$(shell uname -m)" "armv7l"
ARCH=armv7-a
endif

# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=$(ARCH) -mtune=arm1176jzf-s -DRASPBERRYPI_ARCH $(OPTFLAGS)
#CCFLAGS=-g -Wall -Wextra

MYSENSORS_LIB=build/libmysensors.a
SO_MYSENSORS_LIB=$(patsubst %.a,%.so,$(MYSENSORS_LIB))
LIB_MYSENSORS_SRCS=$(wildcard ./*.cpp ./utility/*.cpp)
LIB_MYSENSORS_OBJS=$(patsubst %.cpp,%.o,$(LIB_MYSENSORS_SRCS))
DEPS=$(patsubst %.cpp,%.d,$(LIB_MYSENSORS_SRCS))

GATEWAY_BASIC=bin/PiGatewayBasic
GATEWAY_BASIC_SOURCES=examples_RPi/PiGatewayBasic.cpp
GATEWAY_BASIC_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_BASIC_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_BASIC_SOURCES))

GATEWAY_SERIAL=bin/PiGatewaySerial
GATEWAY_SERIAL_SOURCES=examples_RPi/PiGatewaySerial.cpp
GATEWAY_SERIAL_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_SERIAL_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_SERIAL_SOURCES))

GATEWAY_ETHERNET=bin/PiGatewayEthernet
GATEWAY_ETHERNET_SOURCES=examples_RPi/PiGatewayEthernet.cpp
GATEWAY_ETHERNET_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_ETHERNET_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_ETHERNET_SOURCES))

RF24H = /usr/local/include/RF24
CINCLUDE=-I. -I${RF24H}

.PHONY: all build bin clean install uninstall

all: ${GATEWAY_BASIC} ${GATEWAY_SERIAL} ${GATEWAY_ETHERNET}

# The static Lib Build
$(MYSENSORS_LIB): CCFLAGS += -fPIC
$(MYSENSORS_LIB): build $(LIB_MYSENSORS_OBJS)
	ar rcs $@ $(LIB_MYSENSORS_OBJS)
	ranlib $@

# The dynamic Lib Build
$(SO_MYSENSORS_LIB): $(MYSENSORS_LIB) $(LIB_MYSENSORS_OBJS)
	$(CC) -shared -o $@ $(LIB_MYSENSORS_OBJS)

# Basic Gateway Build
$(GATEWAY_BASIC): bin $(GATEWAY_BASIC_OBJECTS) $(MYSENSORS_LIB)
	$(CC) -o $@ $(GATEWAY_BASIC_OBJECTS) $(MYSENSORS_LIB) $(CCFLAGS) -lrf24-bcm

# Serial Gateway Build
$(GATEWAY_SERIAL): CCFLAGS += -D_TTY_NAME=\"${TTY_NAME}\" -D_TTY_GROUPNAME=\"${TTY_GROUPNAME}\"
$(GATEWAY_SERIAL): bin $(GATEWAY_SERIAL_OBJECTS) $(MYSENSORS_LIB)
	$(CC) -o $@ $(GATEWAY_SERIAL_OBJECTS) $(MYSENSORS_LIB) $(CCFLAGS) -lrf24-bcm -lutil -pthread

# Ethernet Gateway Build
$(GATEWAY_ETHERNET): CCFLAGS += -std=c++0x
$(GATEWAY_ETHERNET): bin $(GATEWAY_ETHERNET_OBJECTS) $(MYSENSORS_LIB)
	$(CC) -o $@ $(GATEWAY_ETHERNET_OBJECTS) $(MYSENSORS_LIB) $(CCFLAGS) -lrf24-bcm -pthread

build:
	@mkdir -p build

bin:
	@mkdir -p bin

# Include all .d files
-include $(DEPS)

%.o: %.cpp
	$(CC) $(CCFLAGS) ${CINCLUDE} -MMD -c -o $@ $<

# The Cleaner
clean:
	rm -rf build $(LIB_MYSENSORS_OBJS) $(GATEWAY_BASIC_OBJECTS) $(GATEWAY_SERIAL_OBJECTS) $(GATEWAY_ETHERNET_OBJECTS) $(DEPS)

install: all install-gatewaybasic install-gatewayserial install-gatewayethernet install-initscripts

install-gatewaybasic: 
	@echo "Installing ${GATEWAY_BASIC} to ${BINDIR}"
	@install -m 0755 ${GATEWAY_BASIC} ${BINDIR}

install-gatewayserial: 
	@echo "Installing ${GATEWAY_SERIAL} to ${BINDIR}"
	@install -m 0755 ${GATEWAY_SERIAL} ${BINDIR}

install-gatewayethernet: 
	@echo "Installing ${GATEWAY_ETHERNET} to ${BINDIR}"
	@install -m 0755 ${GATEWAY_ETHERNET} ${BINDIR}
	
install-initscripts:
	@echo "Installing initscripts to /etc/init.d"
	@install -m 0755 examples_RPi/initscripts/PiGatewayBasic /etc/init.d
	@install -m 0755 examples_RPi/initscripts/PiGatewaySerial /etc/init.d
	@install -m 0755 examples_RPi/initscripts/PiGatewayEthernet /etc/init.d
	@echo "Installing syslog config to /etc/rsyslog.d"
	@install -m 0755 examples_RPi/initscripts/30-PiGatewayBasic.conf  /etc/rsyslog.d
	@install -m 0755 examples_RPi/initscripts/30-PiGatewaySerial.conf /etc/rsyslog.d
	@install -m 0755 examples_RPi/initscripts/30-PiGatewayEthernet.conf /etc/rsyslog.d
	@service rsyslog restart

enable-gwbasic: install
	@update-rc.d PiGatewayBasic defaults
	
enable-gwserial: install
	@update-rc.d PiGatewaySerial defaults

enable-gwethernet: install
	@update-rc.d PiGatewayEthernet defaults

remove-gwbasic:
	@update-rc.d -f PiGatewayBasic remove

remove-gwserial:
	@update-rc.d -f PiGatewaySerial remove

remove-gwethernet:
	@update-rc.d -f PiGatewayEthernet remove

uninstall: remove-gwbasic remove-gwserial remove-gwethernet
	@echo "Stopping daemon PiGatewayBasic (ignore errors)"
	-@service PiGatewayBasic stop
	@echo "Stopping daemon PiGatewaySerial (ignore errors)"
	-@service PiGatewaySerial stop
	@echo "Stopping daemon PiGatewayEthernet (ignore errors)"
	-@service PiGatewayEthernet stop
	@echo "removing files"
	rm ${BINDIR}/PiGatewayBasic ${BINDIR}/PiGatewaySerial ${BINDIR}/PiGatewayEthernet /etc/init.d/PiGatewayBasic /etc/init.d/PiGatewaySerial /etc/init.d/PiGatewayEthernet /etc/rsyslog.d/30-PiGatewayBasic.conf /etc/rsyslog.d/30-PiGatewaySerial.conf /etc/rsyslog.d/30-PiGatewayEthernet.conf
