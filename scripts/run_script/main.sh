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
        --devel)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --upgrade)
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
		print "INFO" "${MSPROF_RUN_NAME} package uninstall success."
		exit 0
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

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
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

function log_init() {
    if [ ! -f "$log_file" ]; then
        touch $log_file
        if [ $? -ne 0 ]; then
            print "ERROR" "touch $log_file permission denied"
            exit 1
        fi
    fi
    chmod 640 $log_file
}

# init log file
log_file=$(get_log_file)
log_init

install_path=$(get_default_install_path)
MSPROF_RUN_NAME="mindstudio-msprof"

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
install_for_all_flag=0

#0, this footnote path;1, path for executing run;2, parents' dir for run package;3, run params
parse_script_args $*
check_args
right=$(get_right)
execute_run
chmod_ini_file
print "INFO" "Mindstudio msprof package install success."
