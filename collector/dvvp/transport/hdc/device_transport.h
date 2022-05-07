/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_DEVICE_TRANSPORT_H
#define ANALYSIS_DVVP_HOST_DEVICE_TRANSPORT_H

#include <memory>
#include "dev_mgr_api.h"
#include "hdc_transport.h"
#include "message/prof_params.h"
#include "singleton/singleton.h"
#include "thread/thread.h"

namespace analysis {
namespace dvvp {
namespace transport {
class DeviceTransport : public IDeviceTransport, public analysis::dvvp::common::thread::Thread {
public:
    DeviceTransport(HDC_CLIENT client,
        const std::string &devId, const std::string &jobId, const std::string &mode);
    ~DeviceTransport() override;

    int Init();
    void CloseConn();
    bool IsInitialized();
    int SendMsgAndRecvResponse(const std::string &msg, TLV_REQ_2PTR packet) override;
    int HandlePacket(TLV_REQ_PTR packet, analysis::dvvp::message::StatusInfo &status) override;

protected:
    void Run(const struct error_message::Context &errorContext) override;

private:
    void Uinit();
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> CreateConn();
    int RecvDataPacket(TLV_REQ_2PTR packet);
    int HandleShake(SHARED_PTR_ALIA<google::protobuf::Message> message, bool ctrlShake);

private:
    HDC_CLIENT client_;
    int devIndexId_;                // for HdcSessionConnect
    std::string devIndexIdStr_;     // for management and log
    std::string jobId_;
    std::string mode_;
    bool dataInitialized_;
    bool ctrlInitialized_;
    bool isClosed_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> dataTran_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> ctrlTran_;
    std::mutex ctrlTransMtx_;
};

class DevTransMgr : public analysis::dvvp::common::singleton::Singleton<DevTransMgr> {
    friend analysis::dvvp::common::singleton::Singleton<DevTransMgr>;
public:
    int Init(std::string jobId, int devId, std::string mode);
    int UnInit();
    SHARED_PTR_ALIA<DeviceTransport> GetDevTransport(std::string jobId, int devId);
    int CloseDevTransport(std::string jobId, int devId);

public:
    static int InitDevTransMgr(std::string jobId, int devId, std::string mode)
    {
        return DevTransMgr::instance()->Init(jobId, devId, mode);
    }
    static int UnInitDevTransMgr()
    {
        return DevTransMgr::instance()->UnInit();
    }
    static int CloseDevTrans(std::string jobId, int devId)
    {
        return DevTransMgr::instance()->CloseDevTransport(jobId, devId);
    }
    static SHARED_PTR_ALIA<IDeviceTransport> GetDevTrans(std::string jobId, int devId)
    {
        return DevTransMgr::instance()->GetDevTransport(jobId, devId);
    }

protected:
    DevTransMgr() {}
    virtual ~DevTransMgr()
    {
    }

private:
    int DoInit(const std::vector<int> &devIds);

private:
    std::mutex devTarnsMtx_;
    std::map<std::string, std::map<int, SHARED_PTR_ALIA<DeviceTransport> > > devTransMap_;
};
}}}

#endif
