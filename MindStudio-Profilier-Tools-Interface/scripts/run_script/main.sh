#!/bin/bash
# the params for checking
install_args_num=0
install_path_num=0

install_for_all_flag=0

function parse_script_args() {
    while true; do
        if [ "$3" = "" ]; then
            break
        fi
        case "$3" in
        --install-path=*)
            let "install_path_num+=1"
            install_path=${3#--install-path=}/ascend-toolkit/latest
            check_path ${install_path}
            install_path=$(readlink -f ${install_path})
            check_path ${install_path}
            shift
            continue
            ;;
        --quiet)
            shift
            continue
            ;;
        --install)
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
            print $LEVEL_ERROR "Input option is invalid. Please try --help."
            exit 1
            ;;
        esac
    done
}

function check_args() {
    if [ ${install_args_num} -eq 0 ]; then
        print $LEVEL_ERROR "Input option is invalid. Please try --help."
        exit 1
    fi

    if [ ${install_path_num} -gt 1 ]; then
        print $LEVEL_ERROR "Do not input --install-path many times. Please try --help."
        exit 1
    fi
}

function execute_run() {
    bash install.sh ${install_path} ${package_arch} ${install_for_all_flag}
}

function get_default_install_path() {
    if [ "$UID" = "0" ]; then
        echo "/usr/local/Ascend/ascend-toolkit/latest"
    else
        echo "${HOME}/Ascend/ascend-toolkit/latest"
    fi
}

# use utils function and constant
source utils.sh
install_path=$(get_default_install_path)
#0, this footnote path;1, path for executing run;2, parents' dir for run package;3, run params
parse_script_args $*
check_args
execute_run
print $LEVEL_INFO "${MSPTI_RUN_NAME} package install success."
