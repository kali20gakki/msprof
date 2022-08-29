/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: prof manager function
 * Author: zcj
 * Create: 2020-07-18
 */

#include "prof_acl_mgr.h"

#include <iomanip>
#include <google/protobuf/util/json_util.h>

#include "config/config.h"
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "msprof_callback_handler.h"
#include "op_desc_parser.h"
#include "platform/platform.h"
#include "prof_manager.h"
#include "prof_params_adapter.h"
#include "proto/profiler.pb.h"
#include "proto/profiler_ext.pb.h"
#include "transport/parser_transport.h"
#include "transport/transport.h"
#include "transport/file_transport.h"
#include "transport/uploader.h"
#include "transport/uploader_mgr.h"
#include "transport/hash_data.h"
#include "param_validation.h"
#include "command_handle.h"
#include "prof_channel_manager.h"
#include "msprof_callback_impl.h"
#include "prof_ge_core.h"
#include "msprof_tx_manager.h"
#include "utils/utils.h"
#include "mmpa_api.h"
#include "params_adapter_impl.h"

using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::host;
using namespace Analysis::Dvvp::Host::Adapter;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::JobWrapper;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Msprof::MsprofTx;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
using namespace Collector::Dvvp::ParamsAdapter;

namespace Msprofiler {
namespace Api {
// callback of Device
void DeviceResponse(int devId)
{
    MSPROF_LOGI("DeviceResponse of device %d called", devId);
    Msprofiler::Api::ProfAclMgr::instance()->HandleResponse(static_cast<uint32_t>(devId));
}

// internal interface
uint64_t ProfGetOpExecutionTime(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data != nullptr) {
        return Analysis::Dvvp::Analyze::OpDescParser::GetOpExecutionTime(data, len, index);
    }
    return 0;
}

ProfAclMgr::ProfAclMgr() : isReady_(false), mode_(WORK_MODE_OFF), params_(nullptr), dataTypeConfig_(0),
                           startIndex_(0) {}

ProfAclMgr::~ProfAclMgr()
{
    (void)UnInit();
}

ProfAclMgr::DeviceResponseHandler::DeviceResponseHandler(const uint32_t devId) : devId_(devId) {}

ProfAclMgr::DeviceResponseHandler::~DeviceResponseHandler() {}

void ProfAclMgr::DeviceResponseHandler::HandleResponse()
{
    MSPROF_EVENT("Device %u finished starting", devId_);
    StopNoWait();
    cv_.notify_one();
}

void ProfAclMgr::DeviceResponseHandler::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    static const int RESPONSE_TIME_S = 30; // device response timeout: 30s
    std::unique_lock<std::mutex> lk(mtx_);
    MSPROF_EVENT("Device %u started to wait for response", devId_);
    cv_.wait_for(lk, std::chrono::seconds(RESPONSE_TIME_S), [&] { return this->IsQuit(); });
    MSPROF_EVENT("Device %u finished waiting for response", devId_);
}

void ProfAclMgr::PrintWorkMode(WorkMode mode)
{
    auto iter = WorkModeStr_.find(mode);
    if (iter != WorkModeStr_.end()) {
        MSPROF_LOGW("%s, mode:%d", iter->second.c_str(), mode);
    } else {
        MSPROF_LOGE("Find WorkModeStr failed, mode:%d", mode);
        MSPROF_INNER_ERROR("EK9999", "Find WorkModeStr failed, mode:%d", mode);
    }
}

int ProfAclMgr::CallbackInitPrecheck()
{
    if (mode_ == WORK_MODE_OFF) {
        return PROFILING_SUCCESS;
    }
    PrintWorkMode(mode_);
    return PROFILING_FAILED;
}

