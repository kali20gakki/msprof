#!/bin/bash
# This script is used to generate llt-cpp coverage.
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
COV_DIR=${TOP_DIR}/test/build_llt/output/cpp_coverage
BUILD_DIR=${TOP_DIR}/test/build_llt

LCOV_RC="--rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1"

if [ ! -d ${COV_DIR} ] ; then
    mkdir -p ${COV_DIR}
fi
#----------------------------------------------------------
# coverage function
generate_coverage(){
    echo "********************** Generate $2 Coverage Start.************************"
    lcov -c -d $1 -o ${COV_DIR}/lcov_$2.info $LCOV_RC
    lcov -r ${COV_DIR}/lcov_$2.info '*c++*' -o ${COV_DIR}/lcov_$2.info $LCOV_RC
    lcov -r ${COV_DIR}/lcov_$2.info '*gtest*' -o ${COV_DIR}/lcov_$2.info $LCOV_RC
    lcov -r ${COV_DIR}/lcov_$2.info '*opensource*' -o ${COV_DIR}/lcov_$2.info $LCOV_RC
    echo "********************** Generate $2 Coverage Stop.*************************"
}
#----------------------------------------------------------
test_obj=(
    activity_utest
    dev_prof_task_utest
    callback_utest
    context_manager_utest
    mspti_utils_utest
    mspti_channel_utest
    mspti_parser_utest
    mspti_reporter_utest
    function_loader_utest
    mspti_adapter_utest
)

str_test=""
for test in ${test_obj[@]} ; do
    str_test=${str_test}"-a ${COV_DIR}/lcov_${test}.info "
    test_dir=`find ${BUILD_DIR} -name "${test}.dir"`
    target_dir=`ls -F ${test_dir} | grep "/$" | grep -v "test" | grep -v "__"`
    echo "${target_dir} ${test_dir} ${str_test}"
    generate_coverage ${test_dir}/${target_dir} ${test}
done

echo "${str_test}"
lcov ${str_test} -o ${COV_DIR}/ut_report.info $LCOV_RC
genhtml ${COV_DIR}/ut_report.info -o ${COV_DIR}/result --branch-coverage
echo "report: ${COV_DIR}"

if [[ -n "$1" && "$1" == "diff" ]];then
  targetBranch=${targetBranch:-master}
  lcov_cobertura ${COV_DIR}/ut_report.info -o ${COV_DIR}/coverage.xml
  diff-cover ${COV_DIR}/coverage.xml --html-report ${COV_DIR}/result/ut_incremental_coverage_report.html --compare-branch="origin/${targetBranch}"  --fail-under=80
fi
