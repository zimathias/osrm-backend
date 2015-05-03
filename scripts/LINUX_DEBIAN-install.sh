#!/usr/bin/env bash

set -e -u
set -o pipefail

echo "installing custom builds:"
# luabind
curl https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh | sudo bash
# cmake
curl -s https://gist.githubusercontent.com/DennisOSRM/5fad9bee5c7f09fd7fc9/raw/ | sudo bash
# osmpbf library
curl -s https://gist.githubusercontent.com/DennisOSRM/13b1b4fe38a57ead850e/raw/install_osmpbf.sh | sudo bash

echo "install success"

set +e +u