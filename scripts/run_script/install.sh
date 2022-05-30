#!/bin/bash
function get_right() {
	if [ "$install_for_all_flag" = 1 ] || [ "$UID" = "0" ]; then
		echo 555
	else
		echo 550
	fi
}

function get_cann_package_name() {
	local name_list=(${install_path//// })
	echo ${cann_package_name}
	cann_package_name=${name_list[-2]}
	if [ "$cann_package_name" = "ascend-toolkit" ] || [ "$cann_package_name" = "nnrt" ] || [ "$cann_package_name" = "nnae" ]; then
		return
	fi
	print "ERROR" "There is no ascend-toolkit, nnrt or nnae."
	exit 1
}

function implement_install() {
    if [ "${package_arch}" != "$(arch)" ] && [ -d "${install_path}/${package_arch}-linux/hetero-arch-scripts" ]; then
		copy_file ${LIBMSPROFILER_STUB} ${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}
		copy_file ${LIBMSPROFILER} ${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER}
		return
	fi

	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		copy_file ${ANALYSIS} ${install_path}${ANALYSIS_PATH}${ANALYSIS}
		copy_file ${MSPROF} ${install_path}${MSPROF_PATH}${MSPROF}
	fi
	
	if [ "$cann_package_name" != "nnae" ]; then
		copy_file ${LIBMSPROFILER_STUB} ${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}
	fi

	copy_file ${LIBMSPROFILER} ${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER}
}

function copy_file() {
	local filename=${1}
	local target_file=$(readlink -f ${2})
	if [ ! -f "$filename" ] && [ ! -d "$filename" ]; then
		return
	fi

	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		chmod u+w $(dirname ${target_file})
		travFolder ${target_file} u+w
		rm -r ${target_file}
		
		cp -r ${filename} ${target_file}
		travFolder ${target_file} $right
		chmod u-w $(dirname ${target_file})
		
		print "INFO" "$filename is replaced."
		return
	fi
	print "WARNING" "$target_file is non-existent."
}

function travFolder(){
	local file_name=${1}
	local right=${2}
	chmod $right ${file_name}
	
	if [ -f "$file_name" ]; then
		return
	fi
	
	if [ -d "$file_name" ]; then
		file_list=`ls $file_name`
		
		for f in $file_list
		do
			travFolder ${file_name}/${f} ${right}
		done
	fi
}

function chmod_ini_file() {
	local ini_config_dir=${install_path}${ANALYSIS_PATH}${ANALYSIS}"/config"
	if [ -d "$ini_config_dir" ]; then
		if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
			find "${ini_config_dir}" -type f -exec chmod 444 {} \;
		else
			find "${ini_config_dir}" -type f -exec chmod 400 {} \;
		fi
	fi
}

LIBMSPROFILER="libmsprofiler.so"
LIBMSPROFILER_STUB="stub/libmsprofiler.so"
ANALYSIS="analysis"
MSPROF="msprof"

LIBMSPROFILER_PATH="/runtime/lib64/"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_PATH="/tools/profiler/bin/"

source utils.sh

install_path=${1}
package_arch=${2}
install_for_all_flag=${3}

right=$(get_right)
get_cann_package_name
implement_install
chmod_ini_file