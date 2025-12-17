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
using MmThread = Collector::Dvvp::Mmpa::MmThread;
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
    MmThread tid_;
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
