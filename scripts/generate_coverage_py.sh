#!/bin/bash
set -e
real_path=$(readlink -f "$0")
script_dir=$(dirname "$real_path")
output_dir="${script_dir}/../test/build_llt/output/python_coverage"
src_code="${script_dir}/../analysis"
test_code="${script_dir}/../test/msprof_python/ut/testcase"
export PYTHONPATH=${src_code}:${test_code}:${PYTHONPATH}
mkdir -p ${output_dir}
cd ${output_dir}
rm -f .coverage
coverage run --branch --source=${src_code} -m pytest -s "${test_code}" --junit-xml=./final.xml
coverage xml
coverage report > python_coverage_report.log
echo "report: ${output_dir}"
find ${script_dir}/.. -name "__pycache__" | xargs rm -r

