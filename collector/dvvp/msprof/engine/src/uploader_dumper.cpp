/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: dump file data
 * Author: dml
 * Create: 2018-06-13
 */
#include "uploader_dumper.h"
#include "config/config.h"
#include "dyn_prof_server.h"
#include "error_code.h"
#include "prof_params.h"
#include "msprof_dlog.h"
#include "prof_acl_mgr.h"
#include "proto/profiler.pb.h"
#include "transport/transport.h"
#include "transport/uploader.h"
#include "transport/uploader_mgr.h"
#include "utils.h"
#include "param_validation.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Statistics;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::DynProf;

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::validation;
/**
* @brief UploaderDumper: the construct function
* @param [in] module: the path of profiling data to be saved
*/
UploaderDumper::UploaderDumper(const std::string& module)
    : DataDumper(), module_(module)
{
    if (module_ == "Framework") {
        needCache_ = true;
    } else {
        needCache_ = false;
    }
}

UploaderDumper::~UploaderDumper()
{
    Stop();
}

/**
* @brief Start: init variables of UploaderDumper for can receive data from user plugin
*               start a new thread to check the data from user and write data to local files
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int UploaderDumper::Start()
{
    if (started_) {
        MSPROF_LOGW("this reporter has been started!");
        return PROFILING_SUCCESS;
    }

    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_UPLOADER_DUMPER_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start the reporter %s in UploaderDumper::Start().", module_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to start the reporter %s in UploaderDumper::Start().",
            module_.c_str());
        return PROFILING_FAILED;
    } else {
        MSPROF_LOGI("Succeeded in starting the reporter %s in UploaderDumper::Start().", module_.c_str());
    }

    size_t buffSize = RING_BUFF_CAPACITY;
    auto iter = std::find_if(std::begin(MSPROF_MODULE_ID_NAME_MAP), std::end(MSPROF_MODULE_ID_NAME_MAP),
        [&](ModuleIdName m) { return m.name == module_; });
    if (iter != std::end(MSPROF_MODULE_ID_NAME_MAP)) {
        buffSize = iter->ringBufSize;
    }
    ReceiveData::moduleName_ = module_;
    ret = ReceiveData::Init(buffSize);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ReceiveData Init failed");
        MSPROF_INNER_ERROR("EK9999", "ReceiveData Init failed");
        return PROFILING_FAILED;
    }
    started_ = true;
    MSPROF_LOGI("start reporter success. module:%s, capacity:%llu", module_.c_str(), buffSize);
    return PROFILING_SUCCESS;
}

int UploaderDumper::Report(CONST_REPORT_DATA_PTR rData)
{
    return DoReport(rData);
}

uint32_t UploaderDumper::GetReportDataMaxLen()
{
    MSPROF_LOGI("GetReporterMaxLen from module: %s", module_.c_str());
    return RECEIVE_CHUNK_SIZE;
}

/**
* @brief Run: the thread function to deal with user datas
*/
void UploaderDumper::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    DoReportRun();
}

/**
* @brief Stop: wait data write finished, then stop the thread, which check data from user
*/
int UploaderDumper::Stop()
{
    int ret = PROFILING_SUCCESS;
    if (started_) {
        started_ = false;
        ReceiveData::StopReceiveData();
        ret = Thread::Stop();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to stop the reporter %s in UploaderDumper::Stop().", module_.c_str());
            MSPROF_INNER_ERROR("EK9999", "Failed to stop the reporter %s in UploaderDumper::Stop().", module_.c_str());
            return PROFILING_FAILED;
        } else {
            MSPROF_LOGI("Succeeded in stopping the reporter %s in UploaderDumper::Stop().", module_.c_str());
        }
    }
    ReceiveData::PrintTotalSize();
    MSPROF_LOGI("UploaderDumper stop module:%s", module_.c_str());
    return ret;
}

void UploaderDumper::WriteDone()
{
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    for (std::string devId : devIds_) {
        MSPROF_LOGI("UploaderDumper WriteDone for device %s", devId.c_str());
        UploaderMgr::instance()->GetUploader(devId, uploader);
        if (uploader != nullptr) {
            auto transport = uploader->GetTransport();
            if (transport != nullptr) {
                transport->WriteDone();
            }
        }
    }
}

