#!/bin/bash

. /etc/os-release

# add the GPG key
sudo apt-get install -y curl
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

# add the Docker repository
sudo apt-get install -y software-properties-common python-software-properties
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

# update repo
sudo apt-get update

# make sure we install Docker from the Docker repo instead of Ubuntu 16.04 repo
sudo apt-cache policy docker-ce

# install Docker
if [ "$VERSION_ID" == "16.04" ]; then
    sudo apt-get install -y docker-ce=17.03.2~ce-0~ubuntu-xenial
elif [ "$VERSION_ID" == "14.04" ]; then
    sudo apt-get install -y docker-ce=17.03.2~ce-0~ubuntu-trusty
fi

# bypass sudo to run the docker command
sudo usermod -aG docker ${USER}

# install docker-compose
sudo curl -L https://github.com/docker/compose/releases/download/1.18.0/docker-compose-`uname -s`-`uname -m` \
          -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

docker-compose --version
