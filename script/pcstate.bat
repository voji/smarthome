@echo off
rem post pc status message to mqtt
rem setup: https://technet.microsoft.com/en-us/library/cc770556(v=ws.11).aspx
rem to use this, you need to disable the fast boot feature.

mosquitto_pub -d -h 192.168.1.xx -u user -P password -m "$1" -t "network/voji-pc/status"