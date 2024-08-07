/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: dump file data
 * Author: dml
 * Create: 2018-06-13
 */
#include "uploader_dumper.h"
#include "config/config.h"
#include "dyn_prof_mgr.h"
#include "error_code.h"
#include "prof_params.h"
#include "msprof_dlog.h"
#include "prof_acl_mgr.h"
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
    : DataDumper(), module_(module), needCache_(false)
{
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

void UploaderDumper::TimedTask()
{
}

int UploaderDumper::SendData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> dataChunk)
{
    MSPROF_LOGI("UploaderDumper::SendData");
    if (dataChunk == nullptr) {
        MSPROF_LOGE("dataChunk is nullptr");
        MSPROF_INNER_ERROR("EK9999", "dataChunk is nullptr");
        return PROFILING_FAILED;
    }
    std::vector<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> > dataChunks;
    dataChunks.clear();
    dataChunks.push_back(dataChunk); // insert the data into the new vector
    return Dump(dataChunks);
}

/**
* @brief Dump: write the user datas in messages into local files
* @param [in] dataChunk: the vector saved the user datas to be write to local files
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int UploaderDumper::Dump(std::vector<SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>> &dataChunk)
{
    for (size_t i = 0; i < dataChunk.size(); i++) {
        if (dataChunk[i] == nullptr) {
            continue;
        }
        if (dataChunk[i]->extraInfo.empty()) {
            MSPROF_LOGE("FileChunk info is empty in Dump, skip message");
            continue;
        }
        std::string tag = Utils::GetInfoSuffix(dataChunk[i]->fileName);
        if (!ParamValidation::instance()->CheckDataTagIsValid(tag)) {
            MSPROF_LOGE("UploaderDumper::Dump, Check tag failed, module:%s, tag:%s",
                module_.c_str(), tag.c_str());
            MSPROF_INNER_ERROR("EK9999", "UploaderDumper::Dump, Check tag failed, module:%s, tag:%s",
                module_.c_str(), tag.c_str());
            continue;
        }
        AddToUploader(dataChunk[i]);
    }
    return PROFILING_SUCCESS;
}

void UploaderDumper::AddToUploader(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> dataChunk)
{
    std::string devId = Utils::GetInfoSuffix(dataChunk->extraInfo);
    if (devId == std::to_string(DEFAULT_HOST_ID)) {
        dataChunk->chunkModule = FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST;
    } else {
        dataChunk->chunkModule = FileChunkDataModule::PROFILING_IS_FROM_MSPROF;
    }

    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(devId, uploader);
    if (uploader == nullptr) {
        MSPROF_LOGW("UploaderDumper::AddToUploader, get uploader failed, fileName:%s, chunkLen:%zu",
            module_.c_str(), dataChunk->chunkSize);
        return;
    }
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(devId, dataChunk);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("UploaderDumper::AddToUploader, UploadData failed, fileName:%s, chunkLen:%zu",
                    module_.c_str(), dataChunk->chunkSize);
        MSPROF_INNER_ERROR("EK9999", "UploaderDumper::AddToUploader, UploadData failed, fileName:%s, chunkLen:%zu",
            module_.c_str(), dataChunk->chunkSize);
    }
}
}
}
