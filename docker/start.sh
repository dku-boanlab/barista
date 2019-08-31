#!/bin/bash

MYSQL=`sudo netstat -ntlp | grep 3306 | wc -l`

if [ "$MYSQL" != "0" ]; then
    sudo service mysql stop
fi

docker-compose up
