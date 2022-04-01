/**
* @file adx_get_file_server.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "adx_get_file_server.h"
#include "component/adx_server_manager.h"
#include "epoll/adx_hdc_epoll.h"
#include "commopts/hdc_comm_opt.h"
#include "component/adx_file_dump.h"
#include "component/adx_get_file.h"
#include "log/adx_log.h"
using namespace Adx;
static IdeThreadArg AdxGetFileServerProcess(IdeThreadArg arg)
{
    UNUSED(arg);
    std::unique_ptr<::AdxEpoll> epoll(new(std::nothrow)AdxHdcEpoll());
    IDE_CTRL_VALUE_FAILED(epoll != nullptr, return nullptr, "register epoll error");
    std::unique_ptr<AdxComponent> cpn(new(std::nothrow)AdxGetFile());
    IDE_CTRL_VALUE_FAILED(cpn != nullptr, return nullptr, "register epoll error");
    std::unique_ptr<AdxCommOpt> opt(new(std::nothrow)HdcCommOpt());
    IDE_CTRL_VALUE_FAILED(opt != nullptr, return nullptr, "register epoll error");
    AdxServerManager manager;
    bool ret = manager.RegisterEpoll(epoll);
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "register epoll error");
    ret = manager.RegisterCommOpt(opt, std::to_string(HDC_SERVICE_TYPE_IDE_FILE_TRANS));
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "register commopt error");
    ret = manager.ComponentAdd(cpn);
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "component add error");
    ret = manager.ComponentInit();
    IDE_CTRL_VALUE_FAILED(ret, return nullptr, "component Init error");
    manager.SetThreadName(std::string("adx_get_file_thread"));
    manager.Start();
    (void)manager.Join();
    return nullptr;
}

int32_t AdxGetFileServerInit()
{
    mmUserBlock_t funcBlock;
    funcBlock.procFunc = AdxGetFileServerProcess;
    funcBlock.pulArg = nullptr;
    mmThread tid = 0;
    int ret = Thread::CreateDetachTaskWithDefaultAttr(tid, funcBlock);
    return (ret != EN_OK) ? IDE_DAEMON_ERROR : IDE_DAEMON_OK;
}