/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_init_processor.h
 * Description        : 从host目录下初始化Hash数据
 * Author             : msprof team
 * Creation Date      : 2024/7/30
 * *****************************************************************************
 */

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
