/**
* @file prof_acl_mgr.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_PROF_ACL_MGR_H
#define MSPROF_ENGINE_PROF_ACL_MGR_H

#include <condition_variable>
#include <string>
#include "acl/acl_prof.h"
#include "common/singleton/singleton.h"
#include "common/thread/thread.h"
#include "message/prof_params.h"
#include "aicpu_plugin.h"
#include "utils/utils.h"
#include "prof_api_common.h"
#include "proto/profiler.pb.h"
#include "proto/profiler_ext.pb.h"
#include "profapi_plugin.h"

namespace Msprofiler {
namespace Api {
using namespace analysis::dvvp::common::utils;
// struct of subscribe model info
struct ProfSubscribeInfo {
    bool subscribed;
    uint32_t devId;
    int *fd;
};

/**
 * @name  WorkMode
 * @brief profiling api work mode
 */
enum WorkMode {
    WORK_MODE_OFF,          // profiling not at work
    WORK_MODE_CMD,          // profiling work on cmd mode
    WORK_MODE_API_CTRL,     // profiling work on api ctrl mode (ProfInit)
    WORK_MODE_SUBSCRIBE,    // profiling work on subscribe mode
};

void DeviceResponse(int devId);

class ProfAclMgr : public analysis::dvvp::common::singleton::Singleton<ProfAclMgr> {
public:
    ProfAclMgr();
    virtual ~ProfAclMgr();

public:
    int Init();
    int UnInit();

    /* Precheck rule:
     *               mode    OFF               CMD                API_CTRL        SUB
     *  api
     *  Callback-Init        ok                already run        conflict        conflict
     *  Callback-Finalize    ok                ok                 conflict        conflict
     *  Api-Init             ok                already run        repeat init     conflict
     *  Api-Start            not inited        already run        ok              conflict
     *  Api-Stop             not inited        already run        ok              conflict
     *  Api-Finalize         not inited        already run        ok              conflict
     *  Sub-Sub              ok                already run        conflict        ok
     *  Sub-UnSub            not subscribed    already run        conflict        ok
     */
    int CallbackInitPrecheck();
    int ProfInitPrecheck();
    int ProfStartPrecheck();
    int ProfStopPrecheck();
    int ProfFinalizePrecheck();
    int ProfSubscribePrecheck();
    void SetModeToCmd();
    void SetModeToOff();
    bool IsCmdMode();
    bool IsModeOff();

    // api ctrl
    int ProfAclInit(const std::string& profResultPath);
    bool IsInited();
    int InitUploader(const std::string& devIdStr);
    int ProfAclStart(PROF_CONF_CONST_PTR profStartCfg);
    int ProfAclStop(PROF_CONF_CONST_PTR profStopCfg);
    int ProfAclFinalize();
    int ProfAclGetDataTypeConfig(const uint32_t devId, uint64_t& dataTypeConfig);
    void HandleResponse(const uint32_t devId);
    int GetDataTypeConfigFromParams(uint64_t &dataTypeConfig);

