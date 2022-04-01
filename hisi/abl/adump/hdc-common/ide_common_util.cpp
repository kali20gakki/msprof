/**
 * @file ide_common_util.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "ide_common_util.h"
#include <cstring>
#include <ctime>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdarg>
#include <csignal>
#include <vector>
#include <string>
#include <mutex>
#if (OS_TYPE == WIN)
#include <direct.h>
#include <cinttypes>
#else
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif
#include "common/config.h"
#include "ide_platform_util.h"
#include "config/adx_config_manager.h"
#include "file_utils.h"
#include "log/adx_log.h"
#include "common/memory_utils.h"
using std::vector;
using namespace std;
using namespace IdeDaemon::Common::Config;
using namespace IdeDaemon::Common::Utils;
using namespace Adx;

struct IdeComponentsFuncs       g_ideComponentsFuncs;

const char *g_ideValidPath[] = {
    "~/hdcd",
    "~/ide_daemon",
    "~/HIAI_PROJECTS",
    "~/HIAI_DATANDMODELSET",
};

#define IDE_VALID_PATH ((sizeof(g_ideValidPath) / sizeof(char *)))

static struct ComponentMap g_compMap[] = {
    {IDE_COMPONENT_HOOK_REG,    IDE_INVALID_REQ,            "hook_reg",       nullptr         },
    {IDE_COMPONENT_HDC,         IDE_INVALID_REQ,            "hdc",            nullptr         },
    {IDE_COMPONENT_CMD,         IDE_EXEC_COMMAND_REQ,       "cmd",            "ExecuteCmd"    },
    {IDE_COMPONENT_SEND_FILE,   IDE_SEND_FILE_REQ,          "send_file",      "TransferFile"  },
    {IDE_COMPONENT_DEBUG,       IDE_DEBUG_REQ,              "debug",          nullptr         },
    {IDE_COMPONENT_LOG,         IDE_LOG_REQ,                "log",            "SetLogLevel"   },
    {IDE_COMPONENT_FILE_SYNC,   IDE_FILE_SYNC_REQ,          "file_sync",      "TransferFile"  },
    {IDE_COMPONENT_API,         IDE_EXEC_API_REQ,           "api",            nullptr         },
    {IDE_COMPONENT_PROFILING,   IDE_PROFILING_REQ,          "profiling",      "Profiling"     },
    {IDE_COMPONENT_DUMP,        IDE_DUMP_REQ,               "dump",           nullptr         },
    {IDE_COMPONENT_HOST_CMD,    IDE_EXEC_HOSTCMD_REQ,       "hostcmd",        "ExecuteCmd"    },
    {IDE_COMPONENT_DETECT,      IDE_DETECT_REQ,             "detect",         "SyncTime"      },
    {IDE_COMPONENT_FILE_GET,    IDE_FILE_GET_REQ,           "file_get",       "TransferFile"  },
    {IDE_COMPONENT_NV,          IDE_NV_REQ,                 "nv",             nullptr         },
    {IDE_COMPONENT_FILE_GETD,   IDE_FILE_GETD_REQ,          "file_getd",      "TransferFile"  },
    {NR_IDE_COMPONENTS,         NR_IDE_CMD_CLASS,           "default",        nullptr         },
};

#define IDE_GET_COMPONENT_NUM    ((sizeof(g_compMap) / sizeof(g_compMap[0])))

IdeString IdeGetCompontName(int type)
{
    unsigned int i;
    for (i = 0; i < IDE_GET_COMPONENT_NUM; i++) {
        if (type == static_cast<int>(g_compMap[i].type)) {
            return g_compMap[i].name;
        }
    }

    return "default";
}

IdeString IdeGetCompontNameByReq(CmdClassT reqType)
{
    unsigned int i;
    for (i = 0; i < IDE_GET_COMPONENT_NUM; i++) {
        if (reqType == g_compMap[i].cmdType) {
            return g_compMap[i].name;
        }
    }

    return "default";
}

/**
 * @brief get the corresponding handler according to tlv
 * @param req: tlv request
 *
 * @return
 *        components_type
 */
enum IdeComponentType IdeGetComponentType(IdeTlvConReq req)
{
    IDE_CTRL_VALUE_FAILED(req != nullptr, return NR_IDE_COMPONENTS, "input req is nullptr");
    enum IdeComponentType type = NR_IDE_COMPONENTS;
    uint32_t i;

    for (i = 0; i < IDE_GET_COMPONENT_NUM; i++) {
        if (req->type == g_compMap[i].cmdType) {
            type = g_compMap[i].type;
            break;
        }
    }

    return type;
}

/**
 * @brief free tlv request
 * @param req: tlv request
 *
 * @return
 */
void IdeReqFree(const IdeTlvReq req)
{
    IdeXfree(req);
}

/**
 * @brief register the SIGPIPE process function
 *
 * @return
 */
void IdeRegisterSig()
{
    signal(SIGPIPE, SIG_IGN);
}

/**
 * @brief initial ide daemon, mkdir workspace and init socket server
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int IdeDaemonSubInit()
{
    IdeRegisterSig();
    return IDE_DAEMON_OK;
}

/**
 * @brief call ide components initial functions
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int IdeComponentsInit()
{
    int i;
    for (i = 0; i < NR_IDE_COMPONENTS; i++) {
        if (g_ideComponentsFuncs.init[i] == nullptr) {
            continue;
        }
        int err = g_ideComponentsFuncs.init[i]();
        if (err != IDE_DAEMON_OK) {
            IDE_LOGE("call [%s] init function failed", IdeGetCompontName(i));
            printf("init [%s] failed\n", IdeGetCompontName(i));
            if (i == IDE_COMPONENT_HDC) {
                return IDE_DAEMON_ERROR;
            }
        } else {
            IDE_LOGI("call [%s] init function succ", IdeGetCompontName(i));
        }
    }

    return IDE_DAEMON_OK;
}

/**
 * @brief call ide components destroy functions
 *
 * @return
 */
void IdeComponentsDestroy()
{
    int i;
    for (i = NR_IDE_COMPONENTS; i > 0; i--) {
        int pos = i - 1;
        if (g_ideComponentsFuncs.destroy[pos] != nullptr) {
            int err = g_ideComponentsFuncs.destroy[pos]();
            if (err != IDE_DAEMON_OK) {
                IDE_LOGE("call ideComponents Funcs destroy[%s] failed", IdeGetCompontName(pos));
            } else {
                IDE_LOGI("call ideComponents Funcs destroy[%s] success", IdeGetCompontName(pos));
            }
        }

        g_ideComponentsFuncs.init[pos] = nullptr;
        g_ideComponentsFuncs.destroy[pos] = nullptr;
        g_ideComponentsFuncs.sockProcess[pos] = nullptr;
        g_ideComponentsFuncs.hdcProcess[pos] = nullptr;
    }

#if (OS_TYPE == WIN)
    BOOL ret = CancelIPChangeNotify(&g_overlapIP);
    if (ret == FALSE) {
        IDE_LOGW("Cancel IP Notify Event");
    }
#endif
    return;
}

/**
 * @brief Create a working path, switch the working path, initialize the socket service
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int DaemonInit()
{
    int err = IdeDaemonSubInit();
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return err, "ide_Daemon_sub_init failed, err: %d", err);

    err = IdeComponentsInit();
    if (err != IDE_DAEMON_OK) {
        return IDE_DAEMON_ERROR;
    }

    IDE_LOGI("Daemon Init done");
    return IDE_DAEMON_OK;
}

/**
 * @brief destory ide daemon context
 *
 * @return
 */
void DaemonDestroy()
{
    IdeComponentsDestroy();
    IDE_LOGI("Components Destroy success.");

#if(OS_TYPE != WIN)
    PopenClean();
    IDE_LOGI("Popen Clean success.");
#endif
    (void)mmSACleanup();
    IDE_LOGI("SACleanup success.");
    return;
}
