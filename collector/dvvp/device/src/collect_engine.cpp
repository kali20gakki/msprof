/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */

#include "collect_engine.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <sstream>
#include "app/application.h"
#include "collection_entry.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "param_validation.h"
#include "securec.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "config/config_manager.h"
#include "prof_channel_manager.h"

namespace analysis {
namespace dvvp {
namespace device {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::JobWrapper;
using namespace analysis::dvvp::common::validation;


std::mutex CollectEngine::staticMtx_;

CollectEngine::CollectEngine()
    : _is_stop(false), isInited_(false), startMono_(0),
      startRealtime_(0), isAicoreSampleBased_(false), isAicoreTaskBased_(false), isAivSampleBased_(false),
      isAivTaskBased_(false), _is_started(false)
{
    cntvct_ = 0;
    collectionjobComnCfg_.reset();
}

CollectEngine::~CollectEngine()
{
    Uinit();
}

int CollectEngine::Init(int devId)
{
    isInited_ = true;
    MSVP_MAKE_SHARED0_RET(collectionjobComnCfg_, CollectionJobCommonParams, PROFILING_FAILED);
    collectionjobComnCfg_->devId = devId;
    CreateCollectionJobArray();
    return PROFILING_SUCCESS;
}

int CollectEngine::Uinit()
{
    if (!isInited_) {
        return PROFILING_SUCCESS;
    }

    isInited_ = false;
    if (_is_started) {
        try {
            analysis::dvvp::message::StatusInfo status;
            int ret = CollectStop(status, false);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGD("[CollectEngine]Collect stop failed.");
                return ret;
            }
        } catch (...) {
            MSPROF_LOGD("[CollectEngine]Uinit failed.");
            return PROFILING_FAILED;
        }
    }
    collectionjobComnCfg_.reset();
    return PROFILING_SUCCESS;
}

void CollectEngine::SetDevIdOnHost(int devIdOnHost)
{
    if (collectionjobComnCfg_ != nullptr) {
        MSPROF_LOGI("SetDevIdOnHost devId :%d", collectionjobComnCfg_->devIdOnHost);
        collectionjobComnCfg_->devIdOnHost = devIdOnHost;
        collectionjobComnCfg_->devIdFlush = devIdOnHost;
    }
}

int CollectEngine::CreateTmpDir(std::string &tmp)
{
    std::string tempDir = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetDefaultWorkDir();
    if (tempDir.empty()) {
        MSPROF_LOGE("GetInotifyDir failed");
        return PROFILING_FAILED;
    }
    tmp = tempDir + collectionjobComnCfg_->params->job_id;

    std::lock_guard<std::mutex> lock(staticMtx_);
    int ret = analysis::dvvp::common::utils::Utils::CreateDir(tmp);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Creating dir failed: %s", Utils::BaseName(tmp).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return ret;
    }
    MSPROF_LOGI("Creating dir: \"%s\", ret=%d", Utils::BaseName(tmp).c_str(), ret);
    std::string perfDataDir =
        Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(collectionjobComnCfg_->devId);
    if (perfDataDir.empty()) {
        MSPROF_LOGE("GetPerfDataDir failed");
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::common::utils::Utils::CreateDir(perfDataDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Creating dir failed: %s", Utils::BaseName(perfDataDir).c_str());
        analysis::dvvp::common::utils::Utils::PrintSysErrorMsg();
        return ret;
    }
    MSPROF_LOGI("Creating dir: \"%s\", ret=%d", Utils::BaseName(perfDataDir).c_str(), ret);
    return PROFILING_SUCCESS;
}

int CollectEngine::CleanupResults()
{
    std::string tempDir = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetDefaultWorkDir();
    if (tempDir.empty()) {
        MSPROF_LOGE("GetInotifyDir failed");
        return PROFILING_FAILED;
    }
    std::lock_guard<std::mutex> lock(staticMtx_);
    std::string tmp = tempDir + collectionjobComnCfg_->params->job_id;
    MSPROF_LOGI("Removing collected data: \"%s\"", tmp.c_str());
    analysis::dvvp::common::utils::Utils::RemoveDir(tmp);
    std::string perfDataDir =
        Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(collectionjobComnCfg_->devId);
    if (perfDataDir.empty()) {
        MSPROF_LOGE("GetPerfDataDir failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Removing collected data: \"%s\"", perfDataDir.c_str());
    analysis::dvvp::common::utils::Utils::RemoveDir(perfDataDir);
    tmpResultDir_.clear();
    return PROFILING_SUCCESS;
}

void CollectEngine::InitTsCpuBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent)
{
    if (tsCpuEvent != nullptr && tsCpuEvent->size()) {
        CollectionJobV_[TS_CPU_DRV_COLLECTION_JOB].jobCfg->jobParams.events = tsCpuEvent;
    }
}

void CollectEngine::InitAiCoreBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
    SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores)
{
    if (isAicoreTaskBased_ && aiCoreEvent != nullptr && aiCoreEvent->size()) {
        CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.events = aiCoreEvent;
    } else if (isAicoreSampleBased_ && aiCoreEvent != nullptr && aiCoreEventCores != nullptr) {
        CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = aiCoreEventCores;
        CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.events = aiCoreEvent;
    }
}

void CollectEngine::InitAivBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > aivEvents,
    SHARED_PTR_ALIA<std::vector<int> > aivEventCores)
{
    if (isAivTaskBased_ && aivEvents != nullptr && aivEvents->size() > 0) {
        CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.events = aivEvents;
    } else if (isAivSampleBased_ && aivEvents != nullptr && aivEventCores != nullptr) {
        CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.cores = aivEventCores;
        CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.events = aivEvents;
    }
}

