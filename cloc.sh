#!/bin/bash

# count the lines of code except docs and libs
cloc --exclude-dir=docs,libs .