int ProfAclMgr::MsprofTxSwitchPrecheck()
{
    if (mode_ == WORK_MODE_CMD) {
        PrintWorkMode(mode_);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfAclMgr::CallbackFinalizePrecheck()
{
    if (mode_ == WORK_MODE_CMD) {
        return PROFILING_SUCCESS;
    }
    PrintWorkMode(mode_);
    return PROFILING_FAILED;
}

int ProfAclMgr::ProfInitPrecheck()
{
    if (mode_ == WORK_MODE_OFF) {
        return ACL_SUCCESS;
    }
    if (mode_ == WORK_MODE_CMD) {
        MSPROF_LOGW("Acl profiling api mode is disabled because working on cmd mode");
        return ACL_ERROR_PROF_ALREADY_RUN;
    }
    if (mode_ == WORK_MODE_API_CTRL) {
        MSPROF_LOGE("Acl profiling is already inited");
        MSPROF_INNER_ERROR("EK9999", "Acl profiling is already inited");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }
    MSPROF_LOGE("Acl profiling api mode conflict with other api mode %d", mode_);
    MSPROF_INNER_ERROR("EK9999", "Acl profiling api mode conflict with other api mode %d", mode_);
    return ACL_ERROR_PROF_API_CONFLICT;
}

int ProfAclMgr::ProfStartPrecheck()
{
    if (mode_ == WORK_MODE_API_CTRL) {
        return ACL_SUCCESS;
    }
    if (mode_ == WORK_MODE_CMD) {
        MSPROF_LOGW("Acl profiling api mode is disabled because working on cmd mode");
        return ACL_ERROR_PROF_ALREADY_RUN;
    }
    if (mode_ == WORK_MODE_OFF) {
        MSPROF_LOGE("Acl profiling api mode is not inited");
        return ACL_ERROR_PROF_NOT_RUN;
    }
    MSPROF_LOGE("Acl profiling api ctrl conflicts with other api mode %d", mode_);
    MSPROF_INNER_ERROR("EK9999", "Acl profiling api ctrl conflicts with other api mode %d", mode_);
    return ACL_ERROR_PROF_API_CONFLICT;
}

int ProfAclMgr::ProfStopPrecheck()
{
    return ProfStartPrecheck();
}

int ProfAclMgr::ProfFinalizePrecheck()
{
    return ProfStartPrecheck();
}

int ProfAclMgr::ProfSubscribePrecheck()
{
    if (mode_ == WORK_MODE_SUBSCRIBE || mode_ == WORK_MODE_OFF) {
        return ACL_SUCCESS;
    }
    if (mode_ == WORK_MODE_CMD) {
        MSPROF_LOGW("Acl profiling api mode is disabled because working on cmd mode");
        return ACL_ERROR_PROF_ALREADY_RUN;
    }
    MSPROF_LOGE("Acl profiling api subscribe conflicts with other api mode %d", mode_);
    MSPROF_INNER_ERROR("EK9999", "Acl profiling api subscribe conflicts with other api mode %d", mode_);
    return ACL_ERROR_PROF_API_CONFLICT;
}

int ProfAclMgr::ProfUnSubscribePrecheck()
{
    if (mode_ == WORK_MODE_SUBSCRIBE) {
        return ACL_SUCCESS;
    }
    if (mode_ == WORK_MODE_CMD) {
        MSPROF_LOGW("Acl profiling api mode is disabled because working on cmd mode");
        return ACL_ERROR_PROF_ALREADY_RUN;
    }
    if (mode_ == WORK_MODE_OFF) {
        MSPROF_LOGE("No model info is subscribed");
        MSPROF_INNER_ERROR("EK9999", "No model info is subscribed");
        return ACL_ERROR_INVALID_MODEL_ID;
    }
    MSPROF_LOGE("Acl profiling api subscribe conflicts with other api mode %d", mode_);
    MSPROF_INNER_ERROR("EK9999", "Acl profiling api subscribe conflicts with other api mode %d", mode_);
    return ACL_ERROR_PROF_API_CONFLICT;
}

void ProfAclMgr::SetModeToCmd()
{
    mode_ = WORK_MODE_CMD;
}

void ProfAclMgr::SetModeToOff()
{
    mode_ = WORK_MODE_OFF;
}

bool ProfAclMgr::IsCmdMode()
{
    if (mode_ == WORK_MODE_CMD) {
        return true;
    }
    return false;
}

bool ProfAclMgr::IsModeOff()
{
    if (mode_ == WORK_MODE_OFF) {
        return true;
    }
    return false;
}

/**
 * Init resources for acl api call
 */
int ProfAclMgr::Init()
{
    MSPROF_LOGI("ProfAclMgr Init");
    if (isReady_) {
        return PROFILING_SUCCESS;
    }

    if (ProfManager::instance()->AclInit() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init ProfManager");
        MSPROF_INNER_ERROR("EK9999", "Failed to init ProfManager");
        return PROFILING_FAILED;
    }

    isReady_ = true;
    return PROFILING_SUCCESS;
}

int ProfAclMgr::UnInit()
{
    for (auto iter = enginMap_.begin(); iter != enginMap_.end(); iter++) {
        (void)iter->second->UnInit();
    }
    enginMap_.clear();
    return PROFILING_SUCCESS;
}

/**
 * Handle ProfInit
 */
int32_t ProfAclMgr::ProfAclInit(const std::string &profResultPath)
{
    MSPROF_EVENT("Received ProfAclInit request from acl");
    std::lock_guard<std::mutex> lk(mtx_);
    if (!isReady_) {
        MSPROF_LOGE("Profiling is not ready");
        MSPROF_INNER_ERROR("EK9999", "Profiling is not ready");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    if (mode_ != WORK_MODE_OFF) {
        MSPROF_LOGE("Profiling already inited");
        MSPROF_INNER_ERROR("EK9999", "Profiling already inited");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }

    MSPROF_LOGI("Input profInitCfg: %s", Utils::BaseName(profResultPath).c_str());
    std::string path = Utils::RelativePathToAbsolutePath(profResultPath);
    if (path.empty()) {
        MSPROF_LOGE("Input profResultPath is empty");
        MSPROF_INNER_ERROR("EK9999", "Input profResultPath is empty");
        return ACL_ERROR_INVALID_FILE;
    }
    if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to create dir: %s", Utils::BaseName(path).c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to create dir: %s", Utils::BaseName(path).c_str());
        return ACL_ERROR_INVALID_FILE;
    }
    path = Utils::CanonicalizePath(path);
    if (path.empty()) {
        MSPROF_LOGE("Invalid path of profInit");
        return ACL_ERROR_INVALID_FILE;
    }

    // Check path is valid
    if (!Utils::IsDirAccessible(path)) {
        MSPROF_LOGE("Dir is not accessible: %s", Utils::BaseName(path).c_str());
        MSPROF_INNER_ERROR("EK9999", "Dir is not accessible: %s", Utils::BaseName(path).c_str());
        return ACL_ERROR_INVALID_FILE;
    }

    // Gen sub dir by time and create it
    resultPath_ = path;
    MSPROF_LOGI("Base directory: %s", Utils::BaseName(resultPath_).c_str());

    // reset device index
    devUuid_.clear();

    mode_ = WORK_MODE_API_CTRL;
    return ACL_SUCCESS;
}

WorkMode ProfAclMgr::GetWorkMode()
{
    return mode_;
}

bool ProfAclMgr::IsInited()
{
    return mode_ != WORK_MODE_OFF;
}

/**
 * Handle ProfStartProfiling
 */
int ProfAclMgr::ProfAclStart(PROF_CONF_CONST_PTR profStartCfg)
{
    MSPROF_EVENT("Received ProfAclStart request from acl");
    std::lock_guard<std::mutex> lk(mtx_);
    if (profStartCfg == nullptr) {
        MSPROF_LOGE("Startcfg is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }

    if (mode_ != WORK_MODE_API_CTRL) {
        MSPROF_LOGE("Profiling has not been inited");
        return ACL_ERROR_PROF_NOT_RUN;
    }

    // check devices
    int ret = CheckDeviceTask(profStartCfg);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    baseDir_ = Utils::CreateTaskId(startIndex_);
    startIndex_++;

    ret = ProfStartAiCpuTrace(profStartCfg->dataTypeConfig, profStartCfg->devNums, profStartCfg->devIdList);
    if (ret != PROFILING_SUCCESS) {
        return ACL_ERROR_PROFILING_FAILURE;
    }
    // generate params
    MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, ACL_ERROR_PROFILING_FAILURE);
    params_->result_dir = resultPath_;
    params_->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    auto paramsAdapter = AclApiParamAdapter();
    ret = paramsAdapter.GetParamFromInputCfg(profStartCfg, argsArr_, params_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[qqq]");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    return ACL_ERROR_PROFILING_FAILURE;
    SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature = nullptr;
    MSVP_MAKE_SHARED0_RET(feature, analysis::dvvp::proto::MsProfStartReq, ACL_ERROR_PROFILING_FAILURE);
    ProfStartCfgToMsprofCfg(profStartCfg->dataTypeConfig, profStartCfg->aicoreMetrics, feature);
    GenerateSystemTraceConf(profStartCfg->dataTypeConfig, profStartCfg->aicoreMetrics, feature, params_);
    ProfParamsAdapter::instance()->UpdateSampleConfig(feature, params_);
    ProfParamsAdapter::instance()->ProfStartCfgToParamsCfg(profStartCfg->dataTypeConfig, params_);
    params_->ts_keypoint = MSVP_PROF_ON;

    for (uint32_t i = 0; i < profStartCfg->devNums; i++) {
        uint32_t devId = profStartCfg->devIdList[i];
        MSPROF_LOGI("Process ProfAclStart of device %u", devId);
        if (devId == DEFAULT_HOST_ID) {
            dataTypeConfig_ = profStartCfg->dataTypeConfig;
            MsprofTxHandle();
            continue;
        }
        ret = StartDeviceTask(devId, params_);
        if (ret != ACL_SUCCESS) {
            return ret;
        }
        devTasks_[devId].dataTypeConfig = profStartCfg->dataTypeConfig;
    }
    WaitAllDeviceResponse();
    return ACL_SUCCESS;
}

/**
 * Handle ProfStopProfiling
 */
int ProfAclMgr::ProfAclStop(PROF_CONF_CONST_PTR profStopCfg)
{
    MSPROF_EVENT("Received ProfAclStop request from acl");
    std::lock_guard<std::mutex> lk(mtx_);
    if (profStopCfg == nullptr) {
        MSPROF_LOGE("Stopcfg is nullptr");
        MSPROF_INNER_ERROR("EK9999", "Stopcfg is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }

    if (mode_ != WORK_MODE_API_CTRL) {
        MSPROF_LOGE("Profiling has not been inited");
        return ACL_ERROR_PROF_NOT_RUN;
    }
    // check device is started and
    for (uint32_t i = 0; i < profStopCfg->devNums; i++) {
        uint32_t devId = profStopCfg->devIdList[i];
        auto iter = devTasks_.find(devId);
        if (iter != devTasks_.end()) {
            if (iter->second.dataTypeConfig != profStopCfg->dataTypeConfig) {
                MSPROF_LOGE("DataTypeConfig stop: 0x%lx different from start: 0x%lx",
                    profStopCfg->dataTypeConfig, iter->second.dataTypeConfig);
                MSPROF_INNER_ERROR("EK9999", "DataTypeConfig stop: 0x%lx different from start: 0x%lx",
                    profStopCfg->dataTypeConfig, iter->second.dataTypeConfig);
                return ACL_ERROR_INVALID_PROFILING_CONFIG;
            }
        } else {
            MSPROF_LOGE("Device %u has not been started", devId);
            return ACL_ERROR_PROF_NOT_RUN;
        }
    }
    // stop devices
    int ret = ACL_SUCCESS;
    for (uint32_t i = 0; i < profStopCfg->devNums; i++) {
        uint32_t devId = profStopCfg->devIdList[i];
        MSPROF_LOGI("Processing ProfAclStop of device %u", devId);
        auto iter = devTasks_.find(devId);
        if (iter != devTasks_.end()) {
            HashData::instance()->SaveHashData(devId);
            iter->second.params->is_cancel = true;
            if (ProfManager::instance()->IdeCloudProfileProcess(iter->second.params) != PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to stop profiling on device %u", devId);
                MSPROF_INNER_ERROR("EK9999", "Failed to stop profiling on device %u", devId);
                ret = ACL_ERROR_PROFILING_FAILURE;
            }
            devTasks_.erase(iter);
        }
    }

    return ret;
}

/**
 * Handle ProfFinalize
 */
int ProfAclMgr::ProfAclFinalize()
{
    MSPROF_EVENT("Received ProfAclFinalize request from acl");
    std::lock_guard<std::mutex> lk(mtx_);
    if (mode_ != WORK_MODE_API_CTRL) {
        MSPROF_LOGE("Profiling has not been inited");
        return ACL_ERROR_PROF_NOT_RUN;
    }
    for (auto iter = devTasks_.begin(); iter != devTasks_.end(); iter++) {
        iter->second.params->is_cancel = true;
        if (ProfManager::instance()->IdeCloudProfileProcess(iter->second.params) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to finalize profiling on device %u", iter->first);
            MSPROF_INNER_ERROR("EK9999", "Failed to finalize profiling on device %u", iter->first);
        }
    }
    UploaderMgr::instance()->DelAllUploader();
    MsprofTxManager::instance()->UnInit();
    devTasks_.clear();

    mode_ = WORK_MODE_OFF;
    if (Utils::IsClusterRunEnv()) {
        std::string jobDir = resultPath_ + MSVP_SLASH + baseDir_;
        if (Utils::CloudAnalyze(jobDir) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Can't Analyze Data on Cloud. Path: %s, Please do it by yourself.", jobDir.c_str());
        }
    }
    return ACL_SUCCESS;
}

/**
 * Handle ProfAclGetDataTypeConfig
 */
int ProfAclMgr::ProfAclGetDataTypeConfig(const uint32_t devId, uint64_t &dataTypeConfig)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = devTasks_.find(devId);
    if (iter == devTasks_.end()) {
        MSPROF_LOGE("Device %u has not been started", devId);
        MSPROF_INNER_ERROR("EK9999", "Device %u has not been started", devId);
        return ACL_ERROR_PROF_NOT_RUN;
    }
    dataTypeConfig = iter->second.dataTypeConfig;
    MSPROF_LOGI("Get dataTypeConfig 0x%lx of device %u", dataTypeConfig, devId);
    return ACL_SUCCESS;
}

/**
 * Handle response from device
 */
void ProfAclMgr::HandleResponse(const uint32_t devId)
{
    auto iter = devResponses_.find(devId);
    if (iter != devResponses_.end()) {
        iter->second->HandleResponse();
    }
}

uint64_t ProfAclMgr::ProfAclGetDataTypeConfig(PROF_SUB_CONF_CONST_PTR profSubscribeConfig)
{
    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("SubscribeConfig is nullptr");
        MSPROF_INNER_ERROR("EK9999", "SubscribeConfig is nullptr");
        return 0;
    }
    uint64_t dataTypeConfig = 0;
    if (profSubscribeConfig->timeInfo) {
        dataTypeConfig |= PROF_TASK_TIME;
    }
    if (profSubscribeConfig->aicoreMetrics != PROF_AICORE_NONE) {
        dataTypeConfig |= PROF_AICORE_METRICS;
    }
    return dataTypeConfig;
}

void ProfAclMgr::AddModelLoadConf(uint64_t &dataTypeConfig) const
{
    dataTypeConfig |= PROF_MODEL_LOAD;
}

void ProfAclMgr::AddAiCpuModelConf(uint64_t &dataTypeConfig) const
{
    dataTypeConfig |= PROF_KEYPOINT_TRACE_HELPER;
}

void ProfAclMgr::AddRuntimeTraceConf(uint64_t &dataTypeConfig) const
{
    dataTypeConfig |= PROF_RUNTIME_TRACE;
}

int ProfAclMgr::ProfAclModelSubscribe(const uint32_t modelId, const uint32_t devId,
                                      PROF_SUB_CONF_CONST_PTR profSubscribeConfig)
{
    if (profSubscribeConfig == nullptr || profSubscribeConfig->fd == nullptr) {
        MSPROF_LOGE("SubscribeConfig is nullptr");
        MSPROF_INNER_ERROR("EK9999", "SubscribeConfig is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (!(profSubscribeConfig->timeInfo) && profSubscribeConfig->aicoreMetrics == PROF_AICORE_NONE) {
        MSPROF_LOGE("SubscribeConfig is invalid");
        MSPROF_INNER_ERROR("EK9999", "SubscribeConfig is invalid");
        return ACL_ERROR_INVALID_PARAM;
    }
    MSPROF_EVENT("Received ProfAclModelSubscribe request from acl, device: %u, model: %u fd: %u",
        devId, modelId, profSubscribeConfig->fd);
    std::lock_guard<std::mutex> lk(mtx_);
    if (!isReady_) {
        MSPROF_LOGE("Profiling is not ready");
        MSPROF_INNER_ERROR("EK9999", "Profiling is not ready");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    if (mode_ == WORK_MODE_API_CTRL) {
        MSPROF_LOGE("Profiling already started in another mode");
        MSPROF_INNER_ERROR("EK9999", "Profiling already started in another mode");
        return ACL_ERROR_PROF_API_CONFLICT;
    }
    auto iter = subscribeInfos_.find(modelId);
    if (iter != subscribeInfos_.end() && iter->second.subscribed) {
        MSPROF_LOGE("Model %u has been subscribed", modelId);
        MSPROF_INNER_ERROR("EK9999", "Model %u has been subscribed", modelId);
        return ACL_ERROR_PROF_REPEAT_SUBSCRIBE;
    }

    auto iterDev = devTasks_.find(devId);
    if (iterDev != devTasks_.end()) {
        // device already started, check cfg and add fd to subscribe list
        return UpdateSubscribeInfo(modelId, devId, profSubscribeConfig);
    }
    // start device
    int ret = StartDeviceSubscribeTask(modelId, devId, profSubscribeConfig);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("StartDeviceSubscribeTask failed, Model:%u", modelId);
        MSPROF_INNER_ERROR("EK9999", "StartDeviceSubscribeTask failed, Model:%u", modelId);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    ret = Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback();
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("RegisterReporterCallback failed, Model:%u", modelId);
        MSPROF_INNER_ERROR("EK9999", "RegisterReporterCallback failed, Model:%u", modelId);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    uint32_t devIdList[1] = {devId};
    uint64_t dataTypeConfig = ProfAclGetDataTypeConfig(profSubscribeConfig);
    MSPROF_LOGI("Allocate subscription config to Runtime, dataTypeConfig 0x%lx", dataTypeConfig);
    return Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(devIdList, 1, dataTypeConfig);
}

int ProfAclMgr::ProfAclModelUnSubscribe(const uint32_t modelId)
{
    MSPROF_EVENT("Received ProfAclModelUnSubscribe request from acl, model: %u", modelId);
    std::lock_guard<std::mutex> lk(mtx_);
    if (!isReady_) {
        MSPROF_LOGE("Model %u has not been subscribed", modelId);
        return ACL_ERROR_INVALID_MODEL_ID;
    }
    auto iter = subscribeInfos_.find(modelId);
    if (iter == subscribeInfos_.end() || !iter->second.subscribed) {
        MSPROF_LOGE("Model %u has not been subscribed", modelId);
        std::string errorReason = "Model id has not been subscribed";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(modelId), "modelId", errorReason}));
        return ACL_ERROR_INVALID_MODEL_ID;
    }
    int ret = ACL_SUCCESS;
    auto iterDev = devTasks_.find(iter->second.devId);
    if (iterDev != devTasks_.end()) {
        iter->second.subscribed = false;
        iterDev->second.count--;
        MSPROF_LOGI("Model %u unsubscribed, device %u count: %u", modelId, iter->second.devId, iterDev->second.count);
        if (iterDev->second.count == 0) {
            uint32_t devIdList[1] = {iter->second.devId};
            uint64_t dataTypeConfig = iterDev->second.dataTypeConfig;
            MSPROF_LOGI("Allocate Unsubscription config to Runtime, dataTypeConfig 0x%lx", dataTypeConfig);
            ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStop(devIdList, 1, dataTypeConfig);
            RETURN_IF_NOT_SUCCESS(ret);
            iterDev->second.params->is_cancel = true;
            if (ProfManager::instance()->IdeCloudProfileProcess(iterDev->second.params) != PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to stop profiling on device %u", iterDev->first);
                ret = ACL_ERROR_PROFILING_FAILURE;
            }
            CloseSubscribeFd(iterDev->first);
            devTasks_.erase(iterDev);
        } else {
#if (defined(linux) || defined(__linux__))
            if (halProfDataFlush) {
                FlushAllData(std::to_string(iter->second.devId));
                CloseSubscribeFd(iterDev->first, modelId);
            }
#endif
        }
    }
    if (devTasks_.empty()) {
        MSPROF_LOGI("All model id unsubscribed, reset mode");
        UploaderMgr::instance()->DelAllUploader();
        mode_ = WORK_MODE_OFF;
    }

    return ret;
}

void ProfAclMgr::FlushAllData(const std::string &devId)
{
    // flush ai stack data
    Msprof::Engine::FlushAllModule();
    // flush drv data
    ProfChannelManager::instance()->FlushChannel();
    // flush parserTransport
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(devId, uploader);
    if (uploader != nullptr) {
        uploader->Flush();
        auto transport = uploader->GetTransport();
        if (transport != nullptr) {
            transport->WriteDone();
        }
    }
}

bool ProfAclMgr::IsModelSubscribed(const uint32_t modelId)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = subscribeInfos_.find(modelId);
    if (iter == subscribeInfos_.end()) {
        return false;
    }
    return iter->second.subscribed;
}

int ProfAclMgr::GetSubscribeFdForModel(const uint32_t modelId)
{
    std::lock_guard<std::mutex> lk(mtxSubscribe_);
    auto iter = subscribeInfos_.find(modelId);
    if (iter == subscribeInfos_.end()) {
        return -1;
    }
    return *(iter->second.fd);
}

void ProfAclMgr::GetRunningDevices(std::vector<uint32_t> &devIds)
{
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto iter = devTasks_.begin(); iter != devTasks_.end(); iter++) {
        devIds.push_back(iter->first);
    }
}

