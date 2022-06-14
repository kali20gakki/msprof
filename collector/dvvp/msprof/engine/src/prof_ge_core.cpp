/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: prof manager function
 * Author: zcj
 * Create: 2020-07-18
 */
#include "prof_ge_core.h"
#include <google/protobuf/util/json_util.h>
#include "ge/ge_prof.h"

#include "ai_drv_dev_api.h"
#include "command_handle.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "msprof_callback_handler.h"
#include "msprof_callback_impl.h"
#include "op_desc_parser.h"
#include "prof_acl_mgr.h"
#include "prof_api_common.h"
#include "utils/utils.h"
#include "param_validation.h"
#include "profapi_plugin.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Analyze;
using namespace Msprofiler::Api;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::validation;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Plugin;
using namespace Analysis::Dvvp::ProfilerCommon;
static std::mutex g_aclgraphProfMutex;

Status aclgrphProfGraphSubscribe(const uint32_t graphId, const aclprofSubscribeConfig *profSubscribeConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfGraphSubscribe");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("Param profSubscribeConfig is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t ret = ProfAclMgr::instance()->ProfSubscribePrecheck();
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    uint32_t deviceId = 0;
    ret = ProfApiPlugin::instance()->MsprofProfGetDeviceIdByGeModelIdx(graphId, &deviceId);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Graph id %u is not loaded", graphId);
        return ACL_ERROR_INVALID_MODEL_ID;
    }

    if (ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        MSPROF_INNER_ERROR("EK9999", "Failed to init acl manager");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    ret = ProfAclMgr::instance()->ProfAclModelSubscribe(graphId, deviceId, &profSubscribeConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Subscribe graph info failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Subscribe graph info failed, prof result = %d", ret);
        return ret;
    }

    uint64_t dataTypeConfig = ProfAclMgr::instance()->ProfAclGetDataTypeConfig(&profSubscribeConfig->config);
    MSPROF_LOGI("Allocate subscription config to Ge, dataTypeConfig %x", dataTypeConfig);
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfSubscribe(graphId, dataTypeConfig);
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Successfully execute aclgrphProfGraphSubscribe");
    return ACL_SUCCESS;
}

Status aclgrphProfGraphUnSubscribe(const uint32_t graphId)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfGraphUnSubscribe ");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    int32_t ret = ProfAclMgr::instance()->ProfSubscribePrecheck();
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    if (!ProfAclMgr::instance()->IsModelSubscribed(graphId)) {
        MSPROF_LOGE("Graph Id %u is not subscribed when unsubcribed", graphId);
        return ACL_ERROR_INVALID_MODEL_ID;
    }

    MSPROF_LOGI("Allocate unsubscription config to Acl, dataTypeConfig");
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfUnSubscribe(graphId);
    RETURN_IF_NOT_SUCCESS(ret);

    uint32_t devId = 0;
    if (ProfAclMgr::instance()->GetDeviceSubscribeCount(graphId, devId) == 0) {
        Msprof::Engine::FlushAllModule(std::to_string(devId));
    }

    ret = ProfAclMgr::instance()->ProfAclModelUnSubscribe(graphId);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    MSPROF_LOGI("Successfully execute aclgrphProfGraphUnSubscribe");
    return ACL_SUCCESS;
}

size_t aclprofGetGraphId(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return static_cast<size_t>(ACL_ERROR_FEATURE_UNSUPPORTED);
    }
    MSPROF_LOGD("Start to execute aclprofGetGraphId");
    uint32_t result;
    int32_t ret = OpDescParser::GetModelId(opInfo, opInfoLen, index, &result);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed execute aclprofGetGraphId");
        MSPROF_INNER_ERROR("EK9999", "Failed execute aclprofGetGraphId");
        return (size_t)ret;
    }
    MSPROF_LOGD("Successfully  execute aclprofGetGraphId");
    return (size_t)result;
}

