/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: prof manager function
 * Author: zcj
 * Create: 2020-07-18
 */
#include "prof_acl_core.h"

#include <google/protobuf/util/json_util.h>

#include "acl/acl_prof.h"

#include "ai_drv_dev_api.h"
#include "command_handle.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "msprof_callback_handler.h"
#include "msprof_callback_impl.h"
#include "op_desc_parser.h"
#include "prof_acl_mgr.h"
#include "msprof_tx_manager.h"
#include "utils/utils.h"
#include "prof_api_common.h"
#include "transport/hash_data.h"
#include "profapi_plugin.h"
#include "platform/platform.h"

using namespace Analysis::Dvvp::Analyze;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Msprofiler::Api;
using namespace Msprof::MsprofTx;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Plugin;

static std::mutex g_aclprofMutex;
static uint64_t g_indexId = 1;

aclError aclprofInit(CONST_CHAR_PTR profilerResultPath, size_t length)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofInit", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofInit");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);

    if (profilerResultPath == nullptr || strlen(profilerResultPath) != length) {
        MSPROF_LOGE("profilerResultPath is nullptr or its length does not equals given length");
        std::string errorReason = "profilerResultPath is nullptr or its length does not equals given length";
        std::string valueStr = (profilerResultPath == nullptr) ? "nullptr" : std::string(profilerResultPath);
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({valueStr, "profilerResultPath", errorReason}));
        return ACL_ERROR_INVALID_PARAM;
    }
    const static size_t aclProfPathMaxLen = 4096;   // path max length: 4096
    if (length > aclProfPathMaxLen || length == 0) {
        MSPROF_LOGE("length of profilerResultPath is illegal, the value is %zu, it should be in (0, %zu)",
                    length, aclProfPathMaxLen);
        std::string ErrorReason = "it should be in (0, " + std::to_string(aclProfPathMaxLen)+ ")";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(length), "profilerResultPath length", ErrorReason}));
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t ret = ProfAclMgr::instance()->ProfInitPrecheck();
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    if (ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    MSPROF_LOGI("Initialize profiling by using ProfInit");
    std::string path(profilerResultPath, length);
    ret = ProfAclMgr::instance()->ProfAclInit(path);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("AclProfiling init fail, profiling result = %d", ret);
        return ret;
    }

    ret = Analysis::Dvvp::ProfilerCommon::RegisterReporterCallback();
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Allocate config of profiling initialize to Acl");
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit();
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Acl has been allocated config of profiling initialize, successfully execute aclprofInit");
    return ACL_SUCCESS;
}

aclError aclprofFinalize()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofFinalize", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofFinalize");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);

    int32_t ret = ProfAclMgr::instance()->ProfFinalizePrecheck();
    if (ret != ACL_SUCCESS) {
        if (ret == ACL_ERROR_PROF_NOT_RUN) {
            MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
                std::vector<std::string>({"aclprofFinalize", "aclprofInit"}));
        }
        return ret;
    }

    MSPROF_LOGI("Allocate config of profiling finalize to Acl");
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfFinalize();
    RETURN_IF_NOT_SUCCESS(ret);

    Msprof::Engine::FlushAllModule();

    MSPROF_LOGI("Finalize profiling by using ProfFinalize");
    ret = ProfAclMgr::instance()->ProfAclFinalize();
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed to finalize profiling, profiling result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Failed to finalize profiling, profiling result = %d", ret);
        return ret;
    }

    MSPROF_LOGI("Successfully execute aclprofFinalize");
    return ACL_SUCCESS;
}

