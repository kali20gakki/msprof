/**
* @file aicpu_plugin.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_AICPU_PLUGIN_H
#define MSPROF_ENGINE_AICPU_PLUGIN_H

#include <string>
#include "utils.h"
#include "transport/transport.h"
#include "thread/thread.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::utils;
class AicpuPlugin: public analysis::dvvp::common::thread::Thread {
public:
    AicpuPlugin();
    virtual ~AicpuPlugin();

public:
    int Init(const int32_t logicDevId);
    int UnInit();
    void Run(const struct error_message::Context &errorContext) override;

private:
    int ReceiveStreamData(CONST_VOID_PTR data, unsigned int dataLen);

private:
    bool dataInitialized_;
    int32_t logicDevId_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> dataTran_;
    HDC_SERVER server_;
    std::string logicDevIdStr_;
};
}
}
#endif
