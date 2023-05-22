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
using namespace Collector::Dvvp::Plugin;
using namespace Analysis::Dvvp::ProfilerCommon;
static std::mutex g_aclgraphProfMutex;

int aclgrphProfGraphSubscribe(const uint32_t graphId, const aclprofSubscribeConfig *profSubscribeConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfGraphSubscribe", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfGraphSubscribe");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("Param profSubscribeConfig is nullptr");
        std::string errorReason = "Param profSubscribeConfig should not be nullptr";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "profSubscribeConfig", errorReason}));
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
        std::string errorReason = "graph id is not loaded";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(graphId), "graphId", errorReason}));
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
    MSPROF_LOGI("Allocate subscription config to Ge, dataTypeConfig 0x%lx", dataTypeConfig);
    ProfAclMgr::instance()->AddModelLoadConf(dataTypeConfig);
    ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfSubscribe(graphId, dataTypeConfig);
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Successfully execute aclgrphProfGraphSubscribe");
    return ACL_SUCCESS;
}

int aclgrphProfGraphUnSubscribe(const uint32_t graphId)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfGraphUnSubscribe", "SocCloud"}));
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
        MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
            std::vector<std::string>({"aclgrphProfGraphUnSubscribe", "aclgrphProfGraphSubscribe"}));
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
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclprofGetGraphId", "SocCloud"}));
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
static int AclGrphProfInitPreCheck()
{
    if (Utils::IsDynProfMode()) {
        MSPROF_LOGI("Start to execute aclgrphProfInit not support in Dynamic profiling mode");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfInit", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    return ACL_SUCCESS;
}

Status aclgrphProfInit(CONST_CHAR_PTR profilerPath, uint32_t length)
{
    int ret = AclGrphProfInitPreCheck();
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    MSPROF_LOGI("Start to execute aclgrphProfInit");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (profilerPath == nullptr || strnlen(profilerPath, length) != length) {
        MSPROF_LOGE("profilerPath is nullptr or its length does not equals given length");
        std::string valueStr = (profilerPath == nullptr) ? "nullptr" : std::string(profilerPath);
        std::string errorReason = "Profiler path can not be nullptr, and its length should equal to the given length";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({valueStr, "profilerPath", errorReason}));
        return FAILED;
    }
    const static size_t aclGrphProfPathMaxLen = 4096;  // path max length: 4096
    if (length > aclGrphProfPathMaxLen || length == 0) {
        MSPROF_LOGE("length of profilerResultPath is illegal, the value is %zu, it should be in (0, %zu)",
                    length, aclGrphProfPathMaxLen);
        std::string errorReason = "The length of profilerResultPath should be in range (0, " +
            std::to_string(aclGrphProfPathMaxLen) + "]";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(length), "profilerResultPath length", errorReason}));
        std::string aclGrphProfPathMaxLenStr = std::to_string(aclGrphProfPathMaxLen);
        return FAILED;
    }

    ret = ProfAclMgr::instance()->ProfInitPrecheck();
    RETURN_IF_NOT_SUCCESS(ret);

    if (ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init acl manager");
        return FAILED;
    }
    std::string absoPath = Utils::RelativePathToAbsolutePath(std::string(profilerPath, length));
    if (absoPath.size() > aclGrphProfPathMaxLen) {
        MSPROF_LOGE("File path length check failed.");
        return ACL_ERROR_INVALID_PARAM;
    }
    ret = ProfAclMgr::instance()->ProfAclInit(absoPath);
    if (ret != ACL_SUCCESS) {
        return FAILED;
    }

    Status geRegisterRet = static_cast<Status>(RegisterReporterCallback());
    RETURN_IF_NOT_SUCCESS(geRegisterRet);

    Status geHandleInitRet = static_cast<Status>(CommandHandleProfInit());
    RETURN_IF_NOT_SUCCESS(geHandleInitRet);

    MSPROF_LOGI("Successfully execute aclgrphProfInit");
    return SUCCESS;
}

