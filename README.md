# senti-tech
4 module example for senti-tech


The 4 modules I have selected to build are:

.  The publish module for the embedded MQTT client


.  The subscribe module for the MQTT client


.  The database input script to put the MQTT data inside of a database
 
 
 . The build script to tie them all together.
 
 To run, chmod +x ./init.sh
 then sudo ./init.sh 
 
 This is designed for a clean installation of Mosquitto and MySQL, if an existing installation exists- please use a clean system.
 The script is also built for Ubuntu/Debian using Bash. In a production system, I would modify this to run on CentOS. 
 
 I built on Ubuntu as this is the OS of my Dev machine.

**The project will NOT work on Windows or Mac. It will work as-is on Debian-based distros such as Ubuntu or Mint... But will require additional config for other distros.
