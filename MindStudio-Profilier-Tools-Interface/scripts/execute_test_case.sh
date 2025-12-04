#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

function add_gcov_excl_line_for_mspti() {
    find ${TOP_DIR}/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*MSPTI_.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*MSPTI_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
}

function add_gcov_excl_line() {
    add_gcov_excl_line_for_mspti
}

function change_file_to_unix_format()
{
    find ${TOP_DIR}/csrc -type f -exec sed -i 's/\r$//' {} +
}

mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
change_file_to_unix_format  # change file from dos to unix format, so that gcov exclude comment can be added
add_gcov_excl_line  # add gcov exclude comment for macro definition code lines to raise branch coverage
cmake ../ -DPACKAGE=ut
make -j$(nproc)