using ACL_PROF_AICORE_EVENTS_PTR = aclprofAicoreEvents *;
bool IsValidProfConfigPreCheck(CONST_UINT32_T_PTR deviceIdList, uint32_t deviceNums,
    ACL_PROF_AICORE_EVENTS_PTR aicoreEvents)
{
    if (deviceNums != 0 && deviceIdList == nullptr) {
        MSPROF_LOGE("deviceIdList is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "deviceIdList", "The value of deviceIdList should not be nullptr"}));
        return false;
    }

    if (aicoreEvents != nullptr) {
        MSPROF_LOGE("aicoreEvents must be nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"*aicoreEvents", "aicoreEvents", "The value of aicoreEvents must be nullptr"}));
        return false;
    }

    if (deviceNums > MSVP_MAX_DEV_NUM) {
        MSPROF_LOGE("The device nums is invalid.");
        std::string errorReason = "The number of device should be smaller than " + std::to_string(MSVP_MAX_DEV_NUM);
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(deviceNums), "deviceNums", errorReason}));
        return false;
    }
    return true;
}

bool IsValidProfConfig(CONST_UINT32_T_PTR deviceIdList, uint32_t deviceNums, ACL_PROF_AICORE_EVENTS_PTR aicoreEvents)
{
    if (!IsValidProfConfigPreCheck(deviceIdList, deviceNums, aicoreEvents)) {
        return false;
    }
    int32_t devCount = analysis::dvvp::driver::DrvGetDevNum();
    if (devCount == PROFILING_FAILED) {
        MSPROF_LOGE("Get the Device count fail.");
        return false;
    }

    if (deviceNums > static_cast<uint32_t>(devCount)) {
        MSPROF_LOGE("Device num(%u) is not in range 1 ~ %d.", deviceNums, devCount);
        std::string errorReason = "device number should be in range [1, " + std::to_string(devCount) + "]";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(deviceNums), "deviceNums", errorReason}));
        return false;
    }

    std::unordered_set<uint32_t> record;
    for (size_t i = 0; i < deviceNums; ++i) {
        uint32_t devId = deviceIdList[i];
        if (devId >= static_cast<uint32_t>(devCount)) {
            MSPROF_LOGE("[IsValidProfConfig]Device id %u is not in range 0 ~ %d(exclude %d)",
                devId, devCount, devCount);
            std::string errorReason = "device id should be in range (0," + std::to_string(devCount) + "]";
            MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
                std::vector<std::string>({std::to_string(devId), "device id", errorReason}));
            return false;
        }
        if (record.count(devId) > 0) {
            MSPROF_LOGE("[IsValidProfConfig]Device id %u is duplicatedly set", devId);
            std::string errorReason = "device id is duplicatedly set";
            MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
                std::vector<std::string>({std::to_string(devId), "device id", errorReason}));
            return false;
        }
        record.insert(devId);
    }
    return true;
}

struct aclprofConfig {
    ProfConfig config;
};
using ACL_PROF_CONFIG_PTR = aclprofConfig *;
using ACL_PROF_CONFIG_CONST_PTR = const aclprofConfig *;

ACL_PROF_CONFIG_PTR aclprofCreateConfig(UINT32_T_PTR deviceIdList, uint32_t deviceNums,
    aclprofAicoreMetrics aicoreMetrics, ACL_PROF_AICORE_EVENTS_PTR aicoreEvents, uint64_t dataTypeConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofCreateConfig", "SocCloud"}));
        return nullptr;
    }
    if (!IsValidProfConfig(deviceIdList, deviceNums, aicoreEvents)) {
        return nullptr;
    }
    if ((deviceNums == 0) && !(dataTypeConfig & PROF_MSPROFTX_MASK)) {
        return nullptr;
    }

    aclprofConfig *profConfig = new (std::nothrow) aclprofConfig();
    if (profConfig == nullptr) {
        MSPROF_LOGE("new aclprofConfig fail");
        MSPROF_ENV_ERROR("EK0201", std::vector<std::string>({"buf_size"}),
            std::vector<std::string>({std::to_string(sizeof(aclprofConfig))}));
        return nullptr;
    }
    profConfig->config.aicoreMetrics = static_cast<ProfAicoreMetrics>(aicoreMetrics);
    profConfig->config.dataTypeConfig = dataTypeConfig;
    ProfAclMgr::instance()->AddAiCpuModelConf(profConfig->config.dataTypeConfig);

    profConfig->config.devNums = deviceNums;
    if (deviceNums != 0) {
        if (memcpy_s(profConfig->config.devIdList, sizeof(profConfig->config.devIdList),
            deviceIdList, deviceNums * sizeof(uint32_t)) != EOK) {
            MSPROF_LOGE("copy devID failed. size = %u", deviceNums);
            delete profConfig;
            return nullptr;
        }
    }
    if (profConfig->config.dataTypeConfig & PROF_MSPROFTX_MASK) {
        profConfig->config.devIdList[profConfig->config.devNums] = DEFAULT_HOST_ID;
        profConfig->config.devNums++;
    }
    MSPROF_LOGI("Successfully create prof config");
    return profConfig;
}

