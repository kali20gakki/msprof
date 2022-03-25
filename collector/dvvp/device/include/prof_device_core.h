/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_PROF_DEVICE_CORE_H
#define ANALYSIS_DVVP_DEVICE_PROF_DEVICE_CORE_H

#include "transport/hdc/hdc_transport.h"

#ifdef __cplusplus
extern "C" {
#endif
using namespace analysis::dvvp::transport;
int IdeDeviceProfileInit();

int IdeDeviceProfileProcess(HDC_SESSION session, CONST_TLV_REQ_PTR req);

int IdeDeviceProfileCleanup();

#ifdef __cplusplus
}
#endif

#endif