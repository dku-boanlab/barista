#!/bin/bash

# wait for backend
/wait-for-it.sh barista:6011

# move to the base directory of Barista NOS
cd /barista/bin

# run L2 learning application
./l2_learning

echo "L2 learning application is terminated"
