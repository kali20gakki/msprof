#!/bin/bash
# get real path of parents' dir of this file
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

# store product
TEMP_OUTPUT=${TOP_DIR}/build/output
MSPROF_TEMP_DIR=${TOP_DIR}/build/msprof_tmp
mkdir ${MSPROF_TEMP_DIR}

# makeself is tool for compiling run package
MAKESELF_DIR=${TOP_DIR}/opensource/makeself

# footnote for creating run package
CREATE_RUN_SCRIPT=${MAKESELF_DIR}/makeself.sh

# footnote for controling params
CONTROL_PARAM_SCRIPT=${MAKESELF_DIR}/makeself-header.sh

# store run package
OUTPUT_DIR=${TOP_DIR}/output
mkdir ${OUTPUT_DIR}

RUN_SCRIPT_DIR=${TOP_DIR}/scripts/run_script
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.sh
MAIN_SCRIPT=/main.sh
MSPROF_RUN_NAME="msprof"
version="unknwon"

function parse_script_args() {
    while true; do
		if [ "$1" = "" ]; then
			break
		fi
        case "$1" in
        version_dir=*)
			version=${1#version_dir=}
			shift
			continue
            ;;
        *)
			echo "[ERROR]" "Input option is invalid"
			exit 1
            ;;
        esac
    done
}

# create temp dir for product
function create_temp_dir() {
	local temp_dir=${1}
	
	cp ${TEMP_OUTPUT}/lib/libmsprofiler.so ${temp_dir}
	cp -r ${TEMP_OUTPUT}/stub ${temp_dir}
	cp ${TEMP_OUTPUT}/bin/msprof ${temp_dir}
	cp -r ${TOP_DIR}/analysis ${temp_dir}
	cp "${RUN_SCRIPT_DIR}/common_script/cann_uninstall.sh" ${temp_dir}
	cp "${RUN_SCRIPT_DIR}/uninstall.sh" ${temp_dir}
	
	cp ${RUN_SCRIPT_DIR}${MAIN_SCRIPT} ${temp_dir}
	chmod 500 ${temp_dir}${MAIN_SCRIPT}
}

function version() {
    local _path="${TOP_DIR}/../manifest/dependency/config.ini"
    local _version=$(grep "^version=" "${_path}" | cut -d"=" -f2)
    echo "${_version}"
}

function get_package_name() {
    local _product="Ascend"
    local _name="mindstudio-msprof"

    local _version=$(echo $(version) | cut -d '.' -f 1,2,3)
    local _os_arch=$(arch)
    echo "${_product}-${_name}_${_version}_linux-${_os_arch}.run"
}

function create_run_package(){
	local _run_name=${1}
	local _temp_dir=${2}
	local _package_name=$(get_package_name)
	
	${CREATE_RUN_SCRIPT} \
	--header ${CONTROL_PARAM_SCRIPT} \
	--help-header ${FILTER_PARAM_SCRIPT} --pigz \
	--complevel 4 \
	--nomd5 \
	--sha256 \
	--chown \
	${_temp_dir} \
	${OUTPUT_DIR}/${_package_name} \
	${_run_name} \
	.${MAIN_SCRIPT}
}

parse_script_args $*
sed -i "2i VERSION=$version" ${RUN_SCRIPT_DIR}${MAIN_SCRIPT}
sed -i "2i package_arch=$(arch)" "${RUN_SCRIPT_DIR}/${MAIN_SCRIPT}"
create_temp_dir ${MSPROF_TEMP_DIR}
create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR}
sed -i '2d' ${RUN_SCRIPT_DIR}${MAIN_SCRIPT}
sed -i '2d' "${RUN_SCRIPT_DIR}/${MAIN_SCRIPT}"