int CollectEngine::CheckPmuEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > ctrlCpuEvent,
    SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent,
    SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
    SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores,
    SHARED_PTR_ALIA<std::vector<std::string> > llcEvent,
    SHARED_PTR_ALIA<std::vector<std::string> > ddrEvent,
    SHARED_PTR_ALIA<std::vector<std::string> > aivEvent,
    SHARED_PTR_ALIA<std::vector<int> > aivEventCores)
{
    if (ctrlCpuEvent != nullptr && !ParamValidation::instance()->CheckCtrlCpuEventIsValid(*ctrlCpuEvent)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]ctrlCpuEvent is not valid!");
        return PROFILING_FAILED;
    }
    if (tsCpuEvent != nullptr && !ParamValidation::instance()->CheckTsCpuEventIsValid(*tsCpuEvent)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]tsCpuEvent is not valid!");
        return PROFILING_FAILED;
    }

    if (CheckAiCoreEventIsValid(aiCoreEvent, aiCoreEventCores) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    if (llcEvent != nullptr) {
        UtilsStringBuilder<std::string> builder;
        std::string eventStr = builder.Join(*llcEvent, ",");
        if (!ParamValidation::instance()->CheckLlcEventsIsValid(eventStr)) {
            MSPROF_LOGE("[CheckPmuEventIsValid]llcEvent is not valid!");
            return PROFILING_FAILED;
        }
    }
    if (ddrEvent != nullptr && !ParamValidation::instance()->CheckDdrEventsIsValid(*ddrEvent)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]ddrEvent is not valid!");
        return PROFILING_FAILED;
    }

    if (CheckAivEventIsValid(aivEvent, aivEventCores) != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int CollectEngine::CheckAiCoreEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
    SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores)
{
    if (aiCoreEvent != nullptr && !ParamValidation::instance()->CheckAiCoreEventsIsValid(*aiCoreEvent)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]aiCoreEvent is not valid!");
        return PROFILING_FAILED;
    }
    if (aiCoreEventCores != nullptr && !ParamValidation::instance()->CheckAiCoreEventCoresIsValid(*aiCoreEventCores)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]aiCoreEventCores is not valid!");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