namespace ge {
Status aclgrphProfInit(CONST_CHAR_PTR profilerPath, uint32_t length)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfInit");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (profilerPath == nullptr || strlen(profilerPath) != length) {
        MSPROF_LOGE("profilerPath is nullptr or its length does not equals given length");
        return FAILED;
    }
    const static size_t aclGrphProfPathMaxLen = 4096;  // path max length: 4096
    if (length > aclGrphProfPathMaxLen || length == 0) {
        MSPROF_LOGE("length of profilerResultPath is illegal, the value is %zu, it should be in (0, %zu)",
                    length, aclGrphProfPathMaxLen);
        std::string aclGrphProfPathMaxLenStr = std::to_string(aclGrphProfPathMaxLen);
        return FAILED;
    }

    int32_t ret = ProfAclMgr::instance()->ProfInitPrecheck();
    if (ret != ACL_SUCCESS) {
        return FAILED;
    }

    if (ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        MSPROF_INNER_ERROR("EK9999", "Failed to init acl manager");
        return FAILED;
    }
    MSPROF_LOGI("Initialize profiling by using ProfInit");
    std::string path(profilerPath, length);
    ret = ProfAclMgr::instance()->ProfAclInit(path);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("AclProfiling init fail, profiling result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "AclProfiling init fail, profiling result = %d", ret);
        return FAILED;
    }

    Status geRegisterRet = static_cast<Status>(RegisterReporterCallback());
    RETURN_IF_NOT_SUCCESS(geRegisterRet);

    MSPROF_LOGI("Allocate config of profiling initialize to Ge");
    Status geHandleInitRet = Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit();
    RETURN_IF_NOT_SUCCESS(geHandleInitRet);

    MSPROF_LOGI("Successfully execute aclgrphProfInit");
    return SUCCESS;
}

Status aclgrphProfFinalize()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfFinalize");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    int32_t ret = ProfAclMgr::instance()->ProfFinalizePrecheck();
    if (ret != ACL_SUCCESS) {
        return FAILED;
    }

    MSPROF_LOGI("Allocate config of profiling finalize to Ge");
    Status geRet = static_cast<Status>(CommandHandleProfFinalize());
    RETURN_IF_NOT_SUCCESS(geRet);

    Msprof::Engine::FlushAllModule();

    MSPROF_LOGI("Finalize profiling by using ProfFinalize");
    ret = ProfAclMgr::instance()->ProfAclFinalize();
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Failed to finalize profiling, profiling result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Failed to finalize profiling, profiling result = %d", ret);
        return FAILED;
    }

    MSPROF_LOGI("Successfully execute aclgrphProfFinalize");
    return ACL_SUCCESS;
}

using PROF_AICORE_EVENTS_PTR = ProfAicoreEvents *;
bool IsProfConfigValid(CONST_UINT32_T_PTR deviceidList, uint32_t deviceNums)
{
    if (deviceidList == nullptr) {
        MSPROF_LOGE("[IsProfConfigValid]deviceIdList is nullptr");
        return false;
    }
    if (deviceNums == 0 || deviceNums > MSVP_MAX_DEV_NUM) {
        MSPROF_LOGE("[IsProfConfigValid]The device nums is invalid.");
        std::string deviceNumsStr = std::to_string(deviceNums);
        std::string maxDevNums = std::to_string(MSVP_MAX_DEV_NUM);
        return false;
    }
    // real device num
    int32_t devCount = analysis::dvvp::driver::DrvGetDevNum();
    if (devCount == PROFILING_FAILED) {
        MSPROF_LOGE("[IsProfConfigValid]Get the Device count fail.");
        return false;
    }
    if (deviceNums > static_cast<uint32_t>(devCount)) {
        MSPROF_LOGE("[IsProfConfigValid]Device num(%u) is not in range 1 ~ %d.", deviceNums, devCount);
        return false;
    }
    std::unordered_set<uint32_t> record;
    for (size_t i = 0; i < deviceNums; ++i) {
        uint32_t devId = deviceidList[i];
        if (devId >= static_cast<uint32_t>(devCount)) {
            MSPROF_LOGE("Device id %u is not in range 0 ~ %d(exclude %d)", devId, devCount, devCount);
            return false;
        }
        if (record.count(devId) > 0) {
            MSPROF_LOGE("Device id %u is duplicatedly set", devId);
            return false;
        }
        record.insert(devId);
    }
    return true;
}

