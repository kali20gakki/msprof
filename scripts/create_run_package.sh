#!/bin/bash
# get real path of parents' dir of this file
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

# store product
MSPROF_TEMP_DIR=${TOP_DIR}/build/msprof_tmp
rm -rf ${MSPROF_TEMP_DIR}
mkdir -p ${MSPROF_TEMP_DIR}

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

MSPROF_RUN_NAME="mindstudio-msprof"
VERSION="none"
BUILD_MODE="analysis"

PKG_LIMIT_SIZE=524288000 # 500M

function parse_script_args() {
    if [ $# -gt 2 ]; then
        echo "[ERROR] Too many arguments. Only one or two arguments are allowed."
        exit 1
    elif [ $# -eq 2 ]; then
        VERSION="$1"
        BUILD_MODE="$2"
    elif [ $# -eq 1 ]; then
        VERSION="$1"
    fi
}

# build python whl
function build_python_whl() {
  cd ${TOP_DIR}/build/analysis/build
  python3  ${TOP_DIR}/build/setup.py bdist_wheel --python-tag=py3 --py-limited-api=cp37
  cd - > /dev/null
}

function create_collector_temp_dir() {
    local temp_dir=${1}
    local runtime_dir=${2}
    local oam_tools_dir=${3}

    # 1. runtime
    cp ${runtime_dir}/runtime/lib/libmsprofiler.so ${temp_dir}
    cp ${runtime_dir}/runtime/lib/libprofapi.so ${temp_dir}
    cp ${runtime_dir}/runtime/lib/libprofimpl.so ${temp_dir}
    cp ${runtime_dir}/runtime/include/external/acl/acl_prof.h ${temp_dir}
    cp ${runtime_dir}/runtime/include/external/ge/ge_prof.h ${temp_dir}
    # 2. oam_tools
    cp ${oam_tools_dir}/oam_tools/profiler/bin/msprof ${temp_dir}
}

function create_analysis_temp_dir() {
    local temp_dir=${1}

    cp ${TOP_DIR}/build/analysis/dist/msprof-0.0.1-py3-none-any.whl ${temp_dir}
    cp -r ${TOP_DIR}/analysis ${temp_dir}
    rm -rf ${temp_dir}/analysis/csrc
}

# create temp dir for product
function create_temp_dir() {
    local temp_dir=${1}
    local runtime_dir=${TOP_DIR}/build/collector/runtime/build_out/runtime_decompress
    local oam_tools_dir=${TOP_DIR}/build/collector/oam-tools/build_out/oam_tools_decompress

    case "$BUILD_MODE" in
        "all")
            create_collector_temp_dir ${temp_dir} ${runtime_dir} ${oam_tools_dir}
            create_analysis_temp_dir ${temp_dir}
            ;;
        "collector")
            create_collector_temp_dir ${temp_dir} ${runtime_dir} ${oam_tools_dir}
            ;;
        "analysis")
            create_analysis_temp_dir ${temp_dir}
            ;;
        *)
            echo "Invalid BUILD_MODE: $BUILD_MODE"
            exit 1
            ;;
        esac

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

function get_version() {
    # 如果 VERSION 不是 "none"，直接返回 VERSION
    if [ "${VERSION}" != "none" ]; then
        echo "${VERSION}"
        return
    fi

    # 定义配置文件路径
    local path="${TOP_DIR}/../manifest/dependency/config.ini"

    # 检查配置文件是否存在
    if [ ! -f "${path}" ]; then
        echo "none"
        return
    fi

    # 从配置文件中读取 version
    local version=$(grep -m 1 "^version=" "${path}" | cut -d"=" -f2)

    # 检查读取到的 version 是否为空
    if [ -z "${version}" ]; then
        echo "none"
    else
        echo "${version}"
    fi
}

function get_package_name() {
    local product="Ascend"
    local name=${MSPROF_RUN_NAME}

    local version=$(echo $(get_version) | cut -d '.' -f 1,2,3)
    local os_arch=$(arch)
    echo "${product}-${name}_${version}_linux-${os_arch}.run"
}

function create_run_package() {
    local run_name=${1}
    local temp_dir=${2}
    local main_script=${3}
    local filer_param=${4}
    local package_name=$(get_package_name)

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

function check_collector_files() {
    local temp_dir="$1"
    local pkg_limit_size="$2"

    check_package ${temp_dir}/acl_prof.h ${pkg_limit_size}
    check_package ${temp_dir}/ge_prof.h ${pkg_limit_size}
    check_package ${temp_dir}/msprof ${pkg_limit_size}
    check_package ${temp_dir}/libmsprofiler.so ${pkg_limit_size}
    check_package ${temp_dir}/libprofapi.so ${pkg_limit_size}
    check_package ${temp_dir}/libprofimpl.so ${pkg_limit_size}
}

function check_analysis_files() {
    local temp_dir="$1"
    local pkg_limit_size="$2"

    check_package ${temp_dir}/analysis ${pkg_limit_size}
    check_package ${temp_dir}/analysis/lib64/msprof_analysis.so ${pkg_limit_size}
}

function check_file_exist() {
    local temp_dir=${1}

    case "$BUILD_MODE" in
        "all")
            check_collector_files ${temp_dir} ${PKG_LIMIT_SIZE}
            check_analysis_files ${temp_dir} ${PKG_LIMIT_SIZE}
            ;;
        "collector")
            check_collector_files ${temp_dir} ${PKG_LIMIT_SIZE}
            ;;
        "analysis")
            check_analysis_files ${temp_dir} ${PKG_LIMIT_SIZE}
            ;;
        *)
            echo "Invalid BUILD_MODE: $BUILD_MODE"
            exit 1
            ;;
    esac

    check_package ${temp_dir}/${MAIN_SCRIPT}
    check_package ${temp_dir}/${INSTALL_SCRIPT} ${PKG_LIMIT_SIZE}
    check_package ${temp_dir}/${UTILS_SCRIPT} ${PKG_LIMIT_SIZE}
}

function check_package() {
    local _path="$1"
    local _limit_size=$2
    echo "check ${_path} exists"
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
    case "$BUILD_MODE" in
        "all" | "analysis")
            build_python_whl
            ;;
        *)
            # 其他 BUILD_MODE 值的处理
            ;;
    esac
	create_temp_dir ${MSPROF_TEMP_DIR}
	check_file_exist ${MSPROF_TEMP_DIR}
	create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR} ${main_script} ${filer}
	check_package ${OUTPUT_DIR}/$(get_package_name) ${PKG_LIMIT_SIZE}
	delete_sed_param ${main_script}
}

parse_script_args $*
main ${MAIN_SCRIPT} ${FILTER_PARAM_SCRIPT}
