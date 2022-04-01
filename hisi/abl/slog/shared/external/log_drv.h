/**
* @log_drv.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef LOG_DRV_H
#define LOG_DRV_H

#include "ascend_hal.h"

#define HDC_RECV_MAX_LEN 524288 // 512KB buffer space
#define MAX_HDC_SESSION_NUM 64

#define DATA_LAST_PACKET 1

#define HDC_START_BUF "###[HDC_MSG]_DEVICE_FRAMEWORK_START_###"
#define HDC_END_BUF "###[HDC_MSG]_DEVICE_FRAMEWORK_END_###"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    unsigned short headInfo;    // head magic data, judge to little
    unsigned char headVer;      // head version
    unsigned char order;        // packet order (reserved)
    unsigned short reqType;     // request type of proto
    unsigned short devId;       // request device Id
    unsigned int totalLen;      // whole message length, only all data[0] length
    unsigned int sliceLen;      // one slice length, only data[0] length
    unsigned int offset;        // offset
    unsigned short msgType;     // message type
    unsigned short status;      // message status data
    unsigned char data[0];      // message data
} LogDataMsg;

enum LogMsgType {
    LOG_MSG_DATA,
    LOG_MSG_CTRL,
    NR_LOG_MSG_TYPE,
};

enum LogPackageType {
    LOG_LITTLE_PACKAGE = 0xB0,
    LOG_BIG_PACKAGE
};

enum Platform {
    DEVICE_SIDE = 0,
    HOST_SIDE,
    INVALID_VALUE = 10000
};

typedef struct {
    unsigned int dataLen;
    enum LogPackageType type;
    char isLast;
    char data[0];
} DataPacket;

typedef struct {
    const char *buf;
    int bufLen;
    unsigned int maxSendLen;
} DataSendMsg;

/**
* @brief: load drv dll and all driver function symbol
*/
int DrvFunctionsInit(void);

/**
* @brief: free drv dll handle
*/
int DrvFunctionsUninit(void);

/**
* @brief DrvClientCreate: create hdc client
* @param [out]client: hdc client
* @param [in]clientType: client type, log: HDC_SERVICE_TYPE_LOG
* @return: 0: success; -1: failed
*/
int DrvClientCreate(HDC_CLIENT *client, int clientType);

/**
* @brief DrvClientRelease: release hdc client
* @param [in]client: hdc client
* @return: 0: success; -1: failed
*/
int DrvClientRelease(HDC_CLIENT client);

/**
* @brief DrvSessionInit: create connection to server
* @param [in]client: hdc client
* @param [out]session: client connection session
* @param [in]devId: device id
* @return: 0: success; -1: failed
*/
int DrvSessionInit(HDC_CLIENT client, HDC_SESSION *session, int devId);

/**
* @brief DrvSessionRelease: release session
* @param [in]session: hdc session
* @return: 0: success; -1: failed
*/
int DrvSessionRelease(HDC_SESSION session);

/**
* @brief DrvGetPlatformInfo: get current platform info
* @param [out]info: platform info, 0: device size, 1: host side
* @return: 0: success, -1: failed
*/
int DrvGetPlatformInfo(unsigned int *info);

/**
* @brief DrvDevIdGetBySession: get session attr by session
* @param [in]session: connection session
* @param [in]attr: session attr enum drvHdcSessionAttr
* @param [out]value: session attr value
* @return: 0: success, -1: failed
*/
int DrvDevIdGetBySession(HDC_SESSION session, int attr, int *value);

/**
* @brief DrvBufWrite: write data by hdc
* @param [in]session: connection session
* @param [in]buf: write data buffer
* @param [in]bufLen: data length
* @return: 0: success; -1: failed
*/
int DrvBufWrite(HDC_SESSION session, const char *buf, int bufLen);

/**
* @brief DrvBufRead: recv data by hdc
* @param [in]session: hdc session
* @param [in]devId: device id
* @param [out]buf: recv data memory
* @param [out]bufLen: recv data length
* @param [in]timeout: recv timeout, if timeout=10, then timeout 10ms
* @return: 0: success; -1: failed
*/
int DrvBufRead(HDC_SESSION session, int devId, char **buf, unsigned int *bufLen, unsigned int timeout);

#ifdef __cplusplus
}
#endif
#endif