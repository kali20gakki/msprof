/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: hdc transport
 * Author:
 * Create: 2019-12-15
 */
#include "hdc_transport.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "adx_prof_api.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;

HDCTransport::HDCTransport(HDC_SESSION session, bool isClient, HDC_CLIENT client)
    : session_(session), isClient_(isClient), client_(client)
{
}

HDCTransport::~HDCTransport()
{
    Destroy();
}

int HDCTransport::SendAdxBuffer(IdeBuffT out, int outLen)
{
    int ret = Analysis::Dvvp::Adx::AdxHdcWrite(session_, out, outLen);
    if (ret != IDE_DAEMON_OK) {
        MSPROF_LOGE("hdc write failed, outLen=%d, err=%d.",
                    outLen, ret);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int HDCTransport::SendBuffer(CONST_VOID_PTR buffer, int length)
{
    return SendBufferWithFixedLength(*this, buffer, length);
}

int HDCTransport::RecvPacket(TLV_REQ_2PTR packet)
{
    if (packet == nullptr) {
        return PROFILING_FAILED;
    }

    void *buffer = nullptr;
    int bufLen = 0;

    int ret = Analysis::Dvvp::Adx::AdxHdcRead(session_, &buffer, &bufLen);
    if ((ret != IDE_DAEMON_OK) || (bufLen < static_cast<int>(sizeof(struct tlv_req)))) {
        MSPROF_LOGW("hdc read failed: ret=%d; bufLen=%d", ret, bufLen);
        return PROFILING_FAILED;
    }

    *packet = (TLV_REQ_PTR)buffer;

    const int containerNoSupportProfiling = 0x544E4F43; // receive "MESSAGE_CONTAINER_NO_SUPPORT" : len = 0x544E4F43
    if ((*packet)->len == containerNoSupportProfiling) {
        std::string receiveBuffer(static_cast<CONST_CHAR_PTR>(buffer), CONTAINTER_NO_SUPPORT_MESSAGE.size());
        if (CONTAINTER_NO_SUPPORT_MESSAGE.compare(receiveBuffer) == 0) {
            return PROFILING_NOTSUPPORT;
        } else {
            MSPROF_LOGE("hdc read TLV data is invalid : dataLen=%d", containerNoSupportProfiling);
            return PROFILING_FAILED;
        }
    }
    return bufLen;
}

void HDCTransport::DestroyPacket(TLV_REQ_PTR packet)
{
    IdeBuffT buf = (IdeBuffT)packet;
    Analysis::Dvvp::Adx::AdxIdeFreePacket(buf);
}

int HDCTransport::CloseSession()
{
    if (session_ != nullptr) {
        MSPROF_LOGI("close HDC session");
        if (isClient_) {
            (void)Analysis::Dvvp::Adx::AdxHdcSessionDestroy(session_);
        } else {
            (void)Analysis::Dvvp::Adx::AdxHdcSessionClose(session_);
        }
        session_ = nullptr;
    }
    return PROFILING_SUCCESS;
}

void HDCTransport::Destroy()
{
    CloseSession();
    if (isClient_) {
        if (client_ != nullptr) {
            (void)Analysis::Dvvp::Adx::AdxHdcClientDestroy(client_);
            client_ = nullptr;
        }
    }
}

SHARED_PTR_ALIA<AdxTransport> HDCTransportFactory::CreateHdcTransport(HDC_SESSION session)
{
    if (session == nullptr) {
        MSPROF_LOGE("HDC session is invalid");
        return nullptr;
    }
    SHARED_PTR_ALIA<HDCTransport> hdcTransport;
    MSVP_MAKE_SHARED1_RET(hdcTransport, HDCTransport, session, hdcTransport);
    int devId = 0;
    int err = Analysis::Dvvp::Adx::AdxIdeGetDevIdBySession(session, &devId);
    if (err != IDE_DAEMON_OK) {
        MSPROF_LOGE("IdeGetDevIdBySession failed, err: %d", err);
        return nullptr;
    }
    if (!ParamValidation::instance()->CheckDeviceIdIsValid(std::to_string(devId))) {
        MSPROF_LOGE("[CreateHdcTransport]devId: %d is not valid!", devId);
        return nullptr;
    }
    MSPROF_LOGI("IdeGetDevIdBySession success, devid:%d", devId);
    std::string moduleName = HDC_PERFCOUNT_MODULE_NAME + "_" + std::to_string(devId);
    MSVP_MAKE_SHARED2_RET(hdcTransport->perfCount_, PerfCount, moduleName, TRANSPORT_PRI_FREQ, nullptr);
    return hdcTransport;
}

SHARED_PTR_ALIA<AdxTransport> HDCTransportFactory::CreateHdcServerTransport(int32_t logicDevId, HDC_SERVER server)
{
    MSPROF_LOGI("CreateHdcServerTransport begin, logicDevId:%d", logicDevId);
    HDC_SESSION session = Analysis::Dvvp::Adx::AdxHdcServerAccept(server);
    if (session == nullptr) {
        MSPROF_LOGW("HDC session is invalid");
        return nullptr;
    }
    SHARED_PTR_ALIA<HDCTransport> hdcTransport;
    MSVP_MAKE_SHARED2_RET(hdcTransport, HDCTransport, session, false, hdcTransport);

    MSPROF_LOGI("CreateHdcServerTransport success, logicDevId:%d", logicDevId);
    std::string moduleName = HDC_PERFCOUNT_MODULE_NAME + "_" + std::to_string(logicDevId);
    MSVP_MAKE_SHARED2_RET(hdcTransport->perfCount_, PerfCount, moduleName, TRANSPORT_PRI_FREQ, nullptr);
    return hdcTransport;
}

SHARED_PTR_ALIA<AdxTransport> HDCTransportFactory::CreateHdcTransport(HDC_CLIENT client, int devId)
{
    if (client == nullptr) {
        MSPROF_LOGE("HDC client is invalid");
        return SHARED_PTR_ALIA<AdxTransport>();
    }

    HDC_SESSION session = nullptr;

    int ret = Analysis::Dvvp::Adx::AdxHdcSessionConnect(0, devId, client, &session);
    if (ret != IDE_DAEMON_OK) {
        MSPROF_LOGW("CreateHdcTransport failed, ret is %d", ret);
        return SHARED_PTR_ALIA<AdxTransport>();
    }
    SHARED_PTR_ALIA<HDCTransport> hdcTransport;
    do {
        MSVP_MAKE_SHARED2_BREAK(hdcTransport, HDCTransport, session, true);
        std::string moduleName = HDC_PERFCOUNT_MODULE_NAME + "_" + std::to_string(devId);
        MSVP_MAKE_SHARED2_BREAK(hdcTransport->perfCount_, PerfCount, moduleName, TRANSPORT_PRI_FREQ);
    } while (0);

    if (hdcTransport == nullptr || hdcTransport->perfCount_ == nullptr) {
        Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
        session = nullptr;
    }

    return hdcTransport;
}

SHARED_PTR_ALIA<AdxTransport> HDCTransportFactory::CreateHdcClientTransport(int32_t hostPid,
    int32_t devId, HDC_CLIENT client)
{
    MSPROF_LOGI("CreateHdcClientTransport, hostPid:%d, devId:%d", hostPid, devId);

    HDC_SESSION session = nullptr;

    int ret = Analysis::Dvvp::Adx::AdxHalHdcSessionConnect(0, devId, hostPid, client, &session);
    if (ret != IDE_DAEMON_OK) {
        MSPROF_LOGW("CreateHdcTransport failed, ret is %d", ret);
        return SHARED_PTR_ALIA<AdxTransport>();
    }
    SHARED_PTR_ALIA<HDCTransport> hdcTransport;
    do {
        MSVP_MAKE_SHARED3_BREAK(hdcTransport, HDCTransport, session, true, client);
        std::string moduleName = HDC_PERFCOUNT_MODULE_NAME + "_" + std::to_string(hostPid);
        MSVP_MAKE_SHARED2_BREAK(hdcTransport->perfCount_, PerfCount, moduleName, TRANSPORT_PRI_FREQ);
    } while (0);

    if (hdcTransport == nullptr || hdcTransport->perfCount_ == nullptr) {
        Analysis::Dvvp::Adx::AdxHdcSessionClose(session);
        session = nullptr;
    }
    return hdcTransport;
}
}}}
