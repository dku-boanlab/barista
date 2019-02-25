#!/bin/bash

# MariaDB and galera
sudo apt-get -y install software-properties-common 
sudo apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 0xf1656f24c74cd1d8
sudo add-apt-repository 'deb http://ftp.osuosl.org/pub/mariadb/repo/10.0/ubuntu xenial main'
sudo apt-get update
sudo apt-get -y install mariadb-galera-server
sudo apt-get -y install mariadb-client libmariadbclient-dev
sudo systemctl enable mysql

