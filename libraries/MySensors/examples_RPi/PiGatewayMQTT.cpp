/*
 * PiGatewayMQTT.cpp - MySensors Gateway for wireless node providing a network MQTT interface
 *
 * Created by: Aaron Rose <https://github.com/aaron832>
 *			   Marcelo Aquino <marceloaqno@gmail.com>
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

#include <MyTransportNRF24.h>
#include <MySigningNone.h>
#include <MySigningAtsha204Soft.h>
#include <MySensor.h>
#include <RF24.h>
#include <mosquitto.h>
#include "EnumMap.h"

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
#define PING_FREQ_SEC 10 //Ping MQTT every 10 seconds.

struct radio_message {
	MyMessage msg;
	uint8_t to;
	uint8_t len;
};

void msg_callback(const MyMessage &message);
void *get_in_addr(struct sockaddr *sa);
void *mqtt_thread(void *);
void parsemqtt_and_send(char *topic, const char *payload);

static MySensor *gw;
struct mosquitto *mosq;
	
// NRFRF24L01 radio driver
#ifdef __PI_BPLUS
	static MyTransportNRF24 transport(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#else
	static MyTransportNRF24 transport(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, RF24_PA_LEVEL_GW, BCM2835_SPI_SPEED_8MHZ);
#endif

static std::vector<int> controllers_sockets;
static std::list<struct radio_message> radio_messages_q;
static pthread_mutex_t controllers_sockets_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t radio_messages_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t radio_messages_cond = PTHREAD_COND_INITIALIZER;

//TODO: not sure if this should increment or what to do with it.
static int message_id = 65;

//TODO: static address for now
const char *mqtthost = "localhost";
const int mqttport = 1883;
const int mqttkeepalive = 60;

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
	pthread_create(&thread_id, &attr, &mqtt_thread, NULL);
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

	//printf("[CALLBACK]%d;%d;%d;%d;%d;%s\n", message.sender,
	//		message.sensor, mGetCommand(message), mGetAck(message),
	//		message.type, convBuf);

	nbytes = strlen(convBuf);

	//Build the MQTT topic from the mysensor message.
	snprintf(topic, MAXTOPICSIZE, "/mySensors/%d/%d/%s", message.sender, message.sensor, subTypeToStr(mGetCommand(message),message.type));
	
	uint8_t command = mGetCommand(message);
	if(command == C_PRESENTATION)
	{
		//Throw the presentations at MQTT
		printf("Publish from C_PRESENTATION: %s:%s\n", topic, convBuf);
																//true to retain the message (get message immediately after subscribe).
		mosquitto_publish(mosq, NULL, topic, nbytes, convBuf, 0, false); // message_id, MG_MQTT_QOS(0), convBuf, nbytes);
	}
	else if(command == C_SET)
	{
		//We got a request
		printf("Publish from C_SET: %s:%s\n", topic, convBuf);
																//true to retain the message (get message immediately after subscribe).
		mosquitto_publish(mosq, NULL, topic, nbytes, convBuf, 0, false); //message_id, MG_MQTT_QOS(0), convBuf, nbytes);
	}
	else if (command == C_REQ)
	{
		//We got a request... we should just treat this as a subscribe which will automatically trigger a response
		//What if we subscribe twice?
		//Doing it this way forces the sensor to first request before receiving anything.
		//If we do not remember our subscriptions then nodes will either need to remind the gateway periodically or be rebooted.

		printf("Subscribe from C_REQ: %s\n", topic);
		mosquitto_subscribe(mosq, NULL, topic, 0);
	}
	else if (command == C_INTERNAL)	{	
	}
	else if (command == C_STREAM)	{
	}
}

void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		printf("Got a MQTT message %s %s\n", message->topic, message->payload);
		
		std::string strpayload((char *)message->payload);
		strpayload = strpayload.substr(0,(int)message->payloadlen);
	
		parsemqtt_and_send(message->topic, strpayload.c_str());
	}else{
		printf("%s (null)\n", message->topic);
	}
	fflush(stdout);
}

void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result){
		printf("Connected!\n");
		/* Subscribe to broker information topics on successful connect. */
		//mosquitto_subscribe(mosq, NULL, "$SYS/#", 2);
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("Log: %s\n", str);
}

void *mqtt_thread(void *)
{
	pthread_t thread_id;
	pthread_attr_t attr;

	mosquitto_lib_init();
	bool clean_session = true; //will clean out subscriptions  on disconnect
	mosq = mosquitto_new(NULL, clean_session, NULL);
	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		exit(1);
	}

	mosquitto_log_callback_set(mosq, mqtt_log_callback);
	mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
	mosquitto_message_callback_set(mosq, mqtt_message_callback);
	mosquitto_subscribe_callback_set(mosq, mqtt_subscribe_callback);

	if(mosquitto_connect(mosq, mqtthost, mqttport, mqttkeepalive)){
		fprintf(stderr, "Unable to connect.\n");
		exit(1);
	}

	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return NULL;
}

void parsemqtt_and_send(char *topic, const char *payload)
{
	//Buffer for parsemqtt_and_send.  Do not want to re-create it every time we get a message
	MyMessage msg;
	char *str, *p;

	char* topic_copy = strdup(topic);
	

	// TODO: Check if we should send ack or not.
	int i = 0;
	for (str = strtok_r(topic_copy,"/",&p) ; str && i<4 ; str = strtok_r(NULL,"/",&p))
	{
		// Example: /mySensors/105/4/V_LIGHT
		// 0: mySensors - Broker
		// 1: 105 - node
		// 2: 4 - sensor
		// 3: V_LIGHT - type
		if (i == 0) {
			//TODO: Add warning when we receive a message from an unexpected broker prefix.
			//if (strcmp_P(str,broker)!=0) {  //look for MQTT_BROKER_PREFIX
			//	return;                 //Message not for us or malformatted!
			//}
		} else if (i==1) {
			printf("Destination parse %s\n", str);
			msg.destination = atoi(str);    //NodeID
		} else if (i==2) {
			msg.sensor = atoi(str);         //SensorID
		} else if (i==3) {
			unsigned char match=255;         //SensorType
			
			//Support for numeric types
			if ( atoi(str)!=0 || (str[0]=='0' && str[1] =='\0') ) {
				match=atoi(str);
			}
			
			//Check through all the data type string attary to find a match.
			//? Not sure if there is any need to be checking for other types like sensor and internal
			//? as those don't seem like they should be outside settable thought MQTT.
			for (uint8_t j=0; match == 255 && j<sizeof(mysensor_data_str)/sizeof(mysensor_data_str[0]); j++) {
				if (strcmp(str,mysensor_data_str[j])==0) { //compare sensors
					match=j;
				}
			}
			
			msg.type = match;
		}
		i++;
	}
	
	msg.sender = GATEWAY_ADDRESS;
	msg.last = GATEWAY_ADDRESS;
	mSetCommand(msg, C_SET);
	mSetRequestAck(msg, 0);
	mSetAck(msg, false);
	msg.set(payload);
	
	if(msg.destination != 0 && msg.sensor != 0 && msg.type != 255)
	{
		if(!gw->sendRoute(msg))
			printf("Message not sent!\n");
	}	
	else
	{
		printf("Recieved a bad MQTT message: '%s':'%s'\n destination:%i, sensor:%i, type:%i\n", 
		topic, payload, msg.destination, msg.sensor, msg.type);
	}
		
	free(topic_copy);
}

