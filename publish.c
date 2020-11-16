/*	C program to publish a message to an MQTT server. This is a crude example, and there are a lot of things that I would do differently upon actual deployment. 
	
	For example, I would allocate the CLIENTID and ADDRESS dynamically.
	I'd also build a better way to handle message persistance, maybe something like RabitMQ with the MQTT plugin, or a custom redis system. 

	Alongside this, I would build some authentication, and send the connection over TLS. As it stands this is an unencrypted connection, with little to no concern for security.

	To compile the publishing client, link paho-mqtt3c.so for usage. The path to PahoMQTT might be different on your system.
	-	gcc publish.c -lm /usr/local/lib/libpaho-mqtt3c.so -o publish
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS		"tcp://localhost:1883"
#define CLIENTID	"patient_1"
#define TOPIC		"patient_1"
#define QOS 		2
#define TIMEOUT		10000L

int main(int argc, char* argv[]) {

	/*	Accomodates for user input mistakes. Prints out format if no arguments are provided... 
		If ran without any paramers, a help guide is spat out. If ran with too many parameters, the other arguments are simply ignored.
	*/
	switch(argc) {
		case 0: 
			printf("System error. This should never happen... \n");
			return 1;
		case 1:
			printf("This command takes exactly one argument. The format is: \n");

			/* Prints a working command input example */
			printf("%s \'\n", argv[0]);
			printf("{\n\t\"PID\": \"0001\",\
				\n\t\"time\": \"04:45\",\
				\n\t\"s1\":\"audio-stream-for-sensor\",\
				\n\t\"s2\":\"audio-stream-for-sensor\",\
				\n\t\"s3\": \"audio-stream-for-sensor\",\
				\n\t\"s4\": \"audio-stream-for-sensor\",\
				\n\t\"s5\": \"audio-stream-for-sensor\",\
				\n\t\"s6\": \"audio-stream-for-sensor\",\
				\n\t\"s7\": \"audio-stream-for-sensor\",\
				\n\t\"s8\": \"audio-stream-for-sensor\",\
				\n\t\"s9\": \"audio-stream-for-sensor\"\n}'\n");
			return 0;
		}

	/* Defines the PahoMQTT API connection options */
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	/* Creates MQTT client ready for connection to the specified server. */
	/* On deployment, I would most likely use MQTT_Persistance, to ensure that no data is lost.*/

	/* For API reference, and detailed explanation of function options: 
		https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/_m_q_t_t_client_8h.html#a9a0518d9ca924d12c1329dbe3de5f2b6
	*/
	MQTTClient_create(&client, ADDRESS, CLIENTID,
		MQTTCLIENT_PERSISTENCE_NONE, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	/* If connection fails, spit it out as the return code. */
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code: %d\n", rc);
		exit(-1);
	}
	/* the payload is the data which is sent across to the subscriber. */
	char * PAYLOAD = argv[1];

	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	printf("Publishing message: \n %s \nto topic:\n %s \nfor clientID: \n%s\n",
	  PAYLOAD, TOPIC, CLIENTID);

	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	printf("Message with delivery token %d delivered succesfully. \n", token);
	MQTTClient_disconnect(client, 100000);
	MQTTClient_destroy(&client);

	return rc;
}
