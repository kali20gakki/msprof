/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : thread_local.h
 * Description        : 使用pthread提供和thread local变量相同的功能.
 * Author             : msprof team
 * Creation Date      : 2025/05/6
 * *****************************************************************************
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
                MSPTI_LOGE("pthread_setspecific failed: %s (%d)", strerror(ret), ret);
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
                MSPTI_LOGE("pthread_setspecific (clear) failed: %s (%d)", strerror(ret), ret);
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
