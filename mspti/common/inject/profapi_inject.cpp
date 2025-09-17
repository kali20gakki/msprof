/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : profapi_inject.cpp
 * Description        : Injection of profapi.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
 */

#include "common/inject/profapi_inject.h"

#include <functional>
#include "activity/ascend/parser/kernel_parser.h"
#include "activity/ascend/parser/parser_manager.h"
#include "activity/ascend/parser/cann_track_cache.h"
#include "activity/ascend/parser/cann_hash_cache.h"
#include "common/function_loader.h"
#include "common/plog_manager.h"
#include "common/context_manager.h"
#include "common/utils.h"
#include "activity/ascend/parser/cann_hash_cache.h"
#include "activity/ascend/parser/communication_calculator.h"

namespace Mspti {
namespace Inject {
class ProfApiInject {
public:
    ProfApiInject() noexcept
    {
        Mspti::Common::RegisterFunction("libprofapi", "MsprofRegisterProfileCallback");
        Mspti::Common::RegisterFunction("libprofapi", "profSetProfCommand");
        Mspti::Common::RegisterFunction("libprofapi", "profRegReporterCallback");
        Mspti::Common::RegisterFunction("libprofapi", "profRegCtrlCallback");
        Mspti::Common::RegisterFunction("libprofapi", "MsprofGetHashId");
        Mspti::Common::RegisterFunction("libprofapi", "MsprofRegTypeInfo");
        auto ctrlHandle = [](uint32_t type, VOID_PTR data, uint32_t len) -> int32_t {
            UNUSED(type);
            UNUSED(data);
            UNUSED(len);
            return MSPTI_SUCCESS;
        };
        Mspti::Inject::profRegCtrlCallback(ctrlHandle);
    }
    ~ProfApiInject() = default;
};

ProfApiInject g_profApiInject;

int32_t MsprofRegisterProfileCallback(int32_t callbackType, VOID_PTR callback, uint32_t len)
{
    using MsprofRegisterProfileCallbackFunc = std::function<decltype(MsprofRegisterProfileCallback)>;
    static MsprofRegisterProfileCallbackFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction("libprofapi", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libprofapi.so");
    return func(callbackType, callback, len);
}

int32_t profRegReporterCallback(ProfReportHandle reporter)
{
    using profRegReporterCallbackFunc = std::function<decltype(profRegReporterCallback)>;
    static profRegReporterCallbackFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction("libprofapi", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libprofapi.so");
    return func(reporter);
}

int32_t profRegCtrlCallback(MsprofCtrlHandle handle)
{
    using profRegCtrlCallbackFunc = std::function<decltype(profRegCtrlCallback)>;
    static profRegCtrlCallbackFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction("libprofapi", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libprofapi.so");
    return func(handle);
}

int32_t profSetProfCommand(VOID_PTR command, uint32_t len)
{
    using profSetProfCommandFunc = std::function<decltype(profSetProfCommand)>;
    static profSetProfCommandFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction("libprofapi", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libprofapi.so");
    return func(command, len);
}

int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    UNUSED(moduleId);
    UNUSED(type);
    UNUSED(data);
    UNUSED(len);
    return MSPTI_SUCCESS;
}

uint64_t MsptiGetHashIdImpl(const char* hashInfo, size_t len)
{
    if (hashInfo == nullptr) {
        MSPTI_LOGE("GenHashId failed. hashInfo is nullptr");
        return 0;
    }
    return Mspti::Parser::CannHashCache::GetInstance().GenHashId(std::string(hashInfo, len));
}

int8_t MsptiHostFreqIsEnableImpl()
{
    constexpr int8_t enable = 1;
    constexpr int8_t disable = 0;
    return Mspti::Common::ContextManager::GetInstance()->HostFreqIsEnable() ? enable : disable;
}

int32_t MsptiApiReporterCallbackImpl(uint32_t agingFlag, const MsprofApi * const data)
{
    if (!data) {
        MSPTI_LOGE("Report Msprof Api data failed with nullptr.");
        return PROFAPI_ERROR;
    }

    if (data->level == MSPROF_REPORT_NODE_BASE_LEVEL) {
        if (data->type == MSPROF_REPORT_NODE_LAUNCH_TYPE &&
            Mspti::Parser::CannTrackCache::GetInstance().AppendNodeLunch(agingFlag == 1, data) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Compact data to ParserManager failed.");
            return PROFAPI_ERROR;
        }

        if (Mspti::Parser::ParserManager::GetInstance()->ReportApi(data) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Api data to ParserManager failed.");
            return PROFAPI_ERROR;
        }
    }

    if (data->level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
        if (Mspti::Parser::CannTrackCache::GetInstance().AppendCommunication(agingFlag, data) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Hccl Api data to ParserManager failed.");
            return PROFAPI_ERROR;
        }
    }

    return PROFAPI_ERROR_NONE;
}

int32_t MsptiEventReporterCallbackImpl(uint32_t agingFlag, const MsprofEvent* const event)
{
    UNUSED(agingFlag);
    UNUSED(event);
    return PROFAPI_ERROR_NONE;
}

int32_t MsptiCompactInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    if (data == nullptr || length != sizeof(struct MsprofCompactInfo)) {
        MSPTI_LOGE("Report Msprof Compact failed with nullptr.");
        return PROFAPI_ERROR;
    }
    const auto* compact = reinterpret_cast<const MsprofCompactInfo*>(data);
    if (compact->level == MSPROF_REPORT_RUNTIME_LEVEL && compact->type == RT_PROFILE_TYPE_TASK_TRACK) {
        if (Mspti::Parser::KernelParser::GetInstance().ReportRtTaskTrack(agingFlag, compact) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Compact data to ParserManager failed.");
            return PROFAPI_ERROR;
        }

        if (Mspti::Parser::CannTrackCache::GetInstance().AppendTsTrack(agingFlag == 1, compact) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Compact data to ParserManager failed.");
            return PROFAPI_ERROR;
        }
    }

    if (compact->level == MSPROF_REPORT_NODE_BASE_LEVEL
        && compact->type == MSPROF_REPORT_NODE_HCCL_OP_INFO_TYPE) {
        if (Mspti::Parser::CommunicationCalculator::GetInstance().AppendCompactInfo(
            agingFlag, compact)!= MSPTI_SUCCESS) {
            MSPTI_LOGE("Report Msprof Compact data to ParserManager failed.");
            return PROFAPI_ERROR;
        }
    }
    return PROFAPI_ERROR_NONE;
}

int32_t MsptiAddiInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    UNUSED(agingFlag);
    UNUSED(data);
    UNUSED(length);
    return PROFAPI_ERROR_NONE;
}

int32_t MsptiRegReportTypeInfoImpl(uint16_t level, uint32_t typeId, const char* name, size_t len)
{
    UNUSED(level);
    UNUSED(typeId);
    UNUSED(name);
    UNUSED(len);
    return PROFAPI_ERROR_NONE;
}
}
}