#/bin/bash

# msprofbin libmsprofiler.so stub/libmsprofiler.so
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/build_test
cd ${TOP_DIR}/build_test
cmake ${TOP_DIR} -Dprotobuf_BUILD_TESTS=OFF
make -j8
