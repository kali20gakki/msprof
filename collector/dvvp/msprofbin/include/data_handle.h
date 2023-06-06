/**
* @file data_handle.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_TRANSPORT_DATA_HANDLE_H
#define ANALYSIS_DVVP_TRANSPORT_DATA_HANDLE_H

#include <map>
#include "config/config.h"
#include "message/prof_params.h"
#include "singleton/singleton.h"
#include "proto/msprofiler.pb.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::utils;
using PFMessagehandler = int (*)(SHARED_PTR_ALIA<google::protobuf::Message> message);
class IDataHandleCB {
public:
    virtual ~IDataHandleCB();
};

class HdcTransportDataHandle : public IDataHandleCB,
                               public analysis::dvvp::common::singleton::Singleton<HdcTransportDataHandle> {
public:
    HdcTransportDataHandle();
    ~HdcTransportDataHandle() override;
    static std::map<const google::protobuf::Descriptor *, PFMessagehandler> CreateHandlerMap();
    static int ReceiveStreamData(CONST_VOID_PTR data, unsigned int dataLen);

private:
    static int ProcessStreamFileChunk(SHARED_PTR_ALIA<google::protobuf::Message> message);
    static int ProcessDataChannelFinish(SHARED_PTR_ALIA<google::protobuf::Message> message);
    static int ProcessFinishJobRspMsg(SHARED_PTR_ALIA<google::protobuf::Message> message);
    static int ProcessResponseMsg(SHARED_PTR_ALIA<google::protobuf::Message> message);
    static int ProcessRspCommon(const std::string &jobId, const std::string &encoded);

private:
    static std::map<const google::protobuf::Descriptor *, PFMessagehandler> handlerMap_;
};
}
}
}
#endif
