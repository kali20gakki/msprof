/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_item_factory.cpp
 * Description        : parserItem工厂
 * Author             : msprof team
 * Creation Date      : 2024/5/6
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/ffts_plus_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/block_pmu_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/chip4_pmu_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/step_trace_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_type_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_memcpy_parser_item.h"

namespace Analysis {
namespace Domain {
std::function<int(uint8_t *, uint32_t, uint8_t *)> GetLogParseItem(uint32_t itemType)
{
    if (itemType == PARSER_ITEM_ACSQ_LOG_START || itemType == PARSER_ITEM_ACSQ_LOG_END) {
        return AcsqLogParseItem;
    } else if (itemType == PARSER_ITEM_FFTS_PLUS_LOG_START || itemType == PARSER_ITEM_FFTS_PLUS_LOG_END) {
        return FftsPlusLogParseItem;
    }
    ERROR("GetLogParseItem return nullptr, itemType: %", itemType);
    return nullptr;
}

std::function<int(uint8_t *, uint32_t, uint8_t *)> GetTrackParseItem(uint32_t itemType)
{
    if (itemType == PARSER_ITEM_TASK_FLIP) {
        return TaskFlipParseItem;
    } else if (itemType == PARSER_ITEM_STEP_TRACE) {
        return StepTraceParseItem;
    } else if (itemType == PARSER_ITEM_TS_TASK_TYPE) {
        return TaskTypeParseItem;
    } else if (itemType == PARSER_ITEM_TS_MEMCPY) {
        return TaskMemcpyParseItem;
    }
    ERROR("GetTrackParseItem return nullptr, itemType: %", itemType);
    return nullptr;
}

std::function<int(uint8_t *, uint32_t, uint8_t *)> GetPmuParseItem(uint32_t itemType)
{
    if (itemType == PARSER_ITEM_CONTEXT_PMU) {
        return Chip4PmuParseItem;
    } else if (itemType == PARSER_ITEM_BLOCK_PMU) {
        return BlockPmuParseItem;
    }
    ERROR("GetPmuParseItem return nullptr, itemType: %", itemType);
    return nullptr;
}

std::function<int(uint8_t *, uint32_t, uint8_t *)> GetParseItem(ParserType parserType, uint32_t itemType)
{
    switch (parserType) {
        case LOG_PARSER:
            return GetLogParseItem(itemType);
        case TRACK_PARSER:
            return GetTrackParseItem(itemType);
        case PMU_PARSER:
            return GetPmuParseItem(itemType);
        default:
            ERROR("GetParseItem return nullptr, parserType: %", parserType);
            return nullptr;
    }
}
}
}