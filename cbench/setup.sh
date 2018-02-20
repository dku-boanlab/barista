#!/bin/bash

# we got oflops from https://github.com/andi-bigswitch/oflops

sudo apt-get install -y gcc-4.8 g++-4.8
sudo apt-get install -y autoconf libtool pkg-config libpcap-dev libconfig-dev
sudo apt-get install -y libnet-snmp-perl libsnmp-dev snmp-mibs-downloader

sudo mv /usr/bin/gcc /usr/bin/gcc.bak
sudo mv /usr/bin/g++ /usr/bin/g++.bak
sudo ln -s /usr/bin/gcc-4.8 /usr/bin/gcc
sudo ln -s /usr/bin/g++-4.8 /usr/bin/g++

cd oflops

./boot.sh
./configure
make
sudo make install

sudo mv /usr/bin/gcc.bak /usr/bin/gcc
sudo mv /usr/bin/g++.bak /usr/bin/g++

