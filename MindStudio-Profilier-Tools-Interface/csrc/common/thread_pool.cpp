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

#include "csrc/common/thread_pool.h"
#include "csrc/common/utils.h"
#include "csrc/common/plog_manager.h"

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
        std::shared_ptr<Thread> thread = nullptr;
        Mspti::Common::MsptiMakeSharedPtr(thread, threadPoolQueueSize_);
        if (!thread) {
            return -1;
        }
        threads_.push_back(thread);
    }
    for (auto& unstartedThread : threads_) {
        unstartedThread->Start();
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
    auto taskQueue = threads_[threadIndex]->GetQueue();
    if (!taskQueue) {
        MSPTI_LOGE("The task queue is null.");
        return -1;
    }
    taskQueue->Push(task);

    return 0;
}
}  // Common
}  // Mspti
