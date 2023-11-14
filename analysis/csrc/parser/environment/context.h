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

#include "nlohmann/json.hpp"
#include "singleton.h"

namespace Analysis {
namespace Parser {
namespace Environment {
const std::string DRV_VERSION = "drvVersion";

// 该类是Context信息单例类，读取info.json和sample.json环境信息
class Context : public Utils::Singleton<Context> {
public:
    bool Load(const std::string &path, uint16_t deviceId);
    std::string Get(const uint16_t &deviceId, const std::string &key) const;

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
