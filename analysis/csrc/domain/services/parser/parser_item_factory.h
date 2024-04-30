/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_item_factory.h
 * Description        : parserItem工厂
 * Author             : msprof team
 * Creation Date      : 2024/4/23
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H

#include <cstdint>
#include <functional>

namespace Analysis {
namespace Domain {
enum ParserType {
    LOG_PARSER,
    PMU_PARSER,
    TRACK_PARSER,
};

std::function<void(void *, uint32_t, void *, uint32_t)> GetParseItem(ParserType parserType, uint32_t itemType);
}
}
#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_FACTORY_H