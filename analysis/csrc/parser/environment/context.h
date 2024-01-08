/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context.h
 * Description        : 采集的Context信息
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H
#define ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H

#include <vector>
#include <map>

#include "opensource/json/include/nlohmann/json.hpp"

#include "analysis/csrc/utils/singleton.h"

namespace Analysis {
namespace Parser {
namespace Environment {
const uint16_t HOST_ID = 64;
// 该类是Context信息单例类，读取info.json和sample.json环境信息
class Context : public Utils::Singleton<Context> {
public:
    bool Load(const std::string &path, uint16_t deviceId);
    bool IsAllExport();
    void Clear();

private:
    enum class Chip : uint16_t {
        CHIP_V1_1_0 = 0,
        CHIP_V2_1_0 = 1,
        CHIP_V3_1_0 = 2,
        CHIP_V3_2_0 = 3,
        CHIP_V3_3_0 = 4,
        CHIP_V4_1_0 = 5,
        CHIP_V1_1_1 = 7,
        CHIP_V1_1_2 = 8,
        CHIP_V5_1_0 = 9,
        CHIP_V1_1_3 = 11,
    };

private:
    std::map<uint16_t, nlohmann::json> context_;
    std::vector<std::string> filePrefix_ = {
        "info.json",
        "sample.json",
    };
};  // class Context
}  // namespace Environment
}  // namespace Parser
}  // namespace Analysis
#endif // ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H
