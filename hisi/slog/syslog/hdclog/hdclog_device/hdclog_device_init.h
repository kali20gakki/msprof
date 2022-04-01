/**
 * @hdclog_device_init.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef HDCLOG_DEVICE_INIT_H
#define HDCLOG_DEVICE_INIT_H
#include "ide_hdc_interface.h"

int HdclogDeviceInit(void);
int HdclogDeviceDestroy(void);
int IdeDeviceLogProcess(HDC_SESSION session, const struct tlv_req *req);

#endif
