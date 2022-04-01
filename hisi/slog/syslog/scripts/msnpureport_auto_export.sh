#!/bin/bash
# function get logs continuously, the download log interval, the directory size and path to store the log are needed

# capture the end signal, perform the final deduplication and exit
trap "LastDeduplication; exit" 2 15

CURRENT_DIR=$(pwd)
LOG_DIR=log-$(date +%Y-%m-%d-%H-%M-%S)
TIME_INTERVAL=$1
LOG_ABSOLUTE_PATH_CAPACITY=$2
LOG_ABSOLUTE_PATH=$3
DRIVER_INSTALL_PATH=$(cat /etc/ascend_install.info |grep "Driver_Install_Path_Param" | cut -d "=" -f 2)
DOWNLOAD_LOG_TOOLS_PATH=${DRIVER_INSTALL_PATH}/driver/tools/
DOWNLOAD_LOG_TOOLS=msnpureport
DEVICE_LOG_DIR=()
LOG_TMP_DIR_NEW=${LOG_ABSOLUTE_PATH}/msnpureport_log_new
LOG_TMP_DIR_OLD=${LOG_ABSOLUTE_PATH}/msnpureport_log_old

function PrintLog() {
    echo "$(date "+%Y-%m-%d %H:%M:%S.%N" | cut -b 1-23) [RECORD_LOG] [$1] $2"
}