uint64_t ProfAclMgr::GetDeviceSubscribeCount(uint32_t modelId, uint32_t &devId)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto iterSub = subscribeInfos_.find(modelId);
    if (iterSub == subscribeInfos_.end()) {
        return 0;
    }
    devId = iterSub->second.devId;
    auto iterDev = devTasks_.find(devId);
    if (iterDev == devTasks_.end()) {
        return 0;
    }
    return (iterDev->second.count - 1);
}

uint64_t ProfAclMgr::GetCmdModeDataTypeConfig()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return dataTypeConfig_;
}

std::string ProfAclMgr::GetParamJsonStr()
{
    if (params_ == nullptr) {
        MSPROF_LOGW("ProfileParams is empty");
        return "{}";
    }
    return params_->ToString();
}

/**
 * Generate name of directory for device
 */
std::string ProfAclMgr::GenerateDevDirName(const std::string& devId)
{
    std::string resultDir = resultPath_ + MSVP_SLASH + baseDir_ + MSVP_SLASH + devUuid_[devId];
    return resultDir;
}

/**
 * Create dir and uploader
 */
int32_t ProfAclMgr::InitUploader(const std::string& devIdStr)
{
    if (mode_ == WORK_MODE_API_CTRL || mode_ == WORK_MODE_CMD) {
        return InitApiCtrlUploader(devIdStr);
    } else if (mode_ == WORK_MODE_SUBSCRIBE) {
        return InitSubscribeUploader(devIdStr);
    } else {
        MSPROF_LOGE("Profiling mode is off, no uploader can be inited");
        MSPROF_INNER_ERROR("EK9999", "Profiling mode is off, no uploader can be inited");
        return ACL_ERROR_PROFILING_FAILURE;
    }
}

