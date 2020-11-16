#!/usr/bin/sh 

# Designed for BASH on Ubuntu, will need adaption for other shells and distros... Other than vanilla Debian.
printf "Please ensure that this script is ran as root \n\n\
It will require installation of packages to system, and direct access to MySQL\n\n"

printf "If MySQL is already installed on the system & MySQL root user is password\n\
 protected you will have to change this script to ensure it can do what it needs\n\
\nIn order to do so, locate any mysql commands then:\n \
add a -U and -P option for user and password\n\n"

printf "Press enter to continue Installing dependencies\n (gcc, libjson-c-dev, mysql, mosquitto and git)\n or ctrl+c to escape\n"

read yesno

apt install gcc libjson-c-dev mysql-client mysql-server mosquitto git -y
systemctl start mosquitto
systemctl start mysql

echo "Default settings for MySQL and Mosquitto are used. This will change on full deployment."

# MySQL commands to create the test database and table, also granting a user access
mysql -e "CREATE DATABASE analytics"
mysql -e "CREATE TABLE analytics.PNSqrq (pid VARCHAR(6), time VARCHAR(5), path VARCHAR(20), token VARCHAR(11))"
mysql -e "CREATE USER 'lewis'@'localhost' IDENTIFIED BY 'TotallyUnsafeMethodThisWillChange'"
mysql -e "GRANT ALL PRIVILEGES ON analytics.* to 'lewis'@'localhost'"
mysql -e "FLUSH PRIVILEGES"

# Download, install and configure the PahoMQTT client
git clone "https://github.com/eclipse/paho.mqtt.c"
echo "Please enter your user and group, so that the library is accessible to non-root user"

echo "user"
read user

echo "group"
read group

chown -R $user:$group paho.mqtt.c

echo "Building paho.mqtt.c"
cd paho.mqtt.c
make .
make install

# Compile the publish client, link paho-mqtt3c.so for usage. The path to PahoMQTT might be different on your system.
gcc publish.c -lm /usr/local/lib/libpaho-mqtt3c.so -o publish
	# Compile the db input script, linking the mysql-client library
gcc db-input.c -lmysqlclient -o db-input
# Compile the subscribe client, linking json-c and paho-mqtt3a.so (async mqtt sub). Again, the path to these modules may be different on your system.
gcc subscriber.c -ljson-c -lm /usr/local/lib/libpaho-mqtt3a.so -o subscribe

echo "The software is now ready to use."
echo "Open three terminals, one for sending the message, one for listening to the message and one for monitoring the database"
echo "This will prove that each element is working as it should be. Run publish "
