#/bin/bash

# msprofbin libmsprofiler.so stub/libmsprofiler.so
set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

rm -rf build_test
rm -rf ${TOP_DIR}/opensource/protobuf
rm -rf ${TOP_DIR}/opensource/json
rm -rf ${TOP_DIR}/opensource/securec
rm -rf ${TOP_DIR}/llt/opensource/googletest
rm -rf ${TOP_DIR}/llt/opensource/mockcpp/install
rm -rf ${TOP_DIR}/llt/build_llt
rm -rf ${TOP_DIR}/llt/output
rm -rf ${TOP_DIR}/output

shopt -s extglob
rm -rf !(analysis|build|cmake|CMakeLists.txt|collector|inc|llt|opensource|README.md|scripts|\.git)