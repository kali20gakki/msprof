/**
* @log_runtime.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef LOG_RUNTIME_H
#define LOG_RUNTIME_H

#include <stdbool.h>

typedef void (*DeviceStateCallback)(unsigned int devId, bool isOpen);

int RuntimeFunctionsInit(void);
int RuntimeFunctionsUninit(void);

int LogRegDeviceStateCallback(DeviceStateCallback callback);

#endif