/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: zcj
 * Create: 2020-09-26
 */
#include "msprof_callback_impl.h"
#include "dyn_prof_server.h"
#include "errno/error_code.h"
#include "msprof_callback_handler.h"
#include "prof_acl_mgr.h"
#include "prof_manager.h"
#include "prof_acl_core.h"
#include "prof_ge_core.h"
#include "profapi_plugin.h"
#include "msprof_tx_manager.h"
#include "command_handle.h"
#include "platform/platform.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::host;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::DynProf;
using namespace Msprof::Engine;

bool CheckMsprofBin(std::string &envValue)
{
    envValue = Utils::GetEnvString(PROFILER_SAMPLE_CONFIG_ENV);
    if (!envValue.empty()) {
        return true;
    }
    return false;
}

int32_t MsprofCtrlCallbackImplHandle(uint32_t type, VOID_PTR data, uint32_t len)
{
    int32_t ret = MSPROF_ERROR;
    std::string envValue;
    if (CheckMsprofBin(envValue)) {
        ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitAclEnv(envValue);
    } else {
        switch (type) {
            case MSPROF_CTRL_INIT_ACL_JSON:
                ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitAclJson(data, len);
                break;
            case MSPROF_CTRL_INIT_GE_OPTIONS:
                ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitGeOptions(data, len);
                break;
            case MSPROF_CTRL_INIT_HELPER:
                ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitHelper(data, len);
                break;
            default:
                MSPROF_LOGE("Invalid MsprofCtrlCallback type: %u", type);
        }
    }
    if (ret == MSPROF_ERROR_ACL_JSON_OFF) {
        return MSPROF_ERROR_NONE;
    }
    if (ret != MSPROF_ERROR_NONE) {
        return ret;
    }
    if (Msprofiler::Api::ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        MSPROF_INNER_ERROR("EK9999", "Failed to init acl manager");
        Msprofiler::Api::ProfAclMgr::instance()->SetModeToOff();
        return MSPROF_ERROR;
    }
    Msprofiler::Api::ProfAclMgr::instance()->MsprofHostHandle();
    return MSPROF_ERROR_NONE;
}

// ctrl callback
int32_t MsprofCtrlCallbackImpl(uint32_t type, VOID_PTR data, uint32_t len)
{
    MSPROF_EVENT("MsprofCtrlCallback called, type: %u", type);

    if (type == MSPROF_CTRL_FINALIZE) {
        DynProfMngSrv::instance()->StopDynProfSrv();
        return Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    }

    if (Utils::IsDynProfMode()) {
        if (DynProfMngSrv::instance()->IsStarted()) {
            return MSPROF_ERROR_NONE;
        }
        if (ProfApiPlugin::instance()->MsprofProfRegDeviceStateCallback(MsprofSetDeviceCallbackForDynProf) != 0) {
            MSPROF_LOGE("Dynamic profiling failed to register device state callback.");
            return MSPROF_ERROR;
        }
        return DynProfMngSrv::instance()->StartDynProfSrv();
    }
    if ((type == MSPROF_CTRL_INIT_DYNA) &&
        (Utils::GetEnvString(PROFILER_SAMPLE_CONFIG_ENV).empty() ||
        !Msprofiler::Api::ProfAclMgr::instance()->IsModeOff())) {
        MSPROF_LOGI("Dynamic profiling environment not really set, so do nothing.");
        return MSPROF_ERROR_NONE;
    }
    int32_t ret = MsprofCtrlCallbackImplHandle(type, data, len);
    if (ret != MSPROF_ERROR_NONE) {
        return ret;
    }

    if (Platform::instance()->PlatformIsHelperHostSide()) {
        ret = RegisterReporterCallback();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceCallbackImpl, RegisterReporterCallback failed");
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceCallbackImpl, RegisterReporterCallback failed");
            return MSPROF_ERROR;
        }
        uint64_t profConfigType = Msprofiler::Api::ProfAclMgr::instance()->GetCmdModeDataTypeConfig();
        ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(nullptr, 0, profConfigType);
        if (ret == ACL_SUCCESS) {
            return PROFILING_SUCCESS;
        }
        return MSPROF_ERROR;
    }
    // register device state callback
    ret = ProfApiPlugin::instance()->MsprofProfRegDeviceStateCallback(MsprofSetDeviceCallbackImpl);
    if (ret != 0) {
        MSPROF_LOGE("Failed to register device state callback");
        MSPROF_CALL_ERROR("EK9999", "ProfStart CommandHandle set failed");
        return MSPROF_ERROR;
    }
    return MSPROF_ERROR_NONE;
}

