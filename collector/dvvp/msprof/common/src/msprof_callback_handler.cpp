/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: zcj
 * Create: 2020-09-26
 */
#include "msprof_callback_handler.h"

#include "errno/error_code.h"
#include "prof_acl_mgr.h"
#include "prof_reporter.h"
#include "receive_data.h"
#include "prof_api_common.h"
#include "uploader_dumper.h"
#include "transport/hash_data.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::queue;
using namespace analysis::dvvp::transport;
// init map
std::map<uint32_t, MsprofCallbackHandler> MsprofCallbackHandler::reporters_;

MsprofCallbackHandler::MsprofCallbackHandler() {}

MsprofCallbackHandler::MsprofCallbackHandler(const std::string &module) : module_(module)
{
}

MsprofCallbackHandler::~MsprofCallbackHandler()
{
}

int MsprofCallbackHandler::HandleMsprofRequest(uint32_t type, VOID_PTR data, uint32_t len)
{
    switch (type) {
        case MSPROF_REPORTER_REPORT:
            return ReportData(data, len);
        case MSPROF_REPORTER_INIT:
            return StartReporter();
        case MSPROF_REPORTER_UNINIT:
            return StopReporter();
        case MSPROF_REPORTER_DATA_MAX_LEN:
            return GetDataMaxLen(data, len);
        case MSPROF_REPORTER_HASH:
            return GetHashId(data, len);
        default:
            MSPROF_LOGE("Invalid reporter callback request type: %d", type);
    }
    return PROFILING_FAILED;
}

void MsprofCallbackHandler::ForceFlush(const std::string &devId)
{
    if (reporter_ != nullptr) {
        MSPROF_LOGI("ForceFlush, module: %s", module_.c_str());
        if (!devId.empty()) {
            reporter_->DumpModelLoadData(devId);
        }
        auto profReport = std::dynamic_pointer_cast<Msprof::Engine::ProfReporter>(reporter_);
        if (profReport == nullptr) {
            MSPROF_LOGE("Failed to call dynamic_pointer_cast.");
            return;
        }
        profReport->Flush();
    }
}

void MsprofCallbackHandler::FlushDynProfCachedMsg(const std::string &devId)
{
    if (reporter_ == nullptr) {
        return;
    }
    MSPROF_LOGI("FlushDynProfCachedMsg, module: %s", module_.c_str());
    reporter_->DumpDynProfCachedMsg(devId);
}

int MsprofCallbackHandler::SendData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk)
{
    int ret = HandleMsprofRequest(MSPROF_REPORTER_INIT, nullptr, 0);
    if (ret == PROFILING_SUCCESS) {
        MSPROF_LOGI("SendData, module: %s", module_.c_str());
        return reporter_->SendData(fileChunk);
    } else {
        MSPROF_LOGE("SendData failed, module: %s", module_.c_str());
    }
    return PROFILING_FAILED;
}

void MsprofCallbackHandler::InitReporters()
{
    if (!reporters_.empty()) {
        return;
    }
    MSPROF_EVENT("Init all reporters");
    for (auto &module : MSPROF_MODULE_ID_NAME_MAP) {
        reporters_.insert(std::make_pair(module.id, MsprofCallbackHandler(module.name)));
    }
}

int MsprofCallbackHandler::ReportData(CONST_VOID_PTR data, uint32_t len) const
{
    if (reporter_ == nullptr) {
        MSPROF_LOGE("reporter is not started, module: %s", module_.c_str());
        return PROFILING_FAILED;
    }
    if (len != sizeof(struct ReporterData)) {
        MSPROF_LOGE("report data len is invalid %u", len);
        return PROFILING_FAILED;
    }
    return reporter_->Report(reinterpret_cast<CONST_REPORT_DATA_PTR>(data));
}

template <typename T>
int MsprofCallbackHandler::ReportData(const T &data) const
{
    if (reporter_ == nullptr) {
        MSPROF_LOGE("reporter is not started, module: %s", module_.c_str());
        return PROFILING_FAILED;
    }
    return reporter_->DoReportData(data);
}
 
template int MsprofCallbackHandler::ReportData(const MsprofApi &data) const;
template int MsprofCallbackHandler::ReportData(const MsprofCompactInfo &data) const;
template int MsprofCallbackHandler::ReportData(const MsprofAdditionalInfo &data) const;

int MsprofCallbackHandler::FlushData()
{
    MSPROF_LOGI("FlushData from module: %s", module_.c_str());
    if (reporter_ == nullptr) {
        MSPROF_LOGE("ProfReporter is not started, module: %s", module_.c_str());
        return PROFILING_FAILED;
    }
    auto profReport = std::dynamic_pointer_cast<Msprof::Engine::ProfReporter>(reporter_);
    if (profReport == nullptr) {
        MSPROF_LOGE("Failed to call dynamic_pointer_cast.");
        return PROFILING_FAILED;
    }
    return profReport->Flush();
}

