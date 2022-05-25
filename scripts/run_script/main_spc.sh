#!/bin/bash
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
    bash backup.sh ${install_path}/${VERSION} ${MSPROF_RUN_NAME}
}

function execute_install() {
    # run package name and parent dir for main are no used, so they are replaced by _
    bash main.sh _ _  --full --install-path=${install_path}
}

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
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

log_file=$(get_log_file)

# the params for checking
install_args_num=0
install_path_num=0

parse_script_args $*
check_args
execute_backup
execute_install
print "INFO" "Mindstudio msprof package install success."