int CollectEngine::CheckAivEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > aivEvent,
    SHARED_PTR_ALIA<std::vector<int> > aivEventCores)
{
    if (aivEvent != nullptr && !ParamValidation::instance()->CheckAivEventsIsValid(*aivEvent)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]aivEvent is not valid!");
        return PROFILING_FAILED;
    }
    if (aivEventCores != nullptr && !ParamValidation::instance()->CheckAivEventCoresIsValid(*aivEventCores)) {
        MSPROF_LOGE("[CheckPmuEventIsValid]aivEventCores is not valid!");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int CollectEngine::CollectStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > ctrlCpuEvent,
                                      SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent,
                                      SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
                                      SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores,
                                      analysis::dvvp::message::StatusInfo &status,
                                      SHARED_PTR_ALIA<std::vector<std::string> > llcEvent,
                                      SHARED_PTR_ALIA<std::vector<std::string> > ddrEvent,
                                      SHARED_PTR_ALIA<std::vector<std::string> > aivEvents,
                                      SHARED_PTR_ALIA<std::vector<int> > aivEventCores)
{
    status.status = analysis::dvvp::message::ERR;
    if (!_is_started || collectionjobComnCfg_ == nullptr) {
        status.info = "collection engine has not been started";
        return PROFILING_FAILED;
    }
    if (CheckPmuEventIsValid(ctrlCpuEvent, tsCpuEvent, aiCoreEvent, aiCoreEventCores, llcEvent,
        ddrEvent, aivEvents, aivEventCores) != PROFILING_SUCCESS) {
        status.info = "[CollectStart] pmu event is not valid.";
        MSPROF_LOGE("[CollectStart] pmu event is not valid.");
        return PROFILING_FAILED;
    }
    MSVP_MAKE_SHARED0_RET(collectionjobComnCfg_->jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    collectionjobComnCfg_->jobCtx->dev_id = std::to_string(collectionjobComnCfg_->devIdFlush);
    collectionjobComnCfg_->jobCtx->job_id = collectionjobComnCfg_->params->job_id;
    InitTsCpuBeforeStartReplay(tsCpuEvent);
    InitAiCoreBeforeStartReplay(aiCoreEvent, aiCoreEventCores);
    InitAivBeforeStartReplay(aivEvents, aivEventCores);
    CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].jobCfg->jobParams.events = ctrlCpuEvent;
    CollectionJobV_[DDR_DRV_COLLECTION_JOB].jobCfg->jobParams.events = ddrEvent;
    CollectionJobV_[LLC_DRV_COLLECTION_JOB].jobCfg->jobParams.events = llcEvent;
    if (collectionjobComnCfg_->params->hbmProfiling.compare("on") == 0 &&
        !collectionjobComnCfg_->params->hbm_profiling_events.empty()) {
        auto hbmEvents = std::make_shared<std::vector<std::string>> ();
        *hbmEvents = Utils::Split(collectionjobComnCfg_->params->hbm_profiling_events, false, "", ",");
        if (!ParamValidation::instance()->CheckHbmEventsIsValid(*hbmEvents)) {
            MSPROF_LOGE("[CollectEngine::CollectStart]hbmEvent is not valid!");
            return PROFILING_FAILED;
        }
        CollectionJobV_[HBM_DRV_COLLECTION_JOB].jobCfg->jobParams.events = hbmEvents;
    }
    return CollectRegister(status);
}

int CollectEngine::CollectRegister(analysis::dvvp::message::StatusInfo &status)
{
    MSPROF_LOGI("Start to register collction job:%s", collectionjobComnCfg_->params->job_id.c_str());
    int registerCnt = 0;
    std::vector<int> registered;
    for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
        // check job availability
        if (CollectionJobV_[cnt].collectionJob != nullptr) {
            MSPROF_LOGI("CollectRegister Start jobId %d ", cnt);
            int ret = CollectionJobV_[cnt].collectionJob->Init(CollectionJobV_[cnt].jobCfg);
            if (ret == PROFILING_SUCCESS) {
                MSPROF_LOGD("Collection Job %d Register", cnt);
                ret = CollectionRegisterMgr::instance()
                    ->CollectionJobRegisterAndRun(collectionjobComnCfg_->devId,
                                                  CollectionJobV_[cnt].jobTag,
                                                  CollectionJobV_[cnt].collectionJob);
            }

            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGD("Collection Job %d No Run; Total: %d", CollectionJobV_[cnt].jobTag, registerCnt);
                registerCnt++;
            } else {
                registered.push_back(cnt);
            }
        }
    }
    UtilsStringBuilder<int> intBuilder;
    MSPROF_LOGI("Total count of job registered: %s", intBuilder.Join(registered, ",").c_str());

    if (registered.empty()) {
        MSPROF_LOGE("CollectionJobRegisterAndRun failed, fail_cnt:%d", registerCnt);
        status.status = analysis::dvvp::message::ERR;
        return PROFILING_FAILED;
    }
    status.status = analysis::dvvp::message::SUCCESS;
    return PROFILING_SUCCESS;
}

