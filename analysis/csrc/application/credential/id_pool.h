/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : id_pool.h
 * Description        : id映射pool
 * Author             : msprof team
 * Creation Date      : 2023/12/8
 * *****************************************************************************
 */

#ifndef ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H
#define ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H

#include <unordered_map>
#include <mutex>

#include "analysis/csrc/infrastructure/utils/singleton.h"

namespace Analysis {
namespace Application {
namespace Credential {
using CorrelationTuple = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
class IdPool : public Utils::Singleton<IdPool> {
public:
    void Clear();
    uint64_t GetId(const CorrelationTuple& key);
    uint64_t GetUint64Id(const std::string& key);
    uint64_t GetUint32Id(const std::string& key);
    std::unordered_map<std::string, uint64_t>& GetAllUint64Ids();
private:
    uint64_t correlationIndex_ = 0;
    uint64_t uint64Index_ = 0;
    uint32_t uint32Index_ = 0;
    std::mutex correlationMutex_;
    std::mutex uint64Mutex_;
    std::mutex uint32Mutex_;
    std::unordered_map<std::string, uint64_t> correlationIds_;
    std::unordered_map<std::string, uint64_t> uint64Ids_;
    std::unordered_map<std::string, uint32_t> uint32Ids_;
};

}
}
}
#endif // ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H
