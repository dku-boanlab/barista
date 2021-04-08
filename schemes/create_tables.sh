#!/bin/bash

BARISTA_PASSWD=barista_passwd

echo "Update 'root' password"
echo "update user set password=password('$BARISTA_PASSWD') where user='root'; flush priviliges;" > mysql -u root mysql

echo "Create barista"
mysql -u root -p < barista.sql
