/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H
#define ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/singleton.h"

namespace Analysis {
namespace Domain {
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
}  // namespace Domain
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_HASH_DATA_H
