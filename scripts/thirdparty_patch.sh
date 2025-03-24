#!/bin/bash
# This script is used to patch protobuf.
# Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
OPENSOURCE_DIR=${TOP_DIR}/opensource

function compile_protobuf() {
    # protobuf_3.14_change_version.patch来源：https://szv-cr-y.codehub.huawei.com/Turing_Solution/hisi/cmake.git
    cp ${TOP_DIR}/build/opensource_patch/protobuf_3.14_change_version.patch ${OPENSOURCE_DIR}/protobuf
    cd ${OPENSOURCE_DIR}/protobuf
    tar -zxvf protobuf-all-3.14.0.tar.gz
    cd protobuf-3.14.0
    ulimit -n 8192
    patch -p1 < ../0001-add-secure-compile-option-in-Makefile.patch
    patch -p1 < ../0002-add-secure-compile-fs-check-in-Makefile.patch
    patch -p1 < ../0003-fix-CVE-2021-22570.patch
    patch -p1 < ../0004-Improve-performance-of-parsing-unknown-fields-in-Jav.patch
    patch -p1 < ../0005-fix-CVE-2022-1941.patch
    patch -p1 < ../0006-fix-CVE-2022-3171.patch
    patch -p1 < ../0007-add-coverage-compile-option.patch
    patch -p1 < ../protobuf_3.14_change_version.patch
    cd ..
    cp -r protobuf-3.14.0 ${OPENSOURCE_DIR}
    cd ${OPENSOURCE_DIR}
    rm -rf protobuf
    mv protobuf-3.14.0 protobuf
}

if [ -f "${OPENSOURCE_DIR}/protobuf/protobuf-all-3.14.0.tar.gz" ]; then
    compile_protobuf
fi