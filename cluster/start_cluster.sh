#!/bin/bash

# start MySQL service
if [ "$1" == "new" ]; then
    # only for the first node in a cluster
    sudo service mysql start --wsrep-new-cluster
else
    # for other nodes
    sudo service mysql start
fi