function CheckParams() {
    re='^[0-9]+$'
    if ! [[ $TIME_INTERVAL =~ $re ]]; then
        PrintLog "ERROR" "the first param: timeInterval must be integer"
        exit 1
    fi
    if [ $TIME_INTERVAL -le 0 ]; then
        PrintLog "ERROR" "the first param: timeInterval must be greater than 0"
        exit 1
    fi
    if ! [[ $LOG_ABSOLUTE_PATH_CAPACITY =~ $re ]]; then
        PrintLog "ERROR" "the second param: logAbsolutePathCapacity must be integer"
        exit 1
    fi
    if [ $LOG_ABSOLUTE_PATH_CAPACITY -le 1 ]; then
        PrintLog "ERROR" "the second param: logAbsolutePathCapacity must be greater than 1"
        exit 1
    fi
    if [[ "$LOG_ABSOLUTE_PATH" != /* ]]; then
        PrintLog "ERROR" "the third param: logAbsolutePath must be absolute path"
        exit 1
    fi
}

function Prepare() {
    # check the absolute path of the incoming storage log
    if [ ! -d "${LOG_ABSOLUTE_PATH}" ]; then
        PrintLog "WARNING" "The device log ${LOG_ABSOLUTE_PATH} storage path does not exist, will create."
        mkdir -p ${LOG_ABSOLUTE_PATH}
    fi
    mkdir -p ${LOG_TMP_DIR_NEW} ${LOG_TMP_DIR_OLD}
    rm -rf ${LOG_TMP_DIR_NEW}/*
    rm -rf ${LOG_TMP_DIR_OLD}/*

    # check the size of the directory where the incoming logs are stored
    MOUNT_DIR=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $6}'`
    LOG_DIR_MAX_SIZE=`echo "scale=2; ${LOG_ABSOLUTE_PATH_CAPACITY} / 2"|bc`

    AVAIL_SPACE=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $4}'`
    AVAIL_SPACE_K=`echo ${AVAIL_SPACE} | grep K`
    AVAIL_TAG_K="$?"
    AVAIL_SPACE_M=`echo ${AVAIL_SPACE} | grep M`
    AVAIL_TAG_M="$?"
    AVAIL_SPACE_G=`echo ${AVAIL_SPACE} | grep G`
    AVAIL_TAG_G="$?"
    AVAIL_SPACE_T=`echo ${AVAIL_SPACE} | grep T`
    AVAIL_TAG_T="$?"

    if [ "-${AVAIL_TAG_K}" == "-0" ]; then
        AVAIL_NUM=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $4}' | awk -F 'K' '{print $1}'`
        AVAIL_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$AVAIL_NUM'/'1024'/'1024'}'`
    elif [ "-${AVAIL_TAG_M}" == "-0" ]; then
        AVAIL_NUM=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $4}' | awk -F 'M' '{print $1}'`
        AVAIL_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$AVAIL_NUM'/'1024'}'`
    elif [ "-${AVAIL_TAG_G}" == "-0" ]; then
        AVAIL_NUM=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $4}' | awk -F 'G' '{print $1}'`
        AVAIL_NUM_SIZE_TO_G=`echo "${AVAIL_NUM} / 1"|bc`
    elif [ "-${AVAIL_TAG_T}" == "-0" ]; then
        AVAIL_NUM=`df -h ${LOG_ABSOLUTE_PATH} | tail -1 | awk '{print $4}' | awk -F 'T' '{print $1}'`
        AVAIL_NUM_SIZE_TO_G=`echo "${AVAIL_NUM} * 1024"|bc`
    else
        PrintLog "ERROR" "Mount dir ${MOUNT_DIR} avail space size ${AVAIL_SPACE} check fail."
        exit 1
    fi
    SIZE_RET=`awk -v num1=${LOG_ABSOLUTE_PATH_CAPACITY} -v num2=${AVAIL_NUM_SIZE_TO_G} 'BEGIN{print(num1>num2)?"0":"1"}'`
    if [ "-${SIZE_RET}" == "-0" ]; then
        PrintLog "WARNING" "The device log dir ${LOG_ABSOLUTE_PATH} size : ${LOG_ABSOLUTE_PATH_CAPACITY}G."
        PrintLog "WARNING" "The device log mount dir ${MOUNT_DIR} size : ${AVAIL_NUM_SIZE_TO_G}G."
        PrintLog "ERROR" "The device log dir${LOG_ABSOLUTE_PATH} size check fail."
        exit 1
    fi
    PrintLog "INFO" "The device log dir ${LOG_ABSOLUTE_PATH} size : ${LOG_ABSOLUTE_PATH_CAPACITY}G."
    PrintLog "INFO" "The device log mount dir ${MOUNT_DIR} size : ${AVAIL_NUM_SIZE_TO_G}G."
    PrintLog "INFO" "The device log dir ${LOG_ABSOLUTE_PATH} size check pass."

    rm -rf ${CURRENT_DIR}/device_log
    mkdir -p ${CURRENT_DIR}/device_log
    # check whether the log export tool msnpureport exists
    if [ ! -f "${DOWNLOAD_LOG_TOOLS_PATH}/${DOWNLOAD_LOG_TOOLS}" ]; then
        PrintLog "ERROR" "Download device log msnpureport tools not exist."
        exit 1
    fi
    PrintLog "INFO" "Check device log msnpureport tools exist."
}

# cp download logs to result log dir.
function CpDownloadLogsToResultLogDir() {
    num=`ls $CURRENT_DIR/device_log | wc -l`
    if [ "-${num}" != "-1" ]; then
        PrintLog "ERROR" "${CURRENT_DIR}/device_log have $num dir, expect 1 dir, please check download log tools."
        exit 1
    fi
    DIR_TEMPLATE=`ls ${CURRENT_DIR}/device_log`
    if [ "-${DIR_TEMPLATE}" == "-" ]; then
        PrintLog "ERROR" "Download device log fail."
        exit 1
    fi
    if [ ! -d "${CURRENT_DIR}/device_log/${DIR_TEMPLATE}" ]; then
        PrintLog "ERROR" "Device log Dir not exist."
        exit 1
    fi
    DEVICE_LOG_DIR=${DIR_TEMPLATE}
    PrintLog "INFO" "Downloading device logs dir is ${CURRENT_DIR}/device_log/${DEVICE_LOG_DIR}"

    # determine whether the size of the log storage directory exceeds LOG_ABSOLUTE_PATH_CAPACITY/2, no judgment needed for the first time
    if [ "-${j}" != "-0" ]; then
        LOG_TMP_DIR_USED_SIZE=`du -sh ${LOG_TMP_DIR} | awk '{print $1}'`
        LOG_TMP_DIR_USED_SPACE_K=`echo ${LOG_TMP_DIR_USED_SIZE} | grep K`
        LOG_TMP_DIR_USED_TAG_K="$?"
        LOG_TMP_DIR_USED_SPACE_M=`echo ${LOG_TMP_DIR_USED_SIZE} | grep M`
        LOG_TMP_DIR_USED_TAG_M="$?"
        LOG_TMP_DIR_USED_SPACE_G=`echo ${LOG_TMP_DIR_USED_SIZE} | grep G`
        LOG_TMP_DIR_USED_TAG_G="$?"
        LOG_TMP_DIR_USED_SPACE_T=`echo ${LOG_TMP_DIR_USED_SIZE} | grep T`
        LOG_TMP_DIR_USED_TAG_T="$?"

        if [ "-${LOG_TMP_DIR_USED_TAG_K}" == "-0" ]; then
            LOG_TMP_DIR_USED_NUM=`echo ${LOG_TMP_DIR_USED_SIZE} | awk -F 'K' '{print $1}'`
            LOG_TMP_DIR_USED_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$LOG_TMP_DIR_USED_NUM'/'1024'/'1024'}'`
        elif [ "-${LOG_TMP_DIR_USED_TAG_M}" == "-0" ]; then
            LOG_TMP_DIR_USED_NUM=`echo ${LOG_TMP_DIR_USED_SIZE} | awk -F 'M' '{print $1}'`
            LOG_TMP_DIR_USED_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$LOG_TMP_DIR_USED_NUM'/'1024'}'`
        elif [ "-${LOG_TMP_DIR_USED_TAG_G}" == "-0" ]; then
            LOG_TMP_DIR_USED_NUM=`echo ${LOG_TMP_DIR_USED_SIZE} | awk -F 'G' '{print $1}'`
            LOG_TMP_DIR_USED_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$LOG_TMP_DIR_USED_NUM'/'1'}'`
        elif [ "-${LOG_TMP_DIR_USED_TAG_T}" == "-0" ]; then
            LOG_TMP_DIR_USED_NUM=`echo ${LOG_TMP_DIR_USED_SIZE} | awk -F 'T' '{print $1}'`
            LOG_TMP_DIR_USED_NUM_SIZE_TO_G=`awk 'BEGIN{printf "%.2f\n",'$LOG_TMP_DIR_USED_NUM'*'1024'}'`
        else
            PrintLog "ERROR" "Space size check fail."
            exit 1
        fi
        SIZE_RET=`awk -v num1=${LOG_TMP_DIR_USED_NUM_SIZE_TO_G} -v num2=${LOG_DIR_MAX_SIZE} 'BEGIN{print(num1>num2)?"0":"1"}'`
        if [ "-${SIZE_RET}" == "-0" ]; then
            PrintLog "INFO" "msnpureport_log_new size had exceeded ${LOG_DIR_MAX_SIZE}G, will move its contents to msnpureport_log_old."
            rm -rf $LOG_TMP_DIR_OLD/*
            mv $LOG_TMP_DIR_NEW/* $LOG_TMP_DIR_OLD/
        fi
    fi
    \cp -rf ${CURRENT_DIR}/device_log/${DEVICE_LOG_DIR}/* ${LOG_TMP_DIR_NEW}/
}

# append and de-duplicate in log files history.log, message, message.0
function AppendAndDeduplication() {
    if [ ! -d "${LOG_TMP_DIR_NEW}/hisi_logs/" ]; then
        PrintLog "WARNING" "dir ${LOG_TMP_DIR_NEW} does not contains dir hisi_logs, please check"
    else
        find ${LOG_TMP_DIR_NEW}/hisi_logs/ -type f -name history.log > ${CURRENT_DIR}/historyList
        NUM_HISTORY=`cat ${CURRENT_DIR}/historyList | wc -l`
        echo "NUM_HISTORY="$NUM_HISTORY
        for((i=1; i<=${NUM_HISTORY}; i++))
        do
            var=`cat ${CURRENT_DIR}/historyList | sed -n "${i}p"`
            echo "history_var="$var
            publicPath=`echo ${var%/*}`
            echo "$var" | xargs cat >> ${publicPath}/history_new.log
            # delete original file history.log
            rm -f ${var}
            # deduplication
            awk '!a[$0]++' ${publicPath}/history_new.log > ${publicPath}/history_new_tmp.log
            mv -f ${publicPath}/history_new_tmp.log ${publicPath}/history_new.log
        done
    fi

    if [ ! -d "${LOG_TMP_DIR_NEW}/message/" ]; then
        PrintLog "WARNING" "dir ${LOG_TMP_DIR_NEW} does not contains dir message, please check"
    else
        find ${LOG_TMP_DIR_NEW}/message/ -type f \( -name "messages" -o -name "messages.0" \) > ${CURRENT_DIR}/messageList
        NUM_MESSAGE=`cat ${CURRENT_DIR}/messageList | wc -l`
        echo "NUM_MESSAGE="$NUM_MESSAGE
        for((i=1; i<=${NUM_MESSAGE}; i++))
        do
            var=`cat ${CURRENT_DIR}/messageList | sed -n "${i}p"`
            echo "message_var="$var
            publicPath=`echo ${var%/*}`
            echo "$var" | xargs cat >> ${publicPath}/message_new.log
            # delete original file message, message.0
            rm -f ${var}
            # deduplication
            awk '!a[$0]++' ${publicPath}/message_new.log > ${publicPath}/message_new_tmp.log
            mv -f ${publicPath}//message_new_tmp.log ${publicPath}/message_new.log
        done
	fi
}

# collect logs continuously
function CollectLog() {
    LOG_TMP_DIR=${LOG_TMP_DIR_NEW}
    for ((j = 0; ; j++))
    do
        PrintLog "INFO" "Start downloading logs."
        cd ${CURRENT_DIR}/device_log
        # download log
        ${DOWNLOAD_LOG_TOOLS_PATH}/${DOWNLOAD_LOG_TOOLS} > ${CURRENT_DIR}/tools.log
        ret=`grep "send file hdc client connect failed" ${CURRENT_DIR}/tools.log | wc -l`
        # abnormal exit
        if [ "-${ret}" != "-0" ]; then
            PrintLog "ERROR" "Error is reported and exits. Check the tools.log file in the current directory."
            exit 1
        fi
        PrintLog "INFO" "Download device logs succeed."
        cd ${CURRENT_DIR}
        # cp download logs to result log dir
        CpDownloadLogsToResultLogDir ${j}
        # append and de-duplicate in log files history.log, message, message.0
        AppendAndDeduplication
        LOG_TMP_DIR_USED_SIZE=`du -sh ${LOG_TMP_DIR} | awk '{print $1}'`
        PrintLog "INFO" "The device log dir ${LOG_TMP_DIR}, used size : ${LOG_TMP_DIR_USED_SIZE}, single max size : ${LOG_DIR_MAX_SIZE}G."
        PrintLog "INFO" "Update log file succeed, tag [$((j+1))]."
        # delete the original download log file
        cd ${CURRENT_DIR}/device_log/
        rm -rf ${DIR_TEMPLATE}
        # download interval
        sleep ${TIME_INTERVAL}
    done
}

function LastDeduplication() {
    AppendAndDeduplication
    rm -rf ${CURRENT_DIR}/device_log
}

function Main() {
    if [ "-$#" != "-3" ]; then
        echo -e "Usage:"
        echo -e "    msnpureport_auto_export.sh <OPTIONS>\n"
        echo -e "Options:"
        echo -e "    timeInterval            : Interval for exporting logs."
        echo -e "    logAbsolutePathCapacity : Log save path, unit: g."
        echo -e "    logAbsolutePath         : Log save path, only absolute paths are supported.\n"
        echo -e "Examples:"
        echo -e "    ./msnpureport_auto_export.sh timeInterval logAbsolutePathCapacity logAbsolutePath"
        return 1
    fi
    CheckParams
    Prepare
    CollectLog
}

Main $1 $2 $3