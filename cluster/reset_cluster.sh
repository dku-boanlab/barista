#!/bin/bash

# reset the Barista database
mysql -u root -p < schemes/schemes.sql
