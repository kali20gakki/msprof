#!/bin/bash
# install constant
install_path=${1}
package_arch=${2}
install_for_all_flag=${3}

function get_right() {
	if [ "$install_for_all_flag" = 1 ] || [ "$UID" = "0" ]; then
		right=${root_right}
	else
		right=${user_right}
	fi
}

function get_cann_package_name() {
	local name_list=(${install_path//// })
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
		local parent_dir=$(dirname ${target_file})
		local parent_right=$(stat -c '%a' ${parent_dir})

		chmod u+w ${parent_dir}
		chmod -R u+w ${target_file}
		rm -r ${target_file}
		
		cp -r ${filename} ${target_file}
		chmod -R ${right} ${target_file}
		chmod ${parent_right} ${parent_dir}
		
		print "INFO" "$filename is replaced."
		return
	fi
	print "WARNING" "target $filename is non-existent."
}

function chmod_ini_file() {
	local ini_config_dir=${install_path}${ANALYSIS_PATH}${ANALYSIS}"/config"
	if [ -d "$ini_config_dir" ]; then
		if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
			find "${ini_config_dir}" -type f -exec chmod ${root_ini_right} {} \;
		else
			find "${ini_config_dir}" -type f -exec chmod ${user_ini_right} {} \;
		fi
	fi
}

function set_libmsprofiler_right() {
	libmsprofiler_right=${user_libmsprofiler_right}
	if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
		libmsprofiler_right=${root_libmsprofiler_right}
	fi
}

function chmod_libmsprofiler() {
	if [ -f "${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}"
	fi

	if [ -f "${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${package_arch}-linux/hetero-arch-scripts${LIBMSPROFILER_PATH}${LIBMSPROFILER}"
	fi

	if [ -f "${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}" ]; then
		chmod ${libmsprofiler_right} "${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB}"
	fi

	if [ -f "${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER}" ]; then
		chmod ${libmsprofiler_right} "${install_path}${LIBMSPROFILER_PATH}${LIBMSPROFILER}"
	fi
}

source utils.sh

right=${user_right}
get_right
get_cann_package_name
implement_install
chmod_ini_file
set_libmsprofiler_right
chmod_libmsprofiler