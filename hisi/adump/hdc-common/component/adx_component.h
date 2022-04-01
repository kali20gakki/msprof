/**
 * @file adx_component.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_COMMON_COMPONENT_H
#define ADX_COMMON_COMPONENT_H
#include <cstdint>
#include <string>
#include "adx_comm_opt_manager.h"
#include "protocol/adx_msg_proto.h"
#include "log/adx_log.h"
namespace Adx {
enum class ComponentType:int {
    COMPONENT_HOOK_REG = 0,  // register driver hook
    COMPONENT_CMD,           // run command on device
    COMPONENT_HOST_CMD,      // run command on aihost
    COMPONENT_SYNC_FILE,     // send file to aihost
    COMPONENT_SYNCD_FILE,    // send file to device
    COMPONENT_GET_FILE,      // get file from aihost
    COMPONENT_GETD_FILE,     // get file from device
    COMPONENT_LOG,           // process log command
    COMPONENT_API,           // qurey the info by api
    COMPONENT_PROFILING,     // process profiling command
    COMPONENT_DUMP,          // process data dump
    COMPONENT_DETECT,        // process adx is alive or not
    COMPONENT_MONITOR,       // process monitor
    NR_COMPONENTS,
};

struct AdxComponentMap {
    ComponentType cmptType;  // component type
    CmdClassT cmdType;       // request type
    std::string cmptName;    // name of request
    std::string oprtName;    // name of record opreate log
};

const AdxComponentMap g_componentsInfo[] = {
    {ComponentType::COMPONENT_HOOK_REG,    IDE_INVALID_REQ,            "hook_reg",       "NoOprateInfo"  },
    {ComponentType::COMPONENT_CMD,         IDE_EXEC_COMMAND_REQ,       "cmd",            "ExecuteCmd"    },
    {ComponentType::COMPONENT_HOST_CMD,    IDE_EXEC_HOSTCMD_REQ,       "hostcmd",        "ExecuteCmd"    },
    {ComponentType::COMPONENT_SYNC_FILE,   IDE_FILE_SYNC_REQ,          "sync_file",      "TransferFile"  },
    {ComponentType::COMPONENT_SYNCD_FILE,  IDE_SEND_FILE_REQ,          "send_file",      "TransferFile"  },
    {ComponentType::COMPONENT_GET_FILE,    IDE_FILE_GET_REQ,           "file_get",       "TransferFile"  },
    {ComponentType::COMPONENT_GETD_FILE,   IDE_FILE_GETD_REQ,          "file_getd",      "TransferFile"  },
    {ComponentType::COMPONENT_LOG,         IDE_LOG_REQ,                "log",            "OpreateLog"    },
    {ComponentType::COMPONENT_API,         IDE_EXEC_API_REQ,           "api",            "QueryApiInfo"  },
    {ComponentType::COMPONENT_PROFILING,   IDE_PROFILING_REQ,          "profiling",      "Profiling"     },
    {ComponentType::COMPONENT_DUMP,        IDE_DUMP_REQ,               "dump",           "DataDump"      },
    {ComponentType::COMPONENT_DETECT,      IDE_DETECT_REQ,             "detect",         "SyncTime"      },
    {ComponentType::COMPONENT_MONITOR,     IDE_INVALID_REQ,            "monitor",        "NoOprateInfo"  },
};

class AdxComponent {
public:
    AdxComponent() {}
    virtual ~AdxComponent() {}
    virtual int32_t Init() = 0;
    virtual const std::string GetInfo() { return "None"; }
    virtual ComponentType GetType() = 0;
    virtual int32_t Process(const CommHandle &handle, const SharedPtr<MsgProto> &req) = 0;
    virtual int32_t UnInit() = 0;
};
}
#endif