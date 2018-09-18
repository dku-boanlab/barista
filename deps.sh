#!/bin/bash

# update repo list
sudo apt-get update

# dependency => build
sudo apt-get -y install build-essential

# dependency => json
sudo apt-get -y install libjansson4 libjansson-dev

# dependency => doxygen, flex, bison, cmake
sudo apt-get -y install doxygen global flex bison cmake libav-tools

# dependency => libcli
cd libraries/libcli
make
sudo make install
make clean
cd -

# dependency => zeromq
cd libraries/zeromq
./compile.sh
cd -
