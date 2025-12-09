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

    print ${LEVEL_INFO} "Start to install ${_package}."
    if [ ! -f "${_package}" ]; then
        print ${LEVEL_ERROR} "Install whl The ${_package} does not exist."
        return 1
    fi
    if [ "-${_pylocal}" = "-y" ]; then
        pip3 install --upgrade --no-deps --force-reinstall "${_package}" -t "${_pythonlocalpath}" > /dev/null 2>&1
    else
        if [ "$(id -u)" -ne 0 ]; then
            pip3 install --upgrade --no-deps --force-reinstall "${_package}" --user > /dev/null 2>&1
        else
            pip3 install --upgrade --no-deps --force-reinstall "${_package}" > /dev/null 2>&1
        fi
    fi
    if [ $? -ne 0 ]; then
        print ${LEVEL_ERROR} "Install ${_package} failed."
        return 1
    fi
    print ${LEVEL_INFO} "Install ${_package} success."
    return 0
}

function implement_install() {
    copy_file ${LIBMSPTI} ${install_path}/${LIBMSPTI_PATH}/lib64/${LIBMSPTI}
    mspti_whl=${install_path}/${LIBMSPTI_PATH}/python/${MSPTI_WHL}
    copy_file ${MSPTI_WHL} $mspti_whl
    install_whl_package $pylocal $mspti_whl ${install_path}/python/site-packages
    if [ $? -ne 0 ]; then
        print $LEVEL_ERROR "Install mspti whl failed."
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

        print $LEVEL_INFO "$filename is replaced."
        return
    fi
    print $LEVEL_WARNING "Target $filename is non-existent."
}

function set_libmspti_right() {
    libmspti_right=${user_libmspti_right}
    if [ "$install_for_all_flag" = "1" ] || [ "$UID" = "0" ]; then
        libmspti_right=${root_libmspti_right}
    fi
}

function chmod_libmspti() {
    if [ -f "${install_path}/${LIBMSPTI_PATH}/lib64/${LIBMSPTI}" ]; then
        chmod ${libmspti_right} "${install_path}/${LIBMSPTI_PATH}/lib64/${LIBMSPTI}"
    fi
}

source utils.sh

right=${user_right}
arch_name="${package_arch}-linux"
get_right
implement_install
set_libmspti_right
chmod_libmspti