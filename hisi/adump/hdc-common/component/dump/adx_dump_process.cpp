/**
* @file adx_dump_process.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "adx_dump_process.h"
#include <string>
#include "log/adx_log.h"
namespace Adx {
void AdxDataDumpProcess::MessageCallbackRegister(const MessageCallback callbackFun)
{
    if (init_) {
        IDE_LOGE("MessageCallback function has been registered, The registration does not take effect.");
        return;
    }
    messageCallback_ = callbackFun;
    init_ = true;
}

const std::function<int(const struct DumpChunk *, int)>& AdxDataDumpProcess::GetCallbackFun() const
{
    return messageCallback_;
}

bool AdxDataDumpProcess::IsRegistered()
{
    return init_;
}

void AdxDataDumpProcess::MessageCallbackUnRegister()
{
    if (!init_) {
        IDE_LOGW("MessageCallback is not registered!");
        return;
    }
    init_ = false;
    messageCallback_ = nullptr;
}
}
