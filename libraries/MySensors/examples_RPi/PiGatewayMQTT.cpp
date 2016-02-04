/*
 * PiGatewayEthernet.cpp - MySensors Gateway for wireless node providing a network interface
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.com>
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
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <list>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <MyTransportNRF24.h>
#include <MySigningNone.h>
#include <MySigningAtsha204Soft.h>
#include <MyParserSerial.h>
#include <MySensor.h>
#include <RF24.h>

#include "mongoose.h"

//#define USE_INTERRUPT
// Oitzu (https://github.com/Oitzu) made possible the use of interrupts
// that reduce CPU utilization and increases throughput
// For this to work, you need two things:
//   - Install http://wiringpi.com/
//   - Connect the nRF24L01 IRQ pin to a free raspberry gpio pin
//     The Pin numbering of the nrf24 library is bcm style
//     You can see the pin numbering on your RPi with "gpio readall"
//     The BCM column is the right column to look at.
#define INTERRUPT_PIN 23

#define CONTROLLERS_PORT "5003"  // the port controllers will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once

struct radio_message {
	MyMessage msg;
	uint8_t to;
	uint8_t len;
};

struct mg_mqtt_topic_expression topic_expressions[] = {
  {"/mySensors", 0}
};

void msg_callback(const MyMessage &message);
void *get_in_addr(struct sockaddr *sa);
void *waiting_controllers(void *);
void *connected_controller(void* thread_arg);
void parse_and_send(long int sockfd, char *buffer);

static MySensor *gw;
// NRFRF24L01 radio driver
#ifdef __PI_BPLUS
	static MyTransportNRF24 transport(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#else
	static MyTransportNRF24 transport(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#endif
static MyParserSerial parser;

static std::vector<int> controllers_sockets;
static std::list<struct radio_message> radio_messages_q;
static pthread_mutex_t controllers_sockets_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t radio_messages_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t radio_messages_cond = PTHREAD_COND_INITIALIZER;

volatile static int running = 1;

/*
 * handler for SIGINT signal
 */
void handle_sigint(int sig)
{
	running = 0;
	std::cout << "Received SIGINT\n" << std::endl;
	pthread_cond_signal(&radio_messages_cond);
}

void intHandler()
{
	struct radio_message r_msg;
	
	//Read as long data is available
	//Single interrupts may be lost if a lot of data comes in.
	pthread_mutex_lock(&radio_messages_mutex);
	while (transport.available(&r_msg.to)) {
		memset(&r_msg.msg, 0, sizeof(MyMessage));
		r_msg.len = transport.receive((uint8_t *)&r_msg.msg);
		radio_messages_q.push_back(r_msg);
	}
	pthread_mutex_unlock(&radio_messages_mutex);
	pthread_cond_signal(&radio_messages_cond);
}

int main()
{
	pthread_t thread_id;
	pthread_attr_t attr;

	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	std::cout << "Starting Gateway..." << std::endl;

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
		std::cout << "Failed to initialize MySensors library" << std::endl;
		exit(1);
	}

	/* initialize the Gateway */
	gw->begin(&msg_callback, 0, true, 0);

	/* network thread */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, &waiting_controllers, NULL);
	pthread_attr_destroy(&attr);

#ifndef USE_INTERRUPT
	while(running) {
		/* process radio msgs */
		gw->process();
		
		usleep(10000);	// 10ms
	}
#else
	// Mask all interrupts except the receive interrupt
	transport.maskIRQ(1,1,0);

	attachInterrupt(INTERRUPT_PIN, INT_EDGE_FALLING, intHandler);

	while(running) {
		/* process radio msgs */
		pthread_mutex_lock(&radio_messages_mutex);
		while (radio_messages_q.empty()) {
			pthread_cond_wait(&radio_messages_cond, &radio_messages_mutex);
			if (!running) {
				pthread_mutex_unlock(&radio_messages_mutex);
				goto end;
			}
		}
		struct radio_message r_msg = radio_messages_q.front();
		radio_messages_q.pop_front();
		pthread_mutex_unlock(&radio_messages_mutex);
		delay(1);

		MyMessage &message = gw->getLastMessage();
		message = r_msg.msg;
		gw->process(r_msg.to);
	}
end:
	detachInterrupt(INTERRUPT_PIN);
#endif

	return 0;
}

void msg_callback(const MyMessage &message)
{
	char buf[MAXDATASIZE];
	char convBuf[MAX_PAYLOAD*2+1];
	int nbytes;

	printf("[CALLBACK]%d;%d;%d;%d;%d;%s\n", message.sender,
			message.sensor, mGetCommand(message), mGetAck(message),
			message.type, message.getString(convBuf));

	nbytes = snprintf(buf, MAXDATASIZE, "%d;%d;%d;%d;%d;%s\n",
			message.sender, message.sensor, mGetCommand(message),
			mGetAck(message), message.type, message.getString(convBuf));

	pthread_mutex_lock(&controllers_sockets_mutex);
	for (unsigned int i = 0; i < controllers_sockets.size(); i++) {
		if (send(controllers_sockets[i], buf, nbytes, 0) == -1)
			perror("send");
	}
	pthread_mutex_unlock(&controllers_sockets_mutex);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *waiting_controllers(void *)
{
	// Code from:
	//  http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver

	int sockfd;  // listen on sock_fd
	long int new_fd; // new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	pthread_t thread_id;
	pthread_attr_t attr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, CONTROLLERS_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections from controllers...\n");

	while (1) {  // accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, &connected_controller, (void *)new_fd);
		pthread_attr_destroy(&attr);

		pthread_mutex_lock(&controllers_sockets_mutex);
		controllers_sockets.push_back(new_fd);
		pthread_mutex_unlock(&controllers_sockets_mutex);
	}

	return NULL;
}

void *connected_controller(void* thread_arg)
{
	long int sockfd = (long int) thread_arg;
	char buf[MAXDATASIZE];
	int nbytes;

	while (1) {
		if ((nbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
			for (int i = 0; i < nbytes; i++) {
				if (buf[i] == '\n') {
					buf[i] = '\0';
					parse_and_send(sockfd, buf);
					break;
				}
			}
		} else {
			break;
		}
	}
	
	if (nbytes == -1)
		perror("recv");

	pthread_mutex_lock(&controllers_sockets_mutex);
	auto it = std::find(controllers_sockets.begin(), controllers_sockets.end(), sockfd);
	if (it != controllers_sockets.end()) {
		// use swap to prevent moving all elements
		std::swap(*it, controllers_sockets.back());
		controllers_sockets.pop_back();
	}
	pthread_mutex_unlock(&controllers_sockets_mutex);

	close(sockfd);
	return NULL;
}

void parse_and_send(long int sockfd, char *buffer)
{
	char buf[MAXDATASIZE];
	MyMessage msg;
	int nbytes;

	if (parser.parse(msg, buffer)) {
		uint8_t command = mGetCommand(msg);

		if (msg.destination==GATEWAY_ADDRESS && command==C_INTERNAL) {
			// Handle messages directed to gateway
			if (msg.type == I_VERSION) {
				// Request for version
				nbytes = snprintf(buf, MAXDATASIZE, "0;0;%d;0;%d;%s\n", C_INTERNAL, I_VERSION, LIBRARY_VERSION);
				if (send(sockfd, buf, nbytes, 0) == -1)
					perror("send");
			}
		} else {
			gw->sendRoute(msg);
		}
	}
}
