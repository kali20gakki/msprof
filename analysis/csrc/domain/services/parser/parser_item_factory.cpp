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
#include <unordered_map>
#include "analysis/csrc/dfx/log.h"

namespace Analysis {
namespace Domain {
using ItemFunc = std::function<int(uint8_t *, uint32_t, uint8_t *)>;
using ItemFuncMap = std::unordered_map<uint32_t, ItemFunc>;
ItemFunc ParserItemFactory::GetParseItem(ParserType parserType, uint32_t itemType)
{
    std::unordered_map<ParserType, ItemFuncMap> parserItemFuncs = GetContainer();
    auto it = parserItemFuncs.find(parserType);
    if (it == parserItemFuncs.end()) {
        ERROR("GetParseItem return nullptr, parserType: %", parserType);
        return nullptr;
    }

    auto parserFunc = it->second;
    auto ans = parserFunc.find(itemType);
    if (ans == parserFunc.end()) {
        ERROR("GetParseItemFunc return nullptr, parserType: %, itemType: %", parserType, itemType);
        return nullptr;
    }
    return ans->second;
}

ParserItemFactory::ParserItemFactory(ParserType parserType, uint32_t itemType, ItemFunc parserItemFunc)
{
    GetContainer()[parserType].emplace(itemType, parserItemFunc);
}

std::unordered_map<ParserType, ItemFuncMap>& ParserItemFactory::GetContainer()
{
    static std::unordered_map<ParserType, ItemFuncMap> parserItemFuncs;
    return parserItemFuncs;
}
}
}