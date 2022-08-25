/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle ide profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */

#include "device_transport.h"
#include "ai_drv_dev_api.h"
#include "config/config.h"
#include "data_handle.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "param_validation.h"
#include "prof_manager.h"
#include "proto/profiler.pb.h"
#include "uploader_mgr.h"
#include "adx_prof_api.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::MsprofErrMgr;

DeviceTransport::DeviceTransport(HDC_CLIENT client,
                                 const std::string &devId,
                                 const std::string &jobId,
                                 const std::string &mode)
    : client_(client),
      devIndexId_(0),
      devIndexIdStr_(devId),
      jobId_(jobId),
      mode_(mode),
      dataInitialized_(false),
      ctrlInitialized_(false),
      isClosed_(false),
      dataTran_(nullptr),
      ctrlTran_(nullptr)
{
}

DeviceTransport::~DeviceTransport()
{
    Uinit();
    Join();
}

bool DeviceTransport::IsInitialized() const
{
    return (dataInitialized_ && ctrlInitialized_);
}

int DeviceTransport::HandlePacket(TLV_REQ_PTR packet, analysis::dvvp::message::StatusInfo &status)
{
    int ret = PROFILING_FAILED;
    do {
        if (packet == nullptr) {
            MSPROF_LOGE("Device(%s) packet is nullptr", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) packet is nullptr", devIndexIdStr_.c_str());
            break;
        }
        std::string responseStr(packet->value, packet->len);
        auto message = analysis::dvvp::message::DecodeMessage(responseStr);
        if (message == nullptr) {
            MSPROF_LOGE("Device(%s) message decode failed", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) message decode failed", devIndexIdStr_.c_str());
            break;
        }

        auto responseMessage = std::dynamic_pointer_cast<analysis::dvvp::proto::Response>(message);
        if (responseMessage == nullptr) {
            MSPROF_LOGE("Device(%s) Not response Message", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) Not response Message", devIndexIdStr_.c_str());
            break;
        }

        std::string statusStr = responseMessage->message();
        if (!status.FromString(statusStr)) {
            MSPROF_LOGE("Device(%s) message from string failed", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) message from string failed", devIndexIdStr_.c_str());
            break;
        }

        if (status.status != analysis::dvvp::message::SUCCESS) {
            MSPROF_LOGW("Device(%s) message status failed, info: %s", devIndexIdStr_.c_str(), status.info.c_str());
            break;
        }
        ret = PROFILING_SUCCESS;
    } while (0);
    ctrlTran_->DestroyPacket(packet);
    packet = nullptr;
    return ret;
}

SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> DeviceTransport::CreateConn()
{
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> tran =
        analysis::dvvp::transport::HDCTransportFactory().CreateHdcTransport(client_, devIndexId_);
    return tran;
}

int DeviceTransport::HandleShake(SHARED_PTR_ALIA<google::protobuf::Message> message, bool ctrlShake)
{
    int ret = PROFILING_FAILED;
    std::string encoded = analysis::dvvp::message::EncodeMessage(message);
    int handshakeCount = 0;
    static const int handshakeRetryTimes = 5;    // try 5 times to handshake
    static const unsigned long handshakeRetryInterval = 100000;   // sleep 100000us before retry
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> tran;
    do {
        if (handshakeCount > 0) {   // no sleep at first time
            analysis::dvvp::common::utils::Utils::UsleepInterupt(handshakeRetryInterval);
        }
        handshakeCount++;

        tran = CreateConn();
        if (tran == nullptr) {
            ret = PROFILING_FAILED;
            MSPROF_LOGW("Device(%s) create conn failed", devIndexIdStr_.c_str());
            continue;
        }
        (ctrlShake ? ctrlTran_ : dataTran_) = tran;

        ret = tran->SendBuffer(encoded.c_str(), static_cast<int>(encoded.size()));
        if (ret < 0) {
            ret = PROFILING_FAILED;
            MSPROF_LOGW("Device(%s) create channel send handshake failed", devIndexIdStr_.c_str());
            continue;
        }

        TLV_REQ_PTR packet = nullptr;
        ret = tran->RecvPacket(&packet);
        if (ret < 0 || packet == nullptr) {
            ret = PROFILING_FAILED;
            MSPROF_LOGW("Device(%s) create channel recv handshake failed", devIndexIdStr_.c_str());
            continue;
        }

        analysis::dvvp::message::StatusInfo status;
        ret = HandlePacket(packet, status);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGW("Device(%s) create channel handle packet failed", devIndexIdStr_.c_str());
            continue;
        }
        break;
    } while (handshakeCount < handshakeRetryTimes && ctrlShake);

    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to handshake with device %s, try times %d", devIndexIdStr_.c_str(), handshakeCount);
        MSPROF_INNER_ERROR("EK9999", "Failed to handshake with device %s, try times %d",
            devIndexIdStr_.c_str(), handshakeCount);
    }
    return ret;
}

