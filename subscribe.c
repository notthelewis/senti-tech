/*
    This software listens for updates to a *hardcoded* MQTT broker's topic, converts the input to JSON objects and
    spits the results back out into a MySQL database by forking a process using popen(), and calling a pre-compiled DB entry script. 

    A lot of the global constants such as the address, clientID, TOPIC and TIMEOUT can be interchanged with dynamic values for ease of use.
    Alongside this, I would build some authentication, and send the connection over TLS. As it stands this is an unencrypted connection, with little to no concern for security.
    
    I'd also build a better way to handle message persistance, maybe something like RabitMQ with the MQTT plugin, or a custom redis system. 

    In order for this to work, json-c library and the PahoMQTTAsync library need to be linked during compile time using the -lm options in GCC.
    I have not tried compiling using Clang, XCode or any other alternative C compilers.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "MQTTAsync.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "patient_1_sub"
#define TOPIC       "patient_1"
#define QOS         2
#define TIMEOUT     10000L
volatile MQTTAsync_token deliveredtoken;

int disc_finished = 0;
int subscribed = 0;
int finished = 0;


/* Use popen function to call executables */
void get_popen(char * to_run, size_t command_len)
{
    FILE *fp;
    char line[command_len];

    fp = popen(to_run, "r");

    /* Prints out program output (if any) */
    while (fgets( line, sizeof line, fp)) {
        printf("%s", line);
    }
    pclose(fp);
}

/*  Takes a pointer to a json structure (from json-c) as an input, 
    then calls the ./db-insert script to input the results to MySQL.
*/
int insertDB(struct json_object *parsed_json)
{
	printf("Function called \n");

    struct json_object *pid;
    struct json_object *time_sent;
    struct json_object *path;
    struct json_object *token;

    // Buf is the buffer to hold the command, sz is the size of the buffer (for use in the snprintf command and the popen callback)
    char *buf;
    size_t sz;

    // Parse the JSON tokens into individual objects. 
    json_object_object_get_ex(parsed_json, "pid", &pid);
    json_object_object_get_ex(parsed_json, "time", &time_sent);
    json_object_object_get_ex(parsed_json, "path", &path);
    json_object_object_get_ex(parsed_json, "token", &token);

    /*
        Each command will be at most 60 characters long. This is a carefully worked out length, and is explained as follows:
        Every data transmission will have a PID (patient ID), time sent, path to audio and a unique audio recording token.

        For reference:-
            pid     =   6 characters
            time    =   5 characters
            path    =   20 characters
            token   =   11 characters
            _________________________
            TOTAL   =   42 Characters
            _________________________

        Quote marks ('') around each parameter: 8 chars (4 parameters, 1 before and after each)
        space inbetween each: 4 characters
        progam name (./db-insert for now) = 7 characters

        program name, plus all parameters, plus speech marks, plus spaces: 57 characters.
    */

    // Sanely work out the length of the proposed insert command using snprintf's overflow return value.
    // JSON-C API reference for get_string: https://json-c.github.io/json-c/json-c-0.10/doc/html/json__object_8h.html#ad24f1c4c22b2a7d33e7b562c01f2ca65
    sz = snprintf(NULL, 0, "./db-insert '%s' '%s' '%s' '%s'",
		json_object_get_string(pid),
		json_object_get_string(time_sent),
		json_object_get_string(path),
		json_object_get_string(token));

    // Store the command in the buffer
    buf = (char *)malloc(sz+1);
    snprintf(buf, sz+1, "./db-insert '%s' '%s' '%s' '%s'",
		json_object_get_string(pid),
		json_object_get_string(time_sent),
		json_object_get_string(path),
		json_object_get_string(token));

    printf("%s\n", buf);

    /* Runs the ./db-insert module */
    get_popen(buf, sz);

	return 0;
}
/* On connection lost, alert user and attempt to reconnect with a clean session */
void connlost(void *context, char *cause)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        int rc;
        printf("\nConnection lost\n");
        printf("cause: %s\n", cause);
        printf("Reconnecting\n");
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to start connect, return code %d\n", rc);
            finished = 1;
        }
}
/*  On reception of a message, spit the message out to the user and call insertDB() to store the message in the database. */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;

    struct json_object *parsed_json;

    printf("Message arrived\n");
    printf("topic: %s\n", topicName);
    printf("message: \n");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

    // Store the payload as JSON object
    parsed_json = json_tokener_parse(message->payload);

    // Free the message
    MQTTAsync_freeMessage(&message);
    // Free the topic
    MQTTAsync_free(topicName);

    /* Call insertDB function to parse json and insert into MySQL database */
    insertDB(parsed_json);

    return 1;
}
/* Alert user on disconnection */
void onDisconnect(void* context, MQTTAsync_successData* response)
{
        printf("Successful disconnection\n");
        disc_finished = 1;
}
/* Alert user on successful subscription to given topic */
void onSubscribe(void* context, MQTTAsync_successData* response)
{
        printf("Subscribe succeeded\n");
        subscribed = 1;
}
/* Alert user on failed subscription to given topic */
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Subscribe failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
/* Alert user on failed connection*/
void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Connect failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
/* On Successful connection, alert user and set some variables */
void onConnect(void* context, MQTTAsync_successData* response)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;
        printf("Successful connection\n");
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.context = client;
        deliveredtoken = 0;
        if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char* argv[])
{
        /* PahoMQTTAsync client API options*/
        MQTTAsync client;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        MQTTAsync_token token;	

        /*  RC is the return code used for collecting sane return values,
            CH is used to store a single character (for display to user)
        */
        int rc;
        int ch;

        /*  Create the MQTT connection using the provided options */
        MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

        /* The following options ensure that each session is a completely clean session, with no persistance at all */
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }

        /*  Whilst subscribed to the topic, print out the messages and listen for finished event, which can be sent
            from a failure to disconnect, failure to subscribe, connection lost or form the user sending a q or Q to the prompt.
        */
        while   (!subscribed)
            usleep(10000L);
        if (finished)
            goto exit;
        do {
            ch = getchar();
        } while (ch!='Q' && ch != 'q');

        disc_opts.onSuccess = onDisconnect;
 
        /* If the client cannot disconnect, print the return code and exit with exit_failure */
        if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start disconnect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        while   (!disc_finished)
            usleep(10000L);

/* If the exit event is received, destroy client connection and return the return code */
exit:
        MQTTAsync_destroy(&client);
        return rc;
}
