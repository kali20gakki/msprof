#!/bin/bash

set -e

CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/llt/build_llt
cd ${TOP_DIR}/llt/build_llt
cmake ../ -DPACKAGE=ut -DFULL_COVERAGE=true -DCOVERAGE_RC_CONFIG=true
make -j64