int DeviceTransport::Init()
{
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    SHARED_PTR_ALIA<analysis::dvvp::proto::DataChannelHandshake> dataMessage;
    MSVP_MAKE_SHARED0_RET(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(dataMessage, analysis::dvvp::proto::DataChannelHandshake, PROFILING_FAILED);
    if (!ParamValidation::instance()->CheckDeviceIdIsValid(devIndexIdStr_)) {
        MSPROF_LOGE("[DeviceTransport::Init]devId %s is not valid!", devIndexIdStr_.c_str());
        MSPROF_INNER_ERROR("EK9999", "devId %s is not valid!", devIndexIdStr_.c_str());
        return PROFILING_FAILED;
    }
    devIndexId_ = std::stoi(devIndexIdStr_);
    MSPROF_LOGI("Try to shake hand with Device %d", devIndexId_);
    ctrlMessage->set_devid(devIndexId_);
    ctrlMessage->set_jobid(jobId_);
    ctrlMessage->set_mode(mode_);
    dataMessage->set_devid(devIndexId_);
    dataMessage->set_jobid(jobId_);
    dataMessage->set_mode(mode_);
    int ret = PROFILING_FAILED;
    do {
        std::lock_guard<std::mutex> lk(ctrlTransMtx_);
        ret = HandleShake(ctrlMessage, true);
        if (ret != PROFILING_SUCCESS) {
            break;
        }
        MSPROF_LOGI("Device(%s) create ctrl channel succeed", devIndexIdStr_.c_str());
        ret = HandleShake(dataMessage, false);
        if (ret != PROFILING_SUCCESS) {
            break;
        }
        MSPROF_LOGI("Device(%s) create data channel succeed", devIndexIdStr_.c_str());
        dataInitialized_ = true;
        ctrlInitialized_ = true;
        ret = PROFILING_SUCCESS;
    } while (0);
    if (ret != PROFILING_SUCCESS) {
        if (ctrlTran_ != nullptr) {
            ctrlTran_.reset();
        }
        if (dataTran_ != nullptr) {
            dataTran_.reset();
        }
    }
    return ret;
}

int DeviceTransport::RecvDataPacket(TLV_REQ_2PTR packet)
{
    return dataTran_->RecvPacket(packet);
}

void DeviceTransport::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    if (!dataInitialized_) {
        MSPROF_LOGE("Device(%s) data channel is not inited", devIndexIdStr_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Device(%s) data channel is not inited", devIndexIdStr_.c_str());
        return;
    }

    MSPROF_LOGI("Device(%s) data channel is running", devIndexIdStr_.c_str());
    TLV_REQ_PTR packet = nullptr;
    int ret = PROFILING_SUCCESS;
    do {
        packet = nullptr;
        ret = RecvDataPacket(&packet);
        if (ret < 0 || packet == nullptr) {
            MSPROF_EVENT("Device(%s) data channel recv data ends, thread exits", devIndexIdStr_.c_str());
            break;
        }
        MSPROF_LOGD("[DeviceTransport] RecvDataPacket %d bytes", packet->len);

        ret = Analysis::Dvvp::Msprof::HdcTransportDataHandle::ReceiveStreamData(packet->value, packet->len);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Device(%s) ReceiveStreamData failed", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) ReceiveStreamData failed", devIndexIdStr_.c_str());
        }
        dataTran_->DestroyPacket(packet);
        packet = nullptr;
    } while (!IsQuit());

    dataInitialized_ = false;
    MSPROF_LOGI("Device(%s) data channel exit", devIndexIdStr_.c_str());
}

void DeviceTransport::CloseConn()
{
    std::lock_guard<std::mutex> lk(ctrlTransMtx_);
    if (ctrlTran_ != nullptr) {
        ctrlTran_->CloseSession();
    }
    isClosed_ = true;
    ctrlInitialized_ = false;
}

void DeviceTransport::Uinit()
{
    MSPROF_LOGI("Uinit DeviceTransport Begin");
    if (dataInitialized_ && dataTran_ != nullptr) {
        dataInitialized_ = false;
        dataTran_->CloseSession();
    }
    std::lock_guard<std::mutex> lk(ctrlTransMtx_);
    if (ctrlInitialized_ && ctrlTran_ != nullptr) {
        ctrlInitialized_ = false;
        ctrlTran_->CloseSession();
    }
    MSPROF_LOGI("Uinit DeviceTransport End");
}

/**
 * @brief Send a message and get back
 * @param [in] msg: Data to send
 * @param [out] packet: Data to send
 * @return :
            success return PROFILING_SUCCESS
            failed return PROFILING_FAILED
 */
