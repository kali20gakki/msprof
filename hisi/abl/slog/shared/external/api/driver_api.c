/**
* @driver_api.c
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "driver_api.h"
#include "print_log.h"

#ifdef PROCESS_LOG

#include "library_load.h"

#if (OS_TYPE_DEF == 0)
#define DRV_HDC_LIBRARY_NAME "libascend_hal.so"
#else
#define DRV_HDC_LIBRARY_NAME "libascend_hal.dll"
#endif

#define DRIVER_FUNCTION_NUM 19
static ArgPtr g_libHandle = NULL;
static SymbolInfo g_drvFuncInfo[DRIVER_FUNCTION_NUM] = {
    { "drvHdcClientCreate", NULL },
    { "drvHdcClientDestroy", NULL },
    { "drvHdcSessionConnect", NULL },
    { "drvHdcSessionClose", NULL },
    { "drvHdcServerCreate", NULL },
    { "drvHdcServerDestroy", NULL },
    { "drvHdcSessionAccept", NULL },
    { "drvHdcAllocMsg", NULL },
    { "drvHdcFreeMsg", NULL },
    { "drvHdcReuseMsg", NULL },
    { "drvHdcAddMsgBuffer", NULL },
    { "drvHdcGetMsgBuffer", NULL },
    { "drvHdcSetSessionReference", NULL },
    { "drvGetPlatformInfo", NULL },
    { "drvHdcGetCapacity", NULL },
    { "halHdcGetSessionAttr", NULL },
    { "halHdcSend", NULL },
    { "halHdcRecv", NULL },
    { "halCtl", NULL }
};

int LoadDriverDllFunctions(void)
{
    g_libHandle = LoadRuntimeDll(DRV_HDC_LIBRARY_NAME);
    if (g_libHandle == NULL) {
        SELF_LOG_ERROR("load driver library failed.");
        return -1;
    }
    SELF_LOG_INFO("load driver library succeed.");
    int ret = LoadDllFunc(g_libHandle, g_drvFuncInfo, DRIVER_FUNCTION_NUM);
    if (ret != 0) {
        SELF_LOG_ERROR("load driver library function failed.");
        return -1;
    }
    SELF_LOG_INFO("load driver library function succeed.");
    return 0;
}

int UnloadDriverDllFunctions(void)
{
    int ret = UnloadRuntimeDll(g_libHandle);
    if (ret != 0) {
        SELF_LOG_WARN("close driver library handle failed.");
    } else {
        SELF_LOG_INFO("close driver library handle succeed.");
    }
    return ret;
}

typedef drvError_t (*DRV_HDC_CLIENT_CREATE)(HDC_CLIENT *, int, int, int);
drvError_t LogdrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    DRV_HDC_CLIENT_CREATE func = (DRV_HDC_CLIENT_CREATE)g_drvFuncInfo[0].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(client, maxSessionNum, serviceType, flag);
}

typedef drvError_t (*DRV_HDC_CLIENT_DESTROY)(HDC_CLIENT);
drvError_t LogdrvHdcClientDestroy(HDC_CLIENT client)
{
    DRV_HDC_CLIENT_DESTROY func = (DRV_HDC_CLIENT_DESTROY)g_drvFuncInfo[1].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(client);
}

typedef drvError_t(*DRV_SESSION_CONNECT)(int, int, HDC_CLIENT, HDC_SESSION *);
drvError_t LogdrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    DRV_SESSION_CONNECT func = (DRV_SESSION_CONNECT)g_drvFuncInfo[2].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(peer_node, peer_devid, client, session);
}

typedef drvError_t (*DRV_HDC_SESSION_CLOSE)(HDC_SESSION);
drvError_t LogdrvHdcSessionClose(HDC_SESSION session)
{
    DRV_HDC_SESSION_CLOSE func = (DRV_HDC_SESSION_CLOSE)g_drvFuncInfo[3].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session);
}

typedef drvError_t (*DRV_HDC_ALLOC_MSG)(HDC_SESSION, struct drvHdcMsg **, int);
drvError_t LogdrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    DRV_HDC_ALLOC_MSG func = (DRV_HDC_ALLOC_MSG)g_drvFuncInfo[7].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session, ppMsg, count);
}

typedef drvError_t (*DRV_HDC_FREE_MSG)(struct drvHdcMsg *);
drvError_t LogdrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    DRV_HDC_FREE_MSG func = (DRV_HDC_FREE_MSG)g_drvFuncInfo[8].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(msg);
}

typedef drvError_t (*DRV_HDC_REUSE_MSG)(struct drvHdcMsg *);
drvError_t LogdrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    DRV_HDC_REUSE_MSG func = (DRV_HDC_REUSE_MSG)g_drvFuncInfo[9].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(msg);
}

typedef drvError_t (*DRV_HDC_ADD_MSG_BUF)(struct drvHdcMsg *, char *, int);
drvError_t LogdrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    DRV_HDC_ADD_MSG_BUF func = (DRV_HDC_ADD_MSG_BUF)g_drvFuncInfo[10].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(msg, pBuf, len);
}

typedef drvError_t (*DRV_HDC_GET_MSG_BUF)(struct drvHdcMsg *, int, char **, int *);
drvError_t LogdrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    DRV_HDC_GET_MSG_BUF func = (DRV_HDC_GET_MSG_BUF)g_drvFuncInfo[11].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(msg, index, pBuf, pLen);
}

typedef drvError_t (*DRV_HDC_SET_SESSION_REF)(HDC_SESSION);
drvError_t LogdrvHdcSetSessionReference(HDC_SESSION session)
{
    DRV_HDC_SET_SESSION_REF func = (DRV_HDC_SET_SESSION_REF)g_drvFuncInfo[12].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session);
}

typedef drvError_t (*DRV_HDC_GET_PLATFORM_INFO)(uint32_t *);
drvError_t LogdrvGetPlatformInfo(uint32_t *info)
{
    DRV_HDC_GET_PLATFORM_INFO func = (DRV_HDC_GET_PLATFORM_INFO)g_drvFuncInfo[13].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(info);
}

typedef drvError_t (*DRV_HDC_GET_CAPACITY)(struct drvHdcCapacity *);
drvError_t LogdrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    DRV_HDC_GET_CAPACITY func = (DRV_HDC_GET_CAPACITY)g_drvFuncInfo[14].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(capacity);
}

typedef drvError_t (*DRV_HDC_GET_SESSION_ATTR)(HDC_SESSION, int, int *);
drvError_t LogdrvHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    DRV_HDC_GET_SESSION_ATTR func = (DRV_HDC_GET_SESSION_ATTR)g_drvFuncInfo[15].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session, attr, value);
}

typedef hdcError_t (*DRV_HDC_SEND)(HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32);
hdcError_t LogdrvHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    DRV_HDC_SEND func = (DRV_HDC_SEND)g_drvFuncInfo[16].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session, pMsg, flag, timeout);
}

typedef hdcError_t (*DRV_HDC_RECV)(HDC_SESSION, struct drvHdcMsg *, int, UINT64, int *, UINT32);
hdcError_t LogdrvHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
                         UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    DRV_HDC_RECV func = (DRV_HDC_RECV)g_drvFuncInfo[17].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(session, pMsg, bufLen, flag, recvBufCount, timeout);
}

typedef hdcError_t (*DRV_CTL)(int, void *, size_t, void *, size_t *);
drvError_t LogdrvCtl(int cmd, void *param_value, size_t param_value_size, void *out_value, size_t *out_size_ret)
{
    DRV_CTL func = (DRV_CTL)g_drvFuncInfo[18].handle;
    ONE_ACT_WARN_LOG(func == NULL, return -1, "Can not find drv func.");
    return func(cmd, param_value, param_value_size, out_value, out_size_ret);
}

#else

int LoadDriverDllFunctions(void)
{
    return 0;
}

int UnloadDriverDllFunctions(void)
{
    return 0;
}

drvError_t LogdrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    return drvHdcClientCreate(client, maxSessionNum, serviceType, flag);
}

drvError_t LogdrvHdcClientDestroy(HDC_CLIENT client)
{
    return drvHdcClientDestroy(client);
}

drvError_t LogdrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    return drvHdcSessionConnect(peer_node, peer_devid, client, session);
}

drvError_t LogdrvHdcSessionClose(HDC_SESSION session)
{
    return drvHdcSessionClose(session);
}

drvError_t LogdrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    return drvHdcAllocMsg(session, ppMsg, count);
}

drvError_t LogdrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    return drvHdcFreeMsg(msg);
}

drvError_t LogdrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    return drvHdcReuseMsg(msg);
}

drvError_t LogdrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    return drvHdcAddMsgBuffer(msg, pBuf, len);
}

drvError_t LogdrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    return drvHdcGetMsgBuffer(msg, index, pBuf, pLen);
}

drvError_t LogdrvHdcSetSessionReference(HDC_SESSION session)
{
    return drvHdcSetSessionReference(session);
}

drvError_t LogdrvGetPlatformInfo(uint32_t *info)
{
    return drvGetPlatformInfo(info);
}

drvError_t LogdrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    return drvHdcGetCapacity(capacity);
}

drvError_t LogdrvHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    return halHdcGetSessionAttr(session, attr, value);
}

hdcError_t LogdrvHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    return halHdcSend(session, pMsg, flag, timeout);
}

hdcError_t LogdrvHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
                         UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    return halHdcRecv(session, pMsg, bufLen, flag, recvBufCount, timeout);
}
#endif
