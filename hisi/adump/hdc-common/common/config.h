/**
 * @file config.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_COMMON_CONFIG_H
#define IDE_DAEMON_COMMON_CONFIG_H
#include <string>
#include "ide_os_type.h"
#include "common/extra_config.h"
namespace IdeDaemon {
namespace Common {
namespace Config {
/* directory umask */
#ifdef PLATFORM_CLOUD
constexpr int32_t DEFAULT_UMASK                  = (0027);
#else
constexpr int32_t DEFAULT_UMASK                  = (0077);
#endif

constexpr int32_t SPECIAL_UMASK                  = (0022);
constexpr int32_t IDE_MSG_BUF_LEN                = 1024;
constexpr int32_t IDE_PACK_DATA_LEN_BYTES        = 4;
constexpr int32_t IDE_HIAI_SERVER_INFO_LEN       = 100;
constexpr int32_t HDC_READ_DATA_WAIT_TIME        = 10;     // 10ms
constexpr int32_t IDE_CREATE_SERVER_TIME         = 5000;   // 5 * 1000ms
constexpr uint32_t MAX_TMP_PATH                  = 512;
constexpr int32_t MAX_BUF_LEN                    = 512;
constexpr int32_t MAX_KEY_BUF_LEN                = 1024;
constexpr int32_t MAX_KEY_LEN                    = 512;
constexpr int32_t MAX_PRINT_BUF_LEN              = 512;
constexpr int32_t MAX_LISTEN_NUM                 = 100;
constexpr int32_t PATH_LEN                       = 200;
constexpr int32_t MAX_PID_BUFF_LEN               = 20;
constexpr int32_t MAX_SEND_DADA_SIZE             = 1024000;
constexpr int32_t IDE_DEFAULT_DEVICE_ID          = 0;
constexpr int32_t IDE_DEFAULT_HOST_ID            = 0;
constexpr int32_t IDE_DEFAULT_PATH_MODE          = (0750);  // default create directory of mode
constexpr int32_t IDE_NV_FILE_MODE               = (0666);  // nv file mode
constexpr int32_t LOG_FILE_READ_MODE             = 0600;
constexpr int32_t IDE_DAEMON_BLOCK               = 0;
constexpr int32_t IDE_DAEMON_NOBLOCK             = 1;
constexpr int32_t IDE_DAEMON_MILLION_SIZE        = 20;
constexpr int32_t SELECT_5S_TIME                 = 5;
constexpr int32_t CONNECT_TIMEOUT                = 15;
constexpr int32_t EXIT_CODE_LEN                  = 3;
constexpr int32_t MAX_DUMP_QUEUE_SIZE            = 10000;
constexpr int32_t MIN_DUMP_QUEUE_SIZE            = 30;
constexpr int32_t IDE_DAEMON_SOCK_DEFAULT_SWITCH = 1;
constexpr int32_t IDE_ERROR_FD                   = (-1);
constexpr int32_t SSL_VERIFY_STATUS_ERROR        = -1;             // openssl strerror status
constexpr int32_t SSL_VERIFY_STATUS_OK           = 0;              // openssl ok status
constexpr int32_t SSL_VERIFY_STATUS_SUSPEND      = 1;              // openssl suspend status
constexpr int32_t SSL_ERROR                      = (-1);
constexpr int32_t SSL_OK                         = (0);
constexpr int32_t IDE_DEFAULT_HDC_HOST_ID        = 0;              // openssl suspend status
constexpr int64_t DEFAULT_IDE_DAEMON_STIME       = (1545901391);   // UTC 2018-12-27
constexpr int64_t IDE_DAY_TO_SECONDS             = 86400;          // 1 day, all 86400s
constexpr uint32_t IDE_MAX_CALLBACK_FUNC_NUM     = 4;              // state callback numbers
constexpr uint32_t IDE_MAX_HDC_SEGMENT           = 524288;         // (512 * 1024) max size of hdc segment
constexpr uint32_t IDE_MIN_HDC_SEGMENT           = 1024;           // min size of hdc segment
constexpr uint32_t DISK_RESERVED_SPACE           = 1048576;        // disk reserved space 1Mb
constexpr uint32_t IDE_MAX_TAR_LIMITS            = 2;              // max size of tar judge condition
constexpr uint32_t HDC_INIT_MAX_TIMES            = 30;             // Hdc init Max times
constexpr uint32_t HDC_WAIT_BASE_SLEEP_TIME      = 100;            // Hdc wait sleep time
constexpr int32_t IDE_MAX_TAR_HOST_SIZE          = 10;             // max 10 GB
constexpr uint64_t IDE_MAX_TAR_DATA_SIZE         = 0x40000000;     // max size of untar data, 1GB
constexpr uint64_t IDE_MAX_TAR_FILES             = 10000;          // max size of tar files, 10000