int ProfAclMgr::RecordOutPut(const std::string &data)
{
    std::string envValue = Utils::GetEnvString(PROFILER_SAMPLE_CONFIG_ENV);
    if (envValue.empty()) {
        MSPROF_LOGI("RecordOutPut, not acl env mode");
        return PROFILING_SUCCESS;
    }
    if (data.empty()) {
        MSPROF_LOGI("RecordOutPut, data is empty");
        return PROFILING_SUCCESS;
    }
    std::string pidStr = std::to_string(params_->msprofBinPid);
    std::string recordFile = pidStr + MSVP_UNDERLINE + OUTPUT_RECORD;
    MSPROF_LOGE("[ddd]recordFile:%s", recordFile.c_str());
    std::string absolutePath = resultPath_ + MSVP_SLASH + recordFile;
    std::string profName = data + "\n";
    int ret = WriteFile(absolutePath, recordFile, profName);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfAclMgr::InitApiCtrlUploader(const std::string& devIdStr)
{
    std::lock_guard<std::mutex> lk(mtxUploader_);
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(devIdStr, uploader);
    if (uploader == nullptr) {
        // first start
        devUuid_[devIdStr] = Utils::CreateResultPath(devIdStr);
        std::string devDir = GenerateDevDirName(devIdStr);
        if (Utils::CreateDir(devDir) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to create device dir: %s", Utils::BaseName(devDir).c_str());
            MSPROF_INNER_ERROR("EK9999", "Failed to create device dir: %s", Utils::BaseName(devDir).c_str());
            Utils::PrintSysErrorMsg();
            return ACL_ERROR_INVALID_FILE;
        }
        std::string outPutStr = baseDir_;
        int ret = RecordOutPut(outPutStr);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGW("Failed to record output dir:%s, devId:%s",
                        Utils::BaseName(devUuid_[devIdStr]).c_str(), devIdStr.c_str());
        }
        auto transport = FileTransportFactory().CreateFileTransport(devDir, storageLimit_, true);
        if (transport == nullptr) {
            MSPROF_LOGE("Failed to create transport for device %s", devIdStr.c_str());
            MSPROF_INNER_ERROR("EK9999", "Failed to create transport for device %s", devIdStr.c_str());
            return ACL_ERROR_INVALID_FILE;
        }
        static const size_t uploaderCapacity = 4096; // 4096 : need more uploader capacity
        ret = UploaderMgr::instance()->CreateUploader(devIdStr, transport, uploaderCapacity);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to create uploader for device %s", devIdStr.c_str());
            MSPROF_INNER_ERROR("EK9999", "Failed to create uploader for device %s", devIdStr.c_str());
            return ACL_ERROR_PROFILING_FAILURE;
        }
    }
    return ACL_SUCCESS;
}

int ProfAclMgr::InitSubscribeUploader(const std::string& devIdStr)
{
    std::lock_guard<std::mutex> lk(mtxUploader_);
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(devIdStr, uploader);
    if (uploader == nullptr) {
        // create transport2 and uploader2
        SHARED_PTR_ALIA<PipeTransport> pipeTransport = nullptr;
        MSVP_MAKE_SHARED0_RET(pipeTransport, PipeTransport, ACL_ERROR_PROFILING_FAILURE);
        SHARED_PTR_ALIA<Uploader> pipeUploader = nullptr;
        MSVP_MAKE_SHARED1_RET(pipeUploader, Uploader, pipeTransport, ACL_ERROR_PROFILING_FAILURE);
        static const size_t subscribeUploaderCapacity = 100000; // subscribe need more capacity (100000 op data)
        int ret = pipeUploader->Init(subscribeUploaderCapacity);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to init uploader for subscribe");
            MSPROF_INNER_ERROR("EK9999", "Failed to init uploader for subscribe");
            return ACL_ERROR_PROFILING_FAILURE;
        }
        std::string uploaderName = analysis::dvvp::common::config::MSVP_UPLOADER_THREAD_NAME;
        uploaderName.append("_").append("Pipe");
        pipeUploader->SetThreadName(uploaderName);
        ret = pipeUploader->Start();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to start uploader thread");
            MSPROF_INNER_ERROR("EK9999", "Failed to start uploader thread");
            return ACL_ERROR_PROFILING_FAILURE;
        }
        // create transport1 and uploader1
        SHARED_PTR_ALIA<ParserTransport> parserTransport = nullptr;
        MSVP_MAKE_SHARED1_RET(parserTransport, ParserTransport, pipeUploader, ACL_ERROR_PROFILING_FAILURE);
        parserTransport->SetDevIdToAnalyzer(devIdStr);
        const uint32_t capacity = 10240;
        ret = UploaderMgr::instance()->CreateUploader(devIdStr, parserTransport, capacity);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to create uploader for subscribe");
            MSPROF_INNER_ERROR("EK9999", "Failed to create uploader for subscribe");
            return ACL_ERROR_PROFILING_FAILURE;
        }
    }
    return ACL_SUCCESS;
}

/**
 * Check if device is free and is online.
 */
int ProfAclMgr::CheckDeviceTask(PROF_CONF_CONST_PTR profStartCfg)
{
    std::vector<uint32_t> devIds;
    for (uint32_t i = 0; i < profStartCfg->devNums; i++) {
        uint32_t devId = profStartCfg->devIdList[i];
        if (devTasks_.find(devId) != devTasks_.end()) {
            MSPROF_LOGE("Device %u already started", devId);
            MSPROF_INNER_ERROR("EK9999", "Device %u already started", devId);
            return ACL_ERROR_PROF_ALREADY_RUN;
        }
        devIds.push_back(devId);
    }
    UtilsStringBuilder<uint32_t> builder;
    std::string info;
    if (!(ProfManager::instance()->CheckIfDevicesOnline(builder.Join(devIds, ","), info))) {
        MSPROF_LOGE("%s", info.c_str());
        MSPROF_INNER_ERROR("EK9999", "%s", info.c_str());
        return ACL_ERROR_INVALID_DEVICE;
    }
    return ACL_SUCCESS;
}

