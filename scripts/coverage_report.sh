#!/usr/bin/env bash

set -e -u
set -o pipefail

COVERAGE=${COVERAGE:-false}

# uploads data to https://coveralls.io/r/Project-OSRM/osrm-backend
if [[ ${COVERAGE} == true ]]; then
    ./mason_packages/.link/bin/cpp-coveralls \
        --build-root build \
        --gcov-options '\-lp' \
        --exclude mason_packages \
        --exclude build/CMakeCXXCompilerId.cpp \
        --exclude scripts \
        --exclude cmake \
        --exclude third_party \
        --exclude test \
        --exclude doc > /dev/null
fi
