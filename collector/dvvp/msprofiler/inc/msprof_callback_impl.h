/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_IMPL_H
#define ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_IMPL_H

#include <cstdint>

#include "acl/acl_base.h"

#include "data_dumper.h"
#include "prof_callback.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
using namespace analysis::dvvp::common::utils;

// defination of MsprofCtrlCallback, see prof_callback.h
int32_t MsprofCtrlCallbackImpl(uint32_t type, VOID_PTR data, uint32_t len);

// defination of MsprofSetDeviceCallback, see prof_callback.h
void MsprofSetDeviceCallbackImpl(uint32_t devId, bool isOpenDevice);

// defination of MsprofReporterCallback, see prof_callback.h
int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len);

int32_t GetRegisterResult();

int32_t RegisterReporterCallback();
void RegisterMsprofTxReporterCallback();
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis

#endif
