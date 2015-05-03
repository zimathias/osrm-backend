#!/usr/bin/env bash

set -e -u
set -o pipefail

CURRENT_DIR=$(pwd)

if [[ $(uname) == 'Linux' ]]; then
    # adapted from https://gist.githubusercontent.com/DennisOSRM/803a64a9178ec375069f/raw/
    mkdir -p osmosos-tmp
    cd osmosos-tmp
    wget http://bretth.dev.openstreetmap.org/osmosis-build/osmosis-latest.tgz
    tar xfz osmosis-latest.tgz
    chmod a+x bin/osmosis
    export PATH="$(pwd)/bin":${PATH}
else
    brew install osmosis
fi

cd ${CURRENT_DIR}
set +e +u