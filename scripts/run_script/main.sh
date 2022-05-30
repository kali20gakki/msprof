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
		bash "${install_path}/${MSPROF_RUN_NAME}/script/uninstall.sh"
		print "INFO" "Mindstudio msprof package uninstall success."
		exit 0
	fi

	if [ ${upgrade_flag} = 1 ]; then
		bash "${install_path}/../latest/${MSPROF_RUN_NAME}/script/uninstall.sh"
		print "INFO" "Mindstudio msprof package uninstall success."
	fi

	bash install.sh ${install_path} ${package_arch} ${install_for_all_flag}

function get_default_install_path() {
	if [ "$UID" = "0" ]; then
		echo "/usr/local/Ascend/ascend-toolkit/latest"
	else
		echo "${HOME}/Ascend/ascend-toolkit/latest"
	fi
}

function store_uninstall_script() {
	local install_right=500

	if [ -f "${install_path}/${MSPROF_RUN_NAME}/script/uninstall.sh" ]; then
		return
	fi

	mkdir -p "${install_path}/${MSPROF_RUN_NAME}/"
	mkdir -p "${install_path}/${MSPROF_RUN_NAME}/script/"
	cp "uninstall.sh" "${install_path}/${MSPROF_RUN_NAME}/script/"

	chmod ${install_right} "${install_path}/${MSPROF_RUN_NAME}/script/uninstall.sh"
	chmod ${install_right} "${install_path}/${MSPROF_RUN_NAME}/script/"
	chmod ${install_right} "${install_path}/${MSPROF_RUN_NAME}/"
}

function set_latest() {
	local latest_path=${install_path}/../latest/
	remove_latest_link ${latest_path}
	add_latest_link ${latest_path}
}

function remove_latest_link() {
	local latest_path=$1
    if [ -L "${latest_path}/${MSPROF_RUN_NAME}" ]; then
        rm_file_safe ${latest_path}/${MSPROF_RUN_NAME}
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
    ln -sf ../${VERSION}/${MSPROF_RUN_NAME} ${latest_path}/${MSPROF_RUN_NAME}
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
    sed -i "/^exit /i uninstall_package \"${MSPROF_RUN_NAME}/script\"" "${install_path}/cann_uninstall.sh"
    chmod u-w ${install_path}/cann_uninstall.sh
}

# use utils function and constant
source utils.sh
install_path=$(get_default_install_path)

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
execute_run
set_latest
regist_uninstall
print "INFO" "${MSPROF_RUN_NAME} package install success."
