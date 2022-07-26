/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "thread_pool.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_error_manager.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace thread {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;

ThreadPool::ThreadPool(LOAD_BALANCE_METHOD method /* = ID_MOD */,
    unsigned int threadNum /* = 4 */)
    : threadNum_(threadNum),
      currIndex_(0),
      balancerMethod_(method),
      isStarted_(false),
      threadPoolNamePrefix_(""),
      threadPoolQueueSize_(THREAD_QUEUE_SIZE_DEFAULT)
{
}

ThreadPool::~ThreadPool()
{
    Stop();
}

void ThreadPool::SetThreadPoolNamePrefix(const std::string &name)
{
    threadPoolNamePrefix_ = name;
}

void ThreadPool::SetThreadPoolQueueSize(const size_t queueSize)
{
    threadPoolQueueSize_ = queueSize;
}

int ThreadPool::Start()
{
    if (threadNum_ == 0) {
        return PROFILING_FAILED;
    }

    for (unsigned int ii = 0; ii < threadNum_; ++ii) {
        SHARED_PTR_ALIA<ThreadPool::InnnerThread> thread;
        MSVP_MAKE_SHARED1_RET(thread, ThreadPool::InnnerThread, threadPoolQueueSize_, PROFILING_FAILED);

        std::string threadName = threadPoolNamePrefix_ + std::to_string(ii);
        thread->SetThreadName(threadName);
        int ret = thread->Start();
        if (ret != PROFILING_SUCCESS) {
            continue;
        }
        threads_.push_back(thread);
    }

    isStarted_ = true;

    return PROFILING_SUCCESS;
}

int ThreadPool::Stop()
{
    isStarted_ = false;
    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        if ((*iter)->Stop() != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to stop thread: %s", (*iter)->GetThreadName().c_str());
        }
    }
    threads_.clear();

    return PROFILING_SUCCESS;
}

int ThreadPool::Dispatch(SHARED_PTR_ALIA<Task> task)
{
    if (task == nullptr) {
        return PROFILING_FAILED;
    }
    if (!isStarted_) {
        return PROFILING_FAILED;
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

    return PROFILING_SUCCESS;
}
}  // namespace thread
}  // namespace common
}  // namespace dvvp
}  // namespace analysis
