/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_COLLECTION_ENTRY_H
#define ANALYSIS_DVVP_DEVICE_COLLECTION_ENTRY_H

#include <memory>
#include <mutex>
#include "collect_engine.h"
#include "message/prof_params.h"
#include "receiver.h"
#include "singleton/singleton.h"
#include "transport/hdc/hdc_transport.h"
#include "uploader.h"

namespace analysis {
namespace dvvp {
namespace device {
class CollectionEntry : public analysis::dvvp::common::singleton::Singleton<CollectionEntry> {
    friend analysis::dvvp::common::singleton::Singleton<CollectionEntry>;

public:
    int Init();
    int Uinit();

    int Handle(SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> transport,
               const std::string &req, int devIndexId);
    int FinishCollection(unsigned int devIdFlush, const std::string &jobId);
    void AddReceiver(const std::string &mode, const std::string &jobId,
                     unsigned int devIndexId, SHARED_PTR_ALIA<Receiver> receiver);
    SHARED_PTR_ALIA<Receiver> GetReceiver(const std::string &jobId, unsigned int devIndexId);
    void DeleteReceiver(const std::string &jobId, unsigned int devIndexId);
    int SendMsgByDevId(const std::string &jobId, unsigned int devIndexId,
        SHARED_PTR_ALIA<google::protobuf::Message> message);

protected:
    CollectionEntry();
    virtual ~CollectionEntry();

private:
    int HandleCtrlSession(SHARED_PTR_ALIA<Receiver> receiver,
                          SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> handshake,
                          analysis::dvvp::message::StatusInfo &statusInfo,
                          int devIndexId);
    int HandleDataSession(SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader,
                          SHARED_PTR_ALIA<analysis::dvvp::proto::DataChannelHandshake> handshake,
                          int devIndexId);

    void AddModeJobIdRelation(unsigned int devId, const std::string &mode, const std::string &jobId);
    std::string GetModeJobIdRelation(unsigned int devId, const std::string &mode);
    std::string DeleteModeJobIdRelation(unsigned int devId, const std::string &mode);

private:
    bool isInited_;
    std::map<std::string, std::map<int, SHARED_PTR_ALIA<Receiver>>> receiverMap_;   // <jobId, <devId, Receiver> >
    std::map<std::string, std::string> modeJobIdRelations_;                         // <devId_mode, jobId>
    std::map<int, int> hostIdMap_;
    std::mutex receiverMtx_;
    std::mutex relatiionMtx_;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
