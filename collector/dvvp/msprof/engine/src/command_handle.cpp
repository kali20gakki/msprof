/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "command_handle.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "prof_api_common.h"
#include "profapi_plugin.h"
#include "prof_common.h"
#include "prof_acl_mgr.h"
#include "platform/platform.h"
#include "msprof_reporter_mgr.h"
#include "config/config_manager.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Plugin;
using ProfCommand = MsprofCommandHandle;
using namespace Msprof::Engine;
using namespace Analysis::Dvvp::Common::Config;

static std::mutex mtx;
static bool g_profInit = false;

int32_t SetCommandHandleProf(ProfCommand &command)
{
    // msprof param
    std::string msprofParams = Msprofiler::Api::ProfAclMgr::instance()->GetParamJsonStr();
    if (msprofParams.size() > PARAM_LEN_MAX) {
        MSPROF_LOGE("tparamsLen:%d, exceeds:%d", msprofParams.size(), PARAM_LEN_MAX);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    command.params.profDataLen = msprofParams.size();
    errno_t err = strncpy_s(command.params.profData, PARAM_LEN_MAX + 1, msprofParams.c_str(), msprofParams.size());
    if (err != EOK) {
        MSPROF_LOGE("string copy failed, err: %d", err);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    return ACL_SUCCESS;
}

int32_t CommandHandleProfInit()
{
    std::unique_lock<std::mutex> lock(mtx);
    if (g_profInit) {
        return ACL_SUCCESS;
    }
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.type = PROF_COMMANDHANDLE_TYPE_INIT;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfInit CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfInit CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    g_profInit = true;
    MSPROF_LOGI("CommandHandleProfInit, type:%u", command.type);
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

void ProcessDeviceList(ProfCommand &command, const uint32_t devIdList[], uint32_t devNums)
{
    for (uint32_t j = 0; j < devNums && j < MSVP_MAX_DEV_NUM; j++) {
        if (devIdList[j] == DEFAULT_HOST_ID) {
            command.devNums -= 1;
            continue;
        }
        command.devIdList[j] = devIdList[j];
    }
}

uint64_t GetProfSwitchHi(const uint64_t &dataTypeConfig)
{
    uint64_t profSwitchHi = 0U;
    auto platform = ConfigManager::instance()->GetPlatformType();
    // mc2, L1级别开启
    if ((platform == PlatformType::DC_TYPE || platform == PlatformType::CHIP_V4_1_0)
        && (dataTypeConfig & PROF_TASK_TIME_L1_MASK) != 0) {
        profSwitchHi |= PROF_DEV_AICPU_CHANNEL;
        return profSwitchHi;
    }
    // hccl aicpu下发, L0级别开启
    if (platform == PlatformType::CHIP_V4_1_0 && (dataTypeConfig & PROF_TASK_TIME_L0_MASK) != 0) {
        profSwitchHi |= PROF_DEV_AICPU_CHANNEL;
    }
    return profSwitchHi;
}

int32_t CommandHandleProfStart(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch)
{
    if (MsprofReporterMgr::instance()->StartReporters() != PROFILING_SUCCESS) {
        MSPROF_LOGE("CommandHandleProfStart start reporters failed");
        return ACL_ERROR;
    }
    if (profSwitch == 0 && Msprofiler::Api::ProfAclMgr::instance()->IsMsprofTxSwitchOn()) {
        MSPROF_LOGI("Profstart with only msproftx switch, need not send command to cann");
        return ACL_SUCCESS;
    }
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.profSwitch = profSwitch;
    command.profSwitchHi = GetProfSwitchHi(profSwitch);
    command.type = PROF_COMMANDHANDLE_TYPE_START;
    command.devNums = devNums;
    command.modelId = PROF_INVALID_MODE_ID;
    // Set 1 to enable clean event to ge and runtime to clear cache after report once
    command.cacheFlag = (Msprofiler::Api::ProfAclMgr::instance()->IsSubscribeMode()) ? 1 : 0;
    ProcessDeviceList(command, devIdList, devNums);
    if (command.devNums == 0) {
        if (!Platform::instance()->PlatformIsHelperHostSide()) {
            return ACL_SUCCESS;
        }
    }
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfStart CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfStart CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    MSPROF_LOGI("CommandHandleProfStart, profSwitch:0x%lx, profSwitchHi:0x%lx, device[0]:%u, devNums:%u",
        command.profSwitch, command.profSwitchHi, command.devIdList[0], command.devNums);
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

int32_t CommandHandleProfStop(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch)
{
    if (profSwitch == 0 && Msprofiler::Api::ProfAclMgr::instance()->IsMsprofTxSwitchOn()) {
        MSPROF_LOGI("Profstop with only msproftx switch, need not send command to cann");
    } else {
        ProfCommand command;
        auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
        FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
        command.profSwitch = profSwitch;
        command.profSwitchHi = GetProfSwitchHi(profSwitch);
        command.type = PROF_COMMANDHANDLE_TYPE_STOP;
        command.devNums = devNums;
        command.modelId = PROF_INVALID_MODE_ID;
        ProcessDeviceList(command, devIdList, devNums);
        if (command.devNums == 0) {
            if (!Platform::instance()->PlatformIsHelperHostSide()) {
                return ACL_SUCCESS;
            }
        }
        if (SetCommandHandleProf(command) != ACL_SUCCESS) {
            MSPROF_LOGE("ProfStop CommandHandle set failed");
            MSPROF_INNER_ERROR("EK9999", "ProfStop CommandHandle set failed");
            return ACL_ERROR_PROFILING_FAILURE;
        }
        MSPROF_LOGI("CommandHandleProfStop, profSwitch:0x%lx, profSwitchHi:0x%lx, device[0]:%u, devNums:%u",
            command.profSwitch, command.profSwitchHi, command.devIdList[0], command.devNums);
        ret = ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
        if (ret != ACL_SUCCESS) {
            return ret;
        }
    }
    return (MsprofReporterMgr::instance()->StopReporters() == PROFILING_SUCCESS) ?  ACL_SUCCESS : ACL_ERROR;
}

int32_t CommandHandleProfFinalize()
{
    std::unique_lock<std::mutex> lock(mtx);
    if (!g_profInit) {
        return ACL_SUCCESS;
    }
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.type = PROF_COMMANDHANDLE_TYPE_FINALIZE;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfFinalize CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfFinalize CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    g_profInit = false;
    MSPROF_LOGI("CommandHandleProfFinalize, type:%u", command.type);
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

int32_t CommandHandleProfSubscribe(uint32_t modelId, uint64_t profSwitch)
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.profSwitch = profSwitch;
    command.modelId = modelId;
    command.type = PROF_COMMANDHANDLE_TYPE_MODEL_SUBSCRIBE;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfSubscribe CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfSubscribe CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    MSPROF_LOGI("CommandHandleProfSubscribe, profSwitch:0x%lx, profSwitchHi:0x%lx, modelId:%u",
        command.profSwitch, command.profSwitchHi, command.modelId);
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

int32_t CommandHandleProfUnSubscribe(uint32_t modelId)
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.modelId = modelId;
    command.type = PROF_COMMANDHANDLE_TYPE_MODEL_UNSUBSCRIBE;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfUnSubscribe Set params failed!");
        MSPROF_INNER_ERROR("EK9999", "ProfUnSubscribe Set params failed!");
        return ACL_ERROR_PROFILING_FAILURE;
    }
    MSPROF_LOGI("CommandHandleProfUnSubscribe, modelId:%u", command.modelId);
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis
