#!/bin/bash

READY=1
while [ $READY -ne 0 ];
do
    /wait-for-it.sh 127.0.0.1:3306 -t 1 -q
    READY=`echo $?`
done

# move the base directory of Barista NOS
cd /barista/bin

# run Barista NOS
./barista -r

echo "Barista NOS is terminated"
