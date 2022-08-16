/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_THREAD_THREAD_H
#define ANALYSIS_DVVP_COMMON_THREAD_THREAD_H

#include <atomic>
#include <thread>
#include "mmpa_api.h"
#include "msprof_error_manager.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace thread {
using namespace analysis::dvvp::common::utils;
using mmThread = Collector::Dvvp::Mmpa::mmThread;
class Thread {
public:
    Thread();
    virtual ~Thread();

public:
    virtual int Start();
    virtual int Stop();
    virtual void StopNoWait()
    {
        quit_ = true;
    }
    int Join();
    bool IsQuit() const;
    void SetThreadName(const std::string &threadName);
    const std::string &GetThreadName();

protected:
    virtual void Run(const struct error_message::Context &errorContext) = 0;

private:
    static void *ThrProcess(VOID_PTR arg);

private:
    mmThread tid_;
    volatile bool quit_;
    volatile bool isStarted_;
    std::string threadName_;
    error_message::Context errorContext_;
};
}  // namespace thread
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