constexpr const char *EXIT_CODE_PREFIX                   = "cmd_exit_code=";
constexpr const char *DEFAULT_IDE_DAEMON_DATE            = ("2019-01-01");
constexpr const char *DEFAULT_IDE_DAEMON_DATE_ROOT       = ("date -s");
constexpr const char *DEFAULT_IDE_DAEMON_DATE_SUDO       = ("sudo date -s");
constexpr const char *IDE_HDC_SERVER_THREAD_NAME         = "ide_hdc_server";
constexpr const char *IDE_HDC_PROCESS_THREAD_NAME        = "ide_hdc_process";
constexpr const char *IDE_DEVICE_MONITOR_THREAD_NAME     = "ide_dev_monitor";
constexpr const char *IDE_SOCK_PROCESS_THREAD_NAME       = "ide_sock_process";
constexpr const char *IDE_UNDERLINE                      = "_";
constexpr const char *IDE_TEMP_KEY                       = "temp";
constexpr const char *IDE_TEMP_VALUE                     = "ide_daemon";
constexpr const char *IDE_CFG_KEY                        = "etc";
constexpr const char *IDE_CFG_VALUE                      = "ide_daemon.cfg";
constexpr const char *IDE_REG_PATH                       = "SOFTWARE\\Huawei\\HiAI Foundation\\Path";
constexpr const char *IDE_INVALID_IP                     = "invalid_ip";
constexpr const char *IDE_NETLINK_IP                     = "netlink_ip";
constexpr const char *IDE_CTRL_MSG_END                   = "end";
constexpr const char *IDE_HOME_WAVE_DIR                  = "~/";
constexpr const char *IDE_SPLIT_CHAR                     = ";";
constexpr const char *SSL_CA_FILE                        = "ide_daemon_cacert.pem";
constexpr const char *SSL_CLIENT_CERT                    = "ide_daemon_client_cert.pem";
constexpr const char *SSL_CLIENT_KEY                     = "ide_daemon_client_key.pem";
constexpr const char *SSL_SERVER_CERT                    = "ide_daemon_server_cert.pem";
constexpr const char *SSL_SERVER_KEY                     = "ide_daemon_server_key.pem";
constexpr const char *IDE_DEVICE_HOME                    = "/home/HwHiAiUser";
constexpr const char *IDE_DETECT_REQ_VALUE               = "detect";
constexpr const char *ADX_PF_LOCAL_CHAN                  = "adserver";
constexpr const char *ADX_CFG_IF                         = "cfgAdxIf";

/* openssl config data */
#if (OS_TYPE == WIN)
constexpr int PATH_MAX                                    = 4096;
constexpr int SOCK_CLOEXEC                                = 0;
constexpr char OS_SPLIT                                   = '\\';
constexpr const char *OS_SPLIT_STR                       = "\\";
constexpr const char *IDE_DAEMON_CFG                     = "~\\ide_daemon\\ide_daemon.cfg";
constexpr const char *IDE_DAEMON_SEC                     = "~\\ide_daemon\\ide_daemon\\ide_daemon.secu";
constexpr const char *IDE_DAEMON_STO                     = "~\\ide_daemon\\ide_daemon\\ide_daemon.store";
#else
constexpr char OS_SPLIT                                   = '/';
constexpr const char *OS_SPLIT_STR                       = "/";
#ifndef IDE_DAEMON_CFG
constexpr const char *IDE_DAEMON_CFG                     = "~/ide_daemon/ide_daemon.cfg";
#endif
constexpr const char *IDE_DAEMON_SEC                     = "~/ide_daemon/ide_daemon.secu";
constexpr const char *IDE_DAEMON_STO                     = "~/ide_daemon/ide_daemon.store";
#endif
} // end config
}
}

#endif

