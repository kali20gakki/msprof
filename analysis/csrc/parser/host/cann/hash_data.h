/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_data.h
 * Description        : hash数据
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H
#define ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "analysis/csrc/utils/singleton.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
// 该类是Hash数据单例类，读取数据，保存在hashData_
class HashData : public Utils::Singleton<HashData> {
public:
    bool Load(const std::string &path);
    void Clear();
    std::string Get(uint64_t hash);
    std::unordered_map<uint64_t, std::string>& GetAll();

private:
    bool ReadFiles(const std::vector<std::string>& files);
    std::unordered_map<uint64_t, std::string> hashData_;
    std::vector<std::string> filePrefix_ = {
        "unaging.additional.hash_dic.slice",
        "aging.additional.hash_dic.slice",
    };
    std::vector<std::string> fileFilter_ = {
        "complete",
        "done",
    };
};  // class HashData
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H
