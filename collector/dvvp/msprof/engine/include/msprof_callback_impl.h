/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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

// definition of MsprofCtrlCallback, see prof_callback.h
int32_t MsprofCtrlCallbackImpl(uint32_t type, VOID_PTR data, uint32_t len);

// definition of MsprofSetDeviceCallback, see prof_callback.h
int32_t MsprofSetDeviceCallbackImpl(VOID_PTR data, uint32_t len);

// set device callback in dynamic profiling mode
int32_t MsprofSetDeviceCallbackForDynProfImpl(VOID_PTR data, uint32_t len);

// definition of MsprofReporterCallback, see prof_callback.h
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
int32_t MsprofCtrlCallbackImplHandle(uint32_t type, VOID_PTR data, uint32_t len);
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis

#endif