int ProfAclMgr::ProfStartAiCpuTrace(const uint64_t dataTypeConfig, const uint32_t devNums, CONST_UINT32_T_PTR devIdList)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        return PROFILING_SUCCESS;
    }
    // aicpu trace
    if (!(dataTypeConfig & PROF_AICPU_MASK)) {
        return PROFILING_SUCCESS;
    }
    for (uint32_t i = 0; i < devNums; i++) {
        uint32_t devId = devIdList[i];
        MSPROF_LOGI("Process ProfStartAiCpuTrace of device %u", devId);
        if (devId == DEFAULT_HOST_ID) {
            continue;
        }
        auto iter = enginMap_.find(devId);
        if (iter == enginMap_.end()) {
            SHARED_PTR_ALIA<Msprof::Engine::AicpuPlugin> engin;
            MSVP_MAKE_SHARED0_RET(engin, Msprof::Engine::AicpuPlugin, ACL_ERROR_PROFILING_FAILURE);
            int ret = engin->Init(devId);
            if (ret != PROFILING_SUCCESS) {
                return ret;
            }
            enginMap_[devId] = engin;
        }
    }
    return PROFILING_SUCCESS;
}

void ProfAclMgr::GenerateSystemTraceConf(const uint64_t dataTypeConfig, ProfAicoreMetrics aicMetrics,
                                         SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature,
                                         SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    bool systemTraceConf = false;
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfilerConf> conf = nullptr;
    MSVP_MAKE_SHARED0_VOID(conf, analysis::dvvp::proto::ProfilerConf);
    if (dataTypeConfig & PROF_SYS_CPU_MASK) {
        systemTraceConf = true;
        conf->set_cpusamplinginterval(DEFAULT_PROFILING_INTERVAL_20MS);
        params->ai_ctrl_cpu_profiling_events = "0x11,0x8";
        params->ts_cpu_profiling_events = "0x11,0x8";
    }
    if (dataTypeConfig & PROF_SYS_HARDWARE_MEM_MASK) {
        systemTraceConf = true;
        conf->set_hardwarememsamplinginterval(analysis::dvvp::message::DEFAULT_PROFILING_INTERVAL_100MS);
        params->ddr_profiling_events = "read,write";
        params->hbm_profiling_events = "read,write";
    }
    if (dataTypeConfig & PROF_SYS_IO_MASK) {
        systemTraceConf = true;
        conf->set_iosamplinginterval(analysis::dvvp::message::DEFAULT_PROFILING_INTERVAL_100MS);
    }
    if (dataTypeConfig & PROF_SYS_INTERCONNECTION_MASK) {
        systemTraceConf = true;
        conf->set_interconnectionsamplinginterval(analysis::dvvp::message::DEFAULT_PROFILING_INTERVAL_100MS);
    }
    if (dataTypeConfig & PROF_DVPP_MASK) {
        systemTraceConf = true;
        conf->set_dvppsamplinginterval(analysis::dvvp::message::DEFAULT_PROFILING_INTERVAL_100MS);
    }
    if ((dataTypeConfig & PROF_SYS_AICORE_SAMPLE_MASK) && (!metrics.empty())) {
        systemTraceConf = true;
        conf->set_aicoresamplinginterval(DEFAULT_PROFILING_INTERVAL_10MS);
        conf->set_aicoremetrics(metrics);
    }
    if ((dataTypeConfig & PROF_AIVECTORCORE_SAMPLE_MASK) && (!metrics.empty())) {
        systemTraceConf = true;
        conf->set_aivsamplinginterval(DEFAULT_PROFILING_INTERVAL_10MS);
        conf->set_aivmetrics(metrics);
    }
    if (systemTraceConf) {
        feature->set_system_trace_conf(analysis::dvvp::message::EncodeJson(conf));
    }
}

/**
 * Transfer dataTypeConfig and aicoreMetrics to MsProfStartReq
 */
void ProfAclMgr::ProfStartCfgToMsprofCfg(const uint64_t dataTypeConfig, ProfAicoreMetrics aicMetrics,
                                         SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature)
{
    feature->set_feature_name(PROF_FEATURE_TASK);
    // ts_timeline & hwts
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        if (dataTypeConfig & PROF_TASK_TIME_MASK) {
            feature->set_ts_timeline("on");
        }
    } else if (dataTypeConfig & PROF_TASK_TIME_MASK) {
            feature->set_hwts_log("on");
    }
    // training trace
    if (dataTypeConfig & PROF_TRAINING_TRACE_MASK) {
        feature->set_ts_fw_training("on");
    }
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfilerConf> conf = nullptr;
    MSVP_MAKE_SHARED0_VOID(conf, analysis::dvvp::proto::ProfilerConf);
    std::string metrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(aicMetrics, metrics);
    // ai_core
    if ((dataTypeConfig & PROF_AICORE_METRICS_MASK) && !metrics.empty()) {
        conf->set_aicoremetrics(metrics);
    }
    // aiv
    if ((dataTypeConfig & PROF_AIV_METRICS_MASK) && !metrics.empty()) {
        conf->set_aivmetrics(metrics);
    }
    // l2cache
    if (dataTypeConfig & PROF_L2CACHE_MASK) {
        conf->set_l2(MSVP_PROF_ON);
    }
    if (!conf->aicoremetrics().empty() || !conf->aivmetrics().empty() || !conf->l2().empty()) {
        feature->set_task_trace_conf(analysis::dvvp::message::EncodeJson(conf));
    }
    MSPROF_LOGI("Transformed msporf cfg result: %s", feature->DebugString().c_str());
}

/**
 * Start device acl-api task
 */
int ProfAclMgr::StartDeviceTask(const uint32_t devId, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    std::string devIdStr = std::to_string(devId);
    // init uploader
    int ret = InitUploader(devIdStr);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // set devId as job_id, set devices, set result_dir
    params->job_id = devIdStr;
    params->devices = devIdStr;
    params->result_dir = GenerateDevDirName(devIdStr);

    // create params
    auto paramsHandled = ProfManager::instance()->HandleProfilingParams(params->ToString());

    // add responseHandler
    SHARED_PTR_ALIA<DeviceResponseHandler> handler = nullptr;
    MSVP_MAKE_SHARED1_RET(handler, DeviceResponseHandler, devId, ACL_ERROR_PROFILING_FAILURE);
    devResponses_[devId] = handler;
    handler->Start();

    // start profiling process
    if (ProfManager::instance()->IdeCloudProfileProcess(paramsHandled) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start profiling on device %u", devId);
        MSPROF_INNER_ERROR("EK9999", "Failed to start profiling on device %u", devId);
        HandleResponse(devId);
        return ACL_ERROR_PROFILING_FAILURE;
    }

    // add taskInfo
    ProfAclTaskInfo taskInfo = {1, 0, paramsHandled};
    devTasks_[devId] = taskInfo;

    return ACL_SUCCESS;
}

/**
 * All starting devices create a thread to wait for response
 */
void ProfAclMgr::WaitAllDeviceResponse()
{
    for (auto iter = devResponses_.begin(); iter != devResponses_.end(); iter++) {
        iter->second->Join();
    }
    MSPROF_EVENT("All devices finished waiting");
    devResponses_.clear();
}

void ProfAclMgr::WaitDeviceResponse(const uint32_t devId)
{
    auto iter = devResponses_.find(devId);
    if (iter != devResponses_.end()) {
        iter->second->Join();
        MSPROF_EVENT("Device:%u finished waiting", devId);
        devResponses_.erase(iter);
    }
}

