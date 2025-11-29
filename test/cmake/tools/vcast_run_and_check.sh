#!/bin/bash
#----------------------------------------------------------------------------
# Purpose:
# Copyright Huawei Technologies Co., Ltd. 2010-2018. All rights reserved.
#----------------------------------------------------------------------------

VCM_FILE="$1"
RUN_ENV_FILE="$2"
REPORT_DIR="$3"
LLT_KILL_TIME="$4"

if [ ! -n "$LLT_KILL_TIME" ]; then
    LLT_KILL_TIME=900
fi
if [ "$DAILYBUILDCAPITAL" == 'true' ];then
    LLT_KILL_TIME=1200
fi

CUR_DIR=$(dirname $(readlink -f "$0"))
SRC_ROOT=$(readlink -f "$CUR_DIR/../../..")
TOP_DIR=$SRC_ROOT

FAIL=1
SUCC=0

export VECTORCAST_DIR="/home/phisik3/vcast"
export VECTOR_LICENSE_FILE=27000@specchecker-pc
export SRC_ROOT
export TOP_DIR
export PATH=/opt/buildtools/qemu-3.0.0/bin:$PATH

PROJECT_FILENAME=`basename ${VCM_FILE}`                             #vcast工程文件文件名
PROJECT_PATH=`dirname ${VCM_FILE}`                                  #vcast工程目录
PROJECT_NAME=${PROJECT_FILENAME%.*}                                 #vcast工程名，不带vcm后缀

echo "VECTORCAST_DIR        is ${VECTORCAST_DIR}"
echo "VECTOR_LICENSE_FILE   is ${VECTOR_LICENSE_FILE}"
echo "VCM_FILE              is ${VCM_FILE}"
echo "RUN_ENV_FILE          is ${RUN_ENV_FILE}"
echo "TOP_DIR               is ${TOP_DIR}"
echo "PROJECT_FILENAME      is ${PROJECT_FILENAME}"
echo "PROJECT_PATH          is ${PROJECT_PATH}"
echo "PROJECT_NAME          is ${PROJECT_NAME}"
echo "LLT_KILL_TIME         is ${LLT_KILL_TIME}"

function run_and_print() { echo "\$ $@" ; "$@" ; }

function run_vcast(){
    echo "\$ $@"
    local n=1
    local max=5
    local delay=1
    local tmpfile=$(mktemp /tmp/tmp.XXXXXXXXXX) || exit 1
    while true;do
        "$@" 1>$tmpfile 2>&1
        local ret=$?
        cat $tmpfile
        if [[ "${ret}" -ne 0 ]];then
            return ${ret}
        fi
        result=$(cat $tmpfile | grep "Licensing Error: Licensed number of users already reached")
        rm -rf $tmpfile
        [[ "$result" == "" ]] && break ||{
            if [[ $n -lt $max ]]; then
                ((n++))
                echo "Licensed number of users already reached. Attempt $n/$max:"
                sleep $delay;
            else
                echo "[error] The command has failed after $n attempts."
                exit "${FAIL}"
            fi
        }
    done
}

function load_env(){
    #加载组件的个性化环境变量
    if [ "${RUN_ENV_FILE}" != "" ]; then
        if [ -f "${RUN_ENV_FILE}" ]; then
            echo "source ${RUN_ENV_FILE}"
            source "${RUN_ENV_FILE}"
        else
            echo "WARNING: RUN_ENV_FILE ${RUN_ENV_FILE} does not exist, skip source."
        fi
    fi
}

function prepare(){
    # 环境及目录准备工作

    # 切换容器化，vcast客户端需要一个固定的hostname名称，不然连不上license
    hi_sudo hostname ci-online-k8s-host
    mkdir -p ${REPORT_DIR}/tmp/${PROJECT_NAME}
    mkdir -p ${REPORT_DIR}/log
    mkdir -p ${REPORT_DIR}/failed_report
}

function check_vcast_cfg(){
    # 检查vcast的基本配置是否合格

    if grep -q 'VCAST_FILE_VERSION_COMMAND' $VCM_FILE; then
        return ${SUCC}
    fi

    for environment in `ls $PROJECT_PATH/$PROJECT_NAME/environment/`; do
        # 要获取vcast覆盖率报告中的文件清单，需要配置：VCAST_FILE_VERSION_COMMAND: readlink -f
        if ! grep -q 'VCAST_FILE_VERSION_COMMAND' $PROJECT_PATH/$PROJECT_NAME/environment/${environment}/${environment}.cfg; then
            echo "[error] $PROJECT_PATH/$PROJECT_NAME/environment/${environment}/${environment}.cfg not config VCAST_FILE_VERSION_COMMAND: readlink -f"
            return  ${FAIL}
        fi
    done
    return ${SUCC}
}

