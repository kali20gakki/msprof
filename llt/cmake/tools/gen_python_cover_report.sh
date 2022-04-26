#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2018. All rights reserved.
# Author: panshuangwei 00444704
#----------------------------------------------------------------------------
set -e

# echo command line
echo "$0 $@"

OUTDIR="$1"
THRESHOLD="$2"
CLASSFILE_RULE="$3"
LOCAL_MODULE="$4"
SRC_PATH="$5"
TEST_PATH="$6"
EXPORT_PYTHONPATH="$7"
COVERAGERC_PATH="$8"
FULL_COVERAGE_CONFIG="$9"
PYTHON_VERSION="${10}"
LLT_KILL_TIME=900

TOPDIR="$(pwd)"

#去掉前后空格
CLASSFILE_RULE=$(echo "${CLASSFILE_RULE}" | sed 's/^ *//g; s/ *$//g')
SRC_PATH=$(echo "${SRC_PATH}" | sed 's/^ *//g; s/ *$//g')
TEST_PATH=$(echo "${TEST_PATH}" | sed 's/^ *//g; s/ *$//g')
EXPORT_PYTHONPATH=$(echo "${EXPORT_PYTHONPATH}" | sed 's/^ *//g; s/ *$//g')
COVERAGERC_PATH=$(echo "${COVERAGERC_PATH}" | sed 's/^ *//g; s/ *$//g')
FULL_COVERAGE_CONFIG=$(echo "${FULL_COVERAGE_CONFIG}" | sed 's/^ *//g; s/ *$//g')
PYTHON_VERSION=$(echo "${PYTHON_VERSION}" | sed 's/^ *//g; s/ *$//g')

echo "${PYTHON_VERSION}"

#默认跑全量覆盖率
if [[ "${FULL_COVERAGE_CONFIG}" != "false" ]]; then
    FULL_COVERAGE_CONFIG="true"
fi


SCRIPT_DIR="${TOPDIR}"/vendor/hisi/llt/ci/script/genaddcov
HTML_PATH="${OUTDIR}"/coverage_result/"${LOCAL_MODULE}"
PYTEST_CACHE="${OUTDIR}"/.pytest_cache/"${LOCAL_MODULE}"
XML_PATH="${OUTDIR}"/coverage_"${LOCAL_MODULE}".xml


#python_path还需要配上系统自带的库路径
export PYTHONPATH="${EXPORT_PYTHONPATH}":"${PYTHONPATH}"

#--------------------------------------开始执行测试用例---------------------------------------


if [[ "${FULL_COVERAGE_CONFIG}" = "true" ]]; then
    #执行用例，并生成全量覆盖率文件
    if [ "${COVERAGERC_PATH}"X != ""X ]; then
        echo "${PYTHON_VERSION} -m pytest --cov-report html:${HTML_PATH}  --cov-config=${COVERAGERC_PATH} --cov=${SRC_PATH}  ${TEST_PATH} -o cache_dir=${PYTEST_CACHE}"
        timeout -s SIGKILL "${LLT_KILL_TIME}"s "${PYTHON_VERSION}" -m pytest --cov-report html:"${HTML_PATH}"  --cov-config="${COVERAGERC_PATH}" --cov="${SRC_PATH}"  "${TEST_PATH}" -o cache_dir="${PYTEST_CACHE}"
    else
        echo "${PYTHON_VERSION} -m pytest --cov-report html:${HTML_PATH}  --cov=${SRC_PATH}  ${TEST_PATH} -o cache_dir=${PYTEST_CACHE}"
        timeout -s SIGKILL "${LLT_KILL_TIME}"s "${PYTHON_VERSION}" -m pytest --cov-report html:"${HTML_PATH}"  --cov="${SRC_PATH}"  "${TEST_PATH}" -o cache_dir="${PYTEST_CACHE}"
    fi

    res=$?
    if [ "$res" != 0 ];then
        echo "ERROR: exec full coverage [pytest fail]"
        exit 1
    else 
        echo "pytest run full coverage success"
        echo "the result path is in ${HTML_PATH}"
    fi

else
    #执行用例，并生成全量覆盖率文件
    if [ "${COVERAGERC_PATH}"X != ""X ];then
        echo "${PYTHON_VERSION} -m pytest --cov-report xml:${XML_PATH}  --cov-config=${COVERAGERC_PATH} --cov=${SRC_PATH}  ${TEST_PATH} -o cache_dir=${PYTEST_CACHE}"
        timeout -s SIGKILL "${LLT_KILL_TIME}"s "${PYTHON_VERSION}" -m pytest --cov-report xml:"${XML_PATH}"  --cov-config="${COVERAGERC_PATH}" --cov="${SRC_PATH}"  "${TEST_PATH}" -o cache_dir="${PYTEST_CACHE}"
    else
        echo "${PYTHON_VERSION} -m pytest --cov-report xml:${XML_PATH}  --cov=${SRC_PATH}  ${TEST_PATH} -o cache_dir=${PYTEST_CACHE}"
        timeout -s SIGKILL "${LLT_KILL_TIME}"s "${PYTHON_VERSION}" -m pytest --cov-report xml:"${XML_PATH}"  --cov="${SRC_PATH}"  "${TEST_PATH}" -o cache_dir="${PYTEST_CACHE}"
    fi

    res=$?
    if [ "$res" != 0 ];then
        echo "ERROR: exec increase coverage [pytest fail]"
        exit 1
    fi

    if [[ ! -d "${HTML_PATH}" ]]; then
        mkdir -p "${HTML_PATH}"
    fi

    #获取分支名称
    MANIFEST_PTH=.repo/manifests
    MANIFEST_BRANCH=$(echo $(cd  "${TOPDIR}"/"${MANIFEST_PTH}" && git config branch.default.merge) | cut -d "/" -f 3)
    GIT_BRANCH=m/"${MANIFEST_BRANCH}"
    echo "GIT_BRANCH is ${GIT_BRANCH}"
    cd "${TOPDIR}"
    #生成增量覆盖率报告
    echo "cd ${SRC_PATH} && diff-cover ${XML_PATH} --compare-branch=${GIT_BRANCH}  --html-report ${HTML_PATH}/report.html"
    cd "${SRC_PATH}" && diff-cover "${XML_PATH}" --compare-branch="${GIT_BRANCH}"  --html-report "${HTML_PATH}"/report.html

    res=$?
    if [ "$res" != 0 ]; then
        echo "ERROR: exec increase coverage [diff-cover fail]"
        exit 1
    else 
        echo "diff-cover run increase coverage success"
        echo "the result path is in ${HTML_PATH}"
    fi
fi

#统计覆盖率数据，根据threshold进行拦截
echo ${PYTHON_VERSION} "${SCRIPT_DIR}"/get_python_report.py "${CLASSFILE_RULE}"  "${HTML_PATH}" "${THRESHOLD}" "${FULL_COVERAGE_CONFIG}"
${PYTHON_VERSION} "${SCRIPT_DIR}"/get_python_report.py "${CLASSFILE_RULE}"  "${HTML_PATH}" "${THRESHOLD}" "${FULL_COVERAGE_CONFIG}"
if [ $? -ne 0 ]; then
    echo "run get_python_report.py failed"
    exit 1
fi
exit 0

#----------------------------------------end------------------------------------------------
