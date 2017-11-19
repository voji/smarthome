#!/bin/bash

#arduino ping file for jenkins ci tool

PING_RESULT_FILE="${WORKSPACE}/wp_xxx_res.txt"
MQTT_USER=user
MQTT_PASS=password

echo sending ping message
mosquitto_pub -u ${MQTT_USER} -P ${MQTT_PASS} -m 'ON' -t 'wemos_xxx_ping'

echo waiting for response
/usr/bin/timeout 90 /usr/bin/mosquitto_sub -C 1 -t 'wemos_xxx_pong' -u ${MQTT_USER} -P ${MQTT_PASS} > ${PING_RESULT_FILE}

PING_RESULT=`cat "${PING_RESULT_FILE}"`
rm ${PING_RESULT_FILE}

if [ "${PING_RESULT}" == "pong" ];
then
 echo XXX Wemos D1 up and running
else
 echo XXX Wemos D1 down
 exit 1
fi