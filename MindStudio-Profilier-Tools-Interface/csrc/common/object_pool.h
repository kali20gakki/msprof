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
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON__OBJECT_POOL_H
#define MSPTI_COMMON__OBJECT_POOL_H

#include <memory>
#include <functional>
#include <queue>
#include <typeinfo>

namespace Mspti {
namespace Common {
template<typename T>
class SimpleObjectPool {
public:
    using Ptr = std::unique_ptr<T, std::function<void(T*)>>;

    explicit SimpleObjectPool(size_t initialSize = 10)
    {
        reserve(initialSize);
    }

    // 获取对象
    Ptr acquire()
    {
        if (pool_.empty()) {
            // 空时直接两倍扩容
            size_t newCount = (capacity_ == 0 ? 1 : capacity_ * 2);
            reserve(newCount);
        }

        auto obj = std::move(pool_.back());
        pool_.pop_back();
        return Ptr(obj.release(), [this](T* ptr) {
            pool_.push_back(std::unique_ptr<T>(ptr));
        });
    }

    // 获取可用对象数量
    size_t size() const { return pool_.size(); }

    // 预创建更多对象
    void reserve(size_t count)
    {
        pool_.reserve(size() + count);
        for (size_t i = 0; i < count; ++i) {
            pool_.push_back(std::make_unique<T>());
        }
        capacity_ += count;
    }

private:
    std::vector<std::unique_ptr<T>> pool_;
    size_t capacity_ = 0; // 当前总容量
};
}
}

#endif // MSPTI_COMMON__OBJECT_POOL_H