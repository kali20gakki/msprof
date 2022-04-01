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
#include "epoll/adx_sock_epoll.h"
#include "commopts/sock_comm_opt.h"
#include "dump/adx_dump_receive.h"
#include "log/adx_log.h"
#include "adx_dump_record.h"
namespace Adx {
static IdeThreadArg AdxDumpRecordSocProcess(IdeThreadArg arg)
{
    UNUSED(arg);
    AdxDumpRecord::Instance().RecordDumpInfo();
    return nullptr;
}

static IdeThreadArg AdxDataDumpServerSocProcess(IdeThreadArg arg)
{
    UNUSED(arg);
    mmUserBlock_t funcBlock;
    funcBlock.pulArg = nullptr;
    mmThread tid = 0;
    funcBlock.procFunc = AdxDumpRecordSocProcess;
    int ret = Thread::CreateTaskWithDefaultAttr(tid, funcBlock);
    if (ret != EN_OK) {
        return nullptr;
    }

    (void)mmJoinTask(&tid);
    return nullptr;
}

int32_t AdxSocDataDumpInit(const std::string &hostPid)
{
    std::unique_ptr<AdxCommOpt> opt (new(std::nothrow) SockCommOpt());
    IDE_CTRL_VALUE_FAILED(opt != nullptr, return false, "create sock commopt exception");
    bool regisRet = AdxCommOptManager::Instance().CommOptsRegister(opt);
    IDE_CTRL_VALUE_FAILED(regisRet, return IDE_DAEMON_ERROR,
        "comm opts register failed");
    mmUserBlock_t funcBlock;
    funcBlock.procFunc = AdxDataDumpServerSocProcess;
    funcBlock.pulArg = nullptr;
    mmThread tid = 0;
    // soc case, pass host pid to record instance
    int ret = AdxDumpRecord::Instance().Init(hostPid);
    if (ret != IDE_DAEMON_OK) {
        IDE_LOGE("AdxDumpRecord init failed.");
        return IDE_DAEMON_ERROR;
    }
    ret = Thread::CreateDetachTaskWithDefaultAttr(tid, funcBlock);
    if (ret != EN_OK) {
        return IDE_DAEMON_ERROR;
    }
    IDE_LOGI("Adx soc dump thread has been started.");
    return IDE_DAEMON_OK;
}

int32_t AdxSocDataDumpUnInit()
{
    IDE_LOGI("start to do soc dump uninit");
    return AdxDumpRecord::Instance().UnInit();
}
}

#if !defined(__IDE_UT) && !defined(__IDE_ST)
/**
 * @brief      stub for init in soc case, real init
 *             in constructor function
 *
 * @return
 *      IDE_DAEMON_OK: datadump server init success
 */
int32_t AdxDataDumpServerInit()
{
    return IDE_DAEMON_OK;
}

/**
 * @brief      stub for uninit in soc case, real uninit
 *             in destructor function
 *
 * @return
 *      IDE_DAEMON_OK: datadump server uninit success
 */
int32_t AdxDataDumpServerUnInit()
{
    return IDE_DAEMON_OK;
}
#endif
