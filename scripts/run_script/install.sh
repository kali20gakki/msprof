#!/bin/bash
# install constant
install_path=${1}
package_arch=${2}
install_for_all_flag=${3}
pylocal=y

function get_right() {
	if [ "$install_for_all_flag" = 1 ] || [ "$UID" = "0" ]; then
		right=${root_right}
	else
		right=${user_right}
	fi
}

function install_whl_package() {
    local _pylocal=$1
    local _package=$2
    local _pythonlocalpath=$3

    print "INFO" "Start to begin install ${_package}."
    if [ ! -f "${_package}" ]; then
        print "ERROR" "Install whl The ${_package} does not exist."
        return 1
    fi
    if [ "-${_pylocal}" = "-y" ]; then
        pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" -t "${_pythonlocalpath}" > /dev/null 2>&1
    else
        if [ "$(id -u)" -ne 0 ]; then
            pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" --user > /dev/null 2>&1
        else
            pip3 install --upgrade --no-deps --force-reinstall --disable-pip-version-check "${_package}" > /dev/null 2>&1
        fi
    fi
    if [ $? -ne 0 ]; then
        print "ERROR" "Install ${_package} failed."
        return 1
    fi
    print "INFO" "Install ${_package} success."
    return 0
}

function implement_install() {
	# 1. install collector
	# msprof bin
	copy_file ${MSPROF} ${install_path}/${MSPROF_PATH}/${MSPROF}
	# libmsprofiler.so
	copy_file ${LIB_MS_PROFILER} ${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}
	# libprofapi.so
	copy_file ${LIB_PROF_API} ${install_path}/${arch_name}/lib64/${LIB_PROF_API}
	# libprofimpl.so
	copy_file ${LIB_PROF_IMPL} ${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}
	# acl_prof.h
	copy_file ${ACL_PROF_H} ${install_path}/${arch_name}/include/acl/${ACL_PROF_H}
	# ge_prof.h
	copy_file ${GE_PROF_H} ${install_path}/${arch_name}/include/ge/${GE_PROF_H}

	# 2. install analyse
	copy_file ${ANALYSIS} ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    msprof_analyse_whl=${install_path}/${ANALYSIS_PATH}/${MSPROF_ANALYSIS_WHL}
    copy_file ${MSPROF_ANALYSIS_WHL} $msprof_analyse_whl
    if [ -f "${MSPROF_ANALYSIS_WHL}" ]; then
        install_whl_package $pylocal ${MSPROF_ANALYSIS_WHL} ${install_path}/${ANALYSIS_PATH}
    fi
    if [ $? -ne 0 ]; then
        print "ERROR" "Install msprof analysis whl failed."
        return 1
    fi
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

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_MS_PROFILER}"
	fi

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_PROF_API}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_PROF_API}"
	fi

	if [ -f "${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}" ]; then
		chmod ${libmsprofiler_right} "${install_path}/${arch_name}/lib64/${LIB_PROF_IMPL}"
	fi
}

source utils.sh

right=${user_right}
arch_name="${package_arch}-linux"
get_right
implement_install
chmod_ini_file
set_libmsprofiler_right
chmod_libmsprofiler