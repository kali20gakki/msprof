/**
 * @dlog_error_code.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef DLOG_ERROR_CODE_H
#define DLOG_ERROR_CODE_H

// level setting result flag
#define LEVEL_SETTING_SUCCESS    "++OK++"
// level setting result message for user
#define SLOGD_ERROR_MSG                  "send message to slogd failed, maybe slogd has been stoped"
#define SLOGD_RCV_ERROR_MSG              "receive message from slogd failed, maybe slogd has been stoped"
#define SLOG_CONF_ERROR_MSG              "open file 'slog.conf' failed, maybe the file doesn't exist or \
                                          its path is error"
#define LEVEL_INFO_ERROR_MSG             "level infomtion is illegal"
#define UNKNOWN_ERROR_MSG                "unknown error, please check log file"
#define MALLOC_ERROR_MSG                 "malloc failed"
#define INITIAL_CHAR_POINTER_ERROR_MSG   "initial char pointer failed"
#define STR_COPY_ERROR_MSG               "str copy failed"
#define COMPUTE_POWER_GROUP              "multiple hosts use one device, prohibit operating log level"

enum {
    ERR_OK = 0,
    ERR_CHANNEL_CREATE_FAIL,
    ERR_CHANNEL_SET_FAIL,
    ERR_CHANNEL_DELETE_FAIL,
    ERR_CHANNEL_READ_FAIL,
    ERR_NULL_PTR,
    ERR_CREATE_MSG_QUEUE_FAIL,
    ERR_INVALID_QUEUE_ID,
    ERR_DELETE_MSG_QUEUE_FAIL,
    ERR_NULL_DATA,
    ERR_INVALID_WAIT_FLAG,
    ERR_SEND_MSG_FAIL,
    ERR_RECV_MSG_FAIL,
    ERR_MSG_QUEUE_NOT_EXIST,
    ERR_PROCESS_STARED
};

typedef enum {
    SUCCESS = 0,
    ARGV_NULL,
    CFG_FILE_INVALID,
    OPEN_FILE_FAILED,
    MALLOC_FAILED,
    CALLOC_FAILED,
    LINE_NO_SYMBLE,
    STR_COPY_FAILED,
    LINE_NOT_FIND,
    KEYVALUE_NOT_FIND,
    OPEN_DIR_FAILED,
    INITIAL_CHAR_POINTER_FAILED,
    SCANDIR_DIR_FAILED = 12,
    LOG_FILE_NUM_ERR,
    GET_FILE_LEN_ERR,
    MV_FILE_PNTR_ERR,
    READ_FILE_ERR,
    GET_CFG_VALUE_ERR,
    GET_NO_VALUE_ERR,
    UNSAFE_REQUEST,
    NO_ENOUTH_SPACE,
    CONF_VALUE_NULL,
    INPUT_INVALID,
    GET_DEVICE_ID_ERR,
    DATA_OVER_FLOW,
    FLAG_TRUE,
    FLAG_FALSE,
    MKDIR_FAILED,
    NOTIFY_INIT_FAILED,
    NOTIFY_WATCH_FAILED,
    SET_LEVEL_ERR,
    LEVEL_INFO_ILLEGAL,
    GET_REALPATH_ERR,
    FILEPATH_INVALID,
    SIGACTION_FAILED,
    PROCESS_NOT_EXIST,
    INIT_FILE_LIST_ERR,
    LEVEL_NOTIFY_FAILED,
    GET_USERID_FAILED,
    CHOWN_FAILED,
    SEND_FAILED,
    MOVE_FAILED,
    // thread
    CREAT_THREAD_ERR = 50,
    MUTEX_INIT_ERR,
    MUTEX_LOCK_ERR,
    MUTEX_UNLOCK_ERR,
    // slog.conf
    OPEN_CONF_FAILED = 60,
    CONF_FILEPATH_INVALID,
    GET_CONF_FILEPATH_FAILED,
    // socket
    SOCKET_PORT_ERR = 100,
    CREATE_SOCK_FAILED,
    SET_SOCK_FAILED,
    SOCK_SEND_FAILED,
    SOCK_RECV_FAILED,
    GETIFADDRS_ERR,
    GET_NO_AFINET_CARD,
    GET_CARD_IP_ERR,
    GET_FILE_ERROR,
    PEER_CLOSE_SOCKET,
    SOCK_BIND_PORT_ERR,
    SOCK_ACCEPT_FAILED,
    SOCK_SEND_FILENAME_FAILED,
    INVALID_LOG_TYPE,
    // zip
    GET_ZIP_LEN_ERR = 200,
    ZIP_BUFFER_ERR,
    UNZIP_BUFF_ERR,
    UNCOMPRESS_ERR,
    GET_FHEAD_ERR,
    WRITE_FILE_ERR,
    GET_BLOCK_ERR,
    NOT_ZIPPED_FILE,
    NOT_COMPLETE_BLOCK,
    BUFFER_LEN_ZERO,
    OVER_UNZIPPED_LENGTH,
    HARDWARE_ZIP_INIT_ERR,
    HARDWARE_ZIP_ERR,
    // ssl
    SSL_INIT_FAILED,
    CREATE_THREAD_FAILED,
    // hdc
    HDC_INIT_ERR,
    INIT_PARENTSOURCE_ERR,
    RELEASE_PARENTSOURCE_ERR,
    LOG_RECV_FAILED,
    HDC_CHANNEL_CLOSE,
    HDC_FREE_ERR,
    INIT_CHILDSOURCE_ERR,
    RELEASE_CHILDSOURCE_ERR,
    // QUEUE
    QUEUE_IS_FULL,
    QUEUE_IS_NULL,
    QUEUE_LOCK_INIT_ERR,
    QUEUE_INIT_ERR,
    ADD_INOTIFY_ERR,
    LOG_DIR_DEPTH_EXCEED,
    LOG_FILE_NUM_EXCEED,
    FAILED,
    LOG_RESERVED
} LogRt;
#endif
