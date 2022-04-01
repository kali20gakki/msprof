#!/bin/sh
# This script can start/stop/restart/uninstall ada process.
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.

FOLDER_PERM=750
INVISIBLE_DIR_PERM=700
FILE_PERM=640
CERT_PERM=400
ADX_CONF_FILE=ide_daemon.cfg
cd "$(dirname $0)"

checkConfFile(){
    if [ ! -f "./ide_daemon.cfg" ];then
        echo "[ERROR] ide_daemon.cfg was not found, ada will not run without config file."
        return 1
    fi

    if [ ! -f "./ide_daemon_cacert.pem" ];then
        echo "[ERROR] ide_daemon_cacert.pem was not found, ada will not run without certificate"
        return 1
    fi

    if [ ! -f "./ide_daemon_server_cert.pem" ];then
        echo "[ERROR] ide_daemon_server_cert.pem was not found, ada will not run without certificate"
        return 1
    fi

    if [ ! -f "./ide_daemon_server_key.pem" ];then
        echo "[ERROR] ide_daemon_server_key.pem was not found, ada will not run without certificate"
        return 1
    fi

    chmod $FILE_PERM ./ide_daemon.cfg
    chmod $CERT_PERM ./ide_daemon_cacert.pem
    chmod $CERT_PERM ./ide_daemon_server_cert.pem
    chmod $CERT_PERM ./ide_daemon_server_key.pem
    return 0
}

checkFileStatus(){
    checkConfFile
    if [ $? -ne 0 ];then
        return 1
    fi
    if [ ! -f "$PWD/ada" ];then
        echo "[ERROR] ada was not fount in "$PWD", please make sure ada is in same folder with this script."
        return 1
    fi
    # current dir is **/obj64/sample/npu/ide_daemon, so files should in **/lib64/share
    local ldPath="$PWD/../../../../lib64/share"
    export LD_LIBRARY_PATH=$ldPath:$LD_LIBRARY_PATH
    return 0
}

startIdeDaemon(){
    isStarted=$(ps | grep "/ada" | grep -v grep | wc -l)
    if [ $isStarted -eq 1 ];then
        echo "[INFO] ada is already running, donot start again."
        exit 1
    fi
    checkFileStatus
    if [ $? -ne 0 ];then
        echo "[ERROR] failed to start ada."
        exit 1
    fi
    # avoid the path too long to pkill /ada failed
    ./ada >/dev/null
    if [ $? -ne 0 ];then
        echo "[ERROR] failed to start ada."
        exit 1
    fi
    sleep 1
    isStarted=$(ps | grep "/ada" | grep -v grep | wc -l)
    if [ $isStarted -ne 1 ];then
        echo "[ERROR] failed to start ada."
        exit 1
    fi
    echo "[INFO] start ada successfully."
}

stopIdeDaemon(){
    pkill "/ada"
    isStopped=$(ps | grep "/ada" | grep -v grep | wc -l)
    if [ $isStopped -eq 1 ];then
        echo "[ERROR] failed to kill ada, possibly permission denined."
        exit 1
    fi
    echo "[INFO] ada has been stopped."
}

restartIdeDaemon(){
    stopIdeDaemon
    startIdeDaemon
    echo "[INFO] restart ada successfully."
}

uninstallIdeDaemon(){
    pkill "/ada"
    isStopped=$(ps | grep "/ada" | grep -v grep | wc -l)
    if [ $isStopped -eq 1 ];then
        echo "[ERROR] Failed to kill ada, possibly permission denined."
        exit 1
    fi
    echo "[INFO] ada has been stopped."
 
    echo "[INFO] ada uninstall successfully"

}

updateWorkPath(){
    if [ -n "$1" ]
    then
        echo "[INFO] start to process workpath updating request."
        if [ ! -x "$1" ] || [ ! -r "$1" ] || [ ! -w "$1" ]
        then
            echo "[ERROR] Specified new workpath does not exist or is not permitted to write, read or execute."
            exit 1
        fi
        new_workpath=$1
        echo "[INFO] input new workpath is "$new_workpath
        if [ ! -w ./$ADX_CONF_FILE ]
        then
            echo "[ERROR] ide_daemon.cfg does not exist or is not permitted to write."
            exit 1
        fi
        sed -i "s?^WORK_PATH=.*?WORK_PATH=${new_workpath}?g" ./$ADX_CONF_FILE
        if [ $? -ne 0 ];then
            echo "[ERROR] failed to update WORK_PATH in "$ADX_CONF_FILE
            exit 1
        fi
        echo "[INFO] WORK_PATH in "$ADX_CONF_FILE" has been successfully updated."
    fi
}

printWarnForWorkPath(){
    if [ -n "$1" ]
    then
        echo "[WARNING] It's not supported to change WORK_PATH with uninstall action, ignore."
    fi
}

case "$1" in
    start)
        updateWorkPath $2
        startIdeDaemon
        ;;
    restart)
        updateWorkPath $2
        restartIdeDaemon
        ;;
    stop)
        updateWorkPath $2
        stopIdeDaemon
        ;;
    uninstall)
        printWarnForWorkPath $2
        uninstallIdeDaemon
        ;;
    *)
        echo "Please specify an action, optionally with a new workpath for start, restart and stop."
        echo "Usage:"
        echo "    sh setup_ide_daemon.sh action (new workpath)"
        echo "    action can be: start | restart | stop | uninstall"
        echo "    new workpath needs to be a readable, writable and executable path for current user."
    ;;
esac

exit 0
