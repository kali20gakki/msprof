/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.cpp
 * Description        : Injection of mstx.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "common/inject/mstx_inject.h"

#include <atomic>
#include <cstring>
#include <mutex>

#include "activity/activity_manager.h"
#include "activity/ascend/parser/parser_manager.h"
#include "common/config.h"
#include "common/plog_manager.h"

using namespace Mspti::Activity;
using namespace Mspti::Common;

namespace MsptiMstxApi {
void MstxMarkAFunc(const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (msg == nullptr) {
        MSPTI_LOGE("Input params msg is nullptr.");
        return;
    }
    if (!Utils::CheckCharValid(msg)) {
        MSPTI_LOGE("msg has invalid characters.");
        return;
    }
    // [0, 255]
    if (strnlen(msg, MAX_MARK_MSG_LEN + 1) > MAX_MARK_MSG_LEN) {
        MSPTI_LOGE("The len of msg is invalid.");
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportMark(msg, stream) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report Mark data failed.");
    }
}
 
uint64_t MstxRangeStartAFunc(const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return 0;
    }
    if (msg == nullptr) {
        MSPTI_LOGE("Input params msg is nullptr.");
        return 0;
    }
    // [0, 255]
    if (strnlen(msg, MAX_MARK_MSG_LEN + 1) > MAX_MARK_MSG_LEN) {
        MSPTI_LOGE("The len of msg is invalid.");
        return 0;
    }
    uint64_t markId = 0;
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeStartA(msg, stream, markId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeStart data failed.");
        return 0;
    }
    return markId;
}
 
void MstxRangeEndFunc(uint64_t rangeId)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeEnd(rangeId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeEnd data failed.");
    }
}
}

using namespace MsptiMstxApi;

int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable)
{
    unsigned int outSize = 0;
    MstxFuncTable outTable;
    if (getFuncTable == nullptr || getFuncTable(MSTX_API_MODULE_CORE, &outTable, &outSize) != MSPTI_SUCCESS ||
        outTable == nullptr) {
        MSPTI_LOGE("Failed to init mstx funcs.");
        return 1; // 1 : init failed
    }

    if (outSize != static_cast<unsigned int>(MSTX_FUNC_END)) {
        MSPTI_LOGE("outSize is not equal to MSTX_FUNC_END, Failed to init mstx funcs.");
        return 1; // 1 : init failed
    }
    *(outTable[MSTX_FUNC_MARKA]) = (MstxFuncPointer)MsptiMstxApi::MstxMarkAFunc;
    *(outTable[MSTX_FUNC_RANGE_STARTA]) = (MstxFuncPointer)MsptiMstxApi::MstxRangeStartAFunc;
    *(outTable[MSTX_FUNC_RANGE_END]) = (MstxFuncPointer)MsptiMstxApi::MstxRangeEndFunc;
    return MSPTI_SUCCESS;
}