int ProfAclMgr::UpdateSubscribeInfo(const uint32_t modelId, const uint32_t devId,
                                    PROF_SUB_CONF_CONST_PTR profSubscribeConfig)
{
    auto iterDev = devTasks_.find(devId);
    if (iterDev == devTasks_.end()) {
        return ACL_ERROR_PROFILING_FAILURE;
    }
    // check dataTypeConfig
    auto dataTypeConfig = ProfAclGetDataTypeConfig(profSubscribeConfig);
    if (iterDev->second.dataTypeConfig != dataTypeConfig) {
        MSPROF_LOGE("Subscribe config 0x%lx is different from previous one: 0x%lx",
            dataTypeConfig, iterDev->second.dataTypeConfig);
        MSPROF_INNER_ERROR("EK9999", "Subscribe config 0x%lx is different from previous one: 0x%lx",
            dataTypeConfig, iterDev->second.dataTypeConfig);
        return ACL_ERROR_INVALID_PROFILING_CONFIG;
    }
    // check aicore
    std::string aicoreMetrics;
    ConfigManager::instance()->AicoreMetricsEnumToName(profSubscribeConfig->aicoreMetrics, aicoreMetrics);
    if (iterDev->second.params->ai_core_metrics != aicoreMetrics) {
        MSPROF_LOGE("Subscribe aicore metrics %s is different from previous one: %s", aicoreMetrics.c_str(),
            iterDev->second.params->ai_core_metrics.c_str());
        MSPROF_INNER_ERROR("EK9999", "Subscribe aicore metrics %s is different from previous one: %s",
            aicoreMetrics.c_str(), iterDev->second.params->ai_core_metrics.c_str());
        return ACL_ERROR_INVALID_PROFILING_CONFIG;
    }
    if (iterDev->second.count + 1 == 0) {
        MSPROF_LOGE("Subscribe count is too large");
        MSPROF_INNER_ERROR("EK9999", "Subscribe count is too large");
        return ACL_ERROR_INVALID_PROFILING_CONFIG;
    }
    iterDev->second.count++;
    std::lock_guard<std::mutex> lk(mtxSubscribe_);
    auto iter = subscribeInfos_.find(modelId);
    if (iter != subscribeInfos_.end()) {
        // re subscribe
        iter->second.subscribed = true;
        iter->second.devId = devId;
        iter->second.fd = static_cast<int *>(profSubscribeConfig->fd);
    } else {
        // new subscribe
        ProfSubscribeInfo subscribeInfo = {true, devId, static_cast<int *>(profSubscribeConfig->fd)};
        subscribeInfos_.insert(std::make_pair(modelId, subscribeInfo));
    }
    return ACL_SUCCESS;
}

int ProfAclMgr::StartDeviceSubscribeTask(const uint32_t modelId, const uint32_t devId,
                                         PROF_SUB_CONF_CONST_PTR profSubscribeConfig)
{
    auto dataTypeConfig = ProfAclGetDataTypeConfig(profSubscribeConfig);
    // generate params
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params = nullptr;
    MSVP_MAKE_SHARED0_RET(params, analysis::dvvp::message::ProfileParams, ACL_ERROR_PROFILING_FAILURE);
    params->profiling_mode = analysis::dvvp::message::PROFILING_MODE_DEF;
    SHARED_PTR_ALIA<analysis::dvvp::proto::MsProfStartReq> feature = nullptr;
    MSVP_MAKE_SHARED0_RET(feature, analysis::dvvp::proto::MsProfStartReq, ACL_ERROR_PROFILING_FAILURE);
    ProfStartCfgToMsprofCfg(dataTypeConfig, profSubscribeConfig->aicoreMetrics, feature);
    ProfParamsAdapter::instance()->UpdateSampleConfig(feature, params);
    ProfParamsAdapter::instance()->ProfStartCfgToParamsCfg(dataTypeConfig, params);
    params->ts_keypoint = MSVP_PROF_ON;
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::CHIP_V4_1_0) {
        params->stars_acsq_task = MSVP_PROF_ON;
    }

    std::string devIdStr = std::to_string(devId);
    params->job_id = devIdStr;
    params->devices = devIdStr;

    int ret = InitSubscribeUploader(devIdStr);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    // start responseHandler
    SHARED_PTR_ALIA<DeviceResponseHandler> handler = nullptr;
    MSVP_MAKE_SHARED1_RET(handler, DeviceResponseHandler, devId, ACL_ERROR_PROFILING_FAILURE);
    devResponses_[devId] = handler;
    (void)handler->Start();

    // start profiling process
    auto paramsHandled = ProfManager::instance()->HandleProfilingParams(params->ToString());
    if (ProfManager::instance()->IdeCloudProfileProcess(paramsHandled) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start profiling on device %u", devId);
        MSPROF_INNER_ERROR("EK9999", "Failed to start profiling on device %u", devId);
        HandleResponse(devId);
        return ACL_ERROR_PROFILING_FAILURE;
    }

    // add taskInfo
    ProfAclTaskInfo taskInfo = {1, dataTypeConfig, paramsHandled};
    devTasks_[devId] = taskInfo;

    mtxSubscribe_.lock();
    ProfSubscribeInfo subscribeInfo = {true, devId, static_cast<int *>(profSubscribeConfig->fd)};
    subscribeInfos_.insert(std::make_pair(modelId, subscribeInfo));
    mtxSubscribe_.unlock();

    WaitAllDeviceResponse();
    MSPROF_LOGI("Finished starting subscribe task on device %u", devId);
    if (mode_ == WORK_MODE_OFF) {
        mode_ = WORK_MODE_SUBSCRIBE;
    }
    return ACL_SUCCESS;
}

void ProfAclMgr::ProfDataTypeConfigHandle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    dataTypeConfig_ = params->dataTypeConfig;
    MSPROF_EVENT("ProfDataTypeConfigHandle dataTypeConfig:0x%llx", dataTypeConfig_);
}

void ProfAclMgr::UpdateDataTypeConfigBySwitch(const std::string &sw, const uint64_t dataTypeConfig)
{
    if (sw == MSVP_PROF_ON) {
        dataTypeConfig_ |= dataTypeConfig;
    }
}

std::string ProfAclMgr::MsprofCheckAndGetChar(CHAR_PTR data, uint32_t dataLen)
{
    uint32_t i;
    for (i = 0; i < dataLen; i++) {
        if (data[i] != '\0') {
            continue;
        } else {
            break;
        }
    }
    if (i > 0 && i < dataLen) {
        return std::string(data, i);
    } else {
        return "";
    }
}

void ProfAclMgr::MsprofSetMemberValue()
{
    storageLimit_ = params_->storageLimit;
    resultPath_ = params_->result_dir;
    baseDir_ = Utils::CreateTaskId(0);
    dataTypeConfig_ = params_->dataTypeConfig;
}

int32_t ProfAclMgr::MsprofInitAclJson(VOID_PTR data, uint32_t len)
{
    MSPROF_EVENT("Init profiling for AclJson");
    static uint32_t ACL_CFG_LEN_MAX = 1024 * 1024;  // max input cfg len is 1024 * 1024
    if (data == nullptr || len > ACL_CFG_LEN_MAX) {
        MSPROF_LOGE("Length of acl json config is too large: %u", len);
        MSPROF_INNER_ERROR("EK9999", "Length of acl json config is too large: %u", len);
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = CallbackInitPrecheck();
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_NONE;
    }
    std::string aclCfg(reinterpret_cast<CHAR_PTR>(data), len);
    MSPROF_LOGI("Input aclJsonConfig: %s", aclCfg.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfAclConfig> inputCfgPb = nullptr;
    MSVP_MAKE_SHARED0_RET(inputCfgPb, analysis::dvvp::proto::ProfAclConfig, MSPROF_ERROR_MEM_NOT_ENOUGH);
    if (!google::protobuf::util::JsonStringToMessage(aclCfg, inputCfgPb.get()).ok()) {
        MSPROF_LOGE("The format of input aclJsonConfig is invalid");
        MSPROF_INNER_ERROR("EK9999", "The format of input aclJsonConfig is invalid");
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    if (params_ != nullptr) {
        MSPROF_LOGW("MsprofInitAclJson params exist");
    } else {
        MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, MSPROF_ERROR_MEM_NOT_ENOUGH);
    }
    auto paramAdapter = AclJsonParamAdapter();
    ret = paramAdapter.GetParamFromInputCfg(inputCfgPb, params_);
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    params_->job_id = Utils::ProfCreateId(0);
    AddAiCpuModelConf(params_->dataTypeConfig);
    MsprofSetMemberValue();
    SetModeToCmd();
    return MSPROF_ERROR_NONE;
}


int32_t ProfAclMgr::MsprofResultPathAdapter(const std::string &dir, std::string &resultPath)
{
    std::string result;
    if (dir.empty()) {
        MSPROF_LOGE("Result path is empty");
        return PROFILING_FAILED;
    }
    std::string path = Utils::RelativePathToAbsolutePath(dir);
    if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to create dir: %s", Utils::BaseName(path).c_str());
    }
    result = analysis::dvvp::common::utils::Utils::CanonicalizePath(path);
    if (result.empty() || !analysis::dvvp::common::utils::Utils::IsDirAccessible(result)) {
        MSPROF_LOGE("Result path is not accessible or not exist, result path: %s", Utils::BaseName(dir).c_str());
        std::string errReason = "result path is not accessible or not exist";
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"output", dir, errReason}));
        return PROFILING_FAILED;
    }
    resultPath = result;
    MSPROF_LOGI("MsprofResultPathAdapter canonicalized path: %s", Utils::BaseName(result).c_str());

    return PROFILING_SUCCESS;
}

