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
 * -------------------------------------------------------------------------*/
#include "thread.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "securec.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace thread {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
Thread::Thread()
    :tid_(0),
     quit_(false),
     isStarted_(false),
     threadName_(MSVP_PROFILER_THREADNAME_MAXNUM, 0),
     errorContext_({0UL, "", "", ""})
{
}

Thread::~Thread()
{
    if (Stop() != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to stop thread.");
    }
}

int Thread::Start()
{
    if (isStarted_) {
        return PROFILING_SUCCESS;
    }

    MmUserBlockT funcBlock;
    if (memset_s(&funcBlock, sizeof(MmUserBlockT), 0, sizeof(MmUserBlockT)) != EOK) {
        MSPROF_LOGE("memset failed");
        return PROFILING_FAILED;
    }
    MmThreadAttr threadAttr;
    if (memset_s(&threadAttr, sizeof(MmThreadAttr), 0, sizeof(MmThreadAttr)) != EOK) {
        MSPROF_LOGE("memset failed");
        return PROFILING_FAILED;
    }

    funcBlock.procFunc = Thread::ThrProcess;
    funcBlock.pulArg = this;
    const unsigned int defaultStackSize = 128 * 1024; // 128 * 1024 means 128 kb
    threadAttr.stackFlag = 0;
    threadAttr.stackSize = defaultStackSize;

    quit_ = false;
    errorContext_ = MsprofErrorManager::instance()->GetErrorManagerContext();
    int ret = MmCreateTaskWithThreadAttr(&tid_, &funcBlock, &threadAttr);
    if (ret != PROFILING_SUCCESS) {
        tid_ = 0;
        return PROFILING_FAILED;
    }
    isStarted_ = true;

    return PROFILING_SUCCESS;
}

int Thread::Stop()
{
    quit_ = true;
    if (isStarted_) {
        isStarted_ = false;
        return Join();
    }

    return PROFILING_SUCCESS;
}

int Thread::Join()
{
    if (tid_ != 0) {
        int ret = MmJoinTask(&tid_);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        isStarted_ = false;
        tid_ = 0;
    }

    return PROFILING_SUCCESS;
}

bool Thread::IsQuit() const
{
    return quit_;
}

void Thread::SetThreadName(const std::string &threadName)
{
    threadName_ = threadName;
}

const std::string &Thread::GetThreadName()
{
    return threadName_;
}

void *Thread::ThrProcess(VOID_PTR arg)
{
    if (arg == nullptr) {
        return nullptr;
    }
    auto runnable = reinterpret_cast<Thread *>(arg);
    if (runnable == nullptr) {
        return nullptr;
    }
    (void)MmSetCurrentThreadName(runnable->threadName_);

    MSPROF_LOGI("New thread %s begins to run", runnable->threadName_.c_str());

    runnable->Run(runnable->errorContext_);
    return nullptr;
}
}  // namespace thread
}  // namespace common
}  // namespace dvvp
}  // namespace analysis
