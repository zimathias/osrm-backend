#!/usr/bin/env bash

set -e -u
set -o pipefail

# Intentionally not using mason here so that we
# replicate a system where mostly apt packages are used
# and only minimal source compiles
# Normally we would install into /usr/local but in this
# case do not to be able to use a sudo-less install on travis
BUILD_DIR="/tmp-osrm-source-installed-deps/"
mkdir -p ${BUILD_DIR}

CURRENT_DIR=$(pwd)
cd ${BUILD_DIR}

echo "installing custom luabind"
# adapted from https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh
git clone --depth 1 https://github.com/DennisOSRM/luabind.git
cd luabind
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=${CXX} \
  -DCMAKE_INSTALL_PREFIX=${BUILD_DIR} \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=${TARGET}
make -j${JOBS}
make install


echo "installing custom cmake because the apt package is too old"
# adapted from https://gist.githubusercontent.com/DennisOSRM/5fad9bee5c7f09fd7fc9/raw/
cd ${BUILD_DIR}
wget http://www.cmake.org/files/v3.2/cmake-3.2.1.tar.gz
tar -xvzf cmake-3.2.1.tar.gz > /tmp/unpack.txt
cd cmake-3.2.1
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_INSTALL_PREFIX=${BUILD_DIR} >/tmp/cmake.log 2> /tmp/cmake_err.log
make >/tmp/make.log 2> /tmp/make_error.log
make install >/tmp/make.log 2> /tmp/make_error.log

echo "install osmpbf library because the apt package is too old"
# adapted from https://gist.githubusercontent.com/DennisOSRM/13b1b4fe38a57ead850e/raw/install_osmpbf.sh
cd ${BUILD_DIR}
git clone --depth 1 https://github.com/scrosby/OSM-binary.git
cd OSM-binary
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}
make install

echo "install success, now setting PATH and LD_LIBRARY_PATH so custom builds are found"
export PATH=${BUILD_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${BUILD_DIR}/lib:${LD_LIBRARY_PATH}

cd ${CURRENT_DIR}
set +e +u