/**
* @log_recv_interface.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef LOG_RECV_INTERFACE_H
#define LOG_RECV_INTERFACE_H
#include "dlog_error_code.h"
#include "log_sys_package.h"
#include "log_common_util.h"
#include "securec.h"

#if defined(RECV_BY_DRV)
    #include "log_recv_by_drv.h"
#else
    #include "log_recv_by_hdc.h"
#endif

#define PACKAGE_END 1
#define TWO_SECOND 2000
#define ALLSOURCE_RESERVE 20
#define CHILDSOURCE_RESERVE 16
#define IDE_DAEMON_OK 0

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char* pBuf;
    int len;
}MsgBuf;

typedef struct {
    int count;
    MsgBuf bufList[0];
}TotalMsgBuf;

// log transfer struct
typedef struct {
    short version;
    short data_compressed : 1;
    short frame_begin : 1;
    short frame_end : 1;
    short smp_flag : 1;
    short slog_flag : 1; // flag to divide device and os log, 1 for device-os and 0 for device
    short reserved_bits : 11;
    int reserved;
    int data_len;
    char data[0];
}LogMsgHead;

typedef struct {
    int peerNode;
    int peerDevId;
    ParentSource parentSource;
    ChildSource *childSource;
    char reserved[CHILDSOURCE_RESERVE];
} LogRecvChildSourceDes;

typedef struct {
    ParentSource parentSource;
    ChildSource *childSources;
    unsigned int deviceNum;
    char reserved[ALLSOURCE_RESERVE];
} LogRecvAllSourceDes;

/**
* @brief LogRecvInitParentSource: init parent source
* @param [in]parentSource: pointer to parent source instance
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvInitParentSource(ParentSource *parentSource);

/**
* @brief LogRecvReleaseParentSource: release parent source
* @param [in]parentSource: parent source instance
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvReleaseParentSource(ParentSource parentSource);

/**
* @brief LogRecvInitChildSource: init child source
* @param [in]LogRecvChildSourceDes: pointer to child source struct
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvInitChildSource(const LogRecvChildSourceDes *childSourceDes);

/**
* @brief LogRecvSafeInitChildSource: init child source by safe mode
* @param [in]LogRecvChildSourceDes: pointer to child source struct
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvSafeInitChildSource(const LogRecvChildSourceDes *childSourceDes);

/**
* @brief LogRecvReleaseChildSource: release child source
* @param [in]LogRecvChildSourceDes: child source instance
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvReleaseChildSource(ChildSource childSource);

/**
* @brief LogRecvReleaseAllSource: release all source
* @param [in]allSourceDes: point to all source struct
* @return: NA
*/
void LogRecvReleaseAllSource(const LogRecvAllSourceDes *allSourceDes);

/**
* @brief LogRecvRead: recv log data
* @param [in]inputDes: input struct about log read
* @param [out]msgBuf: pointer to pointer to msg buf
* @param [out]bufCount: pointer to buf count
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvRead(const ChildSource childSource, TotalMsgBuf **totalMsgBuf, int *bufCount);

/**
* @brief LogRecvSafeRead: recv log data by safe mode
* @param [in]childDes: child source des
* @param [out]msgBuf: pointer to pointer to msg buf
* @param [out]recvBuf: pointer to receive buf
* @return: LogRt, SUCCESS/OTHER
*/
LogRt LogRecvSafeRead(const LogRecvChildSourceDes *childDes, TotalMsgBuf **totalMsgBuf, LogMsgHead **recvBuf);

/**
* @brief LogRecvReleaseMsgBuf: release msg buffer
* @param [in]msgBuf: pointer to msg buffer
* @param [in]recvBuf: pointer to recv buffer
* @return: void
*/
void LogRecvReleaseMsgBuf(TotalMsgBuf *totalMsgBuf, LogMsgHead *recvBuf);
#ifdef __cplusplus
}
#endif
#endif