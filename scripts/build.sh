#/bin/bash

# msprofbin libmsprofiler.so stub/libmsprofiler.so
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/build_test
cd ${TOP_DIR}/build_test
cmake ../cmake/superbuild/
make -j64