aclError aclprofDestroyConfig(ACL_PROF_CONFIG_CONST_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofDestroyConfig", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("destroy profilerConfig failed, profilerConfig must not be nullptr");
        std::string errorReason = "profilerConfig can not be nullptr";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "profilerConfig", errorReason}));
        return ACL_ERROR_INVALID_PARAM;
    }
    delete profilerConfig;
    MSPROF_LOGI("Successfully destroy prof config");
    return ACL_SUCCESS;
}

static aclError PreCheckProfConfig(ACL_PROF_CONFIG_CONST_PTR profilerConfig)
{
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("Param profilerConfig is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "profilerConfig", "Prof config can not be nullptr"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    if (profilerConfig->config.dataTypeConfig == 0) {
        MSPROF_LOGE("Param profilerConfig dataTypeConfig is zero");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"0", "dataTypeConfig", "dataTypeConfig can not be zero"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    // check switch
    if ((profilerConfig->config.dataTypeConfig & (~PROF_SWITCH_SUPPORT)) != 0) {
        MSPROF_LOGE("dataTypeConfig:0x%lx, supported switch is:0x%lx",
                    profilerConfig->config.dataTypeConfig, PROF_SWITCH_SUPPORT);
        std::string dataTypeConfigStr = "0x" + Utils::Int2HexStr<uint64_t>(profilerConfig->config.dataTypeConfig);
        std::string supportConfigStr = "0x" + Utils::Int2HexStr<uint64_t>(PROF_SWITCH_SUPPORT);
        std::string errorReason = "dataTypeConfig is not support, supported switch is:" + supportConfigStr;
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({dataTypeConfigStr, "dataTypeConfig", errorReason}));
        return ACL_ERROR_PROF_MODULES_UNSUPPORTED;
    }
    if (profilerConfig->config.devNums > MSVP_MAX_DEV_NUM + 1) {
        MSPROF_LOGE("Param prolilerConfig is invalid");
        std::string devNumsStr = std::to_string(profilerConfig->config.devNums);
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclError aclprofStart(ACL_PROF_CONFIG_CONST_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofStart", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofStartProfiling");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);
    aclError aclRet = PreCheckProfConfig(profilerConfig);
    if (aclRet != ACL_SUCCESS) {
        MSPROF_LOGE("PreCheck ProfConfig Failed.");
        return aclRet;
    }
    int32_t ret = ProfAclMgr::instance()->ProfStartPrecheck();
    if (ret != ACL_SUCCESS) {
        if (ret == ACL_ERROR_PROF_NOT_RUN) {
            MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
                std::vector<std::string>({"aclprofStart", "aclprofInit"}));
        }
        return ret;
    }

    MSPROF_LOGI("Start profiling config by using aclprofStartProfiling");
    ret = ProfAclMgr::instance()->ProfAclStart(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Start profiling failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Start profiling failed, prof result = %d", ret);
        return ret;
    }

    MSPROF_LOGI("Allocate start profiling config to Acl");
    uint64_t dataTypeConfig = profilerConfig->config.dataTypeConfig;
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    ProfAclMgr::instance()->AddRuntimeTraceConf(dataTypeConfig);
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig | PROF_OP_DETAIL);
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Acl has been allocated start profiling config, successfully execute aclprofStartProfiling");
    return ACL_SUCCESS;
}