// set device callback
int32_t MsprofSetDeviceCallbackImpl(VOID_PTR data, uint32_t len)
{
    ProfSetDevPara *setCfg = (struct ProfSetDevPara *)data;
    MSPROF_EVENT("MsprofSetDeviceCallback called, is open: %d", setCfg->isOpen);
    if (setCfg->isOpen) {
        if (!Msprofiler::Api::ProfAclMgr::instance()->IsCmdMode()) {
            MSPROF_LOGI("MsprofSetDeviceCallbackImpl, not on cmd mode");
            return MSPROF_ERROR;
        }
        std::string info;
        if (!(ProfManager::instance()->CheckIfDevicesOnline(std::to_string(setCfg->deviceId), info))) {
            MSPROF_LOGE("MsprofSetDeviceCallbackImpl, devId:%u is invalid, error info:%s",
                setCfg->deviceId, info.c_str());
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceCallbackImpl, devId:%u is invalid, error info:%s",
                setCfg->deviceId, info.c_str());
            return MSPROF_ERROR;
        }
        int ret = RegisterReporterCallback();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceCallbackImpl, RegisterReporterCallback failed");
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceCallbackImpl, RegisterReporterCallback failed");
            return MSPROF_ERROR;
        }

        ret = ge::GeOpenDeviceHandle(setCfg->deviceId);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceCallbackImpl, GeOpenDeviceHandle failed");
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceCallbackImpl, GeOpenDeviceHandle failed");
            return MSPROF_ERROR;
        }
    }
    return MSPROF_ERROR_NONE;
}

// set device callback for dynamic profiling
int32_t MsprofSetDeviceCallbackForDynProf(VOID_PTR data, uint32_t len)
{
    ProfSetDevPara *setCfg = reinterpret_cast<ProfSetDevPara *>(data);
    DynProfMngSrv::instance()->SetDeviceInfo(setCfg);

    int32_t ret = RegisterReporterCallback();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("MsprofSetDeviceCallbackImpl, RegisterReporterCallback failed, ret=%d", ret);
        return MSPROF_ERROR;
    }

    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling CommandHandleProfInit failed, ret=%d", ret);
        return MSPROF_ERROR;
    }

    return MSPROF_ERROR_NONE;
}

inline int32_t InternalErrorCodeToExternal(int32_t internalErrorCode)
{
    return (internalErrorCode == PROFILING_SUCCESS ? MSPROF_ERROR_NONE : MSPROF_ERROR);
}

// reporter callback
int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    switch (moduleId) {
        case MSPROF_MODULE_DATA_PREPROCESS:
        case MSPROF_MODULE_HCCL:
        case MSPROF_MODULE_ACL:
        case MSPROF_MODULE_FRAMEWORK:
        case MSPROF_MODULE_RUNTIME:
        case MSPROF_MODULE_MSPROF:
            return InternalErrorCodeToExternal(
                Msprof::Engine::MsprofCallbackHandler::reporters_[moduleId].HandleMsprofRequest(type, data, len));
        default:
            MSPROF_LOGE("Invalid reporter callback moduleId: %u", moduleId);
            MSPROF_INNER_ERROR("EK9999", "Invalid reporter callback moduleId: %u", moduleId);
    }
    return ACL_ERROR_PROFILING_FAILURE;
}

int32_t MsprofApiReporterCallbackImpl(uint32_t agingFlag, const MsprofApi &api)
{
    return MSPROF_ERROR_NONE;
}
 
int32_t MsprofEventReporterCallbackImpl(uint32_t agingFlag, const MsprofEvent &event)
{
    return MSPROF_ERROR_NONE;
}
 
int32_t MsprofCompactInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    return MSPROF_ERROR_NONE;
}
 
int32_t MsprofAddiInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    return MSPROF_ERROR_NONE;
}
 
int32_t MsprofRegReportTypeInfoImpl(uint16_t level, uint32_t typeId, const std::string &typeName)
{
    return MSPROF_ERROR_NONE;
}
 
int32_t MsprofGetHashIdImpl(const std::string &hashInfo)
{
    return MSPROF_ERROR_NONE;
}

int32_t RegisterReporterCallback()
{
    if (Msprof::Engine::MsprofCallbackHandler::reporters_.empty()) {
        MSPROF_LOGI("MsprofCallbackHandler InitReporters");
        Msprof::Engine::MsprofCallbackHandler::InitReporters();
    }
    MSPROF_LOGI("Call profRegReporterCallback");
    aclError ret = ProfApiPlugin::instance()->MsprofProfRegReporterCallback(MsprofReporterCallbackImpl);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed to register reporter callback");
        MSPROF_CALL_ERROR("EK9999", "Failed to register reporter callback");
        return ret;
    }
    return RegisterNewReporterCallback();
}

int32_t RegisterNewReporterCallback()
{
    MSPROF_LOGI("Call profRegProfilerCallback");
    return PROFILING_SUCCESS;
}

void RegisterMsprofTxReporterCallback()
{
    if (Msprof::Engine::MsprofCallbackHandler::reporters_.empty()) {
        MSPROF_LOGI("MsprofCallbackHandler InitReporters");
        Msprof::Engine::MsprofCallbackHandler::InitReporters();
    }
    MSPROF_LOGI("Call MsprofTxManager SetReporterCallback");
    Msprof::MsprofTx::MsprofTxManager::instance()->SetReporterCallback(MsprofReporterCallbackImpl);
}

int32_t MsprofilerInit()
{
    MSPROF_EVENT("Started to register profiling ctrl callback.");
    // register ctrl callback
    aclError ret = ProfApiPlugin::instance()->MsprofProfRegCtrlCallback(MsprofCtrlCallbackImpl);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed to register ctrl callback");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

static int32_t g_initResult = MsprofilerInit();

int32_t GetRegisterResult()
{
    return g_initResult;
}
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis
