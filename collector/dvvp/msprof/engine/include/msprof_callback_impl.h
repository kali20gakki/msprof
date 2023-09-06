/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_IMPL_H
#define ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_IMPL_H

#include <cstdint>

#include "data_dumper.h"
#include "prof_callback.h"
#include "utils/utils.h"
#include "msprof_reporter_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
using namespace analysis::dvvp::common::utils;

// defination of MsprofCtrlCallback, see prof_callback.h
int32_t MsprofCtrlCallbackImpl(uint32_t type, VOID_PTR data, uint32_t len);

// defination of MsprofSetDeviceCallback, see prof_callback.h
int32_t MsprofSetDeviceCallbackImpl(VOID_PTR data, uint32_t len);

int32_t MsprofSetDeviceCallbackForDynProf(VOID_PTR data, uint32_t len);

// defination of MsprofReporterCallback, see prof_callback.h
int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len);

// api data reporter call back
int32_t MsprofApiReporterCallbackImpl(uint32_t agingFlag, const MsprofApi &api);

// event data reporter call back
int32_t MsprofEventReporterCallbackImpl(uint32_t agingFlag, const MsprofEvent &event);

// compact info data reporter call back
int32_t MsprofCompactInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length);

// additional info data reporter call back
int32_t MsprofAddiInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length);

// register type info call back
int32_t MsprofRegReportTypeInfoImpl(uint16_t level, uint32_t typeId, const std::string &typeName);

// get hash id call back
uint64_t MsprofGetHashIdImpl(const std::string &hashInfo);

bool MsprofHostFreqIsEnableImpl();

int32_t GetRegisterResult();

int32_t RegisterReporterCallback();
int32_t RegisterNewReporterCallback();
void RegisterMsprofTxReporterCallback();
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis

#endif
