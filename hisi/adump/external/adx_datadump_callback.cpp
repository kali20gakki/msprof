/**
* @file adx_datadump_callback.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "log/adx_log.h"
#include "dump/adx_dump_process.h"

namespace Adx {
int AdxRegDumpProcessCallBack(int (*const messageCallback)(const Adx::DumpChunk *, int))
{
    if (messageCallback == nullptr) {
        IDE_LOGE("The param MessageCallback is null, please check it!");
        return IDE_DAEMON_ERROR;
    }
    Adx::AdxDataDumpProcess::Instance().MessageCallbackRegister(messageCallback);
    IDE_LOGI("MessageCallback registered success!");
    return IDE_DAEMON_OK;
}

void AdxUnRegDumpProcessCallBack()
{
    Adx::AdxDataDumpProcess::Instance().MessageCallbackUnRegister();
    IDE_LOGI("MessageCallback unregistered success!");
}
}