Status aclgrphProfFinalize()
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfFinalize", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfFinalize");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    int32_t ret = ProfAclMgr::instance()->ProfFinalizePrecheck();
    if (ret != ACL_SUCCESS) {
        if (ret == ACL_ERROR_PROF_NOT_RUN) {
            MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
                std::vector<std::string>({"aclgrphProfFinalize", "aclgrphProfInit"}));
        }
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
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "deviceidList", "deviceidList can not be nullptr"}));
        return false;
    }
    if (deviceNums == 0 || deviceNums > MSVP_MAX_DEV_NUM) {
        MSPROF_LOGE("[IsProfConfigValid]The device nums is invalid.");
        std::string errorReason = "The device nums should be in range(0, " + std::to_string(MSVP_MAX_DEV_NUM) + "]";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(deviceNums), "deviceNums", errorReason}));
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
        std::string errorReason = "The device nums should be in range[1, " + std::to_string(devCount) + "]";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(deviceNums), "deviceNums", errorReason}));
        return false;
    }
    std::unordered_set<uint32_t> record;
    for (size_t i = 0; i < deviceNums; ++i) {
        uint32_t devId = deviceidList[i];
        if (devId >= static_cast<uint32_t>(devCount)) {
            MSPROF_LOGE("Device id %u is not in range 0 ~ %d(exclude %d)", devId, devCount, devCount);
            std::string errorReason = "The device id should be in range[0, " + std::to_string(devCount) + ")";
            MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
                std::vector<std::string>({std::to_string(devId), "device id", errorReason}));
            return false;
        }
        if (record.count(devId) > 0) {
            MSPROF_LOGE("Device id %u is duplicatedly set", devId);
            std::string errorReason = "device id is duplicatedly set";
            MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
                std::vector<std::string>({std::to_string(devId), "device id", errorReason}));
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
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfCreateConfig", "SocCloud"}));
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

    config->config.devIdList[config->config.devNums] = DEFAULT_HOST_ID;
    config->config.devNums++;
    config->config.aicoreMetrics = static_cast<ProfAicoreMetrics>(aicoreMetrics);
    config->config.dataTypeConfig = dataTypeConfig;
    MSPROF_LOGI("Successfully create prof config");
    return config;
}

Status aclgrphProfDestroyConfig(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfDestroyConfig", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("Destroy profilerConfig failed, profilerConfig must not be nullptr");
        std::string errorReason = "profilerConfig can not be nullptr";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "profilerConfig", errorReason}));
        return FAILED;
    }
    delete profilerConfig;
    MSPROF_LOGI("Successfully destroy prof config.");
    return SUCCESS;
}

static bool PreCheckGraphProfConfig(const ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (profilerConfig == nullptr) {
        MSPROF_LOGE("Param profilerConfig is nullptr");
        return false;
    }
    if (profilerConfig->config.devNums == 0 || profilerConfig->config.devNums > MSVP_MAX_DEV_NUM) {
        MSPROF_LOGE("Param prolilerConfig is invalid");
        std::string devNumsStr = std::to_string(profilerConfig->config.devNums);
        std::string errorReason = "deviceNums should be in range[1, " + std::to_string(MSVP_MAX_DEV_NUM) + "]";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({devNumsStr, "deviceNums", errorReason}));
        return false;
    }
    return true;
}

Status aclgrphProfStart(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfStart", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfStart");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (!PreCheckGraphProfConfig(profilerConfig)) {
        MSPROF_LOGE("PreCheck GraphProfConfig Failed.");
        return FAILED;
    }

    int32_t ret = ProfAclMgr::instance()->ProfStartPrecheck();
    if (ret != ACL_SUCCESS) {
        if (ret == ACL_ERROR_PROF_NOT_RUN) {
            MSPROF_INPUT_ERROR("EK0002", std::vector<std::string>({"intf1", "intf2"}),
                std::vector<std::string>({"aclgrphProfStart", "aclgrphProfInit"}));
        }
        return FAILED;
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
        return FAILED;
    }

    ret = ProfAclMgr::instance()->ProfAclStart(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Start profiling failed, prof result = %d", ret);
        MSPROF_INNER_ERROR("EK9999", "Start profiling failed, prof result = %d", ret);
        return FAILED;
    }

    uint64_t dataTypeConfig = 0;
    ret = ProfAclMgr::instance()->GetDataTypeConfigFromParams(dataTypeConfig);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[aclgrphProfStart]get dataTypeConfig from params fail.");
        return FAILED;
    }
    Status geRet = static_cast<Status>(CommandHandleProfStart(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig));
    RETURN_IF_NOT_SUCCESS(geRet);

    MSPROF_LOGI("successfully execute aclgrphProfStart");
    return SUCCESS;
}