    // subscribe
    uint64_t ProfAclGetDataTypeConfig(PROF_SUB_CONF_CONST_PTR profSubscribeConfig);
    int ProfAclModelSubscribe(const uint32_t modelId, const uint32_t devId,
                              PROF_SUB_CONF_CONST_PTR profSubscribeConfig);
    int ProfAclModelUnSubscribe(const uint32_t modelId);
    void FlushAllData(const std::string &devId);
    bool IsModelSubscribed(const uint32_t modelId);
    int GetSubscribeFdForModel(const uint32_t modelId);
    // task info query
    void GetRunningDevices(std::vector<uint32_t> &devIds);
    uint64_t GetDeviceSubscribeCount(uint32_t modelId, uint32_t &devId);
    uint64_t GetCmdModeDataTypeConfig();
    std::string GetParamJsonStr();
    // task datatypeconfig add

public:
    int32_t MsprofInitAclJson(VOID_PTR data, uint32_t len);
    int32_t MsprofInitGeOptions(VOID_PTR data, uint32_t len);
    int32_t MsprofInitAclEnv(const std::string &envValue);
    int32_t MsprofInitHelper(VOID_PTR data, uint32_t len);
    int32_t MsprofFinalizeHandle(void);
    int32_t MsprofTxHandle(void);
    void MsprofHostHandle(void);
    int32_t MsprofSetDeviceImpl(uint32_t devId);
    void AddModelLoadConf(uint64_t &dataTypeConfig) const;
    int32_t MsprofSetConfig(aclprofConfigType cfgType, std::string config);

private:
    int MsprofTxSwitchPrecheck();
    int DoHostHandle();
// struct of acltask info
struct ProfAclTaskInfo {
    uint64_t count;
    uint64_t dataTypeConfig;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
};

std::map<int, std::string> WorkModeStr_ = {
    {WORK_MODE_OFF, "Profiling is not enabled"},
    {WORK_MODE_CMD, "Profiling is working on cmd(msprofbin/acl json/env/options/helper) mode"},
    {WORK_MODE_API_CTRL, "Profiling is working on api(acl api/graph api) mode"},
    {WORK_MODE_SUBSCRIBE, "Profiling is working on subscribe mode"}
};

// class for waiting response from device
class DeviceResponseHandler : public analysis::dvvp::common::thread::Thread {
public:
    explicit DeviceResponseHandler(const uint32_t devId);
    virtual ~DeviceResponseHandler();

public:
    void HandleResponse();

private:
    void Run(const struct error_message::Context &errorContext) override;

private:
    uint32_t devId_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

private:
    int InitResources();
    std::string GenerateDevDirName(const std::string& devId);
    int RecordOutPut(const std::string &data);
    int InitApiCtrlUploader(const std::string& devIdStr);
    int InitSubscribeUploader(const std::string& devIdStr);
    int CheckDeviceTask(PROF_CONF_CONST_PTR profStartCfg);
    int StartDeviceTask(const uint32_t devId, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    void WaitAllDeviceResponse();
    void WaitDeviceResponse(const uint32_t devId);
    int ProfStartAiCpuTrace(const uint64_t dataTypeConfig, const uint32_t devNums, CONST_UINT32_T_PTR devIdList);
    int UpdateSubscribeInfo(const uint32_t modelId, const uint32_t devId,
                            PROF_SUB_CONF_CONST_PTR profSubscribeConfig);
    int StartDeviceSubscribeTask(const uint32_t modelId, const uint32_t devId,
                                 PROF_SUB_CONF_CONST_PTR profSubscribeConfig);
    void ProfDataTypeConfigHandle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    std::string MsprofCheckAndGetChar(CHAR_PTR data, uint32_t dataLen);
    void CloseSubscribeFd(const uint32_t devId);
    void CloseSubscribeFd(const uint32_t devId, const uint32_t modelId);
    int32_t MsprofResultPathAdapter(const std::string &dir, std::string &resultPath);
    void PrintWorkMode(WorkMode mode);
    int32_t MsprofHelperParamConstruct(const std::string &msprofPath, const std::string &paramsJson);
    void MsprofSetMemberValue();
    int LaunchHostAndDevTasks(const uint32_t devNums, CONST_UINT32_T_PTR devIdList);
    int CancleHostAndDevTasks(const uint32_t devNums, CONST_UINT32_T_PTR devIdList);
    int StartSubscribeDeviceTask(const uint32_t devId, const uint32_t modelId,
        PROF_SUB_CONF_CONST_PTR profSubscribeConfig);
    int CancleSubscribeDeviceTask(const uint32_t devId, const uint32_t modelId);
    int CancleSubScribeDevTask(const uint32_t devId, const uint32_t modelId);
    int LaunchSubscribeDevTask(const uint32_t devId, const uint32_t modelId,
                               PROF_SUB_CONF_CONST_PTR profSubscribeConfig);

private:
    bool isReady_;
    WorkMode mode_;
    std::string resultPath_;
    std::string baseDir_;
    std::string storageLimit_;
    std::map<uint32_t, ProfAclTaskInfo> devTasks_; // devId, info
    std::map<uint32_t, SHARED_PTR_ALIA<DeviceResponseHandler>> devResponses_;
    std::map<std::string, std::string> devUuid_;
    std::map<uint32_t, ProfSubscribeInfo> subscribeInfos_;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr_;
    std::mutex mtx_; // mutex for start/stop
    std::mutex mtxUploader_; // mutex for uploader
    std::mutex mtxDevResponse_; // mutex for device response
    std::mutex mtxSubscribe_;
    std::map<int32_t, SHARED_PTR_ALIA<Msprof::Engine::AicpuPlugin>> enginMap_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    PROF_CONF_CONST_PTR profStratCfg_;
    uint64_t dataTypeConfig_;
    uint64_t startIndex_;
};
} // namespace Api
} // namespace Msprofiler

#endif