int MsprofCallbackHandler::StartReporter()
{
    MSPROF_LOGI("StartReporter from module: %s", module_.c_str());
    if (module_.empty()) {
        MSPROF_LOGE("Empty module is not allowed");
        return PROFILING_FAILED;
    }
    if (reporter_ != nullptr) {
        MSPROF_LOGW("ProfReporter is already started, module: %s", module_.c_str());
        return PROFILING_SUCCESS;
    }
    // In dynamic profiling mode, initializing ge before initializing profiling.
    if (!Msprofiler::Api::ProfAclMgr::instance()->IsInited() && !Utils::IsDynProfMode()) {
        MSPROF_LOGE("Profiling is not started, reporter can not be inited");
        return PROFILING_FAILED;
    }
    if (HashData::instance()->Init() == PROFILING_FAILED) {
        MSPROF_LOGE("HashData init failed, module: %s", module_.c_str());
    }
    MSVP_MAKE_SHARED1_RET(reporter_, Msprof::Engine::UploaderDumper, module_, PROFILING_FAILED);
    MSPROF_LOGI("The reporter %s is created successfully", module_.c_str());
    int ret = reporter_->Start();
    if (ret != PROFILING_SUCCESS) {
        reporter_->Stop();
        reporter_.reset();
        MSPROF_LOGE("Failed to start reporter of %s", module_.c_str());
    }
    MSPROF_LOGI("The reporter %s started successfully", module_.c_str());
    return ret;
}

int MsprofCallbackHandler::StopReporter()
{
    MSPROF_LOGI("StopReporter from module: %s", module_.c_str());
    if (reporter_ == nullptr) {
        MSPROF_LOGW("ProfReporter is not started, module: %s", module_.c_str());
        return PROFILING_SUCCESS;
    }
    auto profReport = std::dynamic_pointer_cast<Msprof::Engine::ProfReporter>(reporter_);
    if (profReport == nullptr) {
        MSPROF_LOGE("Failed to call dynamic_pointer_cast.");
        return PROFILING_FAILED;
    }
    profReport->Flush();
    if (Utils::IsDynProfMode()) {
        return PROFILING_SUCCESS;
    }
    int ret = reporter_->Stop();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to stop reporter of %s", module_.c_str());
    }
    reporter_.reset();
    return ret;
}

int MsprofCallbackHandler::GetDataMaxLen(VOID_PTR data, uint32_t len)
{
    if (reporter_ == nullptr) {
        MSPROF_LOGE("reporter is not started, module: %s", module_.c_str());
        return PROFILING_FAILED;
    }
    if (len < sizeof(uint32_t)) {
        MSPROF_LOGE("len:%d is so less", len);
        return PROFILING_FAILED;
    }
    *(reinterpret_cast<uint32_t *>(data)) = reporter_->GetReportDataMaxLen();
    return PROFILING_SUCCESS;
}

int MsprofCallbackHandler::GetHashId(VOID_PTR data, uint32_t len) const
{
    if (!HashData::instance()->IsInit()) {
        MSPROF_LOGE("module:%s, HashData not be inited", module_.c_str());
        return PROFILING_FAILED;
    }
    if (data == nullptr || len != sizeof(struct MsprofHashData)) {
        MSPROF_LOGE("module:%s, params error, data:0x%lx len:%llu", module_.c_str(), data, len);
        return PROFILING_FAILED;
    }
    auto hData = reinterpret_cast<MsprofHashData *>(data);
    if (hData == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return PROFILING_FAILED;
    }
    if (hData->data == nullptr || hData->dataLen > HASH_DATA_MAX_LEN || hData->dataLen == 0) {
        MSPROF_LOGE("module:%s, data error, data:0x%lx, dataLen:%llu",
            module_.c_str(), hData->data, hData->dataLen);
        return PROFILING_FAILED;
    }
    hData->hashId = HashData::instance()->GenHashId(module_,
        reinterpret_cast<CONST_CHAR_PTR>(hData->data), hData->dataLen);
    if (hData->hashId == 0) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void FlushAllModule(const std::string &devId)
{
    auto iter = MsprofCallbackHandler::reporters_.begin();
    for (; iter != MsprofCallbackHandler::reporters_.end(); iter++) {
        iter->second.ForceFlush(devId);
    }
}

void FlushAllModuleForDynProf(const std::string &devId)
{
    if (!Utils::IsDynProfMode()) {
        return;
    }
    for (auto &iter : MsprofCallbackHandler::reporters_) {
        iter.second.FlushDynProfCachedMsg(devId);
    }
}

void FlushModule(const std::string &devId)
{
    MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].ForceFlush(devId);
}

int SendAiCpuData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk)
{
    return MsprofCallbackHandler::reporters_[MSPROF_MODULE_DATA_PREPROCESS].SendData(fileChunk);
}
}  // namespace Engine
}  // namespace Msprof
