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
//#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <arpa/inet.h>

#include <MyTransportNRF24.h>
#include <MySigningNone.h>
#include <MySigningAtsha204Soft.h>
#include <MyParserSerial.h>
#include <MySensor.h>
#include <RF24.h>

#include "EnumMap.h"
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
#define MAXTOPICSIZE 512 // max number of characters in a topic

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
void *mongoose_poll(void *);
void ev_handler(struct mg_connection *nc, int ev, void *p);

static MySensor *gw;
struct mg_mgr mgr;
	
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

//TODO not sure if this should increment or what to do with it.
static int message_id = 65;

//TODO static address for now
const char *address = "localhost:1883";

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

const char * subTypeToStr(int command, int type)
{
	switch (command)
	{
	case C_PRESENTATION:
		return mysensor_sensor_str[type];
	case C_SET:
	case C_REQ:
		return mysensor_data_str[type];
		break;
	case C_INTERNAL:
		return mysensor_internal_str[type];
		break;
	default:
		break;
	}
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
	pthread_create(&thread_id, &attr, &mongoose_poll, NULL);
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
	char topic[MAXTOPICSIZE];
	char convBuf[MAX_PAYLOAD*2+1];
	int nbytes;

	message.getString(convBuf);

	printf("[CALLBACK]%d;%d;%d;%d;%d;%s\n", message.sender,
			message.sensor, mGetCommand(message), mGetAck(message),
			message.type, convBuf);

	nbytes = strlen(convBuf);

	//TODO create subTypeToStr function.
	snprintf(topic, MAXTOPICSIZE, "/mySensors/%d/%d/%s", message.sender, message.sensor, subTypeToStr(mGetCommand(message),message.type));

	pthread_mutex_lock(&controllers_sockets_mutex);
	
	mg_mqtt_publish(mgr.active_connections, topic, message_id, MG_MQTT_QOS(0), convBuf, nbytes);
	
	pthread_mutex_unlock(&controllers_sockets_mutex);
}

void *mongoose_poll(void *)
{
	pthread_t thread_id;
	pthread_attr_t attr;

	//Initialize mongoose manager
        mg_mgr_init(&mgr, NULL);

        if(mg_connect(&mgr, address, ev_handler) == NULL) {
                printf("mg_connect(%s) failed\n", address);
                exit(1);
        }

	while (1) { 
		mg_mgr_poll(&mgr, 1000);
	}

	return NULL;
}

void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
  (void) nc;

#if 0
  if (ev != MG_EV_POLL)
    printf("USER HANDLER GOT %d\n", ev);
#endif

  switch (ev) {
    case MG_EV_CONNECT:
      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake(nc, "dummy");
      break;
    case MG_EV_MQTT_CONNACK:
      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
        printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
        exit(1);
      }
      printf("Subscribing to topic_expressions\n");
      mg_mqtt_subscribe(nc, topic_expressions, sizeof(topic_expressions)/sizeof(*topic_expressions), 42);
      break;
    case MG_EV_MQTT_PUBACK:
      printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
      break;
    case MG_EV_MQTT_SUBACK:
      printf("Subscription acknowledged, forwarding to '/test'\n");
      break;
    case MG_EV_MQTT_PUBLISH:
      {
        printf("Got incoming message %s: %.*s\n", msg->topic, (int)msg->payload.len, msg->payload.p);

        printf("Forwarding to /test\n");
        mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), msg->payload.p, msg->payload.len);
      }
      break;
    case MG_EV_CLOSE:
      printf("Connection closed\n");
      exit(1);
  }
}
