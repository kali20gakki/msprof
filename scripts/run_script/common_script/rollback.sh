#!/bin/sh

CURRENT_DIR=$(dirname $(readlink -f $0)) # 脚本目录

for dir in $(ls -d ${CURRENT_DIR}/*/); do
    if [ -f "${dir}/rollback_precheck.sh" ]; then
        "${dir}/rollback_precheck.sh"
        ret=$?
        if [ ${ret} -ne 0 ]; then
            echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [ERROR]: ${dir}/rollback_precheck.sh fialed !"
            exit ${ret}
        fi
    fi
done

for dir in $(ls -d ${CURRENT_DIR}/*/); do
    if [ -f "${dir}/rollback.sh" ]; then
        "${dir}/rollback.sh"
        ret=$?
        if [ ${ret} -ne 0 ]; then
            echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [ERROR]: ${dir}/rollback.sh failed !"
            exit ${ret}
        fi
    fi
done

"${CURRENT_DIR}/uninstall.sh"
ret=$?
if [ ${ret} -ne 0 ]; then
    echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [WARNING]: ${CURRENT_DIR}/uninstall.sh failed !"
fi

echo "[All] [$(date +"%Y-%m-%d %H:%M:%S")] [INFO]: patch rolled back successfully !"
exit 0
