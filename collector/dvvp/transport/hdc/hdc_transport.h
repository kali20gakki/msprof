/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */

#ifndef ANALYSIS_DVVP_COMMON_HDC_TRANSPORT_H
#define ANALYSIS_DVVP_COMMON_HDC_TRANSPORT_H

#include <cstdint>
#include "transport.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
class HDCTransport : public AdxTransport {
public:
    explicit HDCTransport(HDC_SESSION session, bool isClient = false, HDC_CLIENT client = nullptr);
    ~HDCTransport() override;

public:
    int SendBuffer(CONST_VOID_PTR buffer, int length) override;
    int RecvPacket(TLV_REQ_2PTR packet) override;
    void DestroyPacket(TLV_REQ_PTR packet) override;
    int CloseSession() override;
    int SendAdxBuffer(IdeBuffT out, int outLen) override;

private:
    void Destroy();

private:
    HDC_SESSION session_;
    bool isClient_;
    HDC_CLIENT client_;
};

class HDCTransportFactory {
public:
    HDCTransportFactory() {}
    virtual ~HDCTransportFactory() {}

public:
    SHARED_PTR_ALIA<AdxTransport> CreateHdcTransport(HDC_SESSION session);
    SHARED_PTR_ALIA<AdxTransport> CreateHdcTransport(HDC_CLIENT client, int deviceId);
    SHARED_PTR_ALIA<AdxTransport> CreateHdcServerTransport(int32_t logicDevId, HDC_SERVER server);
    SHARED_PTR_ALIA<AdxTransport> CreateHdcClientTransport(int32_t hostPid, int32_t devId, HDC_CLIENT client);
};
}}}
#endif
