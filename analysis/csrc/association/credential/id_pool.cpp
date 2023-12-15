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

#include "id_pool.h"
#include "utils.h"
namespace Analysis {
namespace Association {
namespace Credential {

namespace {
const uint32_t DEVICE_ID = 0;
const uint32_t MODEL_ID = 1;
const uint32_t STREAM_ID = 2;
const uint32_t TASK_ID = 3;
const uint32_t CONTEXT_ID = 4;
const uint32_t BATCH_ID = 5;
}

// 清除map数据
void IdPool::Clear()
{
    stringIds_.clear();
    correlationIds_.clear();
    stringIndex_ = 0;
    correlationIndex_ = 0;
}

// 获取string格式map的key对应value，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetId(const std::string& key)
{
    std::lock_guard<std::mutex> lock(stringMutex_);
    if (stringIds_.find(key) == stringIds_.end()) {
        stringIds_.insert({key, stringIndex_});
        stringIndex_++;
    }
    return stringIds_[key];
}

// 获取CorrelationTuple格式map的key对应value，如果不存在则添加(key, value)并返回value
uint64_t IdPool::GetId(const CorrelationTuple& key)
{
    std::lock_guard<std::mutex> lock(correlationMutex_);
    std::string stringKey = Utils::ConvertToString(Utils::ConvertToString(
        std::get<DEVICE_ID>(key), std::get<MODEL_ID>(key), std::get<STREAM_ID>(key)),
        std::get<TASK_ID>(key), std::get<CONTEXT_ID>(key), std::get<BATCH_ID>(key));
    if (correlationIds_.find(stringKey) == correlationIds_.end()) {
        correlationIds_.insert({stringKey, correlationIndex_});
        correlationIndex_++;
    }
    return correlationIds_[stringKey];
}

// 输出stringIds_ map全部数据
std::unordered_map<std::string, uint64_t>& IdPool::GetAllStringIds()
{
    std::lock_guard<std::mutex> lock(stringMutex_);
    return stringIds_;
}

}
}
}