#!/bin/bash
# get real path of parents' dir of this file
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

# store product
TEMP_OUTPUT=${TOP_DIR}/build/output
MSPROF_TEMP_DIR=${TOP_DIR}/build/msprof_tmp
COMMON_DIR="common_script"
mkdir ${MSPROF_TEMP_DIR}
mkdir ${MSPROF_TEMP_DIR}/${COMMON_DIR}

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
MAIN_SCRIPT=main.sh

# script for spc
MAIN_SPC=main_spc.sh
FILTER_PARAM_SCRIPT_SPC=${RUN_SCRIPT_DIR}/help_spc.sh
BACKUP=backup.sh
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK=rollback_spc.sh
UNINSTALL=uninstall_spc.sh
COMMON_ROLLBACK=rollback.sh
COMMON_UNINSTALL=uninstall.sh

MSPROF_RUN_NAME="mindstudio-msprof"
version="unknwon"
package_type="unknwon"

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
        Patch)
			package_type="Patch"
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
	
	if [ "${package_type}" = "Patch" ];
		then
			cp ${TEMP_OUTPUT}/lib/libmsprofiler.so ${temp_dir}

			copy_script ${MAIN_SPC} ${temp_dir}
			copy_script ${BACKUP} ${temp_dir}
			copy_script ${ROLLBACK_PRECHECK} ${temp_dir}
			copy_script ${ROLLBACK} ${temp_dir}
			copy_script ${UNINSTALL} ${temp_dir}
			copy_script ${COMMON_DIR}/${COMMON_ROLLBACK} ${temp_dir}
			copy_script ${COMMON_DIR}/${COMMON_UNINSTALL} ${temp_dir}

		else
			cp ${TEMP_OUTPUT}/lib/libmsprofiler.so ${temp_dir}
			cp -r ${TEMP_OUTPUT}/stub ${temp_dir}
			cp ${TEMP_OUTPUT}/bin/msprof ${temp_dir}
			cp -r ${TOP_DIR}/analysis ${temp_dir}
	fi

	copy_script ${MAIN_SCRIPT} ${temp_dir}
}

# copy script
function copy_script() {
	local script_name=${1}
	local temp_dir=${2}

	cp ${RUN_SCRIPT_DIR}/${script_name} ${temp_dir}/${script_name}
	chmod 500 ${temp_dir}/${script_name}
}

function version() {
    local _path="${TOP_DIR}/../manifest/dependency/config.ini"
    local _version=$(grep "^version=" "${_path}" | cut -d"=" -f2)
    echo "${_version}"
}

function get_package_name() {
    local _product="Ascend"
    local _name=${MSPROF_RUN_NAME}

    local _version=$(echo $(version) | cut -d '.' -f 1,2,3)
    local _os_arch=$(arch)
    echo "${_product}-${_name}_${_version}_linux-${_os_arch}.run"
}

function create_run_package() {
	local _run_name=${1}
	local _temp_dir=${2}
	local _main_script=${3}
	local _filer_param=${4}
	local _package_name=$(get_package_name)
	
	${CREATE_RUN_SCRIPT} \
	--header ${CONTROL_PARAM_SCRIPT} \
	--help-header ${_filer_param} --pigz \
	--complevel 4 \
	--nomd5 \
	--sha256 \
	--chown \
	${_temp_dir} \
	${OUTPUT_DIR}/${_package_name} \
	${_run_name} \
	./${_main_script}
}

function sed_param() {
	local _main_script=${1}
	sed -i "2i VERSION=$version" "${RUN_SCRIPT_DIR}/${_main_script}"
	sed -i "2i package_arch=$(arch)" "${RUN_SCRIPT_DIR}/${_main_script}"
}

function delete_sed_param() {
	local _main_script=${1}
	sed -i "2d" "${RUN_SCRIPT_DIR}/${_main_script}"
	sed -i "2d" "${RUN_SCRIPT_DIR}/${_main_script}"
}

parse_script_args $*

sed_param ${MAIN_SCRIPT}
sed_param ${MAIN_SPC}

create_temp_dir ${MSPROF_TEMP_DIR}
if [ "${package_type}" = "Patch" ];
	then
		create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR} ${MAIN_SPC} ${FILTER_PARAM_SCRIPT_SPC}
	else
		create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR} ${MAIN_SCRIPT} ${FILTER_PARAM_SCRIPT}
fi

delete_sed_param ${MAIN_SCRIPT}
delete_sed_param ${MAIN_SPC}
