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

#ifndef MSPTI_COMMON_THREAD_LOCAL_H
#define MSPTI_COMMON_THREAD_LOCAL_H

#include <pthread.h>
#include <utility>
#include <functional>

#include "csrc/common/plog_manager.h"

namespace Mspti {
namespace Common {
template<typename T>
class ThreadLocal {
public:
    using Creator = std::function<T*()>;

    explicit ThreadLocal(Creator creator = [] { return new(std::nothrow) T(); })
        : creator_(std::move(creator))
    {
        pthread_key_create(&key_, &ThreadLocal::destructor);
    }

    ~ThreadLocal() = default;

    T* Get()
    {
        void* ptr = pthread_getspecific(key_);
        if (!ptr) {
            ptr = creator_();
            pthread_setspecific(key_, ptr);
            auto ret = pthread_setspecific(key_, ptr);
            if (ret != 0) {
                std::error_code ec(ret, std::generic_category());
                MSPTI_LOGE("pthread_setspecific failed: %s (ret=%d)", ec.message().c_str(), ret);
                delete static_cast<T*>(ptr);
                return nullptr;
            }
        }
        return static_cast<T*>(ptr);
    }

    void Clear()
    {
        void* ptr = pthread_getspecific(key_);
        if (ptr) {
            delete static_cast<T*>(ptr);
            auto ret = pthread_setspecific(key_, nullptr);
            if (ret != 0) {
                std::error_code ec(ret, std::generic_category());
                MSPTI_LOGE("pthread_setspecific failed: %s (ret=%d)", ec.message().c_str(), ret);
            }
        }
    }

private:
    static void destructor(void* ptr)
    {
        delete static_cast<T*>(ptr);
    }

    pthread_key_t key_{};
    Creator creator_;
};
}
}

#endif // MSPTI_COMMON_THREAD_LOCAL_H
