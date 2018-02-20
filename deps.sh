#!/bin/bash

# update repo list
sudo apt-get update

# dependency => build
sudo apt-get -y install build-essential

# dependency => json
sudo apt-get -y install libjansson4 libjansson-dev

# dependency => libcli
cd libraries/libcli
./setup.sh
cd ../..

# dependency => zeromq
cd libraries/zeromq
./setup.sh
cd ../..

# dependency => doxygen, htags
sudo apt-get -y install doxygen global

# development => vim, git, gdb, line of code
sudo apt-get -y install vim git gdb cloc

