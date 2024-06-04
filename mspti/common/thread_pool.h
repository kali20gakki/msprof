/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : thread_pool.h
 * Description        : ThreadPool.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_THREAD_POOL_H
#define MSPTI_COMMON_THREAD_POOL_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "common/bound_queue.h"
#include "common/task.h"
#include "common/utils.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Common {

enum class LOAD_BALANCE_METHOD { ROUND_ROBIN = 0, ID_MOD = 1 };

class Thread {
public:
    explicit Thread(size_t queueSize) : started_(false), queue_(nullptr), queueSize_(queueSize) {}
    ~Thread()
    {
        Stop();
    }

public:
    const std::shared_ptr<TaskQueue> GetQueue()
    {
        return queue_;
    }

    void Start()
    {
        Mspti::Common::MsptiMakeSharedPtr(queue_, queueSize_);
        if (!queue_) {
            return;
        }
        if (!thread_.joinable()) {
            thread_ = std::thread(std::bind(&Thread::Run, this));
        }
        started_ = true;
    }

    void Stop()
    {
        if (started_) {
            started_ = false;
            queue_->Quit();
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }

protected:
    void Run()
    {
        for (;;) {
            std::shared_ptr<Task> task;
            if ((!queue_->TryPop(task)) && (started_ == false)) {
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
    std::shared_ptr<TaskQueue> queue_;
    size_t queueSize_;
    std::thread thread_;
};

class ThreadPool {
public:
    explicit ThreadPool(LOAD_BALANCE_METHOD method = LOAD_BALANCE_METHOD::ID_MOD, unsigned int threadNum = 4);
    virtual ~ThreadPool();

public:
    void SetThreadPoolQueueSize(const size_t queueSize);
    int Start();
    int Stop();
    int Dispatch(std::shared_ptr<Task> task);

private:
    unsigned int threadNum_;
    std::atomic_uint currIndex_;
    LOAD_BALANCE_METHOD balancerMethod_;
    volatile bool isStarted_;
    std::vector<std::shared_ptr<Thread>> threads_;
    size_t threadPoolQueueSize_;
};

}  // Common
}  // Mspti

#endif
