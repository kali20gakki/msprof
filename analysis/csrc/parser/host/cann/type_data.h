/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : type_data.h
 * Description        : type数据
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_TYPE_DATA_H
#define ANALYSIS_PARSER_HOST_CANN_TYPE_DATA_H

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "analysis/csrc/utils/singleton.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
// 该类是Type数据单例类，读取数据，保存在typeData_
class TypeData : public Utils::Singleton<TypeData> {
public:
    bool Load(const std::string &path);
    void Clear();
    std::string Get(uint16_t level, uint64_t type);
    std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>& GetAll();

private:
    bool ReadFiles(const std::vector<std::string>& files);
    std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>> typeData_;
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.type_info_dic.slice",
        "aging.additional.type_info_dic.slice",
    };
    std::vector<std::string> fileFilter_ = {
        "complete",
        "done",
    };
};  // class TypeData
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_TYPE_DATA_H
