/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle ide profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_THREAD_THREAD_POOL_H
#define ANALYSIS_DVVP_COMMON_THREAD_THREAD_POOL_H


#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>

#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "queue/bound_queue.h"
#include "thread.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace thread {
class Task {
public:
    virtual ~Task() {}

public:
    virtual int Execute() = 0;
    virtual size_t HashId() = 0;
};

using TaskQueue = analysis::dvvp::common::queue::BoundQueue<SHARED_PTR_ALIA<Task>>;

enum class LOAD_BALANCE_METHOD {
    ROUND_ROBIN = 0,
    ID_MOD = 1
};

class ThreadPool {
public:
    explicit ThreadPool(LOAD_BALANCE_METHOD method = LOAD_BALANCE_METHOD::ID_MOD,
                        unsigned int threadNum = 4);
    virtual ~ThreadPool();

public:
    void SetThreadPoolNamePrefix(const std::string &name);
    void SetThreadPoolQueueSize(const size_t queueSize);
    int Start();
    int Stop();
    int Dispatch(SHARED_PTR_ALIA<Task> task);

private:
    class InnnerThread : public Thread {
        friend class ThreadPool;

    public:
        explicit InnnerThread(size_t queueSize)
            : started_(false), queue_(nullptr), queueSize_(queueSize)
        {
        }
        ~InnnerThread() override
        {
            (void)Stop();
        }

    public:
        const SHARED_PTR_ALIA<TaskQueue> GetQueue()
        {
            return queue_;
        }

        int Start() override
        {
            MSVP_MAKE_SHARED1_RET(queue_, TaskQueue, queueSize_,
                                  analysis::dvvp::common::error::PROFILING_FAILED);
            auto threadName = GetThreadName();
            queue_->SetQueueName(threadName);
            int ret = Thread::Start();
            if (ret != analysis::dvvp::common::error::PROFILING_SUCCESS) {
                return analysis::dvvp::common::error::PROFILING_FAILED;
            }

            started_ = true;

            return analysis::dvvp::common::error::PROFILING_SUCCESS;
        }

        int Stop() override
        {
            if (started_) {
                started_ = false;
                queue_->Quit();
                return Thread::Stop();
            }

            return analysis::dvvp::common::error::PROFILING_SUCCESS;
        }

    protected:
        void Run(const struct error_message::Context &errorContext) override
        {
            Analysis::Dvvp::MsprofErrMgr::MsprofErrorManager::instance()->SetErrorContext(errorContext);
            for (;;) {
                SHARED_PTR_ALIA<Task> task;
                if ((!queue_->TryPop(task)) &&
                    (Thread::IsQuit())) {
                    break;
                }

                if (!task) {
                    (void)queue_->Pop(task);
                }

                if (task) {
                    (void)task->Execute();
                }
            }
        }

    private:
        volatile bool started_;
        SHARED_PTR_ALIA<TaskQueue> queue_;
        size_t queueSize_;
    };

private:
    unsigned int threadNum_;
    std::atomic_uint currIndex_;
    LOAD_BALANCE_METHOD balancerMethod_;
    volatile bool isStarted_;
    std::vector<SHARED_PTR_ALIA<ThreadPool::InnnerThread> > threads_;
    std::string threadPoolNamePrefix_;
    size_t threadPoolQueueSize_;
};
}  // namespace thread
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
