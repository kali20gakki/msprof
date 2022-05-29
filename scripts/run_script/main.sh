#!/bin/bash
function parse_script_args() {
    while true; do
		if [ "$3" = "" ]; then
			break
		fi
        case "$3" in
        --install-path=*)
			let "install_path_num+=1"
			install_path=${3#--install-path=}/${VERSION}
			shift
			continue
            ;;
        --uninstall)
            uninstall_flag=1
            shift
			continue
            ;;
        --upgrade)
            upgrade_flag=1
            shift
			continue
            ;;
        --devel)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --quiet)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --run)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --full)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --install-for-all)
			install_for_all_flag=1
            shift
			continue
            ;;
        *)
			print "ERROR" "Input option is invalid. Please try --help."
			exit 1
            ;;
        esac
    done
}

function check_args() {
	if [ ${install_args_num} -ne 0 ] && [ ${uninstall_flag} = 1 ]; then
		print "ERROR" "Input option is invalid. Please try --help."
		exit 1
	fi

	if [ ${install_path_num} -gt 1 ]; then
		print "ERROR" "Do not input --install-path many times. Please try --help."
		exit 1
	fi
}

function execute_run() {
	if [ ${uninstall_flag} = 1 ]; then
		bash "${install_path}/mindstudio-msprof/script/uninstall.sh"
		print "INFO" "Mindstudio msprof package uninstall success."
		exit 0
	fi

	if [ ${upgrade_flag} = 1 ]; then
		bash "${install_path}/../latest/mindstudio-msprof/script/uninstall.sh"
		print "INFO" "Mindstudio msprof package uninstall success."
	fi

	get_cann_package_name
	implement_install
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
	
	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		chmod u+w $(dirname ${target_file})
		chmod -R u+w ${target_file}
		rm -r ${target_file}
		
		cp -r ${filename} ${target_file}
		chmod -R ${right} ${target_file}
		chmod u-w $(dirname ${target_file})
		
		print "INFO" "$filename is replaced."
		return
	fi
	print "WARNING" "$target_file is non-existent."
}

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
    fi
}

function get_right() {
	if [ "$install_for_all_flag" = 1 ] || [ "$UID" = "0" ]; then
		echo 555
	else
		echo 550
	fi
}

function get_default_install_path() {
	if [ "$UID" = "0" ]; then
		echo "/usr/local/Ascend/ascend-toolkit/latest"
	else
		echo "${HOME}/Ascend/ascend-toolkit/latest"
	fi
}

function get_log_file() {
	local log_dir
	if [ "$UID" = "0" ]; then
		log_dir="/var/log/ascend_seclog"
	else
		log_dir="${HOME}/var/log/ascend_seclog"
	fi
	echo "${log_dir}/ascend_install.log"
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

function store_uninstall_script() {
	local install_right=500

	if [ -f "${install_path}/mindstudio-msprof/script/uninstall.sh" ]; then
		return
	fi

	mkdir -p "${install_path}/mindstudio-msprof/"
	mkdir -p "${install_path}/mindstudio-msprof/script/"
	cp "uninstall.sh" "${install_path}/mindstudio-msprof/script/"

	chmod ${install_right} "${install_path}/mindstudio-msprof/script/uninstall.sh"
	chmod ${install_right} "${install_path}/mindstudio-msprof/script/"
	chmod ${install_right} "${install_path}/mindstudio-msprof/"
}

function set_latest() {
	local latest_path=${install_path}/../latest/
	remove_latest_link ${latest_path}
	add_latest_link ${latest_path}
}

function remove_latest_link() {
	local latest_path=$1
    if [ -L "${latest_path}/mindstudio-msprof" ]; then
        rm_file_safe ${latest_path}/mindstudio-msprof
    fi
}

function rm_file_safe() {
    local file_path=$1
    if [ -n "${file_path}" ]; then
        if [ -f "${file_path}" ] || [ -h "${file_path}" ]; then
            rm -f "${file_path}"
            print "INFO" "delete file ${file_path} successfully"
        else
            print "WARNING" "the file is not exist"
        fi
    else
        print "WARNING" "the file path is NULL"
    fi
}

function add_latest_link() {
	local latest_path=$1
    ln -sf ../${VERSION}/mindstudio-msprof ${latest_path}/mindstudio-msprof
}

function regist_uninstall() {
    if [ -f "${install_path}/cann_uninstall.sh" ]; then
        write_cann_uninstall
    else
        cp -af script/cann_uninstall.sh ${install_path}
        write_cann_uninstall
    fi
}

function write_cann_uninstall() {
    chmod 500 ${install_path}/cann_uninstall.sh
    chmod u+w ${install_path}/cann_uninstall.sh
    sed -i "/^exit /i uninstall_package \"mindstudio-msprof/script\"" "${install_path}/cann_uninstall.sh"
    chmod u-w ${install_path}/cann_uninstall.sh
}

log_file=$(get_log_file)
install_path=$(get_default_install_path)

# product
LIBMSPROFILER="libmsprofiler.so"
LIBMSPROFILER_STUB="stub/libmsprofiler.so"
ANALYSIS="analysis"
MSPROF="msprof"

LIBMSPROFILER_PATH="/runtime/lib64/"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_PATH="/tools/profiler/bin/"

# the params for checking
install_args_num=0
install_path_num=0

uninstall_flag=0
upgrade_flag=0
install_for_all_flag=0

#0, this footnote path;1, path for executing run;2, parents' dir for run package;3, run params
parse_script_args $*
check_args
store_uninstall_script
right=$(get_right)
execute_run
chmod_ini_file
set_latest
regist_uninstall
print "INFO" "Mindstudio msprof package install success."
