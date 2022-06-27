/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: command handle to acl/ge
 * Author: zcj
 * Create: 2020-11-26
 */
#include "command_handle.h"
#include "prof_api_common.h"
#include "profapi_plugin.h"
#include "prof_common.h"
#include "msprof_dlog.h"
#include "prof_acl_mgr.h"
#include "platform/platform.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Collector::Dvvp::Plugin;
using ProfCommand = MsprofCommandHandle;

int32_t SetCommandHandleProf(ProfCommand &command)
{
    // msprof param
    std::string msprofParams = Msprofiler::Api::ProfAclMgr::instance()->GetParamJsonStr();
    if (msprofParams.size() > PARAM_LEN_MAX) {
        MSPROF_LOGE("tparamsLen:%d, exceeds:%d", msprofParams.size(), PARAM_LEN_MAX);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    command.params.profDataLen = msprofParams.size();
    errno_t err = strncpy_s(command.params.profData, PARAM_LEN_MAX, msprofParams.c_str(), msprofParams.size() + 1);
    if (err != EOK) {
        MSPROF_LOGE("string copy failed, err: %d", err);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    return ACL_SUCCESS;
}

int32_t CommandHandleProfInit()
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.type = PROF_COMMANDHANDLE_TYPE_INIT;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfInit CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfInit CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
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

int32_t CommandHandleProfStart(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch)
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.profSwitch = profSwitch;
    command.type = PROF_COMMANDHANDLE_TYPE_START;
    command.devNums = devNums;
    command.modelId = PROF_INVALID_MODE_ID;
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
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

int32_t CommandHandleProfStop(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch)
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.profSwitch = profSwitch;
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
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}

int32_t CommandHandleProfFinalize()
{
    ProfCommand command;
    auto ret = memset_s(&command, sizeof(command), 0, sizeof(command));
    FUNRET_CHECK_FAIL_RET_VALUE(ret, EOK, PROFILING_FAILED);
    command.type = PROF_COMMANDHANDLE_TYPE_FINALIZE;
    if (SetCommandHandleProf(command) != ACL_SUCCESS) {
        MSPROF_LOGE("ProfFinalize CommandHandle set failed");
        MSPROF_INNER_ERROR("EK9999", "ProfFinalize CommandHandle set failed");
        return ACL_ERROR_PROFILING_FAILURE;
    }
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
    return ProfApiPlugin::instance()->MsprofProfSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(ProfCommand));
}
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis
