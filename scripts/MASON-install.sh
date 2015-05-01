#!/usr/bin/env bash

set -e -u
set -o pipefail

if [[ $(uname -s) == 'Linux' ]]; then
    export CXX="g++-4.8"
    export CC="gcc-4.8"
    export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python2.7/site-packages
else
    export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python/site-packages
fi

source ./bootstrap.sh
