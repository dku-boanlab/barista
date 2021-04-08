#!/bin/bash

echo "Update 'root' password"
mysql -u root mysql < mysql.sql

echo "Create barista"
mysql -u root -p < barista.sql