int DeviceTransport::SendMsgAndRecvResponse(const std::string &msg, TLV_REQ_2PTR packet)
{
    int ret = PROFILING_FAILED;
    do {
        if (packet == nullptr) {
            MSPROF_LOGE("invalid parameter, packet is null");
            MSPROF_INNER_ERROR("EK9999", "invalid parameter, packet is null");
            break;
        }

        std::lock_guard<std::mutex> lk(ctrlTransMtx_);
        if (isClosed_) {
            MSPROF_LOGW("Device(%s) ctrl tran is closed, skip to send message", devIndexIdStr_.c_str());
            break;
        }

        if (ctrlTran_ == nullptr) {
            MSPROF_LOGE("Device(%s) ctrl tran is null", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) ctrl tran is null", devIndexIdStr_.c_str());
            break;
        }

        int sentLen = ctrlTran_->SendBuffer(msg.c_str(), static_cast<int>(msg.size()));
        if (sentLen < 0) {
            ctrlInitialized_ = false;
            MSPROF_LOGE("Device(%s) send message failed", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) send message failed", devIndexIdStr_.c_str());
            break;
        }

        MSPROF_LOGI("Device(%s) send message size %u", devIndexIdStr_.c_str(), msg.size());
        int recvLen = ctrlTran_->RecvPacket(packet);
        if (recvLen < 0 || packet == nullptr) {
            ctrlInitialized_ = false;
            MSPROF_LOGE("Device(%s) the ack of the message is failed", devIndexIdStr_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Device(%s) the ack of the message is failed", devIndexIdStr_.c_str());
            break;
        }
        ret = PROFILING_SUCCESS;
    } while (0);

    MSPROF_LOGI("Device(%s) recv message ret %d", devIndexIdStr_.c_str(), ret);

    return ret;
}

int DevTransMgr::Init(std::string jobId, int devId, std::string mode)
{
    int ret = PROFILING_FAILED;
    // init device conn
    std::lock_guard<std::mutex> lk(devTarnsMtx_);
    HDC_CLIENT client = Analysis::Dvvp::Adx::AdxHdcClientCreate(HDC_SERVICE_TYPE_IDE2);
    if (client == nullptr) {
        MSPROF_LOGE("HDC client is invalid");
        MSPROF_INNER_ERROR("EK9999", "HDC client is invalid");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Device(%d) begin init, jobId: %s, mode: %s", devId, jobId.c_str(), mode.c_str());

    auto iterJobId = devTransMap_.find(jobId);
    if (iterJobId == devTransMap_.end()) {
        std::map<int, SHARED_PTR_ALIA<DeviceTransport>> devTransports;
        iterJobId = devTransMap_.insert(std::make_pair(jobId, devTransports)).first;
    }

    auto iterDevId = iterJobId->second.find(devId);
    if (iterDevId == iterJobId->second.end()) {
        SHARED_PTR_ALIA<DeviceTransport> devTran;
        MSVP_MAKE_SHARED4_RET(devTran, DeviceTransport, client, std::to_string(devId), jobId, mode, ret);
        iterDevId = iterJobId->second.insert(std::make_pair(devId, devTran)).first;
    }

    auto devTran = iterDevId->second;
    if (!devTran->IsInitialized()) {
        ret = devTran->Init();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Init device(%d) trans failed", devId);
            MSPROF_INNER_ERROR("EK9999", "Init device(%d) trans failed", devId);
            return ret;
        }

        // device data channel start
        devTran->SetThreadName(MSVP_DEVICE_TRANSPORT_THREAD_NAME);
        ret = devTran->Start();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Device(%d) start devTran thread failed, error:%d", devId, static_cast<int>(errno));
            MSPROF_INNER_ERROR("EK9999", "Device(%d) start devTran thread failed, error:%d",
                devId, static_cast<int>(errno));
        } else {
            MSPROF_LOGI("Device(%d) init success", devId);
        }
    } else {
        ret = PROFILING_SUCCESS;
    }

    return ret;
}

SHARED_PTR_ALIA<DeviceTransport> DevTransMgr::GetDevTransport(std::string jobId, int devId)
{
    std::lock_guard<std::mutex> lk(devTarnsMtx_);
    if (devTransMap_.find(jobId) != devTransMap_.end() &&
        devTransMap_[jobId].find(devId) != devTransMap_[jobId].end() &&
        devTransMap_[jobId][devId]->IsInitialized()) {
        return devTransMap_[jobId][devId];
    }

    return nullptr;
}

int DevTransMgr::CloseDevTransport(std::string jobId, int devId)
{
    std::lock_guard<std::mutex> lk(devTarnsMtx_);
    auto iter = devTransMap_.find(jobId);
    if (iter != devTransMap_.end()) {
        if (devId < 0) {
            // close all transports by job id
            for (auto iterDevId = iter->second.begin(); iterDevId != iter->second.end(); iterDevId++) {
                iterDevId->second->CloseConn();
            }
            iter->second.clear();
        } else {
            // close one transport by job id and device id
            if (iter->second.find(devId) != iter->second.end()) {
                iter->second[devId]->CloseConn();
                iter->second.erase(devId);
            }
        }
        // no transports, erase job id
        if (iter->second.empty()) {
            devTransMap_.erase(iter);
        }
    }

    return PROFILING_SUCCESS;
}

int DevTransMgr::UnInit()
{
    std::lock_guard<std::mutex> lk(devTarnsMtx_);
    for (auto iterJobId = devTransMap_.begin(); iterJobId != devTransMap_.end(); iterJobId++) {
        for (auto iterDevId = iterJobId->second.begin(); iterDevId != iterJobId->second.end(); iterDevId++) {
            iterDevId->second->CloseConn();
        }
    }
    devTransMap_.clear();

    return PROFILING_SUCCESS;
}
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
