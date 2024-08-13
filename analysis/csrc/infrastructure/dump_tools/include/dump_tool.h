/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dump_tool.h
 * Description        : Json & CSV 格式导出数据的工具
 * Author             : msprof team
 * Creation Date      : 2024/7/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_INCLUDE_DUMP_TOOL_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_INCLUDE_DUMP_TOOL_H

#include "analysis/csrc/infrastructure/dump_tools/include/serialization_helper.h"

namespace Analysis {

namespace Infra {

template<class OStream, class ...Args>
void DumpInJsonFormat(OStream& oStream, const std::array<const char*, sizeof...(Args)>& columns,
                      const std::tuple<Args...>& tp)
{
    SerializationHelper<sizeof...(Args), OStream, decltype(tp)>::DumpInJsonFormat(oStream, columns, tp);
}

template<class OStream, class ...Args>
void DumpInCsvFormat(OStream& oStream, const std::tuple<Args...>& tp)
{
    SerializationHelper<sizeof...(Args), OStream, decltype(tp)>::DumpInCsvFormat(oStream, tp);
}

}

}

#endif