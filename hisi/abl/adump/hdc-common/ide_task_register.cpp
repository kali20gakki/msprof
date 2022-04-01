/**
 * @file ide_task_register.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "ide_task_register.h"
#include "ide_daemon_monitor.h"
/**
 * @brief register component process
 * @param type: the component type
 * @param ide_funcs: contain init/destroy/sock_process/hdc_process to register to ide daemon
 *
 * @return
 */
 namespace Adx {
STATIC void IdeRegisterModule(enum IdeComponentType type, const struct IdeSingleComponentFuncs &ideFuncs)
{
    if (type < NR_IDE_COMPONENTS) {
        g_ideComponentsFuncs.init[type] = ideFuncs.init;
        g_ideComponentsFuncs.destroy[type] = ideFuncs.destroy;
        g_ideComponentsFuncs.sockProcess[type] = ideFuncs.sockProcess;
        g_ideComponentsFuncs.hdcProcess[type] = ideFuncs.hdcProcess;
    }
}

#ifdef IDE_DAEMON_DEVICE
STATIC void IdeDeviceMonitorRegister(struct IdeSingleComponentFuncs &ideFuncs)
{
    ideFuncs.init = IdeMonitorHostInit;
    ideFuncs.destroy = IdeMonitorHostDestroy;
    ideFuncs.sockProcess = nullptr;
    ideFuncs.hdcProcess = nullptr;
    IdeRegisterModule(IDE_COMPONENT_MONITOR, ideFuncs);
}

STATIC void IdeDeviceHdcRegister(struct IdeSingleComponentFuncs &ideFuncs)
{
    ideFuncs.init = HdcDaemonInit;
    ideFuncs.destroy = HdcDaemonDestroy;
    ideFuncs.sockProcess = nullptr;
    ideFuncs.hdcProcess = nullptr;
    IdeRegisterModule(IDE_COMPONENT_HDC, ideFuncs);
}

STATIC void IdeDeviceLogRegister(struct IdeSingleComponentFuncs &ideFuncs)
{
    ideFuncs.init = HdclogDeviceInit;
    ideFuncs.destroy = HdclogDeviceDestroy;
    ideFuncs.sockProcess = nullptr;
    ideFuncs.hdcProcess = IdeDeviceLogProcess;
    IdeRegisterModule(IDE_COMPONENT_LOG, ideFuncs);
}

STATIC void IdeDeviceProfileRegister(struct IdeSingleComponentFuncs &ideFuncs)
{
    ideFuncs.init = IdeDeviceProfileInit;
    ideFuncs.destroy = IdeDeviceProfileCleanup;
    ideFuncs.sockProcess = nullptr;
    ideFuncs.hdcProcess = IdeDeviceProfileProcess;
    IdeRegisterModule(IDE_COMPONENT_PROFILING, ideFuncs);
}

STATIC void IdeDeviceGetdRegister(struct IdeSingleComponentFuncs &ideFuncs)
{
    ideFuncs.init = AdxGetFileServerInit;
    ideFuncs.destroy = nullptr;
    ideFuncs.sockProcess = nullptr;
    ideFuncs.hdcProcess = nullptr;
    IdeRegisterModule(IDE_COMPONENT_FILE_GETD, ideFuncs);
}

void IdeDaemonRegisterModules()
{
    struct IdeSingleComponentFuncs ideFuncs = {nullptr, nullptr, nullptr, nullptr};
    IdeDeviceMonitorRegister(ideFuncs);
    IdeDeviceHdcRegister(ideFuncs);
    IdeDeviceLogRegister(ideFuncs);
    IdeDeviceProfileRegister(ideFuncs);
    IdeDeviceGetdRegister(ideFuncs);
}
#endif
 }

