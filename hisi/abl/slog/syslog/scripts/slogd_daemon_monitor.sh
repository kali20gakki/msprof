#!/bin/bash
# This script restart slogd process by appmond.
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.

LOG_FILE="/usr/slog/slogd_monitor_log.txt"
INSTALL_FILE="/etc/ascend_install.info"
int=1

date > $LOG_FILE

PID_VAR=$(echo "$1" | tr -cd '[0-9]')
if [ $1 -eq $PID_VAR ];then
    echo "Process ID of the input parameter: $PID_VAR" >> $LOG_FILE
else
    echo "Invalid input parameter: $PID_VAR, effective value should be Process Number" >> $LOG_FILE
    exit 1
fi

if [ -f "${INSTALL_FILE}" ];then
    source ${INSTALL_FILE}
    HIAI_DIR=$(Driver_Install_Path_Param)
    PROCESS="${HIAI_DIR}/driver/tools/slogd"
    DATE_FORMATE="+%F %T.%3N"
else
    PROCESS="/var/slogd"
    DATE_FORMATE="+%F %T"
fi

## Determine if the slogd process exists or not
## if  process exists, kill it
if [ $(ps -ef | grep -v grep | grep -c $PROCESS$) -ne 0 ];then
    echo "kill $PROCESS: $PID_VAR" >> $LOG_FILE
    kill -31 "$PID_VAR"
    while(( $int<=10 ))
    do
        if [ $(ps -ef | grep -v grep | grep -c $PROCESS$) -ne 0 ];then
            echo "wait to kill $PROCESS: $PID_VAR" >> $LOG_FILE
            sleep 1
        fi
        let "int++"
    done
fi

## restart slogd process
echo "raise $PROCESS " >> $LOG_FILE
if [ -f "$PROCESS" ]; then
    if [ -f "${INSTALL_FILE}" ];then
        nohup ${HIAI_DIR}/driver/tools/slogd >/dev/null 2>&1 &
    else
        nice -n 20 /var/slogd &
    fi
    int=1
    while(( $int<=5 ))
    do
        date "$DATE_FORMATE" >> $LOG_FILE
        ps -ef | grep "${PROCESS}$" | grep -v grep >> $LOG_FILE
        sleep 0.5
        let "int++"
    done
else
    echo "$PROCESS is not existed" >> $LOG_FILE
fi
date "$DATE_FORMATE" >> $LOG_FILE
ps -ef | grep "${PROCESS}$" | grep -v grep >> $LOG_FILE
