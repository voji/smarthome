# smarthome
Smarthome project files (Openhab2, Arduino, Bash)

My home automation setup, used in production. 
All system comunicate over Mosquitto (MQTT).

Features:
- heater control
- boiler control
- daylight based lightning
- wake up light
- socket control
- garden watering system
- user habit prediction
- user location determination (using [OwnTracks](http://owntracks.org/))
- statistic of values (can process with PowerBI or other tools)

Some source files contains sensitive information (user names, passwords, etc). The content of these files redacted, and files renamed to *.*_example. If you would like to 
test the whole configuration, you need to remove the "_example" endings from these files. 