/**
* @brief Flush: write all datas from user to local files
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int UploaderDumper::Flush()
{
    if (!started_) {
        MSPROF_LOGW("this reporter has been stopped");
        return PROFILING_SUCCESS;
    }
    MSPROF_EVENT("[UploaderDumper::Flush]Begin to flush data, module:%s", module_.c_str());
    ReceiveData::Flush();
    MSPROF_LOGI("ReceiveData Flush finished");
    WriteDone();
    ReceiveData::PrintTotalSize();
    MSPROF_EVENT("[UploaderDumper::Flush]End to flush data, module:%s", module_.c_str());
    return PROFILING_SUCCESS;
}

void UploaderDumper::DumpModelLoadData(const std::string &devId)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = modelLoadData_.find(devId);
    if (iter == modelLoadData_.end()) {
        auto iterCached = modelLoadDataCached_.find(devId);
        if (iterCached != modelLoadDataCached_.end()) {
            MSPROF_LOGI("No model load data, dump cached data");
            if (iterCached->second.size() == MAX_CACHE_SIZE) {
                MSPROF_LOGE("Cached model load data of device %s may exceeded the limit %u and is incomplete",
                            devId.c_str(), MAX_CACHE_SIZE);
                MSPROF_INNER_ERROR("EK9999", "Cached model load data of device %s may exceeded "
                    "the limit %zu and is incomplete", devId.c_str(), MAX_CACHE_SIZE);
            }
            for (auto &message : iterCached->second) {
                AddToUploader(message);
            }
        }
    } else {
        MSPROF_LOGI("Detected new model load data, cache it");
        modelLoadDataCached_.erase(devId);
        modelLoadDataCached_.insert(std::make_pair(iter->first, iter->second));
        modelLoadData_.erase(iter);
    }
}

void UploaderDumper::DumpDynProfCachedMsg(const std::string &devId)
{
    if (!needCache_) {
        return;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    auto devCachedMsg = cachedMsg_.find(devId);
    if (devCachedMsg == cachedMsg_.end()) {
        return;
    }
    uint32_t startTimes = DynProfMngSrv::instance()->startTimes_;
    for (auto &markMsg : devCachedMsg->second) {
        if (markMsg.begin()->first < startTimes) {
            AddToUploader(markMsg.begin()->second);
        }
    }
}

void UploaderDumper::TimedTask()
{
}

int UploaderDumper::SendData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk)
{
    MSPROF_LOGI("UploaderDumper::SendData");
    if (fileChunk == nullptr) {
        MSPROF_LOGE("fileChunk is nullptr");
        MSPROF_INNER_ERROR("EK9999", "fileChunk is nullptr");
        return PROFILING_FAILED;
    }
    std::vector<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> > fileChunks;
    fileChunks.clear();
    fileChunks.push_back(fileChunk); // insert the data into the new vector
    return Dump(fileChunks);
}

/**
* @brief Dump: write the user datas in messages into local files
* @param [in] messages: the vector saved the user datas to be write to local files
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int UploaderDumper::Dump(std::vector<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq>> &messages)
{
    for (size_t i = 0; i < messages.size(); i++) {
        if (messages[i] == nullptr) {
            continue;
        }
        if (!ParamValidation::instance()->CheckDataTagIsValid(messages[i]->tag())) {
            MSPROF_LOGE("UploaderDumper::Dump, Check tag failed, module:%s, tag:%s",
                module_.c_str(), messages[i]->tag().c_str());
            MSPROF_INNER_ERROR("EK9999", "UploaderDumper::Dump, Check tag failed, module:%s, tag:%s",
                module_.c_str(), messages[i]->tag().c_str());
            continue;
        }
        SaveModelLoadData(messages[i]);
        SaveDynProfCachedMsg(messages[i]);
        AddToUploader(messages[i]);
    }
    return PROFILING_SUCCESS;
}

void UploaderDumper::AddToUploader(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    std::string devId = message->tagsuffix();
    if (devId == std::to_string(DEFAULT_HOST_ID)) {
        message->set_datamodule(FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST);
    } else {
        message->set_datamodule(FileChunkDataModule::PROFILING_IS_FROM_MSPROF);
    }

    std::string encoded = analysis::dvvp::message::EncodeMessage(message);
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(devId, uploader);
    if (uploader == nullptr) {
        MSPROF_LOGW("UploaderDumper::AddToUploader, get uploader failed, fileName:%s, chunkLen:%d",
            module_.c_str(), message->chunksizeinbytes());
        return;
    }
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(devId, encoded.c_str(), encoded.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("UploaderDumper::AddToUploader, UploadData failed, fileName:%s, chunkLen:%d",
                    module_.c_str(), message->chunksizeinbytes());
        MSPROF_INNER_ERROR("EK9999", "UploaderDumper::AddToUploader, UploadData failed, fileName:%s, chunkLen:%d",
            module_.c_str(), message->chunksizeinbytes());
    }
}

void UploaderDumper::SaveModelLoadData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (!needCache_) {
        return;
    }
    if (message->tag().find("model_load_info") != std::string::npos ||
        message->tag().find("task_desc_info") != std::string::npos) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto iter = modelLoadData_.find(message->tagsuffix());
        if (iter == modelLoadData_.end()) {
            std::list<SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> > messages;
            messages.push_back(message);
            modelLoadData_.insert(std::make_pair(message->tagsuffix(), messages));
        } else {
            iter->second.push_back(message);
            if (iter->second.size() > MAX_CACHE_SIZE) {
                iter->second.pop_front();
            }
        }
    }
}

void UploaderDumper::SaveDynProfCachedMsg(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (!needCache_) {
        return;
    }
    if (message->tag().find("model_load_info") == std::string::npos &&
        message->tag().find("task_desc_info") == std::string::npos) {
        return;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    uint32_t times = DynProfMngSrv::instance()->startTimes_;
    std::map<uint32_t, SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq>> markMsg;
    markMsg.insert(std::make_pair(times, message));

    std::string deviceId = message->tagsuffix();
    auto devCachedMsg = cachedMsg_.find(deviceId);
    if (devCachedMsg == cachedMsg_.end()) {
        std::list<std::map<uint32_t, SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq>>> messages;
        messages.push_back(markMsg);
        cachedMsg_.insert(std::make_pair(deviceId, messages));
    } else {
        devCachedMsg->second.push_back(markMsg);
        if (devCachedMsg->second.size() > MAX_CACHE_SIZE) {
            MSPROF_LOGD("Dynamic profiling cached message over %d, cur_size=%d",
                MAX_CACHE_SIZE, devCachedMsg->second.size());
            devCachedMsg->second.pop_front();
        }
    }
}
}
}
