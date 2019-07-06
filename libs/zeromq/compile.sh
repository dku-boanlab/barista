#!/bin/bash

# decompress zeromq
tar xvfz zeromq-4.3.0.tar.gz -C /tmp

# move to zeromq
cd /tmp/zeromq-4.3.0

# install zeromq
./autogen.sh && ./configure && make && sudo make install && sudo ldconfig
