#/bin/bash

# msprofbin libmsprofiler.so stub/libmsprofiler.so
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
if [ ! -d ${TOP_DIR}/build ]; then
    mkdir -p  ${TOP_DIR}/build
fi
cd ${TOP_DIR}/build
cmake ../cmake/superbuild/
make -j64

# bash create_run_package.sh
