#/bin/bash

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/build_llt
cd ${TOP_DIR}/build_llt
cmake ${TOP_DIR}/llt -Dprotobuf_BUILD_TESTS=OFF
make -j16