#!/usr/bin/env bash

set -o pipefail

source ./bootstrap.sh

if [[ $(uname -s) == 'Linux' ]]; then
    # TODO: figure out how to avoid sudo
    sudo apt-get -q install rubygems
    export CXX="g++-4.8"
    export CC="gcc-4.8"
fi

if [[ ${COVERAGE} == true ]]; then
    PYTHONUSERBASE=$(pwd)/mason_packages/.link pip install --user cpp-coveralls;
    export LDFLAGS="--coverage"
    export CXXFLAGS="--coverage"
    if [[ $(uname -s) == 'Linux' ]]; then
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python2.7/site-packages
    else
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python/site-packages
    fi
fi