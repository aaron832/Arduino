/*
 * PiGatewaySerial.cpp - MySensors Gateway for wireless node providing a serial interface
 *
 * Copyright 2014 Tomas Hozza <thozza@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pty.h>
#include <termios.h>
#include <poll.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <syslog.h>

#include "MyTransportNRF24.h"
#include "MySigningNone.h"
#include <MySigningAtsha204Soft.h>
#include "MyParserSerial.h" 
#include "MySensor.h"
#include <RF24.h>

#define MAX_SEND_LENGTH 120 // Max buffersize needed for messages destined for controller

#ifndef _TTY_NAME
	#define _TTY_NAME "/dev/ttyMySensorsGateway"
#endif

#ifndef _TTY_GROUPNAME
	#define _TTY_GROUPNAME "tty"
#endif

/* variable indicating if the server is still running */
volatile static int running = 1;

/* PTY file descriptors */
int pty_master = -1;
int pty_slave = -1;

static const mode_t ttyPermissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
static const char *serial_tty = _TTY_NAME;
static const char *devGroupName = _TTY_GROUPNAME;

MyParserSerial parser;

char convBuf[MAX_PAYLOAD*2+1];
char serialBuffer[MAX_SEND_LENGTH]; // Buffer for building string when sending data to the controller

int daemonizeFlag = 0;

void openSyslog()
{
	setlogmask(LOG_UPTO (LOG_INFO));
	openlog(NULL, 0, LOG_USER);
}

void closeSyslog()
{
	closelog();
}

void log(int priority, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	if (daemonizeFlag == 1) {
		vsyslog(priority, format, argptr);
	} else {
		vprintf(format, argptr);
	}
	va_end(argptr);
}

/*
 * handler for SIGINT signal
 */
void handle_sigint(int sig)
{
	log(LOG_INFO,"Received SIGINT\n");
	running = 0;
}

void handle_sigusr1(int sig)
{
	log(LOG_INFO,"Received SIGUSR1\n");
	int curLogLevel = setlogmask(0);
	if (curLogLevel != LOG_UPTO(LOG_DEBUG)) setlogmask(LOG_UPTO (LOG_DEBUG));
	else setlogmask(LOG_UPTO (LOG_INFO));
}

void write_msg_to_pty(char *msg)
{
	size_t len = 0;

	if (msg == NULL)
	{
		log(LOG_WARNING,"[callback] NULL msg received!\n");
		return;
	}
	
	len = strlen(msg);
	write(pty_master, msg, len);
}

void serial(const char *fmt, ... ) {
	va_list args;
	va_start (args, fmt );
	vsnprintf(serialBuffer, MAX_SEND_LENGTH, fmt, args);
	va_end (args);
	write_msg_to_pty(serialBuffer);
}

void parseAndSend(MySensor *gw, char *commandBuffer) {
	MyMessage &msg = gw->getLastMessage();

	if (parser.parse(msg, commandBuffer)) {
		uint8_t command = mGetCommand(msg);

		if (msg.destination==GATEWAY_ADDRESS && command==C_INTERNAL) {
			// Handle messages directed to gateway
			if (msg.type == I_VERSION) {
				// Request for version
				serial(PSTR("0;0;%d;0;%d;%s\n"), C_INTERNAL, I_VERSION, LIBRARY_VERSION);
			}
		} else {
			gw->sendRoute(msg);
		}
	}
}

/*
 * callback function
 */
void msgCallback(const MyMessage &message){
	serial(PSTR("%d;%d;%d;%d;%d;%s\n"),message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type, message.getString(convBuf));
}

/*
 * configure PTY master FD
 */
void configure_master_fd(int fd)
{
	struct termios settings;

	tcgetattr(fd, &settings);
	/* turn off ECHO of written characters */
	settings.c_lflag &= ~ECHO;
	tcsetattr(fd, 0, &settings);
}

