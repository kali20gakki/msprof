#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

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

compile_protobuf

function add_gcov_excl_line_for_analysis() {
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*INFO.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*INFO.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*ERROR.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*ERROR.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*WARN.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*WARN.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*DEBUG.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*DEBUG.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*PRINT_.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*PRINT_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/analysis/csrc -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*MAKE_SHARED.*;$/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*MAKE_SHARED.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
}

function add_gcov_excl_line_for_collector() {
    find ${TOP_DIR}/collector/dvvp \( -name "*.cpp" -o -name "*.h" \) -type f ! -name "op_desc_parser.cpp" -exec sed -i -e '/^[[:blank:]]*MSPROF_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' -e 's/^[[:blank:]]*MSPROF_.*[;,"]$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/collector/dvvp \( -name "*.cpp" -o -name "*.h" \) -type f -exec sed -i -e '/^[[:blank:]]*MSVP_MAKE_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' -e 's/^[[:blank:]]*MSVP_MAKE_.*[;,"]$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    find ${TOP_DIR}/collector/dvvp -name "*.cpp" -type f -exec sed -i -e '/^[[:blank:]]*FUNRET_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' -e 's/^[[:blank:]]*FUNRET_.*[;,"]$/& \/\/ LCOV_EXCL_LINE/g' {} \;
    sed -i -e 's/^[[:blank:]]*CHECK_.*;/& \/\/ LCOV_EXCL_LINE/g' -e 's/^[[:blank:]]*MSPROF_.*;$/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/analyze/src/op_desc_parser.cpp
    sed -i -e 's/^[[:blank:]]*static.*;/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/common/singleton/singleton.h
    sed -i -e 's/^[[:blank:]]*func.*;/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/depend/inc/plugin/plugin_handle.h
    sed -i -e 's/^[[:blank:]]*if.*{/& \/\/ LCOV_EXCL_LINE/g' -e '/)dlogInnerForC_/s/$/\/\/LCOV_EXCL_LINE/' -e 's/^[[:blank:]]*func.*;/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/depend/inc/plugin/slog_plugin.h
    find ${TOP_DIR}/collector/dvvp -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*};/&\/\/ LCOV_EXCL_LINE/' {} \;
    sed -i -e 's/^[[:blank:]]*GET_JSON_STRING_VALUE.*;/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/message/prof_json_config.cpp
    sed -i -e 's/^[[:blank:]]*GET_JSON_INT_VALUE.*;/& \/\/ LCOV_EXCL_LINE/g' ${TOP_DIR}/collector/dvvp/message/prof_json_config.cpp
    find ${TOP_DIR}/collector/dvvp/params_adapter -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*}).swap.*;/& \/\/ LCOV_EXCL_LINE/g' {} \;
    sed -i -e 's/^[[:blank:]]*};/&\/\/ LCOV_EXCL_LINE/' ${TOP_DIR}/collector/dvvp/message/prof_json_config.h
}

function add_gcov_excl_line_for_mspti() {
    find ${TOP_DIR}/mspti -name "*.cpp" -type f -exec sed -i -e 's/^[[:blank:]]*MSPTI_.*;/& \/\/ LCOV_EXCL_LINE/g' -e '/^[[:blank:]]*MSPTI_.*[,"]$/,/.*;$/ s/;$/& \/\/ LCOV_EXCL_LINE/g' {} \;
}

function add_gcov_excl_line() {
    add_gcov_excl_line_for_analysis
    add_gcov_excl_line_for_collector
    add_gcov_excl_line_for_mspti
}

function change_file_to_unix_format()
{
    find ${TOP_DIR}/analysis/csrc -type f -exec sed -i 's/\r$//' {} +
    find ${TOP_DIR}/collector/dvvp -type f -exec sed -i 's/\r$//' {} +
    find ${TOP_DIR}/mspti -type f -exec sed -i 's/\r$//' {} +
}

mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
if [[ -n "$1" && "$1" == "analysis" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=analysis
elif [[ -n "$1" && "$1" == "collector" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=collector
elif [[ -n "$1" && "$1" == "mspti" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=mspti
elif [[ -n "$1" && "$1" == "all" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=all
else
    change_file_to_unix_format # change file from dos to unix format, so that gcov exclude comment can be added
    add_gcov_excl_line # add gcov exclude comment for macro definition code lines to raise branch coverage
    cmake ../ -DPACKAGE=ut -DMODE=all
fi
make -j$(nproc)