aclError aclprofStop(ACL_PROF_CONFIG_CONST_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofStop", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofStopProfiling");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);
    aclError aclRet = PreCheckProfConfig(profilerConfig);
    if (aclRet != ACL_SUCCESS) {
        MSPROF_LOGE("PreCheck ProfConfig Failed.");
        return aclRet;
    }

    int32_t ret = ProfAclMgr::instance()->ProfStopPrecheck();
    if (ret != ACL_SUCCESS) {
        if (ret == ACL_ERROR_PROF_NOT_RUN) {
            MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
                std::vector<std::string>({"aclprofStop", "aclprofInit"}));
        }
        return ret;
    }

    uint64_t dataTypeConfig = 0;
    for (uint32_t i = 0; i < profilerConfig->config.devNums; i++) {
        ret = ProfAclMgr::instance()->ProfAclGetDataTypeConfig(profilerConfig->config.devIdList[i], dataTypeConfig);
        RETURN_IF_NOT_SUCCESS(ret);
        ret = ProfAclMgr::instance()->StopProfConfigCheck(profilerConfig->config.dataTypeConfig, dataTypeConfig);
        RETURN_IF_NOT_SUCCESS(ret);
    }
    MSPROF_LOGI("Allocate stop config of profiling modules to Acl");
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    ProfAclMgr::instance()->AddRuntimeTraceConf(dataTypeConfig);
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStop(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig | PROF_OP_DETAIL);
    RETURN_IF_NOT_SUCCESS(ret);

    for (uint32_t i = 0; i < profilerConfig->config.devNums; i++) {
        Msprof::Engine::FlushAllModule(std::to_string(profilerConfig->config.devIdList[i]));
    }
    ret = ProfAclMgr::instance()->ProfAclStop(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Stop profiling failed, prof result = %d", ret);
        return ret;
    }

    MSPROF_LOGI("Acl has been allocated stop config, successfully execute aclprofStopProfiling");
    return ACL_SUCCESS;
}

