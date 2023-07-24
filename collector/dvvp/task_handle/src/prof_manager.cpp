/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "prof_manager.h"

#include "ai_drv_dev_api.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "platform/platform.h"
#include "platform/platform_adapter.h"
#include "prof_acl_mgr.h"
#include "prof_params_adapter.h"
#include "proto/msprofiler.pb.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "validation/param_validation.h"

namespace analysis {
namespace dvvp {
namespace host {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::JobWrapper;

int ProfManager::AclInit()
{
    if (isInited_) {
        MSPROF_LOGW("ProfManager has been inited, no need to aclinit again");
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("aclinit ProfManager begin.");

    Platform::instance()->SetPlatformSoc();

    MSPROF_LOGI("init ProfManager end.");
    isInited_ = true;
    return PROFILING_SUCCESS;
}

int ProfManager::AclUinit()
{
    MSPROF_LOGI("acluinit ProfManager begin.");
    if (isInited_) {
        isInited_ = false;
    }

    MSPROF_LOGI("acluinit ProfManager end.");
    return PROFILING_SUCCESS;
}

bool ProfManager::CreateDoneFile(const std::string &absolutePath, const std::string &fileSize) const
{
    std::ofstream file;

    file.open(absolutePath, std::ios::out);
    if (!file.is_open()) {
        MSPROF_LOGE("[CreateDoneFile]Failed to open %s", Utils::BaseName(absolutePath).c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return false;
    }
    MSPROF_LOGI("ProfManager::CreateDoneFile");
    file << "filesize:" << fileSize << std::endl;
    file.flush();
    file.close();
    return true;
}

int ProfManager::WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen)
{
    std::ofstream file;

    if (Utils::IsFileExist(absolutePath)) {
        MSPROF_LOGI("[WriteCtrlDataToFile]file exist: %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_SUCCESS;
    }
    if (data.empty() || dataLen <= 0) {
        MSPROF_LOGE("[WriteCtrlDataToFile]Failed to open %s", Utils::BaseName(absolutePath).c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_FAILED;
    }
    file.open(absolutePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        MSPROF_LOGE("[WriteCtrlDataToFile]Failed to open %s", Utils::BaseName(absolutePath).c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_FAILED;
    }
    file.write(data.c_str(), dataLen);
    file.flush();
    file.close();
    if (!(CreateDoneFile(absolutePath + ".done", std::to_string(dataLen)))) {
        MSPROF_LOGE("[WriteCtrlDataToFile]set device done file failed");
        MSPROF_INNER_ERROR("EK9999", "set device done file failed");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

bool ProfManager::CreateSampleJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    const std::string &resultDir)
{
    if (resultDir.empty()) {
        return true;
    }
    static const std::string fileName = "sample.json";
    int ret = Utils::CreateDir(resultDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[CreateSampleJsonFile]create dir error , %s", Utils::BaseName(resultDir).c_str());
        MSPROF_INNER_ERROR("EK9999", "create dir error , %s", Utils::BaseName(resultDir).c_str());
        Utils::PrintSysErrorMsg();
        return false;
    }
    MSPROF_LOGI("ProfManager::CreateSampleJsonFile");
    ret = WriteCtrlDataToFile(resultDir + fileName, params->ToString().c_str(), params->ToString().size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[CreateSampleJsonFile]Failed to write local files");
        MSPROF_INNER_ERROR("EK9999", "Failed to write local files");
        return false;
    }

    return true;
}

bool ProfManager::CheckHandleSuc(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    analysis::dvvp::message::StatusInfo &statusInfo)
{
    bool isOk = false;
    do {
        MSPROF_LOGI("jobId:%s, period:%d, devices:%s, is_cancel:%d", params->job_id.c_str(),
                    params->profiling_period, params->devices.c_str(), params->is_cancel);
        if (params->is_cancel) { // judge is_cancel
            StopTask(params->job_id);
            isOk = true;
            break;
        }

        std::vector<std::string> devices =
            Utils::Split(params->devices, false, "", ",");

        MSPROF_EVENT("Check device profiling status");
        std::lock_guard<std::mutex> lk(taskMtx_);
        if (IsDeviceProfiling(devices)) {
            statusInfo.info = "device is already in profiling, skip the task";
            MSPROF_LOGE("Device is already in profiling");
            MSPROF_INNER_ERROR("EK9999", "Device is already in profiling");
            break;
        }
        SHARED_PTR_ALIA<ProfTask> task = nullptr;
        MSVP_MAKE_SHARED2_RET(task, ProfTask, devices, params, PROFILING_FAILED);
        int ret = LaunchTask(task, params->job_id, statusInfo.info);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to init profiling task, ret = %d", ret);
            MSPROF_INNER_ERROR("EK9999", "Failed to init profiling task, ret = %d", ret);
            break;
        }
        isOk = true;
        MSPROF_LOGI("Profiling task started");
    } while (0);
    return isOk;
}

int ProfManager::ProcessHandleFailed(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    std::string jobId = params->job_id;

    std::vector<std::string> devices = Utils::Split(params->devices, false, "", ",");
    for (size_t ii = 0; ii < devices.size(); ++ii) {
        MSPROF_LOGE("handle task failed, devid:%s, jobid:%s", devices[ii].c_str(), jobId.c_str());
        MSPROF_INNER_ERROR("EK9999", "handle task failed, devid:%s, jobid:%s", devices[ii].c_str(), jobId.c_str());
        Msprofiler::Api::DeviceResponse(std::stoi(devices[ii]));
    }
    return PROFILING_SUCCESS;
}

void ProfManager::SendFailedStatusInfo(const analysis::dvvp::message::StatusInfo &statusInfo,
    const std::string &jobId)
{
    if (jobId.empty()) {
        MSPROF_LOGE("Invalid params, jobId");
        MSPROF_INNER_ERROR("EK9999", "Invalid params, jobId");
        return;
    }
    analysis::dvvp::message::Status status;
    status.status = statusInfo.status;
    status.AddStatusInfo(statusInfo);
    MSPROF_LOGE("Failed to start profiling task, status=%s", status.ToString().c_str());
    MSPROF_INNER_ERROR("EK9999", "Failed to start profiling task, status=%s", status.ToString().c_str());
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> res;
    MSVP_MAKE_SHARED0_VOID(res, analysis::dvvp::proto::Response);
    res->set_jobid(jobId);
    res->set_status(analysis::dvvp::proto::FAILED);
    res->set_message(status.ToString());
    std::string encoded = analysis::dvvp::message::EncodeMessage(res);
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(jobId, encoded.c_str(), encoded.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to get transport, key=%s", jobId.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to get transport, key=%s", jobId.c_str());
    }
}

int ProfManager::Handle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    analysis::dvvp::message::StatusInfo statusInfo;
    statusInfo.status = analysis::dvvp::message::ERR;
    
    if (params == nullptr) {
        return PROFILING_FAILED;
    }
    statusInfo.dev_id = params->devices;

    if (!isInited_) {
        statusInfo.info = "Profiling is not inited";
        SendFailedStatusInfo(statusInfo, params->job_id);
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Handle profiling task");

    // check if device online
    if (!(params->host_profiling) && !(CheckIfDevicesOnline(params->devices, statusInfo.info))) {
        SendFailedStatusInfo(statusInfo, params->job_id);
        return PROFILING_FAILED;
    }
    bool isOk = CheckHandleSuc(params, statusInfo);
    if (isOk) {
        return PROFILING_SUCCESS;
    }
    if (ProcessHandleFailed(params) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Create state file failed!");
        MSPROF_INNER_ERROR("EK9999", "Create state file failed!");
    }
    return PROFILING_FAILED;
}

bool ProfManager::IsDeviceProfiling(const std::vector<std::string> &devices)
{
    for (size_t ii = 0; ii < devices.size(); ++ii) {
        for (auto iter = _tasks.begin(); iter != _tasks.end();) {
            if (iter->second->GetIsFinished()) {
                MSPROF_LOGI("_task(%s), GetIsFinished", iter->first.c_str());
                iter = _tasks.erase(iter);
                continue;
            }

            if (iter->second->IsDeviceRunProfiling(devices[ii])) {
                MSPROF_LOGE("device %s is running profiling", devices[ii].c_str());
                MSPROF_INNER_ERROR("EK9999", "device %s is running profiling", devices[ii].c_str());
                return true;
            }
            ++iter;
        }
    }
    return false;
}

int ProfManager::OnTaskFinished(const std::string &jobId)
{
    std::lock_guard<std::mutex> lk(taskMtx_);
    auto iter = _tasks.find(jobId);
    if (iter != _tasks.end()) {
        iter->second->Uinit();
        iter->second->SetIsFinished(true);
        MSPROF_LOGI("job_id %s finished", jobId.c_str());
    }

    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<ProfTask> ProfManager::GetTaskNoLock(const std::string &jobId)
{
    SHARED_PTR_ALIA<ProfTask> task = nullptr;
    auto iter = _tasks.find(jobId);
    if (iter != _tasks.end()) {
        task = iter->second;
    }

    return task;
}

SHARED_PTR_ALIA<ProfTask> ProfManager::GetTask(const std::string &jobId)
{
    std::lock_guard<std::mutex> lk(taskMtx_);
    return GetTaskNoLock(jobId);
}

int ProfManager::LaunchTask(SHARED_PTR_ALIA<ProfTask> task, const std::string &jobId, std::string &info)
{
    MSPROF_EVENT("Begin to launch task, jobId:%s", jobId.c_str());
    if (task == nullptr) {
        return PROFILING_FAILED;
    }
    if (GetTaskNoLock(jobId) != nullptr) {
        MSPROF_LOGE("task(%s) already exist, don't start again", jobId.c_str());
        task.reset();
        return PROFILING_FAILED;
    }

    int ret = task->Init();
    if (ret != PROFILING_SUCCESS) {
        info = "Init task failed";
        return ret;
    }

    do {
        MSPROF_LOGI("Profiling has %d tasks are running on the host, add new task(%s)", _tasks.size(), jobId.c_str());
        _tasks.insert(std::make_pair(jobId, task));
        task->SetThreadName(MSVP_PROF_TASK_THREAD_NAME);
        ret = task->Start();
        if (ret != PROFILING_SUCCESS) {
            info = "start task failed";
            return ret;
        }
        ret = PROFILING_SUCCESS;
    } while (0);

    return ret;
}

int ProfManager::StopTask(const std::string &jobId)
{
    MSPROF_EVENT("Begin to stop task, jobId:%s", jobId.c_str());
    auto task = GetTask(jobId);
    if (task != nullptr) {
        int ret = task->Stop();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Job_id %s stop failed", jobId.c_str());
            MSPROF_INNER_ERROR("EK9999", "Job_id %s stop failed", jobId.c_str());
            return PROFILING_FAILED;
        }
        MSPROF_LOGI("job_id %s stop", jobId.c_str());
    } else {
        MSPROF_LOGW("Job_id %s is invalid", jobId.c_str());
    }

    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> ProfManager::HandleProfilingParams(
    const std::string &sampleConfig)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params = nullptr;
    MSVP_MAKE_SHARED0_RET(params, analysis::dvvp::message::ProfileParams, nullptr);
    if (!(params->FromString(sampleConfig))) {
        MSPROF_LOGE("[ProfManager::HandleProfilingParams]Failed to parse sample config.");
        MSPROF_INNER_ERROR("EK9999", "Failed to parse sample config.");
        return nullptr;
    }
    MSPROF_LOGI("init platform adapter");
    if (PlatformAdapter::instance()->Init(params, ConfigManager::instance()->GetPlatformType()) != PROFILING_SUCCESS) {
        MSPROF_LOGE("platform adapter init fail.");
        return nullptr;
    }
    MSPROF_LOGI("HandleProfilingParams checking params");
    bool checkRet = ParamValidation::instance()->CheckProfilingParams(params);
    if (!checkRet) {
        MSPROF_LOGE("ProfileParams is not valid!");
        MSPROF_INNER_ERROR("EK9999", "ProfileParams is not valid!");
        return nullptr;
    }
    Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::instance()->GenerateLlcEvents(params);

    analysis::dvvp::common::utils::Utils::EnsureEndsInSlash(params->result_dir);
    MSPROF_LOGI("job_id:%s, result_dir:%s", params->job_id.c_str(), Utils::BaseName(params->result_dir).c_str());
    if (!CreateSampleJsonFile(params, params->result_dir)) {
        MSPROF_LOGE("Failed to create sample.json");
        MSPROF_INNER_ERROR("EK9999", "Failed to create sample.json");
        return nullptr;
    }

    return params;
}

int ProfManager::IdeCloudProfileProcess(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    int ret = PROFILING_FAILED;

    if (params == nullptr) {
        MSPROF_LOGE("Failed to check profiling params");
        MSPROF_INNER_ERROR("EK9999", "Failed to check profiling params");
        return PROFILING_FAILED;
    }
    do {
        if (params->is_cancel) {
            MSPROF_EVENT("Received libmsprof message to stop profiling, job_id:%s", params->job_id.c_str());
        } else {
            MSPROF_EVENT("Received libmsprof message to start profiling, job_id:%s", params->job_id.c_str());
            Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::instance()->SetSystemTraceParams(params, params);
        }

        MSVP_TRY_BLOCK_BREAK({
            ret = Handle(params);
        });
    } while (0);

    return ret;
}

bool ProfManager::PreGetDeviceList(std::vector<int> &devIds) const
{
    int numDevices = DrvGetDevNum();
    if (numDevices <= 0) {
        MSPROF_LOGE("Get dev's num %d failed", numDevices);
        MSPROF_INNER_ERROR("EK9999", "Get dev's num %d failed", numDevices);
        return false;
    }
    int ret = DrvGetDevIds(numDevices, devIds);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Get dev's id failed");
        MSPROF_INNER_ERROR("EK9999", "Get dev's id failed");
        return false;
    }
    UtilsStringBuilder<int> intBuilder;
    MSPROF_LOGI("Devices online: %s", intBuilder.Join(devIds, ",").c_str());
    return true;
}

bool ProfManager::CheckIfDevicesOnline(const std::string paramsDevices, std::string &statusInfo) const
{
    if (paramsDevices.compare("all") == 0) {
        return true;
    }
    std::vector<int> devIds;
    if (!PreGetDeviceList(devIds)) {
        MSPROF_LOGE("Get DevList failed.");
        MSPROF_INNER_ERROR("EK9999", "Get DevList failed.");
        return false;
    }

    bool ret = true;
    std::vector<std::string> devices =
        Utils::Split(paramsDevices, false, "", ",");
    std::vector<std::string> offlineIds;
    for (size_t i = 0; i < devices.size(); ++i) {
        if (!Utils::CheckStringIsNonNegativeIntNum(devices[i])) {
            MSPROF_LOGE("devId(%s) is not valid.", devices[i].c_str());
            MSPROF_INNER_ERROR("EK9999", "devId(%s) is not valid.", devices[i].c_str());
            return false;
        }
        int devId = std::stoi(devices[i]);
        if (devId == DEFAULT_HOST_ID) {
            MSPROF_LOGI("devId(%s) is host device", devices[i].c_str());
            continue;
        }
        int checkId = devId;
        if (Platform::instance()->RunSocSide()) {
            // on device, DrvGetDevIds returns host phyIds, transfer input by drvGetDevIDByLocalDevID before check
            checkId = DrvGetHostPhyIdByDeviceIndex(devId);
            MSPROF_LOGI("SOC scene, devId input: %d, devId to check: %d", devId, checkId);
        }
        auto it = find(devIds.begin(), devIds.end(), checkId);
        if (it == devIds.end()) {
            MSPROF_LOGE("device:%d is not online!", devId);
            MSPROF_INNER_ERROR("EK9999", "device:%d is not online!", devId);
            offlineIds.push_back(devices[i]);
            ret = false;
        }
    }
    if (!ret) {
        UtilsStringBuilder<std::string> strBuilder;
        std::string invalidIds = strBuilder.Join(offlineIds, ",");
        statusInfo = "device:" + invalidIds + " is not online";
    }
    return ret;
}
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