void CollectEngine::StoreData(const std::string &path, const std::string &fileName)
{
    static const int fileBufLen = 1 * 1024 * 1024; // 1 * 1024 * 1024 means 1mb
    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(path);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGE("file size is invalid");
        return;
    }
    SHARED_PTR_ALIA<char> buffer;
    long long dataSize = 0;
    MSVP_MAKE_SHARED_ARRAY_VOID(buffer, char, fileBufLen);
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk = nullptr;
    MSVP_MAKE_SHARED0_VOID(fileChunk, analysis::dvvp::proto::FileChunkReq);

    std::ifstream ifs(path, std::ifstream::in);
    if (!ifs.is_open()) {
        MSPROF_LOGE("open file %s failed .errno :%d", Utils::BaseName(path).c_str(), static_cast<int>(errno));
        return;
    }
    if (collectionjobComnCfg_ == nullptr || collectionjobComnCfg_->params == nullptr) {
        MSPROF_LOGE("StoreData GET Params Error");
        ifs.close();
        return;
    }

    InitFileChunkOfStoreData(fileChunk, fileName);
    while (ifs.good()) {
        ifs.read(buffer.get(), fileBufLen - 1);
        try {
            fileChunk->set_chunk(buffer.get(), ifs.gcount());
            fileChunk->set_chunksizeinbytes(ifs.gcount());
            std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
            int ret = UploaderMgr::instance()->UploadData(collectionjobComnCfg_->params->job_id,
                static_cast<void *>(const_cast<CHAR_PTR>(encoded.c_str())), (uint32_t)encoded.size());
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("StoreData failed, because UploadData error.");
                break;
            }
            dataSize += (long long)ifs.gcount();
        } catch (google::protobuf::FatalException &e) {
            MSPROF_LOGE("StoreData failed, because fileChunck update data error.");
            break;
        }
    }
    ifs.close();
    MSPROF_LOGI("send file: %s .data size: %lu", fileName.c_str(), dataSize);
}

void CollectEngine::InitFileChunkOfStoreData(
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk, const std::string &fileName)
{
    if (fileChunk != nullptr) {
        fileChunk->set_filename(fileName);
        fileChunk->set_offset(-1);
        fileChunk->set_islastchunk(false);
        fileChunk->set_needack(false);
        fileChunk->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

        analysis::dvvp::message::JobContext jobCtx;
        jobCtx.dev_id = std::to_string(collectionjobComnCfg_->devIdFlush);
        jobCtx.job_id = collectionjobComnCfg_->params->job_id;

        fileChunk->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    }
}

int CollectEngine::CollectStopJob(analysis::dvvp::message::StatusInfo &status)
{
    int ret = PROFILING_FAILED;
    status.status = analysis::dvvp::message::ERR;

    do {
        MSPROF_LOGI("Stop Job");

        if (!_is_started || collectionjobComnCfg_ == nullptr) {
            status.info = "collection engine has not been started";
            break;
        }
        for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
            if (CollectionJobV_[cnt].collectionJob == nullptr) {
                continue;
            }
            int retn = CollectionRegisterMgr::instance()->CollectionJobUnregisterAndStop(collectionjobComnCfg_->devId,
                                                                                         CollectionJobV_[cnt].jobTag);
            CollectionJobV_[cnt].jobCfg->jobParams.events.reset();
            CollectionJobV_[cnt].jobCfg->jobParams.cores.reset();
            if (retn != PROFILING_SUCCESS) {
                MSPROF_LOGD("Device %d Collection Job %d Unregister", collectionjobComnCfg_->devIdOnHost,
                            CollectionJobV_[cnt].jobTag);
            }
        }
        ret = PROFILING_SUCCESS;
    } while (0);
    return ret;
}

int CollectEngine::CollectStopReplay(analysis::dvvp::message::StatusInfo &status)
{
    int ret = PROFILING_FAILED;
    status.status = analysis::dvvp::message::ERR;

    do {
        MSPROF_LOGI("Stop");

        if (CollectStopJob(status) != PROFILING_SUCCESS) {
            break;
        }
        collectionjobComnCfg_->jobCtx.reset();
        status.status = analysis::dvvp::message::SUCCESS;
        ret = PROFILING_SUCCESS;
    } while (0);

    return ret;
}

