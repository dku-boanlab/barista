#!/bin/bash

MYSQL=`sudo netstat -ntlp | grep 3306 | wc -l`

if [ "$MYSQL" != "0" ]; then
    sudo service mysql stop
fi

if [ -z $1 ]; then
    docker-compose up
elif [ "$1" == "--API-monitor" ]; then
    export API_monitor=API_monitor
    docker-compose up
elif [ "$1" == "--cbench" ]; then
    export CBENCH=CBENCH
    docker-compose up
else
    echo "Usage: $0 [ NONE | --API-monitor | --cbench ]"
fi
