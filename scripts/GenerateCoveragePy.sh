#!/bin/bash
#Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
# ================================================================================
set -e
script=$(readlink -f "$0")
route=$(dirname "$script")

export PYTHONPATH="${route}/../llt/msprof_python":$PYTHONPATH
echo "PYTHONPATH is ${PYTHONPATH}"

rm -rf ${route}/.coverage ${route}/report
mkdir -p ${route}/report

ret=0
code_dir=${route}/../src
coverage3 run --source=${code_dir} -m pytest cases --junitxml="${route}/report/final.xml" || ret=1
coverage3 combine
coverage3 xml -o ${route}/report/coverage.xml

exit ${ret}
