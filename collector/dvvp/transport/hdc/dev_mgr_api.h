/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: device transport interface
 * Author: 
 * Create: 2019-12-22
 */
#ifndef ANALYSIS_DVVP_HOST_DEV_MGR_API_H
#define ANALYSIS_DVVP_HOST_DEV_MGR_API_H
#include <string>
#include "hdc_transport.h"
#include "message/prof_params.h"

namespace analysis {
namespace dvvp {
namespace transport {
class IDeviceTransport;

using PFDevMgrInit = int (*)(const std::string jobId, int devId, const std::string mode);
using PFDevMgrUnInit = int (*)();
using PFDevMgrCloseDevTrans = int (*)(const std::string jobId, int devId);
using PFDevMgrGetDevTrans = SHARED_PTR_ALIA<IDeviceTransport> (*)(const std::string jobId, int devId);

class DevMgrAPI {
public:
    DevMgrAPI()
        : pfDevMgrInit(nullptr),
          pfDevMgrUnInit(nullptr),
          pfDevMgrCloseDevTrans(nullptr),
          pfDevMgrGetDevTrans(nullptr) {}
    virtual ~DevMgrAPI() {}
public:
    PFDevMgrInit pfDevMgrInit;
    PFDevMgrUnInit pfDevMgrUnInit;
    PFDevMgrCloseDevTrans pfDevMgrCloseDevTrans;
    PFDevMgrGetDevTrans pfDevMgrGetDevTrans;
};

extern void LoadDevMgrAPI(DevMgrAPI &devMgrAPI);

class IDeviceTransport {
public:
    virtual ~IDeviceTransport() {}
public:
    virtual int SendMsgAndRecvResponse(const std::string &msg, TLV_REQ_2PTR packet) = 0;
    virtual int HandlePacket(TLV_REQ_PTR packet, analysis::dvvp::message::StatusInfo &status) = 0;
};
}}}

#endif
