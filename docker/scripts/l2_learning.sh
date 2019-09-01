#!/bin/bash

READY=1
while [ $READY -ne 0 ];
do
    /wait-for-it.sh 127.0.0.1:6633 -t 1 -q
    READY=`echo $?`
done

# move to the base directory of Barista NOS
cd /barista/bin

# run L2 learning application
./l2_learning docker

echo "L2 learning application is terminated"