struct aclgrphProfConfig {
    ProfConfig config;
};
using ACL_GRPH_PROF_CONFIG_PTR = aclgrphProfConfig *;

ACL_GRPH_PROF_CONFIG_PTR aclgrphProfCreateConfig(UINT32_T_PTR deviceidList, uint32_t deviceNums,
    ProfilingAicoreMetrics aicoreMetrics, PROF_AICORE_EVENTS_PTR aicoreEvents, uint64_t dataTypeConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return nullptr;
    }
    UNUSED(aicoreEvents);
    if (!IsProfConfigValid(deviceidList, deviceNums)) {
        return nullptr;
    }
    aclgrphProfConfig *config = new (std::nothrow) aclgrphProfConfig();
    if (config == nullptr) {
        MSPROF_LOGE("new aclgrphProfConfig fail");
        MSPROF_ENV_ERROR("EK0201", std::vector<std::string>({"buf_size"}),
            std::vector<std::string>({std::to_string(sizeof(aclgrphProfConfig))}));
        return nullptr;
    }
    config->config.devNums = deviceNums;
    if (memcpy_s(config->config.devIdList, sizeof(config->config.devIdList),
                 deviceidList, deviceNums * sizeof(uint32_t)) != EOK) {
        MSPROF_LOGE("copy devID failed. size = %u", deviceNums);
        delete config;
        return nullptr;
    }

    config->config.aicoreMetrics = static_cast<ProfAicoreMetrics>(aicoreMetrics);
    ProfAclMgr::instance()->AddAiCpuModelConf(dataTypeConfig);
    config->config.dataTypeConfig = dataTypeConfig;
    MSPROF_LOGI("Successfully create prof config");
    return config;
}

Status aclgrphProfDestroyConfig(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("Destroy profilerConfig failed, profilerConfig must not be nullptr");
        return FAILED;
    }
    delete profilerConfig;
    MSPROF_LOGI("Successfully destroy prof config.");
    return SUCCESS;
}

static bool PreCheckGraphProfConfig(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("Param profilerConfig is nullptr");
        return false;
    }
    if (profilerConfig->config.devNums == 0 || profilerConfig->config.devNums > MSVP_MAX_DEV_NUM) {
        MSPROF_LOGE("Param prolilerConfig is invalid");
        std::string devNumsStr = std::to_string(profilerConfig->config.devNums);
        return false;
    }
    return true;
}

Status aclgrphProfStart(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfStart");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (!PreCheckGraphProfConfig(profilerConfig)) {
        MSPROF_LOGE("PreCheck GraphProfConfig Failed.");
        MSPROF_INNER_ERROR("EK9999", "PreCheck GraphProfConfig Failed.");
        return FAILED;
    }

    int32_t ret = ProfAclMgr::instance()->ProfStartPrecheck();
    if (ret != ACL_SUCCESS) {
        return FAILED;
    }
    // check switch
    if ((profilerConfig->config.dataTypeConfig & (~PROF_SWITCH_SUPPORT)) != 0) {
        MSPROF_LOGE("dataTypeConfig:0x%x, supported switch is:0x%x",
                    profilerConfig->config.dataTypeConfig, PROF_SWITCH_SUPPORT);
        std::string dataTypeConfigStr = std::to_string(profilerConfig->config.dataTypeConfig);
        return FAILED;
    }

    MSPROF_LOGI("Start profiling config by using aclprofStartProfiling");
    ret = ProfAclMgr::instance()->ProfAclStart(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Start profiling failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Start profiling failed, prof result = %d", ret);
        return FAILED;
    }

    MSPROF_LOGI("Allocate start profiling config to Ge");
    uint64_t dataTypeConfig = profilerConfig->config.dataTypeConfig;
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    Status geRet = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig | PROF_OP_DETAIL);
    RETURN_IF_NOT_SUCCESS(geRet);

    MSPROF_LOGI("successfully execute aclgrphProfStart");
    return SUCCESS;
}

