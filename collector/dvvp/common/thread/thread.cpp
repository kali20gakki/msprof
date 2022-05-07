/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "thread.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "securec.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace thread {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Analysis::Dvvp::Plugin;

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

    mmUserBlock_t funcBlock;
    (void)memset_s(&funcBlock, sizeof(mmUserBlock_t), 0, sizeof(mmUserBlock_t));
    mmThreadAttr threadAttr;
    (void)memset_s(&threadAttr, sizeof(mmThreadAttr), 0, sizeof(mmThreadAttr));

    funcBlock.procFunc = Thread::ThrProcess;
    funcBlock.pulArg = this;
    const unsigned int defaultStackSize = 128 * 1024; // 128 * 1024 means 128 kb
    threadAttr.stackFlag = 1;
    threadAttr.stackSize = defaultStackSize;

    quit_ = false;
    errorContext_ = MsprofErrorManager::instance()->GetErrorManagerContext();
    int ret = MmpaPlugin::instance()->MsprofMmCreateTaskWithThreadAttr(&tid_, &funcBlock, &threadAttr);
    if (ret != EN_OK) {
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
        int ret = MmpaPlugin::instance()->MsprofMmJoinTask(&tid_);
        if (ret != EN_OK) {
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
    (void)MmpaPlugin::instance()->MsprofMmSetCurrentThreadName(runnable->threadName_.c_str());

    MSPROF_LOGI("New thread %s begins to run", runnable->threadName_.c_str());

    runnable->Run(runnable->errorContext_);
    return nullptr;
}
}  // namespace thread
}  // namespace common
}  // namespace dvvp
}  // namespace analysis
