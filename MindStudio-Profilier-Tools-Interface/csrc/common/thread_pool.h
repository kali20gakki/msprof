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

#ifndef MSPTI_COMMON_THREAD_POOL_H
#define MSPTI_COMMON_THREAD_POOL_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "csrc/common/bound_queue.h"
#include "csrc/common/task.h"
#include "csrc/common/utils.h"
#include "csrc/include/mspti_result.h"

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

        started_ = true;
        if (!thread_.joinable()) {
            thread_ = std::thread(std::bind(&Thread::Run, this));
        }
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
