/**
* @file adx_datadump_server.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "adx_datadump_server.h"
#include "component/adx_server_manager.h"
#include "epoll/adx_hdc_epoll.h"
#include "commopts/hdc_comm_opt.h"
#include "dump/adx_dump_receive.h"
#include "log/adx_log.h"
#include "adx_dump_record.h"
using namespace Adx;
namespace {
AdxServerManager g_manager;
IdeThreadArg AdxDataDumpServerProcess(const IdeThreadArg arg)
{
    UNUSED(arg);
    std::unique_ptr<AdxEpoll> epoll(new (std::nothrow)AdxHdcEpoll());
    IDE_CTRL_VALUE_FAILED(epoll != nullptr, return nullptr, "create epoll error");
    std::unique_ptr<AdxComponent> cpn(new (std::nothrow)AdxDumpReceive());
    IDE_CTRL_VALUE_FAILED(cpn != nullptr, return nullptr, "create component error");
    std::unique_ptr<AdxCommOpt> opt(new (std::nothrow)HdcCommOpt());
    IDE_CTRL_VALUE_FAILED(opt != nullptr, return nullptr, "create commopt error");
    bool ret = g_manager.RegisterEpoll(epoll);
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "register epoll error");
    ret = g_manager.RegisterCommOpt(opt, std::to_string(HDC_SERVICE_TYPE_DUMP));
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "register commopt error");
    ret = g_manager.ComponentAdd(cpn);
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "component add error");
    ret = g_manager.ComponentInit();
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "component Init error");
    g_manager.SetThreadName(std::string("adx_data_dump_thread"));
    g_manager.Start();
    (void)g_manager.Join();
    return nullptr;
}

IdeThreadArg AdxDumpRecordProcess(const IdeThreadArg arg)
{
    UNUSED(arg);
    AdxDumpRecord::Instance().RecordDumpInfo();
    return nullptr;
}
}
int32_t AdxDataDumpServerInit()
{
    int32_t dumpNum = AdxDumpRecord::Instance().GetDumpInitNum();
    if (dumpNum > 0) {
        AdxDumpRecord::Instance().UpdateDumpInitNum(true);
        IDE_LOGI("dump init process has already been called, dumpNum : %d", dumpNum);
        return IDE_DAEMON_OK;
    }
    IDE_LOGI("start to do dump init");
    AdxDumpRecord::Instance().UpdateDumpInitNum(true);
    mmUserBlock_t funcBlock;
    funcBlock.procFunc = AdxDataDumpServerProcess;
    funcBlock.pulArg = nullptr;
    mmThread tid = 0;
    // non-soc case, no need to pass host pid
    int ret = AdxDumpRecord::Instance().Init("");
    if (ret != IDE_DAEMON_OK) {
        IDE_LOGE("AdxDumpRecord init failed.");
        return IDE_DAEMON_ERROR;
    }
    ret = Thread::CreateDetachTaskWithDefaultAttr(tid, funcBlock);
    if (ret != EN_OK) {
        return IDE_DAEMON_ERROR;
    }
    funcBlock.procFunc = AdxDumpRecordProcess;
    funcBlock.pulArg = nullptr;
    ret = Thread::CreateDetachTaskWithDefaultAttr(tid, funcBlock);
    return (ret != EN_OK) ? IDE_DAEMON_ERROR : IDE_DAEMON_OK;
}

int32_t AdxDataDumpServerUnInit()
{
    AdxDumpRecord::Instance().UpdateDumpInitNum(false);
    int32_t dumpNum = AdxDumpRecord::Instance().GetDumpInitNum();
    if (dumpNum > 0) {
        IDE_LOGI("still have %d dump init times, return", dumpNum);
        return IDE_DAEMON_OK;
    }
    IDE_LOGI("start to do dump uninit");
    g_manager.Exit();
    return AdxDumpRecord::Instance().UnInit();
}