aclError aclprofModelSubscribe(const uint32_t modelId, const aclprofSubscribeConfig *profSubscribeConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofModelSubscribe", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofModelSubscribe");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);

    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("Param profSubscribeConfig is nullptr");
        std::string errorReason = "Param profSubscribeConfig can not be nullptr";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "profSubscribeConfig", errorReason}));
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t ret = ProfAclMgr::instance()->ProfSubscribePrecheck();
    RETURN_IF_NOT_SUCCESS(ret);

    uint32_t deviceId = 0;
    aclError aclRet = ProfApiPlugin::instance()->MsprofProfGetDeviceIdByGeModelIdx(modelId, &deviceId);
    if (aclRet != ACL_SUCCESS) {
        MSPROF_LOGE("Model id %u is not loaded", modelId);
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(modelId), "modelId", "model id is not loaded"}));
        return ACL_ERROR_INVALID_MODEL_ID;
    }

    if (ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        MSPROF_INNER_ERROR("EK9999", "Failed to init acl manager");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    ret = ProfAclMgr::instance()->ProfAclModelSubscribe(modelId, deviceId, &profSubscribeConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Subscribe model info failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Subscribe model info failed, prof result = %d", ret);
        return ret;
    }

    uint64_t dataTypeConfig = ProfAclMgr::instance()->ProfAclGetDataTypeConfig(&profSubscribeConfig->config);
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    MSPROF_LOGI("Allocate subscription config to Acl, dataTypeConfig:0x%lx, modelId:%ul", dataTypeConfig, modelId);
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfSubscribe(modelId, dataTypeConfig);
    RETURN_IF_NOT_SUCCESS(ret);

    if (HashData::instance()->Init() == PROFILING_FAILED) {
        MSPROF_LOGE("HashData init failed in aclprofModelSubscribe");
        MSPROF_INNER_ERROR("EK9999", "HashData init failed in aclprofModelSubscribe");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    MSPROF_LOGI("Successfully execute aclprofModelSubscribe");
    return ACL_SUCCESS;
}

aclError aclprofModelUnSubscribe(const uint32_t modelId)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofModelUnSubscribe", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclprofModelUnSubscribe");
    std::lock_guard<std::mutex> lock(g_aclprofMutex);

    int32_t ret = ProfAclMgr::instance()->ProfSubscribePrecheck();
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    if (!ProfAclMgr::instance()->IsModelSubscribed(modelId)) {
        MSPROF_LOGE("Model Id %u is not subscribed when unsubcribed", modelId);
        MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
            std::vector<std::string>({"aclprofModelUnSubscribe", "aclprofModelSubscribe"}));
        return ACL_ERROR_INVALID_MODEL_ID;
    }

    MSPROF_LOGI("Allocate unsubscription config to Acl, dataTypeConfig");
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfUnSubscribe(modelId);
    RETURN_IF_NOT_SUCCESS(ret);

    uint32_t devId = 0;
    if (ProfAclMgr::instance()->GetDeviceSubscribeCount(modelId, devId) == 0) {
        Msprof::Engine::FlushAllModule(std::to_string(devId));
    }

    ret = ProfAclMgr::instance()->ProfAclModelUnSubscribe(modelId);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    MSPROF_LOGI("Successfully execute aclprofModelUnSubscribe");
    return ACL_SUCCESS;
}

size_t aclprofGetModelId(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofGetModelId", "SocCloud"}));
        return static_cast<size_t>(ACL_ERROR_FEATURE_UNSUPPORTED);
    }
    MSPROF_LOGD("Start to execute aclprofGetModelId");
    uint32_t result;
    int32_t ret = OpDescParser::GetModelId(opInfo, opInfoLen, index, &result);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed execute aclprofGetModelId");
        MSPROF_INNER_ERROR("EK9999", "Failed execute aclprofGetModelId");
        return (size_t)ret;
    }
    MSPROF_LOGD("Successfully execute aclprofGetModelId");
    return (size_t)result;
}

struct aclprofStepInfo {
    bool startFlag;
    bool endFlag;
    uint64_t indexId;
};

using ACLPROF_STEPINFO_PTR = aclprofStepInfo *;

aclError aclprofGetStepTimestamp(ACLPROF_STEPINFO_PTR stepInfo, aclprofStepTag tag, aclrtStream stream)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofGetStepTimestamp", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    if (stepInfo == nullptr) {
        MSPROF_LOGE("stepInfo is nullptr.");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stepInfo", "stepInfo can not be nullptr"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    if (stepInfo->startFlag && tag == ACL_STEP_START) {
        MSPROF_LOGE("This stepInfo already started.");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (stepInfo->endFlag && tag == ACL_STEP_END) {
        MSPROF_LOGE("This stepInfo already stop.");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (tag == ACL_STEP_START) {
        stepInfo->startFlag = true;
    } else {
        stepInfo->endFlag = true;
    }
    auto geRet = ProfApiPlugin::instance()->MsprofProfSetStepInfo(stepInfo->indexId, tag, stream);
    if (geRet != PROFAPI_SUCCESS) {
        MSPROF_LOGE("[aclprofGetStepTimestamp]Call ProfSetStepInfo function failed, ge result = %d", geRet);
        return ACL_ERROR_GE_FAILURE;
    }
    MSPROF_LOGI("[aclprofGetStepTimestamp] Call ProfSetStepInfo success, indexId:%u", stepInfo->indexId);
    return ACL_SUCCESS;
}

