#!/bin/bash
# get real path of parents' dir of this file
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
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=$(readlink -f "${CUR_DIR}/..")

# store product
PREFIX_DIR=${TOP_DIR}/prefix
MSPTI_TEMP_DIR=${TOP_DIR}/../output/mspti

# makeself is tool for compiling run package
MAKESELF_DIR=${TOP_DIR}/opensource/makeself

# footnote for creating run package
CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh

# footnote for controling params
CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh

# store run package
OUTPUT_DIR=${TOP_DIR}/output
mkdir -p "${OUTPUT_DIR}"

RUN_SCRIPT_DIR=${TOP_DIR}/scripts/run_script
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.conf
MAIN_SCRIPT=main.sh
INSTALL_SCRIPT=install.sh
UTILS_SCRIPT=utils.sh

MSPTI_RUN_NAME="mindstudio-mspti"
VERSION="none"

PKG_LIMIT_SIZE=524288000 # 500M

function parse_script_args() {
    if [ $# -gt 1 ]; then
        echo "[ERROR] Too many arguments. Only one argument (version) is allowed."
        exit 1
    elif [ $# -eq 1 ]; then
        VERSION="$1"
    fi
}

# build python whl
function build_python_whl() {
    cd ${TOP_DIR}/build
    python3  ${TOP_DIR}/scripts/setup.py bdist_wheel --python-tag=py3 --py-limited-api=cp37
    rm -rf ${TOP_DIR}/mspti/mspti.egg-info
    cd - > /dev/null
}

# create temp dir for product
function create_temp_dir() {
    local temp_dir=${1}
    if [ -d "${temp_dir}" ]; then
        rm -rf "${temp_dir}"
    fi
    mkdir -p ${temp_dir}
    local mspti_dir="${PREFIX_DIR}/mspti"

    cp ${mspti_dir}/libmspti.so ${temp_dir}
    cp ${TOP_DIR}/build/dist/mspti-0.0.1-py3-none-any.whl ${temp_dir}
    cp ${TOP_DIR}/csrc/include/mspti.h ${temp_dir}
    cp ${TOP_DIR}/csrc/include/mspti_activity.h ${temp_dir}
    cp ${TOP_DIR}/csrc/include/mspti_callback.h ${temp_dir}
    cp ${TOP_DIR}/csrc/include/mspti_cbid.h ${temp_dir}
    cp ${TOP_DIR}/csrc/include/mspti_result.h ${temp_dir}
    cp -r ${TOP_DIR}/samples ${temp_dir}
    copy_script ${MAIN_SCRIPT} ${temp_dir}
    copy_script ${INSTALL_SCRIPT} ${temp_dir}
    copy_script ${UTILS_SCRIPT} ${temp_dir}
}

# copy script
function copy_script() {
    local script_name=${1}
    local temp_dir=${2}

    if [ -f "${temp_dir}/${script_name}" ]; then
        rm -f "${temp_dir}/${script_name}"
    fi

    cp ${RUN_SCRIPT_DIR}/${script_name} ${temp_dir}/${script_name}
    chmod 500 "${temp_dir}/${script_name}"
}

function version() {
    local path="${TOP_DIR}/../manifest/dependency/config.ini"
    if [[ "$VERSION" != "none" ]]; then
        echo "${VERSION}"
    elif [ -f "${path}" ]; then
        local version=$(grep "^version=" "${path}" | cut -d"=" -f2)
        echo "${version}"
    elif [ -f "${TOP_DIR}/version.txt" ]; then
        local version=$(grep "^version=" "${TOP_DIR}/version.txt" | cut -d"=" -f2)
        echo "${version}"
    else
        echo "${VERSION}"
    fi
}

function get_package_name() {
    local product="Ascend"
    local name=${MSPTI_RUN_NAME}

    local version=$(echo $(version) | cut -d '.' -f 1,2,3)
    local os_arch=$(arch)
    echo "${product}-${name}_${version}_linux-${os_arch}.run"
}

function create_run_package() {
    local run_name=${1}
    local temp_dir=${2}
    local main_script=${3}
    local filer_param=${4}
    local package_name=$(get_package_name)
    if [ -f "${OUTPUT_DIR}/${package_name}" ]; then
        rm -rf "${OUTPUT_DIR}/${package_name}"
    fi
    ${CREATE_RUN_SCRIPT} \
    --header ${CONTROL_PARAM_SCRIPT} \
    --help-header ${filer_param} \
    --pigz \
    --tar-quietly \
    --complevel 4 \
    --nomd5 \
    --sha256 \
    --chown \
    ${temp_dir} \
    ${OUTPUT_DIR}/${package_name} \
    ${run_name} \
    ./${main_script}
}

function sed_param() {
    local main_script=${1}
    sed -i "2i VERSION=$version" "${RUN_SCRIPT_DIR}/${main_script}"
    sed -i "2i package_arch=$(arch)" "${RUN_SCRIPT_DIR}/${main_script}"
}

function delete_sed_param() {
    local main_script=${1}
    sed -i "2d" "${RUN_SCRIPT_DIR}/${main_script}"
    sed -i "2d" "${RUN_SCRIPT_DIR}/${main_script}"
}

function check_file_exist() {
    local temp_dir=${1}
    check_package ${temp_dir}/mspti-0.0.1-py3-none-any.whl ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${INSTALL_SCRIPT} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${UTILS_SCRIPT} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${MAIN_SCRIPT} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${COMMON_DIR}/${COMMON_UNINSTALL} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${UNINSTALL} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/libmspti.so ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/mspti.h ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/mspti_activity.h ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/mspti_callback.h ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/mspti_cbid.h ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/mspti_result.h ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/samples ${PKG_LIMIT_SIZE}
}

function check_package() {
    local _path="$1"
    local _limit_size=$2
    # 检查路径是否存在
    if [ ! -e "${_path}" ]; then
        echo "${_path} does not exist."
        exit 1
    fi

    # 检查路径是否为文件
    if [ -f "${_path}" ]; then
        local _file_size=$(stat -c%s "${_path}")
        # 检查文件大小是否超过限制
        if [ "${_file_size}" -gt "${_limit_size}" ] || [ "${_file_size}" -eq 0 ]; then
            echo "package size exceeds limit:${_limit_size}"
            exit 1
        fi
    fi
}

function main() {
    local main_script=${1}
    local filer=${2}
    sed_param ${main_script}
    build_python_whl
    create_temp_dir ${MSPTI_TEMP_DIR}
    check_file_exist ${MSPTI_TEMP_DIR}
    create_run_package ${MSPTI_RUN_NAME} ${MSPTI_TEMP_DIR} ${main_script} ${filer}
    check_package ${OUTPUT_DIR}/$(get_package_name) ${PKG_LIMIT_SIZE}
    delete_sed_param ${main_script}
}

parse_script_args $*
main ${MAIN_SCRIPT} ${FILTER_PARAM_SCRIPT}
