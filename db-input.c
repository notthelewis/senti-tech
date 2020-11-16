/*
	C program to input data collected from a senti-tech smart garment, sent from embedded paho MQTT client.... Or pretty much any other MQTT publishing client.

	** In this proof of concept code, there are quite a lot of hard-coded values and general malpractices (db pw stored in file etc)
	 	In a production environment, these values will be dynamically allocated and the malpractices addressed accordingly.
	 	Alongside this, this file assumes that you already have MySQL up and running. This code also does not check for the existence of the receiving table.
	 	This is all managed in the bash init script. 

	Each patient using the garment will have their own table in a database, which is setout as so:
	patient_id (######) INT, time_sent (##:##), path (/var/usrdat/patient/), token ([0-9A-Za-z_-] * 10).

	So, the four values for use in the DB:

	a) Patient ID 
	The patient ID is a unique identifier, to link patients to their records. It will be a randomly generated, 6 character long
	string, comprising of 64 usable characters. The users (clinicians, doctors etc) will never have to see this field.
	It will simply be used as a foreign key. 

	b) Time sent
	Time sent is the time that the data was sent from the garment.

	c) Path to this patient's data.
	Path is the path to the patient's audio recordings, and is intended for use by the client-side application to locate audio. 
	The benefit of storing the path (as opposed to audio blobs for example) is that it decreases database bloat, takes up very little bandwidth, 
	and allows for scalablility, as if more storage media is required at a later date, you can just link to the new storage device without having
	to worry about long migrations. 
	
	d) Unique token to identify current minute's audio recordings
	Token is a randomized 11 character string, comprised of uppercase usable URI tokens. Each minute's audio data for all
	9 sensors will have the same token. I have done it in this way as this is secure, scalable and reliable.
	This method doesn't use an incremental counter, making it hard to enumerate and is essentially limitless 
	(64 ^ 11 unique numbers). It's also the system that YouTube uses for unique video IDs.

	The production system will also need to check whether the token has been generated previously.

	Tom Scott explains what I'm trying to very nicely:
	 https://www.youtube.com/watch?v=gocwRvLhDf8

	
	The program binds all of these values to a MySQL prepared statement object, and fills it in with the sensor data,
	then uploads it to the patient's table in MySQL.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "mysql/mysql.h"

/*	Preparing the statement... This wouldn't be hardcoded in production, as it would be dynamically decided based on MQTT data packets. 
	PNSqrq is a randomly generated 6 character long patient ID which I have allocated and hard coded for this example.
*/
#define INSERT_NEW "INSERT INTO analytics.PNSqrq (pid, time, path, token) VALUES (?,?,?,?)"

int main(int argc, char const *argv[])
{
	MYSQL *mysql = mysql_init(NULL);	// MySQL connection object
	MYSQL_STMT *stmt;					// MySQL statmenet object
	MYSQL_BIND bind[4];				// Bindable parameters for the statement object
	
	/*	Each of the following declarations correspond to a row in the individual patient's table.
		Any strings require an accompanying length of input
	*/

	/* Patient id */
	unsigned long pid_len = 6;
	char pid[pid_len];

	/* Time saved in 24 hour format, with : for delimiter (15:23) */
	unsigned long time_len = 5; // Length of time
	char time[time_len];		// Time parameter

	/* Path to audio location */
	unsigned long path_len = 20;
	char path[path_len];

	unsigned long token_len = 11;
	char token[token_len];

	/* Connect to MySQL, if not, fail with error. */
	if (mysql_real_connect(mysql, "localhost", "lewis", "TotallyUnsafeMethodThisWillChange",
	 NULL, 0, NULL, 0) == NULL) {
		fprintf(stderr, "%s\n",mysql_error(mysql));
		mysql_close(mysql);
		exit(1);
	}

	/* Initializes the mysql statement object, stored in stmt. */
	stmt = mysql_stmt_init(mysql);
	if (!stmt) {
		fprintf(stderr, "mysql_stmt_init(), out of memory \n");
		exit(1);
	}

	/* Prepare an INSERT query with 11 paramers, in accordance to INSERT_NEW */
	if (mysql_stmt_prepare(stmt, INSERT_NEW, strlen(INSERT_NEW))) {
		fprintf(stderr, "mysql_stmt_prepare() INSERT failed \n");
		fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
		exit(1);
	}

	// Begin binding data types and parameters to the statement
	memset(bind, 0, sizeof(bind));

	/* STRING PARAM (patient_id) */
	bind[0].buffer_type= MYSQL_TYPE_STRING;
	bind[0].buffer= (char *)pid;
	bind[0].buffer_length= 6;
	bind[0].is_null= 0;
	bind[0].length= &pid_len;

    /* STRING PARAM (time) */
    bind[1].buffer_type= MYSQL_TYPE_STRING;
    bind[1].buffer= (char *)time;
    bind[1].buffer_length= 5;
    bind[1].is_null= 0;
    bind[1].length= &time_len;

    /* STRING PARAM (path) */
    bind[2].buffer_type= MYSQL_TYPE_STRING;
    bind[2].buffer= (char *)path;
    bind[2].buffer_length= 6;
    bind[2].is_null= 0;
    bind[2].length= &path_len;

    /* STRING PARAM (token) */
    bind[3].buffer_type= MYSQL_TYPE_STRING;
    bind[3].buffer= (char *)token;
    bind[3].buffer_length= 6;
    bind[3].is_null= 0;
    bind[3].length= &token_len;

    /* Actually bind the bind array to mysql prepared statement object.. Return error code and exit if failed */
    if (mysql_stmt_bind_param(stmt, bind)){
        fprintf(stderr, " mysql_stmt_bind_param() failed\n");
        fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
        exit(1);
    }

    /* Fill in the data values for the bindings */
    strncpy(pid, argv[1], pid_len);
    strncpy(time, argv[2], time_len);
    strncpy(path, argv[3], path_len);
    strncpy(token, argv[4], token_len);


    /* Execute the prepared statement, with the apropriate data values... Return error code and exit if failed */
    if (mysql_stmt_execute(stmt)) {
    	fprintf(stderr, "mysql_stmt_execute() failed \n");
    	fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
    	exit(1);
    }

    /*Close the connection ... Return error code and exit if failed*/
    if (mysql_stmt_close(stmt)) {
    	fprintf(stderr, "Failed while closing the statement \n");
    	fprintf(stderr, "%s\n", mysql_error(mysql));
    	exit(1);
    }

 	/* Close connection */
    mysql_close(mysql);

	return 0;
}
