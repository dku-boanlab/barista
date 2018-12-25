#!/bin/bash

# kill the Barista NOS
ps -ef | grep barista | grep -v grep | awk '{print $2}' | xargs -I{} sudo kill -9 {}
