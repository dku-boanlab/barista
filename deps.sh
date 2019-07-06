#!/bin/bash

CURR=`pwd`

# update repo list
sudo apt-get update

# dependency => build
sudo apt-get -y install build-essential

# dependency => json
sudo apt-get -y install libjansson4 libjansson-dev

# dependency => libcli
cd $CURR/libs/libcli
./compile.sh

# dependency => zeromq
cd $CURR/libs/zeromq
./compile.sh

# dependency => doxygen, flex, bison, cmake (optional)
sudo apt-get -y install doxygen global flex bison cmake libav-tools

# dependency => LP solver (optional)
sudo apt-get -y install python-pip
sudo pip install pulp
sudo apt-get -y install glpk-utils coinor-cbc

# dependency => cloc (optional)
sudo apt-get -y install cloc
