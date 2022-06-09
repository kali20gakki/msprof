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
FILTER_PARAM_SCRIPT=${RUN_SCRIPT_DIR}/help.conf
MAIN_SCRIPT=main.sh
INSTALL_SCRIPT=install.sh
UTILS_SCRIPT=utils.sh
CANN_UNINSTALL=cann_uninstall.sh
UNINSTALL=uninstall.sh

# script for spc
MAIN_SPC=main_spc.sh
FILTER_PARAM_SCRIPT_SPC=${RUN_SCRIPT_DIR}/help_spc.conf
BACKUP=backup.sh
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK_SPC=rollback_spc.sh
UNINSTALL_SPC=uninstall_spc.sh
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
			# if we want to change product, we also need to change rollback_precheck
			cp ${TEMP_OUTPUT}/lib/libmsprofiler.so ${temp_dir}
			cp -r ${TEMP_OUTPUT}/stub ${temp_dir}
			cp ${TEMP_OUTPUT}/bin/msprof ${temp_dir}
			cp -r ${TOP_DIR}/analysis ${temp_dir}

			copy_script ${MAIN_SPC} ${temp_dir}
			copy_script ${BACKUP} ${temp_dir}
			copy_script ${ROLLBACK_PRECHECK} ${temp_dir}
			copy_script ${ROLLBACK_SPC} ${temp_dir}
			copy_script ${UNINSTALL_SPC} ${temp_dir}
			copy_script ${COMMON_DIR}/${COMMON_ROLLBACK} ${temp_dir}
			copy_script ${COMMON_DIR}/${COMMON_UNINSTALL} ${temp_dir}

		else
			copy_script ${MAIN_SCRIPT} ${temp_dir}
			copy_script ${COMMON_DIR}/${COMMON_UNINSTALL} ${temp_dir}
			copy_script ${UNINSTALL} ${temp_dir}
			cp ${TEMP_OUTPUT}/lib/libmsprofiler.so ${temp_dir}
			cp -r ${TEMP_OUTPUT}/stub ${temp_dir}
			cp ${TEMP_OUTPUT}/bin/msprof ${temp_dir}
			cp -r ${TOP_DIR}/analysis ${temp_dir}
	fi
	copy_script ${INSTALL_SCRIPT} ${temp_dir}
	copy_script ${UTILS_SCRIPT} ${temp_dir}
}

# copy script
function copy_script() {
	local script_name=${1}
	local temp_dir=${2}

	cp ${RUN_SCRIPT_DIR}/${script_name} ${temp_dir}/${script_name}
	chmod 500 ${temp_dir}/${script_name}
}

function version() {
    local path="${TOP_DIR}/../manifest/dependency/config.ini"
    local version=$(grep "^version=" "${path}" | cut -d"=" -f2)
    echo "${version}"
}

function get_package_name() {
    local product="Ascend"
    local name=${MSPROF_RUN_NAME}

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
	
	${CREATE_RUN_SCRIPT} \
	--header ${CONTROL_PARAM_SCRIPT} \
	--help-header ${filer_param} --pigz \
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

function sed_main_param() {
	local main_script=${1}
	local filer=${2}
	sed_param ${main_script}
	create_temp_dir ${MSPROF_TEMP_DIR}
	create_run_package ${MSPROF_RUN_NAME} ${MSPROF_TEMP_DIR} ${main_script} ${filer}
	delete_sed_param ${main_script}
}

parse_script_args $*

if [ "${package_type}" = "Patch" ];
	then
		sed_main_param ${MAIN_SPC} ${FILTER_PARAM_SCRIPT_SPC}
	else
		sed_main_param ${MAIN_SCRIPT} ${FILTER_PARAM_SCRIPT}
fi
