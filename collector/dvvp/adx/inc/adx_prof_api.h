/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author:
 * Create: 2020-08-05
 */
#ifndef ANALYSIS_DVVP_DEVICE_ADX_API_H
#define ANALYSIS_DVVP_DEVICE_ADX_API_H
#include <cstdio>
#include "utils/utils.h"
#include "hdc_api.h"
namespace Analysis {
namespace Dvvp {
namespace Adx {
using namespace analysis::dvvp::common::utils;
using HDC_SESSION_PTR = HDC_SESSION *;
int AdxIdeCreatePacket(CONST_VOID_PTR buffer, int length, IdeBuffT &outPut, int &outLen);
void AdxIdeFreePacket(IdeBuffT &out);
HDC_CLIENT AdxHdcClientCreate(drvHdcServiceType type);
int32_t AdxHdcClientDestroy(HDC_CLIENT client);
HDC_SERVER AdxHdcServerCreate(int32_t logDevId, drvHdcServiceType type);
void AdxHdcServerDestroy(HDC_SERVER server);
HDC_SESSION AdxHdcServerAccept(HDC_SERVER server);
int32_t AdxHdcSessionConnect(int32_t peerNode, int32_t peerDevid, HDC_CLIENT client, HDC_SESSION_PTR session);
int32_t AdxHalHdcSessionConnect(int32_t peerNode, int32_t peerDevid,
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION_PTR session);
int32_t AdxHdcSessionClose(HDC_SESSION session);
int32_t AdxIdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId);
int32_t AdxHdcSessionDestroy(HDC_SESSION session);
int32_t AdxHdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len);
int AdxHdcRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen);
}  // namespace Adx
}  // namespace Dvvp
}  // namespace Analysis
#endif
