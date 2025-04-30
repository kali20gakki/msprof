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
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {

namespace Infra {
using namespace Analysis::Utils;

// 两种dump接口为初版落盘方案,当前实现未使用此流程
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

class DumpTool {
public:
    static bool WriteToFile(const std::string &fileName, const char *content, std::size_t len, FileCategory type)
    {
        if (content == nullptr || len == 0) {
            ERROR("The content to write is nullptr, or the length to write is zero!");
            return false;
        }
        auto it = mutexMap.find(static_cast<int>(type));
        if (it == mutexMap.end()) {
            ERROR("file mutex is not adapt current file name");
            return false;
        }
        std::lock_guard<std::mutex> lock(it->second.fileMutex_);
        FileWriter fileWriter(fileName, std::ios::app);
        fileWriter.WriteText(content, len);
        return true;
    }
};

}
}

#endif