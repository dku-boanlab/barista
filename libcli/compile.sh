#!/bin/bash

# install libcli
make && sudo make install && make clean

# load library
sudo ldconfig
