#!/bin/bash
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
cmake ../ -DPACKAGE=ut
make -j64