int CollectEngine::CollectStop(analysis::dvvp::message::StatusInfo &status,
                               bool isReset /* = false */)
{
    int ret = PROFILING_FAILED;
    status.status = analysis::dvvp::message::SUCCESS;
    if (!collectionjobComnCfg_) {
        status.status = analysis::dvvp::message::ERR;
        MSPROF_LOGE("[CollectStop]Config is null");
        return PROFILING_FAILED;
    }
    if (_is_started) {
        MSPROF_LOGI("stop collect ...");
        ret = CollectStopReplay(status);
        if (ret != PROFILING_SUCCESS) {
            status.status = analysis::dvvp::message::ERR;
            MSPROF_LOGE("[CollectStop]Collect stop failed");
        }
        MSPROF_LOGI("Data collection finished under agent mode.");
    }
    (void)CleanupResults();
    // finish data session
    ret = analysis::dvvp::device::CollectionEntry::instance()->FinishCollection(
        collectionjobComnCfg_->devIdFlush, collectionjobComnCfg_->params->job_id);
    if (ret != PROFILING_SUCCESS) {
        status.status = analysis::dvvp::message::ERR;
        MSPROF_LOGE("[CollectStop]FinishCollection failed");
    }
    ProfChannelManager::instance()->UnInit();
    _is_started = false;
    _is_stop = true;
    if (status.status == analysis::dvvp::message::ERR) {
        status.info = "Stop profiling failed, please check it or see log for more info";
        return PROFILING_FAILED;
    } else {
        return PROFILING_SUCCESS;
    }
}

int CollectEngine::CheckDeviceValid()
{
    int ret = PROFILING_FAILED;
    do {
        MSPROF_LOGI("CollectEngine checking params");
        bool isValid = ParamValidation::instance()->CheckProfilingParams(collectionjobComnCfg_->params);
        if (!isValid) {
            MSPROF_LOGE("[CollectEngine::CheckDeviceValid]Failed to check profiling params");
            break;
        }
        return PROFILING_SUCCESS;
    } while (0);
    return ret;
}

int CollectEngine::InitBeforeCollectStart(const std::string &sampleConfig,
    analysis::dvvp::message::StatusInfo &status, std::string &projectDir)
{
    _is_started = false;
    _is_stop = false;
    _sample_config = sampleConfig;
    do {
        MSVP_MAKE_SHARED0_BREAK(collectionjobComnCfg_->params, analysis::dvvp::message::ProfileParams);
        if (!collectionjobComnCfg_->params->FromString(sampleConfig)) {
            MSPROF_LOGE("Failed to parse sampleConfig: \"%s\".", sampleConfig.c_str());
            status.info = "invalid sample configuration";
            break;
        }
        if (CheckDeviceValid() != PROFILING_SUCCESS) {
            MSPROF_LOGE("[InitBeforeCollectStart]CheckDeviceValid failed.");
            break;
        }
        MSPROF_LOGI("parse sampleConfig: \"%s\".", sampleConfig.c_str());
        int ret = CreateTmpDir(tmpResultDir_);
        if (ret != PROFILING_SUCCESS) {
            status.info = "Failed to create tmp root result directory.";
            break;
        }
        return PROFILING_SUCCESS;
    } while (0);
    return PROFILING_FAILED;
}

