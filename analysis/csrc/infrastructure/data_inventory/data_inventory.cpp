/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_inventory.cpp
 * Description        : 数据中心：Process存储数据的类
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include <algorithm>
#include "analysis/csrc/dfx/log.h"

using namespace Analysis;

namespace Infra {

bool DataInventory::InputToData(std::type_index idx, BaseTypePtr ptr)
{
    if (ptr == nullptr) {
        ERROR("invalid ptr, type name: %", idx.name());
        return false;
    }
    
    std::lock_guard<std::mutex> lg(mutex_);
    auto ret = data_.emplace(idx, ptr);
    return ret.second;
}

void DataInventory::RemoveRestData(const std::set<std::type_index>& keepingDataType)
{
    std::lock_guard<std::mutex> lg(mutex_);
    for (auto it = data_.begin(); it != data_.end();) {
        if (std::find(keepingDataType.begin(), keepingDataType.end(), it->first) ==
                std::end(keepingDataType)) {
            it = data_.erase(it);
            continue;
        }
        ++it;
    }
}

BaseTypePtr DataInventory::GetPtr(std::type_index idx) const
{
    std::lock_guard<std::mutex> lg(mutex_);
    auto it = data_.find(idx);
    if (it == data_.end()) {
        return {};
    }

    return it->second;
}

}