Status aclgrphProfStop(ACL_GRPH_PROF_CONFIG_PTR profilerConfig)
{
    if (Platform::instance()->PlatformIsHelperHostSide()) {
        MSPROF_LOGE("aclgrph api not support in helper");
        MSPROF_ENV_ERROR("EK0004", std::vector<std::string>({"intf", "platform"}),
            std::vector<std::string>({"aclgrphProfStop", "SocCloud"}));
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    MSPROF_LOGI("Start to execute aclgrphProfStop");
    std::lock_guard<std::mutex> lock(g_aclgraphProfMutex);
    if (!PreCheckGraphProfConfig(profilerConfig)) {
        MSPROF_LOGE("PreCheck GraphProfConfig Failed.");
        return FAILED;
    }

    int32_t ret = ProfAclMgr::instance()->ProfStopPrecheck();
    RETURN_IF_NOT_SUCCESS(ret);

    MSPROF_LOGI("Allocate stop config of profiling modules to Acl");
    uint64_t dataTypeConfig = 0;
    ret = ProfAclMgr::instance()->GetDataTypeConfigFromParams(dataTypeConfig);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[aclgrphProfStop]get dataTypeConfig from params fail.");
        return FAILED;
    }
    Status geRet = static_cast<Status>(CommandHandleProfStop(
        profilerConfig->config.devIdList, profilerConfig->config.devNums, dataTypeConfig));
    RETURN_IF_NOT_SUCCESS(geRet);

    for (uint32_t i = 0; i < profilerConfig->config.devNums; i++) {
        Msprof::Engine::FlushAllModule(std::to_string(profilerConfig->config.devIdList[i]));
    }

    ret = ProfAclMgr::instance()->ProfAclStop(&profilerConfig->config);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Stop profiling failed, prof result = %d", ret);
        return FAILED;
    }

    MSPROF_LOGI("Successfully execute aclprofStopProfiling");
    return ACL_SUCCESS;
}

int32_t GeOpenDeviceHandle(const uint32_t devId)
{
    if (ProfAclMgr::instance()->MsprofSetDeviceImpl(devId) != PROFILING_SUCCESS) {
        MSPROF_LOGE("MsprofSetDeviceImpl failed, devId=%d", devId);
        return PROFILING_FAILED;
    }
    if (!Utils::IsDynProfMode()) {
        MSPROF_LOGI("CommandHandleProfStart, Allocate config of profiling initialize");
        int32_t ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfInit();
        if (ret != SUCCESS) {
            MSPROF_LOGE("MsprofSetDeviceImpl, CommandHandleProfInit failed, devId:%u", devId);
            MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceImpl, CommandHandleProfInit failed, devId:%u", devId);
            return PROFILING_FAILED;
        }
    }
    MSPROF_LOGI("CommandHandleProfStart, Allocate start profiling config");
    uint32_t devIdList[1] = {devId};
    uint64_t dataTypeConfig = ProfAclMgr::instance()->GetCmdModeDataTypeConfig();
    int32_t ret = Analysis::Dvvp::ProfilerCommon::CommandHandleProfStart(devIdList, 1, dataTypeConfig | PROF_OP_DETAIL);
    if (ret != SUCCESS) {
        MSPROF_LOGE("MsprofSetDeviceImpl, CommandHandleProfStart failed, dataTypeConfig:0x%lx", dataTypeConfig);
        MSPROF_INNER_ERROR("EK9999", "MsprofSetDeviceImpl, CommandHandleProfStart failed, dataTypeConfig:0x%lx",
            dataTypeConfig);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
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
        if (CommandHandleProfStop(devIdList, 1, dataTypeConfig) != SUCCESS) {
            MSPROF_LOGE("Failed to CommandHandleProfStop on device:%u", devId);
            MSPROF_INNER_ERROR("EK9999", "Failed to CommandHandleProfStop on device:%u", devId);
        }
    }
    if (CommandHandleProfFinalize() != SUCCESS) {
        MSPROF_LOGE("Failed to CommandHandleProfFinalize");
        MSPROF_INNER_ERROR("EK9999", "Failed to CommandHandleProfFinalize");
    }
    for (uint32_t devId : devIds) {
        Msprof::Engine::FlushAllModuleForDynProf(std::to_string(devId));
    }
    Msprof::Engine::FlushAllModule();
}
} // namespace ge
