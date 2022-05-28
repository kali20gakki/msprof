#!/bin/bash
package_arch=x86_64
VERSION=5.1.T200
function parse_script_args() {
    while true; do
		if [ "$3" = "" ]; then
			break
		fi
        case "$3" in
        --install-path=*)
			let "install_path_num+=1"
			install_path=${3#--install-path=}
			shift
			continue
            ;;
        --full)
            let "install_args_num+=1"
            shift
			continue
            ;;
        --quiet)
            shift
			continue
            ;;
        --nox11)
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
	if [ "$install_path_num" = "1" ] && [ "$install_args_num" = "1" ]; then
        return
	fi

    print "ERROR" "Input option is invalid. Please try --help."
    exit 1
}

function execute_backup() {
    bash backup.sh ${install_path}/${VERSION}
}

function execute_install() {
    # run package name and parent dir for main are no used, so they are replaced by _
    bash main.sh _ _  --full --install-path=${install_path}
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

MSPROF_RUN_NAME="mindstudio-msprof"

# the params for checking
install_args_num=0
install_path_num=0

parse_script_args $*
check_args
execute_backup
execute_install
print "INFO" "${MSPROF_RUN_NAME} package install success."
