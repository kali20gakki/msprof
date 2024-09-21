#!/bin/bash
# This script is used to generate llt-cpp coverage.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
COV_DIR=${TOP_DIR}/test/build_llt/output/cpp_coverage
BUILD_DIR=${TOP_DIR}/test/build_llt

if [ ! -d ${COV_DIR} ] ; then
    mkdir -p ${COV_DIR}
fi
#----------------------------------------------------------
# coverage function
generate_coverage(){
    echo "********************** Generate $2 Coverage Start.************************"
    lcov -c -d $1 -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*c++*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*msprof_llt*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*msprof_cpp*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*ascend_protobuf*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*gtest*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*opensource*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    lcov -r ${COV_DIR}/lcov_$2.info '*proto*' -o ${COV_DIR}/lcov_$2.info --rc lcov_branch_coverage=1
    echo "********************** Generate $2 Coverage Stop.*************************"
}
#----------------------------------------------------------
test_obj=(
    job_wrapper_utest
    driver_utest
    common_utest
    msprof_bin_utest
    streamio_common_utest
    plugin_utest
    application_utest
    message_utest
    msprofiler_utest
    transport_utest
    params_adapter_utest
    entities_utest
    utils_utest
    dfx_utest
    association_utest
    viewer_utest
    parser_utest
    device_association_utest
    device_entities_utest
    device_modeling_utest
    device_parser_utest
    device_persistence_utest
    activity_utest
    dev_prof_task_utest
    callback_utest
    context_manager_utest
    mspti_utils_utest
    mspti_channel_utest
    mspti_parser_utest
    function_loader_utest
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
lcov ${str_test} -o ${COV_DIR}/ut_report.info --rc lcov_branch_coverage=1
genhtml ${COV_DIR}/ut_report.info -o ${COV_DIR}/result --branch-coverage
echo "report: ${COV_DIR}"

if [[ -n "$1" && "$1" == "diff" ]];then
  targetBranch=${targetBranch:-master}
  lcov_cobertura ${COV_DIR}/ut_report.info -o ${COV_DIR}/coverage.xml
  diff-cover ${COV_DIR}/coverage.xml --html-report ${COV_DIR}/result/ut_incremental_coverage_report.html --compare-branch="origin/${targetBranch}"  --fail-under=80
fi
