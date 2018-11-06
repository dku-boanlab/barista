#!/bin/bash

# extract the source files of ZeroMQ
tar xvfz zeromq-4.2.2.tar.gz -C /tmp

# move to ZeroMQ
cd /tmp/zeromq-4.2.2

# compile ZeroMQ
./autogen.sh && ./configure
make && sudo make install && sudo ldconfig
