#!/usr/bin/env bash

sudo add-apt-repository -y ppa:boost-latest/ppa
sudo apt-get update >/dev/null
sudo apt-get -q install protobuf-compiler libprotoc-dev libprotobuf7 libprotobuf-dev libbz2-dev libstxxl-dev libstxxl1 libxml2-dev libzip-dev lua5.1 liblua5.1-0-dev rubygems libtbb-dev
sudo apt-get install libboost1.54-all-dev
sudo apt-get install libgdal-dev
# luabind
curl https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh | sudo bash
# osmosis
curl -s https://gist.githubusercontent.com/DennisOSRM/803a64a9178ec375069f/raw/ | sudo bash
# cmake
curl -s https://gist.githubusercontent.com/DennisOSRM/5fad9bee5c7f09fd7fc9/raw/ | sudo bash
# osmpbf library
curl -s https://gist.githubusercontent.com/DennisOSRM/13b1b4fe38a57ead850e/raw/install_osmpbf.sh | sudo bash
