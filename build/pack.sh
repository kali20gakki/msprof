#!/bin/bash
# This script is used to replace msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so&&analysis in CANN with those in Ascend_mindstudio_toolkit,
# and repack CANN components to .run package
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e


CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
OUT_DIR=${TOP_DIR}/..

if [ -e ${CUR_DIR}/tmp ]
then
    rm -rf ${CUR_DIR}/tmp
fi

# binary check
function bep_env_init() {
    source /etc/profile
    local bep_env_config=${CUR_DIR}/bep/bep_env.conf
    local bep_sh=$(which bep_env.sh)
    echo "has bep sh :${bep_sh}"
    if [ ! -d "${SECBEPKIT_HOME}" ] && [ ! -f "$bep_sh" ]; then
        echo "BepKit is not installed, Please install the tool and configure the env var \$SECBEPKIT_HOME"
    else
        source ${SECBEPKIT_HOME}/bep_env.sh -s $bep_env_config
        if [ $? -ne 0 ]; then
            echo "build bep failed!"
            exit 1
        else
            echo "build bep success."
        fi
    fi
}

bep_env_init
MINDSTUDIO_TOOLKIT_NAME=Ascend-mindstudio-toolkit*.run
CANN_TOOLKIT_NAME=CANN-toolkit-*.run
CANN_RUNTIME_NAME=CANN-runtime-*linux*.run
# only find x86 or ARM
for DIR in ${OUT_DIR}/platform/Tuscany/*centos*;
  do
    RM_DIR=${DIR}
    MINDSTUDIO_TOOLKIT_DIR=${RM_DIR}/$MINDSTUDIO_TOOLKIT_NAME
    CANN_TOOLKIT_DIR=${RM_DIR}/$CANN_TOOLKIT_NAME
    CANN_RUNTIME_DIR=${RM_DIR}/$CANN_RUNTIME_NAME

    mkdir ${CUR_DIR}/tmp
    cp $MINDSTUDIO_TOOLKIT_DIR ${CUR_DIR}/tmp
    PACKAGE_NAME_MINDSTUDIO_SUFFIX=$(ls ${CUR_DIR}/tmp)
    mkdir ${CUR_DIR}/tmp/cann_toolkit
    mkdir ${CUR_DIR}/tmp/cann_runtime
    cp $CANN_TOOLKIT_DIR ${CUR_DIR}/tmp/cann_toolkit
    cp $CANN_RUNTIME_DIR ${CUR_DIR}/tmp/cann_runtime
    chmod -R +x ${CUR_DIR}/tmp
    PACKAGE_NAME_TOOLKIT_suffix=$(ls ${CUR_DIR}/tmp/cann_toolkit)
    PACKAGE_NAME_RUNTIME_suffix=$(ls ${CUR_DIR}/tmp/cann_runtime)
    PACKAGE_NAME_TOOLKIT=$(basename $PACKAGE_NAME_TOOLKIT_suffix .run)
    PACKAGE_NAME_RUNTIME=$(basename $PACKAGE_NAME_RUNTIME_suffix .run)

    ${CUR_DIR}/tmp/$MINDSTUDIO_TOOLKIT_NAME --noexec --extract=${CUR_DIR}/tmp/mindstudio
    ${CUR_DIR}/tmp/cann_toolkit/$CANN_TOOLKIT_NAME --noexec --extract=${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}
    ${CUR_DIR}/tmp/cann_runtime/$CANN_RUNTIME_NAME --noexec --extract=${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}

    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/stub/libmsprofiler.so ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/lib64/stub/libmsprofiler.so
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/libmsprofiler.so ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/lib64/libmsprofiler.so
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/msprof ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/bin/msprof
    rm -rf ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/profiler_tool/analysis
    cp -r ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/analysis ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit/tools/profiler/profiler_tool/analysis
    cp -f ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof/acl_prof.h ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime/include/acl/acl_prof.h
    rm -rf ${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit/tools/msprof

    # makeself is tool for compiling run package
    MAKESELF_DIR=${TOP_DIR}/opensource/makeself
    # footnote for creating run package
    CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh
    # footnote for controling params
    CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh
    CANN_TOOLKIT=${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT}/toolkit
    CANN_RUNTIME=${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME}/runtime
    MINDSTUDIO_TOOLKIT=${CUR_DIR}/tmp/mindstudio/mindstudio-toolkit
    COMMENTS=comments

    ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT}\
    --help-header ${CANN_TOOLKIT}/scripts/help.info \
    --pigz \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --nooverwrite \
    --chown \
    --tar-format gnu\
    --tar-extra \
    --numeric-owner \
    ${CUR_DIR}/tmp/cann_toolkit/${PACKAGE_NAME_TOOLKIT} \
    ${CUR_DIR}/tmp/${PACKAGE_NAME_TOOLKIT_suffix} \
    ${COMMENTS} \
    ./toolkit/scripts/install.sh
    mv ${CUR_DIR}/tmp/${PACKAGE_NAME_TOOLKIT_suffix} ${RM_DIR}/$PACKAGE_NAME_TOOLKIT_suffix

     ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT}\
    --help-header ${CANN_RUNTIME}/scripts/help.info \
    --pigz \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --nooverwrite \
    --chown \
    --tar-format gnu\
    --tar-extra \
    --numeric-owner \
    ${CUR_DIR}/tmp/cann_runtime/${PACKAGE_NAME_RUNTIME} \
    ${CUR_DIR}/tmp/${PACKAGE_NAME_RUNTIME_suffix} \
    ${COMMENTS} \
    ./runtime/scripts/install.sh
    mv ${CUR_DIR}/tmp/${PACKAGE_NAME_RUNTIME_suffix} ${RM_DIR}/$PACKAGE_NAME_RUNTIME_suffix

    filelist_csv=${MINDSTUDIO_TOOLKIT}/script/filelist.csv
    file_interval=${MINDSTUDIO_TOOLKIT}/script/file_interval.csv
    awk -F, '$4 !~ /msprof/ || $2 !~ /mkdir/' ${filelist_csv} > ${file_interval}
    awk -F, '$1 !~ /msprof/' ${file_interval} > ${filelist_csv}
    rm -f $file_interval

    ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT}\
    --help-header ${MINDSTUDIO_TOOLKIT}/script/help.info \
    --pigz \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --nooverwrite \
    --chown \
    --tar-format gnu\
    --tar-extra \
    --numeric-owner \
    ${CUR_DIR}/tmp/mindstudio \
    ${CUR_DIR}/tmp/${PACKAGE_NAME_MINDSTUDIO_SUFFIX} \
    ${COMMENTS} \
    ./mindstudio-toolkit/script/install.sh
    mv ${CUR_DIR}/tmp/${PACKAGE_NAME_MINDSTUDIO_SUFFIX} ${RM_DIR}/${PACKAGE_NAME_MINDSTUDIO_SUFFIX}

    rm -rf ${CUR_DIR}/tmp
  done