/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author:
 * Create: 2020-08-05
 */
#include "adx_prof_api.h"
#include "memory_utils.h"

namespace Analysis {
namespace Dvvp {
namespace Adx {
using namespace analysis::dvvp::common::error;

int AdxIdeCreatePacket(CONST_VOID_PTR buffer, int length, IdeBuffT &outPut, int &outLen)
{
    MSPROF_LOGD("AdxIdeCreatePacket, length:%d, outLen:%d", length, outLen);
    return IDE_DAEMON_ERROR;
}

void AdxIdeFreePacket(IdeBuffT &out)
{
    return;
}

HDC_CLIENT AdxHdcClientCreate(drvHdcServiceType type)
{
    return nullptr;
}

int32_t AdxHdcClientDestroy(HDC_CLIENT client)
{
    return IDE_DAEMON_ERROR;
}

HDC_SERVER AdxHdcServerCreate(int32_t logDevId, drvHdcServiceType type)
{
    return nullptr;
}

void AdxHdcServerDestroy(HDC_SERVER server)
{
    return;
}

HDC_SESSION AdxHdcServerAccept(HDC_SERVER server)
{
    return nullptr;
}

int32_t AdxHdcSessionConnect(int32_t peerNode, int32_t peerDevid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    return IDE_DAEMON_ERROR;
}

int32_t AdxHalHdcSessionConnect(int32_t peerNode, int32_t peerDevid,
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    return IDE_DAEMON_ERROR;
}

int32_t AdxHdcSessionClose(HDC_SESSION session)
{
    return IDE_DAEMON_ERROR;
}

int32_t AdxIdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId)
{
    return IDE_DAEMON_ERROR;
}

int32_t AdxHdcSessionDestroy(HDC_SESSION session)
{
    return IDE_DAEMON_ERROR;
}

int32_t AdxHdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    return IDE_DAEMON_ERROR;
}

int AdxHdcRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    return IDE_DAEMON_ERROR;
}
}  // namespace Adx
}  // namespace Dvvp
}  // namespace Analysis
