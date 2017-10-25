#!/bin/bash

if [ $1 = "ON" ]; then
	/usr/bin/curl -s -X POST -F 'limit=64' "http://localhost:8080/command/setGlobalDlLimit"
	/usr/bin/curl -s -X POST -F 'limit=64' "http://localhost:8080/command/setGlobalUpLimit"	
	echo ON
else
	/usr/bin/curl -s -X POST -F 'limit=NaN' "http://localhost:8080/command/setGlobalDlLimit"
    /usr/bin/curl -s -X POST -F 'limit=NaN' "http://localhost:8080/command/setGlobalUpLimit"
	echo OFF
fi

