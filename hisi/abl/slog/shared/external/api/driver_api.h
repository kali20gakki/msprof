/**
* @driver_api.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef DRIVER_API_H
#define DRIVER_API_H

#include "ascend_hal.h"

int LoadDriverDllFunctions(void);

int UnloadDriverDllFunctions(void);

drvError_t LogdrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag);

drvError_t LogdrvHdcClientDestroy(HDC_CLIENT client);

drvError_t LogdrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session);

drvError_t LogdrvHdcSessionClose(HDC_SESSION session);

drvError_t LogdrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count);

drvError_t LogdrvHdcFreeMsg(struct drvHdcMsg *msg);

drvError_t LogdrvHdcReuseMsg(struct drvHdcMsg *msg);

drvError_t LogdrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len);

drvError_t LogdrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen);

drvError_t LogdrvHdcSetSessionReference(HDC_SESSION session);

drvError_t LogdrvGetPlatformInfo(uint32_t *info);

drvError_t LogdrvHdcGetCapacity(struct drvHdcCapacity *capacity);

drvError_t LogdrvHdcGetSessionAttr(HDC_SESSION session, int attr, int *value);

hdcError_t LogdrvHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout);

hdcError_t LogdrvHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
                         UINT64 flag, int *recvBufCount, UINT32 timeout);

drvError_t LogdrvCtl(int cmd, void *param_value, size_t param_value_size,
                     void *out_value, size_t *out_size_ret);
#endif