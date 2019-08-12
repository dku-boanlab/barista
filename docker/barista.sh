#!/bin/bash

# wait for backend
/wait-for-it.sh backend:3306

# move the base directory of Barista NOS
cd /barista/bin

# run Barista NOS
./barista -r

echo "Barista NOS is terminated"