Status aclgrphProfStop(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfStop");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (!PreCheckGraphProfConfig(profilerConfig)) {
        MSPROF_LOGE("PreCheck GraphProfConfig Failed.");
        MSPROF_INNER_ERROR("EK9999", "PreCheck GraphProfConfig Failed.");
        return FAILED;
    }

    int32_t ret = ProfAclMgr::instance()->ProfStopPrecheck();
    if (ret != ACL_SUCCESS) {
        return FAILED;
    }

    // check config
    uint64_t dataTypeConfig = 0;
    for (uint32_t i = 0; i < profilerConfig->config.devNums; i++) {
        ret = ProfAclMgr::instance()->ProfAclGetDataTypeConfig(
            profilerConfig->config.devIdList[i], dataTypeConfig);
        if (ret != ACL_SUCCESS) {
            return ret;
        }
        if (dataTypeConfig != profilerConfig->config.dataTypeConfig) {
            MSPROF_LOGE("DataTypeConfig stop: %x different from start: %x",
                        profilerConfig->config.dataTypeConfig, dataTypeConfig);
            std::string dataTypeConfigStr = std::to_string(profilerConfig->config.dataTypeConfig);
            return ACL_ERROR_INVALID_PROFILING_CONFIG;
        }
    }

    MSPROF_LOGI("Allocate stop config of profiling modules to Acl");
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    Status geRet = static_cast<Status>(CommandHandleProfStop(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig | PROF_OP_DETAIL));
    RETURN_IF_NOT_SUCCESS(geRet);

    for (uint32_t i = 0; i < profilerConfig->config.devNums; i++) {
        Msprof::Engine::FlushAllModule(std::to_string(profilerConfig->config.devIdList[i]));
    }

    ret = ProfAclMgr::instance()->ProfAclStop(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Stop profiling failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Stop profiling failed, prof result = %d", ret);
        return FAILED;
    }

    MSPROF_LOGI("Successfully execute aclprofStopProfiling");
    return ACL_SUCCESS;
}

void GeOpenDeviceHandle(const uint32_t devId)
{
    if (ProfAclMgr::instance()->MsprofSetDeviceImpl(devId) == PROFILING_SUCCESS) {
        MSPROF_LOGI("CommandHandleProfStart, Allocate config of profiling initialize");
        int32_t ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit();
        if (ret != SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceImpl, CommandHandleProfInit failed, devId:%u", devId);
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceImpl, CommandHandleProfInit failed, devId:%u", devId);
            return;
        }
        MSPROF_LOGI("CommandHandleProfStart, Allocate start profiling config");
        uint32_t devIdList[1] = {devId};
        uint64_t dataTypeConfig = ProfAclMgr::instance()->GetCmdModeDataTypeConfig();
        ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
        ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(devIdList, 1,
            dataTypeConfig | PROF_OP_DETAIL);
        if (ret != SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceImpl, CommandHandleProfStart failed, dataTypeConfig:0x%x", dataTypeConfig);
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceImpl, CommandHandleProfStart failed, dataTypeConfig:0x%lx",
                dataTypeConfig);
        }
    }
}

void GeFinalizeHandle()
{
    std::vector<uint32_t> devIds;
    Msprofiler::Api::ProfAclMgr::instance()->GetRunningDevices(devIds);
    for (uint32_t devId : devIds) {
        if (devId == DEFAULT_HOST_ID) {
            continue;
        }
        uint64_t dataTypeConfig = 0;
        if (Msprofiler::Api::ProfAclMgr::instance()->ProfAclGetDataTypeConfig(devId, dataTypeConfig) != ACL_SUCCESS) {
            continue;
        }
        uint32_t devIdList[1] = {devId};
        ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
        if (CommandHandleProfStop(devIdList, 1, dataTypeConfig) != SUCCESS) {
            MSPROF_LOGE("Failed to CommandHandleProfStop on device:%u", devId);
            MSPROF_INNER_ERROR("EK9999", "Failed to CommandHandleProfStop on device:%u", devId);
        }
    }
    if (CommandHandleProfFinalize() != SUCCESS) {
        MSPROF_LOGE("Failed to CommandHandleProfFinalize");
        MSPROF_INNER_ERROR("EK9999", "Failed to CommandHandleProfFinalize");
    }
    Msprof::Engine::FlushAllModule();
}
} // namespace ge
