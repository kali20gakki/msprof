/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.cpp
 * Description        : Realization of mstx inject func.
 * Author             : msprof team
 * Creation Date      : 2024/07/31
 * *****************************************************************************
*/
#include "mstx_inject.h"
#include <unordered_map>
#include "msprof_dlog.h"
#include "errno/error_code.h"
#include "runtime_plugin.h"
#include "mstx_data_handler.h"
#include "utils/utils.h"

using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mstx;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;

static std::mutex g_mutex;
static std::unordered_map<uint64_t, aclrtStream> g_eventIdsWithStream;

namespace MsprofMstxApi {
constexpr uint64_t MSTX_MODEL_ID = 0xFFFFFFFFU;
constexpr uint16_t MSTX_TAG_ID = 11;

void MstxMarkAFunc(const char* msg, aclrtStream stream)
{
    MSPROF_LOGI("Start to execute %s", __func__);
    if (!MstxDataHandler::instance()->IsStart()) {
        MSPROF_LOGW("Mstx data handler is not started");
        return;
    }
    if (msg == nullptr || strnlen(msg, MAX_MESSAGE_LEN) == MAX_MESSAGE_LEN) {
        MSPROF_LOGE("Input msg for %s is invalid", __func__);
        return;
    }
    if (!Utils::CheckCharValid(msg)) {
        MSPROF_LOGE("msg has invalid characters.");
        return;
    }
    uint64_t mstxEventId = MsprofTxManager::instance()->GetEventId();
    if (stream && RuntimePlugin::instance()->MsprofRtProfilerTraceEx(mstxEventId, MSTX_MODEL_ID,
        MSTX_TAG_ID, stream) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to run mstx task for %s", __func__);
        return;
    }
    if (MstxDataHandler::instance()->SaveMstxData(msg, mstxEventId, MstxDataType::DATA_MARK) !=
        PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to save data for %s, msg: %s", __func__, msg);
        return;
    }
    MSPROF_LOGI("Successfully to execute %s", __func__);
}

uint64_t MstxRangeStartAFunc(const char* msg, aclrtStream stream)
{
    MSPROF_LOGI("Start to execute %s", __func__);
    if (!MstxDataHandler::instance()->IsStart()) {
        MSPROF_LOGW("Mstx data handler is not started");
        return MSTX_INVALID_RANGE_ID;
    }
    if (msg == nullptr || strnlen(msg, MAX_MESSAGE_LEN) == MAX_MESSAGE_LEN) {
        MSPROF_LOGE("input for %s is nullptr", __func__);
        return MSTX_INVALID_RANGE_ID;
    }
    uint64_t mstxEventId = MsprofTxManager::instance()->GetEventId();
    if (stream && RuntimePlugin::instance()->MsprofRtProfilerTraceEx(mstxEventId, MSTX_MODEL_ID,
        MSTX_TAG_ID, stream) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to run mstx task for %s", __func__);
        return MSTX_INVALID_RANGE_ID;
    }
    if (MstxDataHandler::instance()->SaveMstxData(msg, mstxEventId, MstxDataType::DATA_RANGE_START) !=
        PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to save data for %s, msg: %s", __func__, msg);
        return MSTX_INVALID_RANGE_ID;
    }
    if (stream) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_eventIdsWithStream[mstxEventId] = stream;
    }
    MSPROF_LOGI("Successfully to execute %s, range id %lu", __func__, mstxEventId);
    return mstxEventId;
}

void MstxRangeEndFunc(uint64_t id)
{
    MSPROF_LOGI("Start to execute %s", __func__);
    if (!MstxDataHandler::instance()->IsStart()) {
        MSPROF_LOGW("Mstx data handler is not started");
        return;
    }
    if (id == MSTX_INVALID_RANGE_ID) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_eventIdsWithStream.find(id);
        if (it != g_eventIdsWithStream.end()) {
            if (it->second == nullptr) {
                MSPROF_LOGE("Stream for range id %lu is null", id);
                return;
            }
            if (RuntimePlugin::instance()->MsprofRtProfilerTraceEx(id, MSTX_MODEL_ID, MSTX_TAG_ID, it->second) !=
                PROFILING_SUCCESS) {
                MSPROF_LOGE("Failed to run mstx task for %s, range id %lu", __func__, id);
                return;
            }
            g_eventIdsWithStream.erase(it);
        }
    }
    if (MstxDataHandler::instance()->SaveMstxData(nullptr, id,  MstxDataType::DATA_RANGE_END) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to save data for %s, range id %lu", __func__, id);
        return;
    }
    MSPROF_LOGI("Successfully to execute %s, range id %lu", __func__, id);
}
}

using namespace MsprofMstxApi;

extern "C" int __attribute__((visibility("default"))) InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable)
{
    unsigned int outSize = 0;
    MstxFuncTable outTable;
    if (getFuncTable == nullptr || getFuncTable(MSTX_API_MODULE_CORE, &outTable, &outSize) != MSTX_SUCCESS ||
        outTable == nullptr) {
        MSPROF_LOGE("Failed to call getFuncTable");
        return MSTX_FAIL;
    }

    if (outSize != static_cast<unsigned int>(MSTX_FUNC_END)) {
        MSPROF_LOGE("outSize is not equal to MSTX_FUNC_END, Failed to init mstx funcs.");
        return MSTX_FAIL; // 1 : init failed
    }
    *(outTable[MSTX_FUNC_MARKA]) = (MstxFuncPointer)MstxMarkAFunc;
    *(outTable[MSTX_FUNC_RANGE_STARTA]) = (MstxFuncPointer)MstxRangeStartAFunc;
    *(outTable[MSTX_FUNC_RANGE_END]) = (MstxFuncPointer)MstxRangeEndFunc;
    return MSTX_SUCCESS;
}