static void daemonize(void)  
{
	pid_t pid, sid;
	int fd;

	// already a daemon
	if ( getppid() == 1 ) return;

	// Fork off the parent process
	pid = fork();
	if (pid < 0)  exit(EXIT_FAILURE);  // fork() failed
	if (pid > 0)  exit(EXIT_SUCCESS);  // fork() successful, this is the parent process, kill it

	// From here on it is child only

	// Create a new SID for the child process
	sid = setsid();
	if (sid < 0) exit(EXIT_FAILURE);  // Not logging as nobody can see it.

	// Change the current working directory.
	if ((chdir("/")) < 0) exit(EXIT_FAILURE);

	// Divert the standard file desciptors to /dev/null
	fd = open("/dev/null",O_RDWR, 0);
	if (fd != -1)  
	{
		dup2 (fd, STDIN_FILENO);
		dup2 (fd, STDOUT_FILENO);
		dup2 (fd, STDERR_FILENO);

		if (fd > 2) close (fd);
	}  

	// reset File Creation Mask 
	umask(027);  
}

/*
 * Main gateway logic
 */
int main(int argc, char **argv)
{
	struct pollfd fds;
	struct group* devGrp;

	MySensor *gw = NULL;

	int status = EXIT_SUCCESS;
	int ret, c;
	
	while ((c = getopt (argc, argv, "d")) != -1) 
	{
		switch (c)
		{
			case 'd':
				daemonizeFlag = 1;
				break;
		}
	}
	openSyslog();
	log(LOG_INFO,"Starting PiGatewaySerial...\n");
	log(LOG_INFO,"Protocol version - %s\n", LIBRARY_VERSION);

	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGUSR1, handle_sigusr1);

	// NRFRF24L01 radio driver
#ifdef __PI_BPLUS
	MyTransportNRF24 transport(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#else
	MyTransportNRF24 transport(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#endif

	// Hardware profile
	MyHwRaspberryPi hw;

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
		log(LOG_ERR,"Could not create MyGateway! (%d) %s\n", errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}

	/* create PTY - Pseudo TTY device */
	ret = openpty(&pty_master, &pty_slave, NULL, NULL, NULL);
	if (ret != 0) 
	{
		log(LOG_ERR,"Could not create a PTY! (%d) %s\n", errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}
	errno = 0;
	devGrp = getgrnam(devGroupName);
	if(devGrp == NULL) 
	{
		log(LOG_ERR,"getgrnam: %s failed. (%d) %s\n", devGroupName, errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}
	ret = chown(ttyname(pty_slave),-1,devGrp->gr_gid);
	if (ret == -1) 
	{
		log(LOG_ERR,"chown failed. (%d) %s\n", errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}
	ret = chmod(ttyname(pty_slave),ttyPermissions);
	if (ret != 0) 
	{
		log(LOG_ERR,"Could not change PTY permissions! (%d) %s\n", errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}
	log(LOG_INFO,"Created PTY '%s'\n", ttyname(pty_slave));
	
	/* create a symlink with predictable name to the PTY device */
	unlink(serial_tty);	// remove the symlink if it already exists
	ret = symlink(ttyname(pty_slave), serial_tty);
	if (ret != 0)
	{
		log(LOG_ERR,"Could not create a symlink '%s' to PTY! (%d) %s\n", serial_tty, errno, strerror(errno));
		status = EXIT_FAILURE;
		goto cleanup;
	}
	log(LOG_INFO,"Gateway tty: %s\n", serial_tty);

	close(pty_slave);
	configure_master_fd(pty_master);

	fds.events = POLLRDNORM;
	fds.fd = pty_master;
	if (daemonizeFlag) daemonize();
	/* we are ready, initialize the Gateway */
	gw->begin(&msgCallback, 0, true, 0);

	/* Do the work until interrupted */
	while(running)
	{
		/* process radio msgs */
		gw->process();
		
		/* process serial port msgs */
		ret = poll(&fds, 1, 500);
		if (ret == -1)
		{
			log(LOG_ERR,"poll() error (%d) %s\n", errno, strerror(errno));
		}
		else if (ret == 0)
		{
			/* timeout */
			continue;
		}
		else
		{
			if (fds.revents & POLLRDNORM)
			{
				char buff[256];
				ssize_t size;

				fds.revents = 0;
				size = read(pty_master, buff, sizeof(buff));
				if (size < 0)
				{
					log(LOG_ERR,"read error (%d) %s\n", errno, strerror(errno));
					continue;
				}
				buff[size] = '\0';
				
				parseAndSend(gw, buff);
			}
		}
	}


cleanup:
	log(LOG_INFO,"Exiting...\n");
	if (gw)
		delete(gw);
	(void) unlink(serial_tty);
	closeSyslog();
	return status;
}