function vcast_process(){
    # vcast 执行流程

    # 多个vcm工程并行会冲突，需要从不同的路径下分别执行
    cd ${REPORT_DIR}/tmp/${PROJECT_NAME}
    local start_time=$(date +%s)
    VCAST_RUN_STARTTIME=$(date "+%Y-%m-%d %H:%M:%S")
    run_vcast ${VECTORCAST_DIR}/manage -p ${VCM_FILE} --build-execute 1>${REPORT_DIR}/log/${PROJECT_NAME}.log 2>&1
    run_ret=${PIPESTATUS[0]}
    echo "exit code: ${run_ret}"
    local end_time=$(date +%s)
    local duration=$(( $end_time - $start_time ))

    # 用例执行时长超过阈值，判定为不通过
    if [ "${duration}" -gt "$LLT_KILL_TIME" ]
    then
        echo "[error] timeout: vcast project=${PROJECT_NAME} run time duration is ${duration}s, greater than LLT_KILL_TIME $LLT_KILL_TIME seconds!"
        exit "${FAIL}"
    fi

    if [ "${run_ret}" -eq 137 ]
    then
        echo "[error] timeout: execute ${VCM_FILE} more than ${LLT_KILL_TIME}s killed"
        exit "${FAIL}"
    fi

    if [ "${run_ret}" -ne 0 ]
    then
        cat ${REPORT_DIR}/log/${PROJECT_NAME}.log
        echo "[error] execute ${VCM_FILE} failed!"
        exit "${FAIL}"
    fi

    if grep 'Warning! Environment script specified a Search Directory that does not exist' "${REPORT_DIR}/log/${PROJECT_NAME}.log" >/dev/null
    then
        echo "[error] (${PROJECT_NAME}) Environment script specified a Search Directory that does not exist."
        exit "${FAIL}"
    fi

    if grep 'The environment is invalid because' "${REPORT_DIR}/log/${PROJECT_NAME}.log" >/dev/null
    then
        echo "[error] (${PROJECT_NAME}) The environment is invalid"
        exit "${FAIL}"
    fi

    if grep '** ERROR: Environment Creation Failed' "${REPORT_DIR}/log/${PROJECT_NAME}.log" >/dev/null
    then
        echo "[error] (${PROJECT_NAME}) Environment Creation Failed"
        exit "${FAIL}"
    fi

    if grep '** ERROR:' "${REPORT_DIR}/log/${PROJECT_NAME}.log" >/dev/null
    then
        echo "[error] (${PROJECT_NAME}) some error found"
        exit "${FAIL}"
    fi
}

function collect_error_report(){
    for environment in `ls $PROJECT_PATH/$PROJECT_NAME/environment/`; do
        run_vcast ${VECTORCAST_DIR}/manage -p ${VCM_FILE} -e ${environment} --clicast-args Reports Custom FULL ${REPORT_DIR}/failed_report/${environment}.html
    done
}

function check_testcase(){
    # 检查vcast的testcase是否都执行成功

    $VECTORCAST_DIR/manage --python-script=${TOP_DIR}/vendor/hisi/llt/ci/script/vcast/vcast_check.py project_path=${VCM_FILE} check=testcase
    local ret=$?
    if [[ "${ret}" -ne 0 ]];then
        echo "[error] testcase check failed!"
        return ${FAIL}
    fi
    return ${SUCC}
}

load_env

prepare

if ( check_vcast_cfg );then
    echo -e "${PROJECT_NAME} \t\t\t check_vcast_cfg success"
else
    echo -e "${PROJECT_NAME} \t\t\t check_vcast_cfg failed"
    exit "${FAIL}"
fi

vcast_process
if ( check_testcase );then
    echo -e "${PROJECT_NAME} \t\t\t testcase check success"
else
    echo -e "${PROJECT_NAME} \t\t\t testcase check failed"
    # 有用例不通过，这里生成详细报告主要是为了方便开发定位报错原因
    collect_error_report
    exit "${FAIL}"
fi

mkdir -p ${REPORT_DIR}/cobertura/
run_vcast ${VECTORCAST_DIR}/manage --python-script=${TOP_DIR}/vendor/hisi/llt/ci/script/vcast/vcast_cobertura.py \
    project_path=${VCM_FILE} \
    base_dir=${TOP_DIR} \
    output=${REPORT_DIR}/cobertura/${PROJECT_NAME}.xml

if [ "$DAILYBUILDCAPITAL" == 'true' ];then
    $VECTORCAST_DIR/manage --python-script=/home/works/tools/ias_build-hisi_released_20220224/scripts/utest_dashboard_stat_scrpts/GetVectorResScript.py project_path=${VCM_FILE} tengwu_workspace="Florence V100R002C20" oem_project="Turing" src_dir=${TOP_DIR} git_address="http://gerrit.turing-ci.hisilicon.com:80"
fi

VCAST_RUN_ENDTIME=$(date "+%Y-%m-%d %H:%M:%S")
echo "vcast run task succ."
echo "${VCM_FILE}" >> ${REPORT_DIR}/vcm_projects.txt
echo "${PROJECT_NAME},${VCAST_RUN_STARTTIME},${VCAST_RUN_ENDTIME},${SECONDS}s" >> ${REPORT_DIR}/vcm_consume_time.txt
echo "vcast run starttime (module=${PROJECT_NAME}) is :${VCAST_RUN_STARTTIME}"
echo "vcast run endtime (module=${PROJECT_NAME}) is :${VCAST_RUN_ENDTIME}"
echo "vcast run time consuming (module=${PROJECT_NAME}):$SECONDS seconds"
exit "${SUCC}"
