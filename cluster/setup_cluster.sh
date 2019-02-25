#!/bin/bash

if [ -z $1 ]; then
    echo $0" [MY_IP_ADDRESS] none (at a master node)"
    echo $0" [MY_IP_ADDRESS] [OTHER_IP_ADDRESSES(,)] (at other nodes)"
    exit
fi

if [ -z $2 ]; then
    echo $0" [MY_IP_ADDRESS] none (at a master node)"
    echo $0" [MY_IP_ADDRESS] [OTHER_IP_ADDRESSES(,)] (at other nodes)"
    exit
fi

DB_HOME=/etc/mysql
CONF_HOME=$DB_HOME/conf.d/

echo "Cluster setting is started"
echo

if [ ! -f $CONF_HOME/mariadb.cnf.bak ];
then
    echo "Backup the original cluster configuration file"
    echo
    sudo cp $CONF_HOME/mariadb.cnf $CONF_HOME/mariadb.cnf.bak
fi

echo "Copy the cluster configuration file to "$CONF_HOME
echo
sudo cp mariadb/mariadb.cnf $CONF_HOME/mariadb.cnf

echo "MY_IP_ADDRESS = "$1
echo "OTHER_IP_ADDRESSES = "$2
echo
sudo sed -i 's/MY_IP_ADDRESS/'$1'/g' $CONF_HOME/mariadb.cnf
if [ "$2" == "none" ]; then
	sudo sed -i 's/OTHER_IP_ADDRESSES//g' $CONF_HOME/mariadb.cnf
else
	sudo sed -i 's/OTHER_IP_ADDRESSES/'$2'/g' $CONF_HOME/mariadb.cnf
fi

if [ ! -f $DB_HOME/debian.cnf.bak ];
then
    echo "Backup the original client configuration file"
    echo
    sudo cp $DB_HOME/debian.cnf $DB_HOME/debian.cnf.bak
fi

echo "Copy the client configuration file to "$DB_HOME
echo
sudo cp mariadb/debian.cnf $DB_HOME/debian.cnf
sudo chown root:root $DB_HOME/debian.cnf

echo "Enter your DB ROOT password: "
read root_pw

echo
echo "Reset 'debian-sys-maint' password"
echo
mysql -u root -p$root_pw -e "SET PASSWORD FOR 'debian-sys-maint'@'localhost' = PASSWORD('MwE5FG8Vx6kTvxvP')"

echo "Enter the password of a default account: "
read default_pw

echo
echo "Create a default account (barista)"
echo
mysql -u root -p$root_pw -e "create user barista@localhost identified by '$default_pw'; grant all privileges on Barista.* to barista@localhost identified by '$default_pw'; grant all privileges on Barista_Mgmt.* to barista@localhost identified by '$default_pw'; flush privileges;"

echo "Create Barista database"
echo
mysql -u root -p$root_pw < schemes/schemes.sql

echo "Cluster setting is done"

