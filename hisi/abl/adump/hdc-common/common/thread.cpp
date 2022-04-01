/**
 * @file thread.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "thread.h"
#include "common/utils.h"
namespace Adx {
static const int32_t WAIT_TID_TIME   = 500;
/**
 * @brief create thread
 * @param [out]tid : thread id
 * @param [int]funcBlock : function name and parameter
 *
 * @return
 *        EN_OK: succ
 *        other: failed
 */
int32_t Thread::CreateTask(mmThread &tid, mmUserBlock_t &funcBlock)
{
#ifndef IDE_UNIFY_HOST_DEVICE
    return mmCreateTask(&tid, &funcBlock);
#else
    return CreateTaskWithDefaultAttr(tid, funcBlock);
#endif
}

/**
 * @brief create thread with detach
 * @param [out]tid : thread id
 * @param [int]funcBlock : function name and parameter
 *
 * @return
 *        EN_OK: succ
 *        other: failed
 */
int32_t Thread::CreateDetachTask(mmThread &tid, mmUserBlock_t &funcBlock)
{
#ifndef IDE_UNIFY_HOST_DEVICE
    return mmCreateTaskWithDetach(&tid, &funcBlock);
#else
    return CreateDetachTaskWithDefaultAttr(tid, funcBlock);
#endif
}

/**
 * @brief create thread with default attributes
 * @param [out]tid : thread id
 * @param [int]funcBlock : function name and parameter
 *
 * @return
 *        EN_OK: succ
 *        other: failed
 */
int32_t Thread::CreateTaskWithDefaultAttr(mmThread &tid, mmUserBlock_t &funcBlock)
{
    mmThreadAttr threadAttr = IDE_DAEMON_DEFAULT_THREAD_ATTR;
    return mmCreateTaskWithThreadAttr(&tid, &funcBlock, &threadAttr);
}

/**
 * @brief create detach thread with default attributes
 * @param [out] tid : thread id
 * @param [in]  funcBlock : function name and parameter
 *
 * @return
 *        EN_OK: succ
 *        other: failed
 */
int32_t Thread::CreateDetachTaskWithDefaultAttr(mmThread &tid, mmUserBlock_t &funcBlock)
{
    mmThreadAttr threadAttr = IDE_DAEMON_DEFAULT_DETACH_THREAD_ATTR;
    return mmCreateTaskWithThreadAttr(&tid, &funcBlock, &threadAttr);
}

Runnable::Runnable()
    : tid_(0), quit_(false), isStarted_(false), threadName_("adx")
{
}

Runnable::~Runnable()
{
    Stop();
}

int32_t Runnable::Start()
{
    if (isStarted_) {
        return IDE_DAEMON_ERROR;
    }

    mmUserBlock_t funcBlock;
    funcBlock.procFunc = Runnable::Process;
    funcBlock.pulArg = this;
    quit_ = false;
    int ret = Thread::CreateTaskWithDefaultAttr(tid_, funcBlock);
    if (ret != EN_OK) {
        tid_ = 0;
        return IDE_DAEMON_ERROR;
    }
    isStarted_ = true;
    return IDE_DAEMON_OK;
}

int32_t Runnable::Terminate()
{
    quit_ = true;
    while (tid_ != 0) {
        mmSleep(WAIT_TID_TIME);
    }
    isStarted_ = false;
    return IDE_DAEMON_OK;
}

int32_t Runnable::Stop()
{
    quit_ = true;
    if (isStarted_) {
        isStarted_ = false;
        return Join();
    }

    return IDE_DAEMON_OK;
}

int32_t Runnable::Join()
{
    if (tid_ != 0) {
        int ret = mmJoinTask(&tid_);
        if (ret != EN_OK) {
            return IDE_DAEMON_ERROR;
        }
        isStarted_ = false;
        tid_ = 0;
    }

    return IDE_DAEMON_OK;
}

bool Runnable::IsQuit() const
{
    return quit_;
}

void Runnable::SetThreadName(const std::string &name)
{
    threadName_ = name;
}

const std::string &Runnable::GetThreadName()
{
    return threadName_;
}

IdeThreadArg Runnable::Process(IdeThreadArg arg)
{
    if (arg == nullptr) {
        return nullptr;
    }
    auto runnable = reinterpret_cast<Runnable *>(arg);
    (void)mmSetCurrentThreadName(runnable->threadName_.c_str());
    runnable->Run();
    return nullptr;
}
}