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
#ifndef ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_HANDLER_H
#define ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_HANDLER_H

#include <cstdint>
#include "data_dumper.h"
#include "prof_callback.h"
#include "utils/utils.h"
#include "prof_common.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::utils;
class MsprofCallbackHandler {
public:
    MsprofCallbackHandler();
    explicit MsprofCallbackHandler(const std::string &module);
    ~MsprofCallbackHandler();

public:
    int HandleMsprofRequest(uint32_t type, VOID_PTR data, uint32_t len);
    void ForceFlush(const std::string &devId);
    void FlushDynProfCachedMsg(const std::string &devId);
    int SendData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk);
    template <typename T> int ReportData(const T &data) const;
    int ReportVariableData(std::shared_ptr<MsprofVariableInfo> data, uint32_t len) const;

public:
    static void InitReporters();
    int StartReporter();
    int StopReporter();

public:
    static std::map<uint32_t, MsprofCallbackHandler> reporters_;

private:
    int ReportData(CONST_VOID_PTR data, uint32_t len) const;
    int FlushData();
    int GetDataMaxLen(VOID_PTR data, uint32_t len);
    int GetHashId(VOID_PTR data, uint32_t len) const;

private:
    std::string module_;
    SHARED_PTR_ALIA<Msprof::Engine::DataDumper> reporter_;
};

void FlushAllModule(const std::string &devId = "");
void FlushModule(const std::string &devId);
int SendAiCpuData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk);
}  // namespace Engine
}  // namespace Msprof

#endif
