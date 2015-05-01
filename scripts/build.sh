#!/usr/bin/env bash

set -o pipefail

rm -rf build
mkdir -p build
cd build
cmake ../ ${CMAKEOPTIONS} -DCMAKE_INSTALL_PREFIX=${MASON_HOME} \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DBoost_NO_SYSTEM_PATHS=ON \
  -DTBB_INSTALL_DIR=${MASON_HOME} \
  -DCMAKE_INCLUDE_PATH=${MASON_HOME}/include \
  -DCMAKE_LIBRARY_PATH=${MASON_HOME}/lib \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="${LINK_FLAGS}"
make -j${JOBS}
make install

cd ../
echo "success"
