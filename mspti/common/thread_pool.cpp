/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : thread_pool.cpp
 * Description        : ThreadPool.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "common/thread_pool.h"

namespace Mspti {
namespace Common {
static const uint32_t THREAD_POOL_TASKQUEUE_SIZE = 64;
ThreadPool::ThreadPool(LOAD_BALANCE_METHOD method, unsigned int threadNum)
    : threadNum_(threadNum),
      currIndex_(0),
      balancerMethod_(method),
      isStarted_(false),
      threadPoolQueueSize_(THREAD_POOL_TASKQUEUE_SIZE)
{
}

ThreadPool::~ThreadPool()
{
    Stop();
}


void ThreadPool::SetThreadPoolQueueSize(const size_t queueSize)
{
    threadPoolQueueSize_ = queueSize;
}

int ThreadPool::Start()
{
    if (threadNum_ == 0) {
        return -1;
    }

    for (unsigned int ii = 0; ii < threadNum_; ++ii) {
        auto thread = std::make_shared<Thread>(threadPoolQueueSize_);
        thread->Start();
        threads_.push_back(thread);
    }
    isStarted_ = true;
    return 0;
}

int ThreadPool::Stop()
{
    isStarted_ = false;
    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        (*iter)->Stop();
    }
    threads_.clear();
    return 0;
}

int ThreadPool::Dispatch(std::shared_ptr<Task> task)
{
    if (task == nullptr) {
        return -1;
    }
    if (!isStarted_) {
        return -1;
    }

    unsigned int threadIndex = 0;

    switch (balancerMethod_) {
        case LOAD_BALANCE_METHOD::ID_MOD:
            threadIndex = (task->HashId()) % ((size_t)threadNum_);
            break;
        case LOAD_BALANCE_METHOD::ROUND_ROBIN:
        default:
            threadIndex = currIndex_++ % threadNum_;
            break;
    }

    threads_[threadIndex]->GetQueue()->Push(task);

    return 0;
}
}  // Common
}  // Mspti
