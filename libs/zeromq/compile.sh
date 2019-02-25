#!/bin/bash

# extract the source code of ZeroMQ
tar xvfz zeromq-4.3.0.tar.gz -C /tmp

# move to ZeroMQ
cd /tmp/zeromq-4.3.0

# compile ZeroMQ
./autogen.sh && ./configure && make && sudo make install && sudo ldconfig

