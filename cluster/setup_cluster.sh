#!/bin/bash

if [ -z $1 ]; then
    echo $0" [MY_IP_ADDRESS] [OTHER_IP_ADDRESSES(,)]"
    exit
fi

if [ -z $2 ]; then
    echo $0" [MY_IP_ADDRESS] [OTHER_IP_ADDRESSES(,)]"
    exit
fi

DB_HOME=/etc/mysql
CONF_HOME=$DB_HOME/conf.d/

echo "Cluster setting is started"
echo

if [ ! -f $CONF_HOME/mariadb.cnf.bak ];
then
    echo "Backup the original cluster configuration file"
    sudo cp $CONF_HOME/mariadb.cnf $CONF_HOME/mariadb.cnf.bak
fi

echo "Copy the cluster configuration file to "$CONF_HOME
sudo cp mariadb/mariadb.cnf $CONF_HOME/mariadb.cnf

echo
echo "MY_IP_ADDRESS = "$1
echo "OTHER_IP_ADDRESSES = "$2

sudo sed -i 's/MY_IP_ADDRESS/'$1'/g' $CONF_HOME/mariadb.cnf
sudo sed -i 's/OTHER_IP_ADDRESSES/'$2'/g' $CONF_HOME/mariadb.cnf

if [ ! -f $DB_HOME/debian.cnf.bak ];
then
    echo "Backup the original client configuration file"
    sudo cp $DB_HOME/debian.cnf $DB_HOME/debian.cnf.bak
fi

echo
echo "Copy the client configuration file to "$DB_HOME
sudo cp mariadb/debian.cnf $DB_HOME/debian.cnf
sudo chown root:root $DB_HOME/debian.cnf

echo "Enter your DB ROOT password"
read root_pw

echo "Reset 'debian-sys-maint' password"
mysql -u root -p$root_pw -e "SET PASSWORD FOR 'debian-sys-maint'@'localhost' = PASSWORD('MwE5FG8Vx6kTvxvP')"

echo
echo "Enter the password of a default account"
read default_pw

echo
echo "Create a default account"
mysql -u root -p$root_pw -e "create user barista@localhost identified by '$default_pw'; grant all privileges on Barista.* to barista@localhost identified by '$default_pw'; grant all privileges on Barista_Mgmt.* to barista@localhost identified by '$default_pw'; flush privileges;"

echo
echo "Create Barista database"
mysql -u root -p$root_pw < schemes/schemes.sql

echo
echo "Cluster setting is done"