void CollectEngine::SetCollectControlCpuJobParam(const std::string &projectDir)
{
    std::vector<std::string> ctrlCpuDataPathVec;
    std::string perfDataDir =
        Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPerfDataDir(collectionjobComnCfg_->devId);
    ctrlCpuDataPathVec.push_back(perfDataDir);
    ctrlCpuDataPathVec.push_back("ai_ctrl_cpu.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(ctrlCpuDataPathVec);
    MSPROF_LOGI("ctrl_cpu_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectTsCpuJobParam(const std::string &projectDir)
{
    std::vector<std::string> tsCpuDataPathVec;
    tsCpuDataPathVec.push_back(projectDir);
    tsCpuDataPathVec.push_back("data");
    tsCpuDataPathVec.push_back("tscpu.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(tsCpuDataPathVec);
    MSPROF_LOGI("ts_cpu_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[TS_CPU_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectAiCoreJobParam(const std::string &projectDir)
{
    std::vector<std::string> aiCoreDataPathVec;
    aiCoreDataPathVec.push_back(projectDir);
    aiCoreDataPathVec.push_back("data");
    aiCoreDataPathVec.push_back("aicore.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(aiCoreDataPathVec);
    MSPROF_LOGI("ai_core_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
    CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
    isAicoreSampleBased_ = false;
    if (collectionjobComnCfg_->params->ai_core_profiling_mode.compare(PROFILING_MODE_SAMPLE_BASED) == 0) {
        MSPROF_LOGI("SetCollectAiCoreJobParam set aicoresamplebase");
        isAicoreSampleBased_ = true;
    }
    isAicoreTaskBased_ = false;
    if (collectionjobComnCfg_->params->ai_core_profiling_mode.compare(PROFILING_MODE_TASK_BASED) == 0) {
        MSPROF_LOGI("SetCollectAiCoreJobParam set aicoretaskbase");
        isAicoreTaskBased_ = true;
    }
}

void CollectEngine::SetCollectAivJobParam(const std::string &projectDir)
{
    std::vector<std::string> aivDataPathVec;
    aivDataPathVec.push_back(projectDir);
    aivDataPathVec.push_back("data");
    aivDataPathVec.push_back("aiVectorCore.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(aivDataPathVec);
    MSPROF_LOGI("ai_vector_core_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
    CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
    isAivSampleBased_ = false;
    if (collectionjobComnCfg_->params->aiv_profiling_mode.compare(PROFILING_MODE_SAMPLE_BASED) == 0) {
        isAivSampleBased_ = true;
    }
    isAivTaskBased_ = false;
    if (collectionjobComnCfg_->params->aiv_profiling_mode.compare(PROFILING_MODE_TASK_BASED) == 0) {
        isAivTaskBased_ = true;
    }
}

void CollectEngine::SetCollectTsFwJobParam(const std::string &projectDir)
{
    std::vector<std::string> tsFwDataPathVec;
    tsFwDataPathVec.push_back(projectDir);
    tsFwDataPathVec.push_back("data");
    tsFwDataPathVec.push_back("ts_track.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(tsFwDataPathVec);
    MSPROF_LOGI("ts_fw_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[TS_TRACK_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectTs1FwJobParam(const std::string &projectDir)
{
    std::vector<std::string> tsFwDataPathVec;
    tsFwDataPathVec.push_back(projectDir);
    tsFwDataPathVec.push_back("data");
    tsFwDataPathVec.push_back("ts_track.aiv_data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(tsFwDataPathVec);
    MSPROF_LOGI("ts1_fw_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[AIV_TS_TRACK_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectLlcJobParam(const std::string &projectDir)
{
    // llc_data path
    std::vector<std::string> llcDataPathVec;
    llcDataPathVec.push_back(projectDir);
    llcDataPathVec.push_back("data");
    llcDataPathVec.push_back("llc.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(llcDataPathVec);
    MSPROF_LOGI("llc_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[LLC_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectDdrJobParam(const std::string &projectDir)
{
    // ddr_data path
    std::vector<std::string> ddrDataPathVec;
    ddrDataPathVec.push_back(projectDir);
    ddrDataPathVec.push_back("data");
    ddrDataPathVec.push_back("ddr.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(ddrDataPathVec);
    MSPROF_LOGI("ddr_data_path = %s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[DDR_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectHbmJobParam(const std::string &projectDir)
{
    // hbm_data path
    std::vector<std::string> hbmDataPathVec;
    hbmDataPathVec.push_back(projectDir);
    hbmDataPathVec.push_back("data");
    hbmDataPathVec.push_back("hbm.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(hbmDataPathVec);
    MSPROF_LOGI("hbm_data_path = %s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[HBM_DRV_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectHwTsJobParam(const std::string &projectDir)
{
    // hwts log data path
    std::vector<std::string> hwtsLogPathV;
    hwtsLogPathV.push_back(projectDir);
    hwtsLogPathV.push_back("data");
    hwtsLogPathV.push_back("hwts.data");
    CollectionJobV_[HWTS_LOG_COLLECTION_JOB].jobCfg->jobParams.dataPath =
        analysis::dvvp::common::utils::Utils::JoinPath(hwtsLogPathV);
}

void CollectEngine::SetCollectHwTs1JobParam(const std::string &projectDir)
{
    // hwts log data path
    std::vector<std::string> hwtsLogPathV;
    hwtsLogPathV.push_back(projectDir);
    hwtsLogPathV.push_back("data");
    hwtsLogPathV.push_back("hwts.aiv_data");
    CollectionJobV_[AIV_HWTS_LOG_COLLECTION_JOB].jobCfg->jobParams.dataPath =
        analysis::dvvp::common::utils::Utils::JoinPath(hwtsLogPathV);
}

void CollectEngine::SetCollectFmkJobParam(const std::string &projectDir)
{
    // fmk data path
    std::vector<std::string> fmkPathV;
    fmkPathV.push_back(projectDir);
    fmkPathV.push_back("data");
    fmkPathV.push_back("training_trace.data");
    CollectionJobV_[FMK_COLLECTION_JOB].jobCfg->jobParams.dataPath =
        analysis::dvvp::common::utils::Utils::JoinPath(fmkPathV);
}

void CollectEngine::SetCollectL2CacheJobParam(const std::string &projectDir)
{
    std::vector<std::string> l2CacheDataPathV;
    l2CacheDataPathV.push_back(projectDir);
    l2CacheDataPathV.push_back("data");
    l2CacheDataPathV.push_back("l2_cache.data");
    std::string dataPath = analysis::dvvp::common::utils::Utils::JoinPath(l2CacheDataPathV);
    MSPROF_LOGI("l2_cache_data_path=%s", Utils::BaseName(dataPath).c_str());
    CollectionJobV_[L2_CACHE_TASK_COLLECTION_JOB].jobCfg->jobParams.dataPath = dataPath;
}

void CollectEngine::SetCollectJobParam(const std::string &projectDir)
{
    SetCollectControlCpuJobParam(projectDir);
    SetCollectTsCpuJobParam(projectDir);
    SetCollectAiCoreJobParam(projectDir);
    SetCollectAivJobParam(projectDir);
    SetCollectTsFwJobParam(projectDir);
    SetCollectTs1FwJobParam(projectDir);
    SetCollectLlcJobParam(projectDir);
    SetCollectDdrJobParam(projectDir);
    SetCollectHbmJobParam(projectDir);
    SetCollectHwTsJobParam(projectDir);
    SetCollectHwTs1JobParam(projectDir);
    SetCollectFmkJobParam(projectDir);
    SetCollectL2CacheJobParam(projectDir);
}

int CollectEngine::CollectStart(const std::string &sampleConfig,
                                analysis::dvvp::message::StatusInfo &status)
{
    int ret = PROFILING_FAILED;
    status.status = analysis::dvvp::message::ERR;
    do {
        if (!isInited_ || collectionjobComnCfg_ == nullptr) {
            status.info = "Collection engine was not initialized.";
            break;
        }
        analysis::dvvp::common::utils::Utils::GetTime(startRealtime_, startMono_, cntvct_);
        MSPROF_LOGI("CollectStart startRealtime=%llu ns, startMono=%llu ns, cntvct=%llu",
            startRealtime_, startMono_, cntvct_);
        std::string projectDir;
        if (InitBeforeCollectStart(sampleConfig, status, projectDir) == PROFILING_FAILED) {
            break;
        }
        MSPROF_LOGI("Collection started, sample config:%s", sampleConfig.c_str());
        MSPROF_LOGI("Saving data to tmp dir:%s", Utils::BaseName(projectDir).c_str());
        SetCollectJobParam(projectDir);
        if (ProfChannelManager::instance()->Init() != PROFILING_SUCCESS) {
            MSPROF_LOGE("[CollectEngine::CollectStart]Failed to init channel poll");
            ret = PROFILING_FAILED;
            break;
        }
        if (DrvChannelsMgr::instance()->GetAllChannels(collectionjobComnCfg_->devId) != PROFILING_SUCCESS) {
            MSPROF_LOGE("[CollectEngine::CollectStart]Failed to GetAllChannels");
            ret = PROFILING_FAILED;
            break;
        }
        _is_started = true;
        ret = PROFILING_SUCCESS;
    } while (0);
    if (ret != PROFILING_SUCCESS) {
        (void)CleanupResults();
    } else {
        status.status = analysis::dvvp::message::SUCCESS;
    }
    return ret;
}

int CollectEngine::SendFiles(const std::string &path, const std::string &destDir)
{
    int ret = PROFILING_SUCCESS;
    std::vector<std::string> files;
    std::vector<std::string> rootDir;
    rootDir.push_back(tmpResultDir_);
    std::string rootPath = analysis::dvvp::common::utils::Utils::JoinPath(rootDir);
    analysis::dvvp::common::utils::Utils::GetFiles(path, true, files);
    for (size_t ii = 0; ii < files.size(); ++ii) {
        auto filePath = files[ii];
        std::string fileName;
        ret = analysis::dvvp::common::utils::Utils::RelativePath(filePath, rootPath, fileName);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to get relative path for: %s", Utils::BaseName(filePath).c_str());
            continue;
        }
        StoreData(filePath, fileName);
        MSPROF_LOGI("filePath:%s, fileName:%s, destDir:%s",
                    Utils::BaseName(filePath).c_str(), fileName.c_str(), Utils::BaseName(destDir).c_str());
    }
    return ret;
}

std::string CollectEngine::GetEventsStr(const std::vector<std::string> &events,
                                        const std::string &separator /* = "," */)
{
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;

    return builder.Join(events, separator);
}

std::string CollectEngine::BindFileWithChannel(const std::string &fileName, unsigned int channelId)
{
    std::stringstream ssProfDataFilePath;

    ssProfDataFilePath << fileName;
    ssProfDataFilePath << ".";
    ssProfDataFilePath << channelId;

    return ssProfDataFilePath.str();
}

void CollectEngine::CreateCollectionJobArray()
{
    if (Utils::IsDeviceMapping()) {
        // on device mapping scene, device just proc system profiling job
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].collectionJob, ProfCtrlcpuJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSSTAT_PROC_COLLECTION_JOB].collectionJob, ProfSysStatJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSMEM_PROC_COLLECTION_JOB].collectionJob, ProfSysMemJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[ALLPID_PROC_COLLECTION_JOB].collectionJob, ProfAllPidsJob);
    } else {
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[DDR_DRV_COLLECTION_JOB].collectionJob, ProfDdrJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HBM_DRV_COLLECTION_JOB].collectionJob, ProfHbmJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[LLC_DRV_COLLECTION_JOB].collectionJob, ProfLlcJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[DVPP_COLLECTION_JOB].collectionJob, ProfDvppJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[PCIE_DRV_COLLECTION_JOB].collectionJob, ProfPcieJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[NIC_COLLECTION_JOB].collectionJob, ProfNicJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HCCS_DRV_COLLECTION_JOB].collectionJob, ProfHccsJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[ROCE_DRV_COLLECTION_JOB].collectionJob, ProfRoceJob);
        // ts
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[TS_CPU_DRV_COLLECTION_JOB].collectionJob, ProfTscpuJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[TS_TRACK_DRV_COLLECTION_JOB].collectionJob, ProfTsTrackJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_TS_TRACK_DRV_COLLECTION_JOB].collectionJob, ProfAivTsTrackJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].collectionJob, ProfAicoreJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].collectionJob, ProfAicoreTaskBasedJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_SAMPLE_DRV_COLLECTION_JOB].collectionJob, ProfAivJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_TASK_DRV_COLLECTION_JOB].collectionJob, ProfAivTaskBasedJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[HWTS_LOG_COLLECTION_JOB].collectionJob, ProfHwtsLogJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[AIV_HWTS_LOG_COLLECTION_JOB].collectionJob, ProfAivHwtsLogJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[FMK_COLLECTION_JOB].collectionJob, ProfFmkJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[L2_CACHE_TASK_COLLECTION_JOB].collectionJob, ProfL2CacheTaskJob);
        // system
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[CTRLCPU_PERF_COLLECTION_JOB].collectionJob, ProfCtrlcpuJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSSTAT_PROC_COLLECTION_JOB].collectionJob, ProfSysStatJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[SYSMEM_PROC_COLLECTION_JOB].collectionJob, ProfSysMemJob);
        MSVP_MAKE_SHARED0_VOID(CollectionJobV_[ALLPID_PROC_COLLECTION_JOB].collectionJob, ProfAllPidsJob);
    }
    MSPROF_LOGI("CreateCollectionJobArray to set jobCfg");

    for (int cnt = 0; cnt < NR_MAX_COLLECTION_JOB; cnt++) {
        // check job availability
        if (CollectionJobV_[cnt].collectionJob != nullptr) {
            MSVP_MAKE_SHARED0_VOID(CollectionJobV_[cnt].jobCfg, CollectionJobCfg);
            CollectionJobV_[cnt].jobTag = (ProfCollectionJobE)cnt;
            CollectionJobV_[cnt].jobCfg->jobParams.jobTag = (ProfCollectionJobE)cnt;
            CollectionJobV_[cnt].jobCfg->comParams = collectionjobComnCfg_;
        }
    }
}
}  // namespace device
}  // namespace dvvp
}  // namespace analysis