int32_t ProfAclMgr::MsprofInitForDynamic(VOID_PTR data /* = nullptr */, uint32_t len /* = 0 */) const
{
    UNUSED(data);
    UNUSED(len);
    MSPROF_EVENT("Init profiling for dynamic profiling");
    return MSPROF_ERROR_NONE;
}

int32_t ProfAclMgr::MsprofInitGeOptions(VOID_PTR data, uint32_t len)
{
    MSPROF_EVENT("Init profiling for GeOptions");
    uint32_t structLen = sizeof(struct MsprofGeOptions);
    if (data == nullptr || len != structLen) {
        MSPROF_LOGE("MsprofInitGeOptions input arguments is invalid, len:%u, structLen:%u", len, structLen);
        MSPROF_INNER_ERROR("EK9999", "MsprofInitGeOptions input arguments is invalid, len:%u, structLen:%u",
            len, structLen);
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = CallbackInitPrecheck();
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_NONE;
    }
    MsprofGeOptions *optionCfg = (struct MsprofGeOptions *)data;
    std::string jobInfo = MsprofCheckAndGetChar(optionCfg->jobId, MSPROF_OPTIONS_DEF_LEN_MAX);
    std::string options = MsprofCheckAndGetChar(optionCfg->options, MSPROF_OPTIONS_DEF_LEN_MAX);
    MSPROF_LOGI("MsprofInitGeOptions, jobInfo:%s, options:%s", jobInfo.c_str(), options.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfGeOptionsConfig> inputCfgPb = nullptr;
    MSVP_MAKE_SHARED0_RET(inputCfgPb, analysis::dvvp::proto::ProfGeOptionsConfig, MSPROF_ERROR_MEM_NOT_ENOUGH);
    if (!google::protobuf::util::JsonStringToMessage(options, inputCfgPb.get()).ok()) {
        MSPROF_LOGE("The format of input ge options is invalid");
        MSPROF_INNER_ERROR("EK9999", "The format of input ge options is invalid");
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    if (params_ != nullptr) {
        MSPROF_LOGW("MsprofInitAclJson params exist");
    } else {
        MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, MSPROF_ERROR_MEM_NOT_ENOUGH);
    }
    auto paramAdapter = GeOptParamAdapter();
    ret = paramAdapter.GetParamFromInputCfg(inputCfgPb, params_);
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    params_->job_id = Utils::ProfCreateId(0);
    params_->jobInfo = jobInfo;
    AddAiCpuModelConf(params_->dataTypeConfig);
    MsprofSetMemberValue();
    SetModeToCmd();
    return MSPROF_ERROR_NONE;
}

int32_t ProfAclMgr::MsprofInitAclEnv(const std::string &envValue)
{
    MSPROF_EVENT("Init profiling for CommandLine");
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = CallbackInitPrecheck();
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_NONE;
    }
    if (params_ != nullptr) {
        MSPROF_LOGW("MsprofInitAclEnv params exist");
    } else {
        MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, MSPROF_ERROR_MEM_NOT_ENOUGH);
    }
    if (!params_->FromString(envValue)) {
        MSPROF_LOGE("ProfileParams Parse Failed %s", envValue.c_str());
        MSPROF_INNER_ERROR("EK9999", "ProfileParams Parse Failed %s", envValue.c_str());
        return MSPROF_ERROR;
    }
    params_->runtimeTrace = MSVP_PROF_ON;
    params_->host_sys_pid = analysis::dvvp::common::utils::Utils::GetPid();
    resultPath_ = params_->result_dir;
    baseDir_ = Utils::CreateTaskId(0);
    storageLimit_ = params_->storageLimit;
    if (!ParamValidation::instance()->CheckStorageLimit(storageLimit_)) {
        MSPROF_LOGE("storage_limit para is invalid");
        MSPROF_INNER_ERROR("EK9999", "storage_limit para is invalid");
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    ProfDataTypeConfigHandle(params_);
    SetModeToCmd();
    MSPROF_LOGI("MsprofInitAclEnv, mode:%d, dataTypeConfig:0x%lx, baseDir:%s",
                mode_, dataTypeConfig_, Utils::BaseName(baseDir_).c_str());
    return MSPROF_ERROR_NONE;
}

int32_t ProfAclMgr::MsprofHelperParamConstruct(const std::string &msprofPath, const std::string &paramsJson)
{
    if (params_ != nullptr) {
        MSPROF_LOGW("MsprofHelper params exist");
    } else {
        MSVP_MAKE_SHARED0_RET(params_, analysis::dvvp::message::ProfileParams, MSPROF_ERROR_MEM_NOT_ENOUGH);
    }
    params_->FromString(paramsJson);
    int ret = MsprofResultPathAdapter(msprofPath, params_->result_dir);
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    resultPath_ = params_->result_dir;
    baseDir_ = Utils::CreateTaskId(0);
    if (!ParamValidation::instance()->CheckStorageLimit(params_->storageLimit)) {
        MSPROF_LOGE("storage_limit para is invalid");
        MSPROF_INNER_ERROR("EK9999", "storage_limit para is invalid");
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    storageLimit_ = params_->storageLimit;
    return MSPROF_ERROR_NONE;
}

int32_t ProfAclMgr::MsprofInitHelper(VOID_PTR data, uint32_t len)
{
    MSPROF_EVENT("Init profiling for Helper");
    uint32_t structLen = sizeof(struct MsprofCommandHandleParams);
    if (data == nullptr || len != structLen) {
        MSPROF_LOGE("MsprofInitHelper input arguments is invalid, len:%u, structLen:%u", len, structLen);
        MSPROF_INNER_ERROR("EK9999", "MsprofInitHelper input arguments is invalid, len:%u, structLen:%u",
            len, structLen);
        return MSPROF_ERROR_CONFIG_INVALID;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    int ret = CallbackInitPrecheck();
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_NONE;
    }
    MsprofCommandHandleParams *commandHandleParams = static_cast<struct MsprofCommandHandleParams *>(data);
    std::string msprofPath = MsprofCheckAndGetChar(commandHandleParams->path, PATH_LEN_MAX);
    std::string msprofParams = MsprofCheckAndGetChar(commandHandleParams->profData, PARAM_LEN_MAX);
    MSPROF_LOGI("MsprofInitHelper, path:%s, params:%s", msprofPath.c_str(), msprofParams.c_str());
    ret = MsprofHelperParamConstruct(msprofPath, msprofParams);
    if (ret != MSPROF_ERROR_NONE) {
        return ret;
    }
    ProfDataTypeConfigHandle(params_);
    SetModeToCmd();
    return MSPROF_ERROR_NONE;
}

int ProfAclMgr::DoHostHandle()
{
    params_->host_profiling = true;
    int ret = PROFILING_SUCCESS;
    if (params_->IsMsprofTx()) {
        Analysis::Dvvp::ProfilerCommon::RegisterMsprofTxReporterCallback();
        ret = MsprofTxManager::instance()->Init();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGW("MsprofTxManager init failed");
        }
    }
    ret = MsprofSetDeviceImpl(DEFAULT_HOST_ID);
    params_->host_profiling = false;
    return ret;
}

int32_t ProfAclMgr::MsprofTxHandle(void)
{
    MSPROF_EVENT("Init profiling for msproftx");
    int ret = MsprofTxSwitchPrecheck();
    if (ret != PROFILING_SUCCESS) {
        return MSPROF_ERROR_NONE;
    }
    params_->msproftx = "on";
    return DoHostHandle();
}

void ProfAclMgr::MsprofHostHandle(void)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGW("Msprof not support collect host sys and msprofTx data in helper");
        return;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    if (Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("MsprofHostHandle, not on host side");
        return;
    }

    if (!IsCmdMode()) {
        MSPROF_LOGI("MsprofHostHandle, not on cmd mode");
        return;
    }

    if (!params_->IsHostProfiling()) {
        MSPROF_LOGI("MsprofHostHandle, host profiling disabled");
        return;
    }
    auto ret = DoHostHandle();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[MsprofHostHandle] host profiling handle failed, ret is %d", ret);
        MSPROF_INNER_ERROR("EK9999", "host profiling handle failed, ret is %d", ret);
    }
}

