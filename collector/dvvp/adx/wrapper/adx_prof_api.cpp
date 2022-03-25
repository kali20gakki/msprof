/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author:
 * Create: 2020-08-05
 */
#include "adx_prof_api.h"
#include <cerrno>
#include <map>
#include "errno/error_code.h"
#include "securec.h"

namespace Analysis {
namespace Dvvp {
namespace Adx {
using namespace analysis::dvvp::common::error;

int AdxIdeCreatePacket(CONST_VOID_PTR buffer, int length, IdeBuffT &outPut, int &outLen)
{
    int len = 0;
    IdeBuffT out = nullptr;
    int ret = IdeCreatePacket(IDE_PROFILING_REQ, // enum cmd_class
        (const char *)buffer, // value,
        length,               // value_len,
        &out,                 // buf
        &len);             // buf_len
    outLen = len;
    outPut = out;

    return ret;
}

void AdxIdeFreePacket(IdeBuffT &out)
{
    if (out != nullptr) {
        IdeFreePacket(out);
        out = nullptr;
    }
}

int32_t AdxHdcClientDestroy(HDC_CLIENT client)
{
    return Analysis::Dvvp::Adx::HdcClientDestroy(client);
}

HDC_SESSION AdxHdcServerAccept(HDC_SERVER server)
{
    return Analysis::Dvvp::Adx::HdcServerAccept(server);
}

void AdxHdcServerDestroy(HDC_SERVER server)
{
    Analysis::Dvvp::Adx::HdcServerDestroy(server);
}

int32_t AdxHdcSessionConnect(int32_t peerNode, int32_t peerDevid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    return Analysis::Dvvp::Adx::HdcSessionConnect(peerNode, peerDevid, client, session);
}

int32_t AdxHdcSessionClose(HDC_SESSION session)
{
    return Analysis::Dvvp::Adx::HdcSessionClose(session);
}

int32_t AdxHalHdcSessionConnect(int32_t peerNode, int32_t peerDevid,
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    return Analysis::Dvvp::Adx::HalHdcSessionConnect(peerNode, peerDevid, hostPid, client, session);
}

int32_t AdxIdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId)
{
    return Analysis::Dvvp::Adx::IdeGetDevIdBySession(session, devId);
}

int32_t AdxHdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    return Analysis::Dvvp::Adx::HdcWrite(session, buf, len);
}

int32_t AdxHdcSessionDestroy(HDC_SESSION session)
{
    return Analysis::Dvvp::Adx::HdcSessionDestroy(session);
}

int AdxHdcRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    return Analysis::Dvvp::Adx::HdcRead(session, recvBuf, recvLen);
}
}  // namespace Adx
}  // namespace Dvvp
}  // namespace Analysis
