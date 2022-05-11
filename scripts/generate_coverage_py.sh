#!/bin/bash
set -e
real_path=$(readlink -f "$0")
script_dir=$(dirname "$real_path")
output_dir="${script_dir}/../test/llt/output"
export PYTHONPATH="${script_dir}/../analysis":"${script_dir}/../test/llt/msprof_python/ut/testcase":${PYTHONPATH}
cd ${output_dir}
rm -f .coverage
coverage run --source="/home/wwx56293/msprof/analysis/" -m pytest -s "/home/wwx56293/msprof/test/llt/msprof_python/ut/testcase"
coverage report > python_coverage_report.log