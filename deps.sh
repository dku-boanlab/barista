#!/bin/bash

. /etc/os-release

# update repo list
sudo apt-get update

# dependency => build
sudo apt-get -y install build-essential

# dependency => json
sudo apt-get -y install libjansson4 libjansson-dev

# dependency => doxygen, flex, bison, cmake
sudo apt-get -y install doxygen global flex bison cmake #libav-tools

# dependency => zeromq
sudo apt-get -y install libzmq3-dev

# dependency => libcli
cd libcli; ./compile.sh; cd ..

# dependency => mariadb
sudo apt-get -y install software-properties-common

if [ "$VERSION_ID" == "14.04" ]; then
    sudo apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 0xcbcb082a1bb943db
    sudo add-apt-repository "deb http://ftp.kaist.ac.kr/mariadb/repo/10.0/ubuntu trusty main"
elif [ "$VERSION_ID" == "16.04" ]; then
    sudo apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 0xf1656f24c74cd1d8
    sudo add-apt-repository "deb http://ftp.kaist.ac.kr/mariadb/repo/10.0/ubuntu xenial main"
else
    sudo apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 0xF1656F24C74CD1D8
    sudo add-apt-repository "deb [arch=amd64,arm64,ppc64el] http://ftp.kaist.ac.kr/mariadb/repo/10.4/ubuntu $(lsb_release -cs) main"
fi

sudo apt-get update
sudo apt-get -y install mariadb-client libmariadbclient-dev

read -p "Do you want to install MariaDB here (y/N)?"
if [ "$REPLY" != "y" ]; then
    exit
fi

if [ "$VERSION_ID" == "14.04" ]; then
    sudo apt-get -y install mariadb-galera-server
elif [ "$VERSION_ID" == "16.04" ]; then
    sudo apt-get -y install mariadb-galera-server
else
    sudo apt-get -y install mariadb-server
fi

sudo systemctl enable mysql
sudo systemctl start mysql

# required databases
cd schemes; ./create_tables.sh; cd ..
