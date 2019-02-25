#!/bin/bash

DB_HOME=/etc/mysql
CONF_HOME=$DB_HOME/conf.d/

echo "Enter your DB ROOT password: "
read root_pw

echo "Enter the password of a default account: "
read default_pw

echo
echo "Create a default account (barista)"
echo
mysql -u root -p$root_pw -e "create user barista@localhost identified by '$default_pw'; grant all privileges on Barista.* to barista@localhost identified by '$default_pw'; grant all privileges on Barista_Mgmt.* to barista@localhost identified by '$default_pw'; flush privileges;"

echo "Create Barista database"
echo
mysql -u root -p$root_pw < schemes/schemes.sql

echo "Single setting is done"

