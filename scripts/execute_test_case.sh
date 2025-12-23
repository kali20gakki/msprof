#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

function add_gcov_excl_line_for_analysis() {
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*INFO.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*INFO.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*ERROR.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*ERROR.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*WARN.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*WARN.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*DEBUG.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*DEBUG.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*PRINT_.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*PRINT_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*MAKE_SHARED.*;$/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*MAKE_SHARED.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
}

function add_gcov_excl_line() {
    add_gcov_excl_line_for_analysis
}

function change_file_to_unix_format()
{
    find ${TOP_DIR}/analysis/csrc -type f -exec sed -i 's/\r$//' {} +
}

mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
if [[ -n "$1" && "$1" == "analysis" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=analysis
elif [[ -n "$1" && "$1" == "all" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=all
else
    change_file_to_unix_format # change file from dos to unix format, so that gcov exclude comment can be added
    add_gcov_excl_line # add gcov exclude comment for macro definition code lines to raise branch coverage
    cmake ../ -DPACKAGE=ut -DMODE=all
fi
make -j$(nproc)
