#!/bin/bash
# This script is used to generate fuzz coverage.
# Copyright Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
FUZZ_WORK_PATH=${TOP_DIR}/test/fuzz_test/build_fuzz

bash ${TOP_DIR}/scripts/fuzz_cpp_testcase.sh

echo "*************[INFO] generate coverage report..."
if [ ! -d ${TOP_DIR}/test/opensource/SecTracy ]; then
    echo "*************[WARNING] cannot find SecTracy to generate coverage report"
    exit 1
fi
cd ${TOP_DIR}/test/opensource/SecTracy
python3 SecTracy.py -m ${FUZZ_WORK_PATH} -s ${FUZZ_WORK_PATH} -T ${TOP_DIR}/test/fuzz_test/test_api.txt
echo "[INFO] Coverage report is generated in ${TOP_DIR}/test/opensource/SecTracy/static"
echo "*************[INFO] generate coverage report succ"