int32_t ProfAclMgr::MsprofFinalizeHandle(void)
{
    if (!Msprofiler::Api::ProfAclMgr::instance()->IsCmdMode()) {
        MSPROF_LOGW("Profiling does not work on cmd mode");
        return MSPROF_ERROR_NONE;
    }

    ge::GeFinalizeHandle();

    MSPROF_EVENT("Finalize profiling");
    std::lock_guard<std::mutex> lk(mtx_);
    if (!IsCmdMode()) {
        MSPROF_LOGI("MsprofFinalizeHandle, not on cmd mode, mode:%d", mode_);
        return MSPROF_ERROR_NONE;
    }
    for (auto iter = devTasks_.begin(); iter != devTasks_.end(); iter++) {
        HashData::instance()->SaveHashData(iter->first);
        iter->second.params->is_cancel = true;
        if (ProfManager::instance()->IdeCloudProfileProcess(iter->second.params) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to finalize profiling on device %u", iter->first);
            MSPROF_INNER_ERROR("EK9999", "Failed to finalize profiling on device %u", iter->first);
        }
    }
    MsprofTxManager::instance()->UnInit();
    UploaderMgr::instance()->DelAllUploader();
    devTasks_.clear();
    SetModeToOff();
    if (Utils::IsClusterRunEnv()) {
        std::string jobDir = resultPath_ + MSVP_SLASH + baseDir_;
        if (Utils::CloudAnalyze(jobDir) != PROFILING_SUCCESS) {
            MSPROF_LOGW("Can't analyze data on cloud. path: %s, please do it by yourself.", jobDir.c_str());
        }
    }
    return MSPROF_ERROR_NONE;
}

int32_t ProfAclMgr::MsprofSetDeviceImpl(uint32_t devId)
{
    MSPROF_EVENT("MsprofSetDeviceImpl, devId:%u", devId);
    auto iterDev = devTasks_.find(devId);
    if (iterDev != devTasks_.end()) {
        MSPROF_LOGI("MsprofSetDeviceImpl, device:%u is running", devId);
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("MsprofSetDeviceImpl, Process ProfStart of device:%u", devId);
    int ret = StartDeviceTask(devId, params_);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("MsprofSetDeviceImpl, StartDeviceTask failed, devId:%u, mode:%d", devId, mode_);
        MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceImpl, StartDeviceTask failed, devId:%u, mode:%d", devId, mode_);
        return PROFILING_FAILED;
    }
    devTasks_[devId].dataTypeConfig = dataTypeConfig_;
    WaitDeviceResponse(devId);
    uint32_t devIdList[1] = {devId};
    ret = ProfStartAiCpuTrace(dataTypeConfig_, 1, devIdList);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start aicpu trace");
        MSPROF_INNER_ERROR("EK9999", "Failed to start aicpu trace");
    }
    return ret;
}

void ProfAclMgr::CloseSubscribeFd(const uint32_t devId)
{
    std::lock_guard<std::mutex> lk(mtxSubscribe_);
    std::set<int *> closedFds;
    std::set<int> usedFds;
    for (auto iter = subscribeInfos_.begin(); iter != subscribeInfos_.end();) {
        if (iter->second.devId == devId) {
            // fd on same device, should close
            closedFds.insert(iter->second.fd);
            iter = subscribeInfos_.erase(iter);
        } else {
            // fd on different device, should not close
            usedFds.insert(*(iter->second.fd));
            iter++;
        }
    }
    for (int *fd : closedFds) {
        if (usedFds.find(*fd) == usedFds.end()) {
            MSPROF_EVENT("Close subscribe fd %d", *fd);
            if (MmClose(*fd) != EOK) {
                MSPROF_LOGE("Failed to close subscribe fd %d", *fd);
                MSPROF_INNER_ERROR("EK9999", "Failed to close subscribe fd %d", *fd);
                Utils::PrintSysErrorMsg();
            }
            *fd = -1;
        }
    }
}

void ProfAclMgr::CloseSubscribeFd(const uint32_t devId, const uint32_t modelId)
{
    std::lock_guard<std::mutex> lk(mtxSubscribe_);
    int *fd = nullptr;
    auto iter = subscribeInfos_.find(modelId);
    if ((iter != subscribeInfos_.end()) && (iter->second.devId == devId)) {
        fd = iter->second.fd;
        subscribeInfos_.erase(iter);
    } else {
        MSPROF_LOGE("Model %u has not been subscribed.", modelId);
        MSPROF_INNER_ERROR("EK9999", "Model %u has not been subscribed.", modelId);
        return;
    }
    for (iter = subscribeInfos_.begin(); iter != subscribeInfos_.end(); iter++) {
        if (*fd == *iter->second.fd) {
            MSPROF_LOGI("Fd %d is still used by the model %u.Can't close it now.", *fd, modelId);
            return;
        }
    }
    MSPROF_EVENT("Close subscribe fd %d", *fd);
    if (MmClose(*fd) != EOK) {
        MSPROF_LOGE("Failed to close subscribe fd: %d, modelId: %u, devId: %u", *fd, modelId, devId);
        MSPROF_INNER_ERROR("EK9999", "Failed to close subscribe fd: %d, modelId: %u, devId: %u", *fd, modelId, devId);
        Utils::PrintSysErrorMsg();
    }
    *fd = -1;
}

int32_t ProfAclMgr::StopProfConfigCheck(uint64_t dataTypeConfigStop, uint64_t dataTypeConfigStart)
{
    if (dataTypeConfigStop != dataTypeConfigStart) {
        MSPROF_LOGE("DataTypeConfig stop:0x%lx different from start:0x%lx",
            dataTypeConfigStop, dataTypeConfigStart);
        std::string dataTypeConfigStr = "0x" +
            Utils::Int2HexStr<uint64_t>(dataTypeConfigStop);
        std::string errorReason = "dataTypeConfig is different from start:0x" +
            Utils::Int2HexStr<uint64_t>(dataTypeConfigStart);
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({dataTypeConfigStr, "dataTypeConfig", errorReason}));
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t ProfAclMgr::MsprofSetConfig(aclprofConfigType cfgType, std::string config)
{
    int ret = PROFILING_SUCCESS;
    std::string configStr;
    // param check
    switch(cfgType) {
        case ACL_PROF_STORAGE_LIMIT:
            ret = ParamValidation::instance()->CheckStorageLimit(config) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_AIV_METRICS:
            ConfigManager::instance()->AicoreMetricsEnumToName(static_cast<ProfAicoreMetrics>(std::stoi(config)), configStr);
            ret = ParamValidation::instance()->CheckProfilingAicoreMetricsIsValid(configStr) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_USAGE_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, SYS_SAMPLING_FREQ_MIN_NUM, SYS_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_PID_USAGE_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, PID_SAMPLING_FREQ_MIN_NUM, PID_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_CPU_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, CPU_SAMPLING_FREQ_MIN_NUM, CPU_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_HARDWARE_MEM_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, HARDWARE_MEM_SAMPLING_FREQ_MIN_NUM, HARDWARE_MEM_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_LLC_MODE:
            ret = ParamValidation::instance()->CheckLlcModeIsValid(config) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_IO_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, IO_SAMPLING_FREQ_MIN_NUM, IO_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_SYS_INTERCONNECTION_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, INTERCONNECTION_SAMPLING_FREQ_MIN_NUM, INTERCONNECTION_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_DVPP_FREQ:
            ret = ParamValidation::instance()->CheckSamplingFreq(config, DVPP_SAMPLING_FREQ_MIN_NUM, DVPP_SAMPLING_FREQ_MAX_NUM) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        case ACL_PROF_HOST_SYS:
            ret = ParamValidation::instance()->CheckHostSysOptionsIsValid(config) ? PROFILING_SUCCESS : PROFILING_FAILED;
            break;
        default:
            ret = PROFILING_FAILED;
    }
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[MsprofSetConfig]profiling config check fail");
        return ret;
    }
    argsArr_[cfgType] = (cfgType == ACL_PROF_AIV_METRICS) ? configStr : config;
    return PROFILING_SUCCESS;
}
}   // namespace Api
}   // namespace Msprofiler
