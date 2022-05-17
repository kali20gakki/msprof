#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2018. All rights reserved.
# Author: lilinxin 00437233
#----------------------------------------------------------------------------
set -e

# echo command line
echo "$0 $@"

FULL_COVERAGE_CONFIG="$1"
OUTDIR="$2"
TARGETS="$3"
BLACKLIST="$4"
INC_COV="$5"
THRESHOLD="$6"
RUN_ST_FULL="$7"
COVERAGEAREA="$8"
BUILD_MOD="$9"
TEST_TYPE="${10}"
CUR_DIR=$(dirname $(readlink -f "$0"))
prefix=${CUR_DIR%%/cmake*}
FAIL=1
SUCC=0
RES_OUT="${OUTDIR}/llt"
FLAG="success"
export PATH="${prefix}/build/prebuilts/gcc/linux-x86/x86/x86_64-unknown-linux-gnu-4.9.3/bin:${PATH}"

HI_PYTHON=python3.9

echo "FULL_COVERAGE_CONFIG is ${FULL_COVERAGE_CONFIG}"
echo "OUTDIR is ${OUTDIR}"
echo "TARGETS is ${TARGETS}"
echo "BLACKLIST is ${BLACKLIST}"
echo "INC_COV is ${INC_COV}"
echo "THRESHOLD is ${THRESHOLD}"
echo "RES_OUT is ${RES_OUT}"
echo "RUN_ST_FULL is ${RUN_ST_FULL}"
echo "COVERAGEAREA is ${COVERAGEAREA}"


#默认跑全量覆盖率
if [[ "${FULL_COVERAGE_CONFIG}" != "false" ]]; then
    FULL_COVERAGE_CONFIG="true"
fi

if [[ "${INC_COV}" != "false" ]]; then
    INC_COV="true"
fi

FULL_COVERAGE_CONFIG=$(echo "${FULL_COVERAGE_CONFIG}" | sed 's/^ *//g; s/ *$//g')
TARGETS=$(echo "${TARGETS}" | sed 's/^ *//g; s/ *$//g')
BLACKLIST=$(echo "${BLACKLIST}" | sed 's/^ *//g; s/ *$//g')
THRESHOLD=$(echo "${THRESHOLD}" | sed 's/^ *//g; s/ *$//g')
RUN_ST_FULL=$(echo "${RUN_ST_FULL}" | sed 's/^ *//g; s/ *$//g')

echo "TARGETS=${TARGETS}"
echo "BLACKLIST=${BLACKLIST}"
echo "INC_COV is ${INC_COV}"
echo "FULL_COVERAGE_CONFIG is ${FULL_COVERAGE_CONFIG}"
echo "RUN_ST_FULL is ${RUN_ST_FULL}"

if [ ! -n "$THRESHOLD" ]; then
    THRESHOLD="80"
fi
echo "final THRESHOLD is ${THRESHOLD}"

#lcov_result_path=${RES_OUT}/coverage_result
ut_info_path="${OUTDIR}/llt/coverage.info"
#src_prefix=${prefix%%/vendor*}
SCRIPT_DIR="${prefix}/vendor/hisi/llt/ci/script/genaddcov"

#get inc coverage
if [ "${INC_COV}"X = "true"X ]
then
    set +e
    echo ${HI_PYTHON} "${SCRIPT_DIR}"/get_ai_inc_cov.py "${TARGETS}" "${BLACKLIST}" "${RES_OUT}" "${INC_COV}" "${THRESHOLD}" "${ut_info_path}" "${COVERAGEAREA}"
    ${HI_PYTHON} "${SCRIPT_DIR}"/get_ai_inc_cov.py "${TARGETS}" "${BLACKLIST}" "${RES_OUT}" "${INC_COV}" "${THRESHOLD}" "${ut_info_path}" "${COVERAGEAREA}"
    if [ $? -ne 0 ]
    then
        echo "generate inc coverage failed"
        FLAG="fail"
    else
        FLAG="success"
    fi
    set -e
fi

echo "FLAG: $FLAG, THRESHOLD: $THRESHOLD, FULL_COVERAGE_CONFIG: $FULL_COVERAGE_CONFIG"
if [[ "${FULL_COVERAGE_CONFIG}" == "true" ]]; then
    rm -f "${ut_info_path}"

    ${HI_PYTHON} "${SCRIPT_DIR}"/get_full_lcov.py  "${RES_OUT}" "${ut_info_path}" "${prefix}"
    # lcov -c -d ${OUTDIR} -o ${ut_info_path}
    lcov -r "${ut_info_path}" \
    "${prefix}/prebuilts/gcc*" \
    "${prefix}/build/llt*" \
    "${prefix}/llt/*" \
    "${prefix}/abl/libc_sec/*" \
    "${prefix}/framework/domi/common/auth/wbcipher/*" \
    "${prefix}/framework/domi/proto/*" \
    "${prefix}/third_party/*" \
    "${prefix}/framework/domi/calibration/proto/*" \
    "${prefix}/framework/domi/common/cce*" \
    "${prefix}/framework/domi/common/runtime*" \
    "${prefix}/matrix/stub*" \
    "${prefix}/out*" \
    "${prefix}/inc/engine/proto/*" \
    "${prefix}/inc/engine/*" \
    "${prefix}/toolchain/gdb/gdb/stub-termcap.c" \
    "${prefix}/tmp/*" \
    -o "${ut_info_path}"

    set +e
    ${HI_PYTHON} "${SCRIPT_DIR}"/get_component_cov.py "${TARGETS}" "${RES_OUT}" "${THRESHOLD}" "${ut_info_path}"
    if [[ $? -ne 0 && "${FLAG}" = "fail" ]]; then
        echo "ERROR:generate full coverage failed!"
    else
        echo "generate full coverage html success"
        FLAG="success"
    fi
    set -e
fi
if [[ "$DAILYBUILDCAPITAL" == "true" ]];then
    ${HI_PYTHON} "${SCRIPT_DIR}/../coverage.py" --code-root="$prefix" --classify_rule_name="$TARGETS" --test-type="${TEST_TYPE}"
fi
if [[ "${FLAG}" != "success" ]]; then
    exit "${FAIL}"
fi
