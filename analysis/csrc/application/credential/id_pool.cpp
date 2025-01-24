/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : id_pool.cpp
 * Description        : id映射pool
 * Author             : msprof team
 * Creation Date      : 2023/12/8
 * *****************************************************************************
 */

#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Application {
namespace Credential {

namespace {
const uint32_t DEVICE_ID = 0;
const uint32_t STREAM_ID = 1;
const uint32_t TASK_ID = 2;
const uint32_t CONTEXT_ID = 3;
const uint32_t BATCH_ID = 4;
}

// 清除map数据
void IdPool::Clear()
{
    uint64Ids_.clear();
    correlationIds_.clear();
    uint32Ids_.clear();
    uint64Index_ = 0;
    correlationIndex_ = 0;
    uint32Index_ = 0;
}

// 获取string格式map的key对应value(64位)，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetUint64Id(const std::string& key)
{
    std::lock_guard<std::mutex> lock(uint64Mutex_);
    if (uint64Ids_.find(key) == uint64Ids_.end()) {
        uint64Ids_.insert({key, uint64Index_});
        uint64Index_++;
    }
    return uint64Ids_[key];
}

// 获取string格式map的key对应value(32位)，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetUint32Id(const std::string& key)
{
    std::lock_guard<std::mutex> lock(uint32Mutex_);
    if (uint32Ids_.find(key) == uint32Ids_.end()) {
        uint32Ids_.insert({key, uint32Index_});
        uint32Index_++;
    }
    return uint32Ids_[key];
}

// 获取CorrelationTuple格式map的key对应value，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetId(const CorrelationTuple& key)
{
    std::lock_guard<std::mutex> lock(correlationMutex_);
    std::string stringKey = Utils::Join("_", Utils::Join("_",
        std::get<DEVICE_ID>(key), std::get<STREAM_ID>(key)),
        std::get<TASK_ID>(key), std::get<CONTEXT_ID>(key), std::get<BATCH_ID>(key));
    if (correlationIds_.find(stringKey) == correlationIds_.end()) {
        correlationIds_.insert({stringKey, correlationIndex_});
        correlationIndex_++;
    }
    return correlationIds_[stringKey];
}

// 输出stringIds_ map全部数据
std::unordered_map<std::string, uint64_t>& IdPool::GetAllUint64Ids()
{
    std::lock_guard<std::mutex> lock(uint64Mutex_);
    return uint64Ids_;
}

}
}
}