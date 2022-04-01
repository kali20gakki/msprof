/**
* @file adx_dump_process.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ADX_DUMP_PROCESS_H
#define ADX_DUMP_PROCESS_H
#include <string>
#include <functional>
#include <atomic>
#include "log/adx_log.h"
#include "common/singleton.h"
#include "adx_datadump_callback.h"
namespace Adx {
using MessageCallback = int (*)(const struct DumpChunk *, int);

class AdxDataDumpProcess : public Adx::Common::Singleton::Singleton<AdxDataDumpProcess> {
public:
    AdxDataDumpProcess() : messageCallback_(nullptr), init_(false) {}
    ~AdxDataDumpProcess() {}
    void MessageCallbackRegister(const MessageCallback callbackFun);
    void MessageCallbackUnRegister();
    const std::function<int(const struct DumpChunk *, int)>& GetCallbackFun() const;
    bool IsRegistered();

private:
    std::function<int(const struct DumpChunk *, int)> messageCallback_;
    std::atomic<bool> init_;
};
}
#endif
