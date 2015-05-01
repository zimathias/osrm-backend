#!/usr/bin/env bash

set -e -u
set -o pipefail

PYTHONUSERBASE=$(pwd)/mason_packages/.link pip install --user cpp-coveralls;
LDFLAGS="--coverage"
CXXFLAGS="--coverage"

source ./bootstrap.sh
