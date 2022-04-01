/**
 * @log_to_file.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_TO_FILE_H
#define LOG_TO_FILE_H
#include "log_common_util.h"
#include "cfg_file_parse.h"

#ifndef OK
#define OK 0
#endif

#ifndef NOK
#define NOK 1
#endif

#define UNIT_US_TO_MS 1000
#define DEFAULT_MAX_HOST_FILE_SIZE (20 * 1024 * 1024)
#define DEFAULT_MAX_HOST_FILE_NUM   10
#define DEFAULT_MAX_NDEBUG_FILE_SIZE (10 * 1024 * 1024)
#define DEFAULT_MAX_NDEBUG_FILE_NUM 8

#if defined(RC_HISI_MODE) || defined(RC_MODE)
#define DEFAULT_MAX_APP_FILE_SIZE (1 * 1024 * 1024)
#define DEFAULT_MAX_APP_FILE_NUM 3
#else
#define DEFAULT_MAX_APP_FILE_SIZE (512 * 1024)
#define DEFAULT_MAX_APP_FILE_NUM 2
#endif

#ifdef SORTED_LOG
    #define DEFAULT_MAX_FILE_SIZE     (100 * 1024 * 1024)
    #define DEFAULT_MAX_OS_FILE_SIZE  (100 * 1024 * 1024)
    #define DEFAULT_MAX_FILE_NUM 8
    #define DEFAULT_MAX_OS_FILE_NUM 8
#else
    #define DEFAULT_MAX_FILE_SIZE (2 * 1024 * 1024)
    #define DEFAULT_MAX_OS_FILE_SIZE  (2 * 1024 * 1024)
    #define DEFAULT_MAX_FILE_NUM 10
    #define DEFAULT_MAX_OS_FILE_NUM 3
#endif

#define RUN_DIR_NAME "run"
#define HOST_DIR_NAME "host-0"
#define PROC_DIR_NAME "plog"
#define DEBUG_DIR_NAME "debug"
#define SECURITY_DIR_NAME "security"
#define OPERATION_DIR_NAME "operation"

#define LOG_FILE_RDWR_MODE                   0640
#define LOG_FILE_ARCHIVE_MODE                0440
#define MAX_FILEDIR_LEN       CFG_LOGAGENT_PATH_MAX_LENGTH
#define MAX_FILENAME_LEN      64
#define HOST_FILE_MAX_SIZE    (100 * 1024 * 1024)
#define HOST_FILE_MIN_SIZE    (1024 * 1024)
#define HOST_FILE_MAX_NUM     1000
#define HOST_FILE_MIN_NUM     1
#define HOST_APP_FILE_MIN_SIZE (512 * 1024)
#define HOST_APP_FILE_MAX_SIZE (100 * 1024 * 1024)

#define NUM_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define DEVICE_HEAD                          "device-"
#define DEVICE_APP_HEAD                      "device-app-"
#define DEVICE_OS_HEAD                       "device-os"
#define HOST_HEAD                            "host-"
#define PROC_HEAD                            "plog"
#define MAX_NAME_HEAD_LEN                    20
#define DEVICE_MAX_FILE_NUM_STR              "DeviceMaxFileNum"
#define DEVICE_OS_MAX_FILE_NUM_STR           "DeviceOsMaxFileNum"
#define DEVICE_NDEBUG_MAX_FILE_NUM_STR       "DeviceOsNdebugMaxFileNum"
#define DEVICE_APP_MAX_FILE_NUM_STR          "DeviceAppMaxFileNum"
#define DEVICE_MAX_FILE_SIZE_STR             "DeviceMaxFileSize"
#define DEVICE_OS_MAX_FILE_SIZE_STR          "DeviceOsMaxFileSize"
#define DEVICE_NDEBUG_MAX_FILE_SIZE_STR      "DeviceOsNdebugMaxFileSize"
#define DEVICE_APP_MAX_FILE_SIZE_STR         "DeviceAppMaxFileSize"

#define TIME_STR_SIZE                        32
#define MAX_FILEPATH_LEN                     (MAX_FILEDIR_LEN + MAX_NAME_HEAD_LEN)

#if (OS_TYPE_DEF == WIN)
#define FILE_SEPERATOR                       "\\"
#else
#define FILE_SEPERATOR                       "/"
#endif

typedef struct {  // log data block paramter
    unsigned int ucDeviceID;
    unsigned int ulDataLen;
    char *paucData;
} StLogDataBlock;

typedef struct {
    LogType type;               //  debug/security/run/operation for sorted log
    ProcessType processType;    //  APPLICATION/SYSTEM(0/1)
    unsigned int pid;           //  aicpu process pid, it is available when processType is APPLICATION
    unsigned int deviceId;
} LogInfo;

typedef struct {  // sub log file list paramter
    int fileNum;
    int currIndex;
    int maxFileNum;
    unsigned int pid;
    unsigned int ulMaxFileSize;
    char aucFilePath[MAX_FILEPATH_LEN + 1];
    char aucFileHead[MAX_NAME_HEAD_LEN + 1];
    char **aucFileName;
    unsigned char devWriteFileFlag;
} StSubLogFileList;

typedef struct {  // log file list paramter
    unsigned char ucDeviceNum;
    int maxFileNum;
    int maxOsFileNum;
    int maxAppFileNum;
    int maxNdebugFileNum;
    unsigned int ulMaxFileSize;
    unsigned int ulMaxOsFileSize;
    unsigned int ulMaxAppFileSize;
    unsigned int ulMaxNdebugFileSize;
    char aucFilePath[MAX_FILEPATH_LEN + 1];
    StSubLogFileList hostLogList;
    StSubLogFileList deviceOsLogList;
    StSubLogFileList sortDeviceOsLogList[LOG_TYPE_NUM];
    StSubLogFileList *deviceLogList;
} StLogFileList;

typedef struct {
    unsigned int len;
    unsigned int deviceId;
    short smpFlag;
    short slogFlag;
} DeviceWriteLogInfo;

void LogAgentCleanUp(StLogFileList *logList);
void LogAgentFreeMaxFileNumHelper(StSubLogFileList *pstSubInfo);
void LogAgentFreeHostMaxFileNum(StLogFileList *logList);
void LogAgentFreeMaxFileNum(StLogFileList *logList);
int LogFilter(const ToolDirent *dir);
int SortFunc(const ToolDirent **a, const ToolDirent **b);
unsigned int FilePathSplice(const StSubLogFileList *pstSubInfo, char *pFileName, unsigned int ucMaxLen);
int GetFileOfSize(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                  const char *pFileName, off_t *filesize);
void LogAgentGetFileNum(StLogFileList *logList);
void LogAgentGetFileSize(StLogFileList *logList);
void LogAgentGetOsFileNum(StLogFileList *logList);
void LogAgentGetOsFileSize(StLogFileList *logList);
void LogAgentGetNdebugFileNum(StLogFileList *logList);
void LogAgentGetNdebugFileSize(StLogFileList *logList);
unsigned int LogAgentGetFileDir(StLogFileList *logList);
unsigned int LogAgentGetCfg(StLogFileList *logList);
unsigned int GetLocalTimeHelper(unsigned int bufLen, char *timeBuffer);
unsigned int LogAgentGetFileListForModule(StSubLogFileList *pstSubInfo, const char *dir,
                                          unsigned int ulMaxFileNum);
unsigned int LogAgentGetfileList(StLogFileList *logList);
unsigned int LogAgentCreateNewFileName(StSubLogFileList *pstSubInfo);
unsigned int LogAgentGetFileName(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                                 char *pFileName, unsigned int ucMaxLen);
unsigned int LogAgentWriteFile(StSubLogFileList *subList, StLogDataBlock *logData);
unsigned int LogAgentRemoveFile(const char *filename);
unsigned int LogAgentGetHostfileList(StLogFileList *logList);
unsigned int LogAgentDeleteCurrentFile(const StSubLogFileList *pstSubInfo);
unsigned int LogAgentInitMaxFileNumHelper(StSubLogFileList *pstSubInfo, const char *logPath, int length);
unsigned int LogAgentInitHostMaxFileNum(StLogFileList *logList);
unsigned int LogAgentInitDeviceMaxFileNum(StLogFileList *logList);
unsigned int LogAgentInitMaxFileNum(StLogFileList *logList);
unsigned int LogAgentWriteHostLog(LogType logType, StLogFileList *logList, const char *msg, unsigned int len);
#ifdef HARDWARE_ZIP
unsigned int CompressWrite(StSubLogFileList *pstSubInfo, StLogDataBlock *const pstLogData,
                           char *const aucFileName, unsigned int aucFileNameLen);
#endif
unsigned int NoCompressWrite(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                             char *const aucFileName, unsigned int aucFileNameLen);
void FsyncLogToDisk(const char *logPath);

unsigned int LogAgentInitDeviceOs(StLogFileList *logList);
unsigned int LogAgentWriteDeviceOsLog(LogType logType, StLogFileList *logList, const char *msg, unsigned int len);
void LogAgentCleanUpDeviceOs(StLogFileList *logList);

unsigned int LogAgentInitDevice(StLogFileList *logList, unsigned char deviceNum);
unsigned int LogAgentWriteDeviceLog(const StLogFileList *logList, char *msg, const DeviceWriteLogInfo *info);
void LogAgentCleanUpDevice(StLogFileList *logList);

unsigned int LogAgentWriteDeviceAppLog(const char *msg, unsigned int len, const LogInfo* logInfo);

unsigned int LogAgentInitProc(StLogFileList *logList);
unsigned int LogAgentWriteProcLog(StLogFileList *logList, const char *buf, unsigned int bufLen);
void LogAgentCleanUpProc(StLogFileList *logList);
bool IsUseEnvPath(const char *ppath);
#endif /* LOG_TO_FILE_H */
