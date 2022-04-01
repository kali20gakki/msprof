/**
 * @file thread.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_COMMON_THREAD_H
#define IDE_DAEMON_COMMON_THREAD_H
#include <string>
#include <cstdint>
#include "mmpa_api.h"
#include "extra_config.h"

namespace Adx {
const mmThreadAttr IDE_DAEMON_DEFAULT_THREAD_ATTR        = {0, 0, 0, 0, 0, 1, 128 * 1024};
const mmThreadAttr IDE_DAEMON_DEFAULT_DETACH_THREAD_ATTR = {1, 0, 0, 0, 0, 1, 128 * 1024};

class Thread {
public:
    static int32_t CreateTask(mmThread &tid, mmUserBlock_t &funcBlock);
    static int32_t CreateDetachTask(mmThread &tid, mmUserBlock_t &funcBlock);
    static int32_t CreateTaskWithDefaultAttr(mmThread &tid, mmUserBlock_t &funcBlock);
    static int32_t CreateDetachTaskWithDefaultAttr(mmThread &tid, mmUserBlock_t &funcBlock);
};

class Runnable {
public:
    Runnable();
    virtual ~Runnable();
    virtual int32_t Start();
    virtual int32_t Terminate();
    int32_t Stop();
    int32_t Join();
    bool IsQuit() const;
    void SetThreadName(const std::string &name);
    const std::string &GetThreadName();

protected:
    virtual void Run() = 0;

private:
    static IdeThreadArg Process(IdeThreadArg arg);
private:
    mmThread tid_;
    mutable bool quit_;
    mutable bool isStarted_;
    std::string threadName_;
};
}
#endif
