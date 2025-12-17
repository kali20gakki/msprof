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

#ifndef ANALYSIS_DOMAIN_HASH_INIT_PROCESS_H
#define ANALYSIS_DOMAIN_HASH_INIT_PROCESS_H

#include <unordered_map>
#include "analysis/csrc/domain/data_process/data_processor.h"

namespace Analysis {
namespace Domain {
// GeHash表映射map结构
using GeHashMap = std::unordered_map<std::string, std::string>;
using StreamMap = std::unordered_map<uint32_t, uint32_t>;

class HashInitProcessor : public DataProcessor {
public:
    HashInitProcessor() = default;
    explicit HashInitProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessLogicStream(DataInventory &dataInventory);
    bool ProcessHashMap(DataInventory &dataInventory);
};
}
}

#endif // ANALYSIS_DOMAIN_HASH_INIT_PROCESS_H