ACLPROF_STEPINFO_PTR aclprofCreateStepInfo()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofCreateStepInfo", "SocCloud"}));
        return nullptr;
    }
    auto stepInfo = new (std::nothrow)aclprofStepInfo();
    if (stepInfo == nullptr) {
        MSPROF_LOGE("new stepInfo fail");
        MSPROF_ENV_ERROR("EK0201", std::vector<std::string>({"buf_size"}),
            std::vector<std::string>({std::to_string(sizeof(aclprofStepInfo))}));
        return nullptr;
    }
    stepInfo->startFlag = false;
    stepInfo->endFlag = false;
    std::lock_guard<std::mutex> lock(g_aclprofMutex);
    stepInfo->indexId = g_indexId++;
    return stepInfo;
}

void aclprofDestroyStepInfo(ACLPROF_STEPINFO_PTR stepInfo)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofDestroyStepInfo", "SocCloud"}));
        return;
    }
    if (stepInfo == nullptr) {
        MSPROF_LOGE("destroy stepInfo failed, stepInfo must not be nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stepInfo", "stepInfo can not be nullptr when destroy"}));
    } else {
        delete stepInfo;
        MSPROF_LOGI("Successfully destroy stepInfo");
    }
}

void *aclprofCreateStamp()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofCreateStamp", "SocCloud"}));
        return nullptr;
    }
    return MsprofTxManager::instance()->CreateStamp();
}

void aclprofDestroyStamp(VOID_PTR stamp)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofDestroyStamp", "SocCloud"}));
        return;
    }
    if (stamp == nullptr) {
        return;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    MsprofTxManager::instance()->DestroyStamp(stampInstancePtr);
}

aclError aclprofSetCategoryName(uint32_t category, const char *categoryName)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofSetCategoryName", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    return MsprofTxManager::instance()->SetCategoryName(category, categoryName);
}

aclError aclprofSetStampCategory(VOID_PTR stamp, uint32_t category)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofSetStampCategory", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->SetStampCategory(stampInstancePtr, category);
}

aclError aclprofSetStampPayload(VOID_PTR stamp, const int32_t type, VOID_PTR value)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofSetStampPayload", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->SetStampPayload(stampInstancePtr, type, value);
}

aclError aclprofSetStampTraceMessage(VOID_PTR stamp, const char *msg, uint32_t msgLen)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofSetStampTraceMessage", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->SetStampTraceMessage(stampInstancePtr, msg, msgLen);
}

aclError aclprofMark(VOID_PTR stamp)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofMark", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->Mark(stampInstancePtr);
}

aclError aclprofPush(VOID_PTR stamp)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofPush", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->Push(stampInstancePtr);
}

aclError aclprofPop()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofPop", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    return MsprofTxManager::instance()->Pop();
}

aclError aclprofRangeStart(VOID_PTR stamp, uint32_t *rangeId)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofRangeStart", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    auto stampInstancePtr = static_cast<ACL_PROF_STAMP_PTR>(stamp);
    return MsprofTxManager::instance()->RangeStart(stampInstancePtr, rangeId);
}

aclError aclprofRangeStop(uint32_t rangeId)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofRangeStop", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    return MsprofTxManager::instance()->RangeStop(rangeId);
}

aclError aclprofSetConfig(aclprofConfigType configType, const char *val, uint32_t valLen)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("acl api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofSetConfig", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    if (configType < ACL_PROF_STORAGE_LIMIT || configType >= ACL_PROF_ARGS_MAX) {
        MSPROF_LOGE("[qqq]");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (val == nullptr || strlen(val) != valLen) {
        MSPROF_LOGE("[www]");
        return ACL_ERROR_INVALID_PARAM;
    }
    std::string config(val, valLen);
    int32_t ret = ProfAclMgr::instance()->MsprofSetConfig(configType, config);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[eee]");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}