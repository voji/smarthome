#!/bin/bash
IPRESOLVERS=("api.ipify.org" "bot.whatismyipaddress.com" "ipecho.net/plain")

LAST_IP_FILE="/var/jenkins_data/last_ip.txt"

if [ ! -f ${LAST_IP_FILE} ]; then
    echo "Last ip file ${LAST_IP_FILE} not exists..."
    exit 1
fi

LAST_IP=`cat "${LAST_IP_FILE}"`
if [ -z "$LAST_IP" ];
then
	echo "Last ip file ${LAST_IP_FILE} is empty..."
    exit 1
fi


CURR_IP=''
for IPR in ${IPRESOLVERS[@]}
do
 echo "Try to resolve ip address by ${IPR}"
 CURR_IP=`/usr/bin/curl -sS ${IPR}`
 if [ ! -z "${CURR_IP}" ];
 then
   break
 fi 
done

if [ -z "${CURR_IP}" ];
then
	echo "Unable to determine current ip... "
    exit 1
fi


if [ "${LAST_IP}" != "${CURR_IP}" ];
then
 echo url="https://zonomi.com/app/dns/ipchange.jsp?old_ip=${LAST_IP}&new_ip=${CURR_IP}&api_key=XXXXXX" | /usr/bin/curl -s -S -k -K - > /dev/null
 echo "${CURR_IP}" > "${LAST_IP_FILE}"
 echo "Server ip address updated: old_ip:${LAST_IP} / new_ip:${CURR_IP}"
else 
 echo "Server ip address (${LAST_IP}) not changed"
fi