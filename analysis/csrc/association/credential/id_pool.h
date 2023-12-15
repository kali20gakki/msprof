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
#include "singleton.h"

namespace Analysis {
namespace Association {
namespace Credential {
using CorrelationTuple = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;
class IdPool : public Utils::Singleton<IdPool> {
public:
    void Clear();
    uint64_t GetId(const std::string& key);
    uint64_t GetId(const CorrelationTuple& key);
    std::unordered_map<std::string, uint64_t>& GetAllStringIds();
private:
    uint64_t correlationIndex_ = 0;
    uint64_t stringIndex_ = 0;
    std::mutex correlationMutex_;
    std::mutex stringMutex_;
    std::unordered_map<std::string, uint64_t> correlationIds_;
    std::unordered_map<std::string, uint64_t> stringIds_;
};
}
}
}
#endif // ANALYSIS_ASSOCIATION_CREDENTIAL_ID_POOL_H
