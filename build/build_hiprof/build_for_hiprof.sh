#!/bin/bash
# This script is used to build the profiling command line tool package.
# Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
set -e
CURRENT_DIR="$(dirname $(readlink -e $0))"
MSVP_ROOT_DIR="${CURRENT_DIR}/../../"
UNUSED_FILES=("third_party" "patch")

function del_unused_files() {
    for tgt in "${UNUSED_FILES[@]}"
    do
        if [ -d "${MSVP_ROOT_DIR}/${tgt}" ]; then
            rm -rf "${MSVP_ROOT_DIR}/${tgt}"
        elif [ -f "${MSVP_ROOT_DIR}/${tgt}" ]; then
            rm -f "${MSVP_ROOT_DIR}/${tgt}"
        fi
    done
}

function change_package_auth() {
    echo "start change package filemode"
    find "${MSVP_ROOT_DIR}" -name "*.sh" | xargs chmod 500
    find "${MSVP_ROOT_DIR}" -name "*.py" | xargs chmod 600
    find "${MSVP_ROOT_DIR}" -type d -exec chmod 700 {} \;
    echo "change package filemode finished"
}

function build_main() {
    del_unused_files
    change_package_auth
}

build_main "$@"
