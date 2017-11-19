#!/bin/bash
mosquitto_pub -d -h 192.168.1.xx -u user -P password -m "$1" -t "network/voji-pc/status"
