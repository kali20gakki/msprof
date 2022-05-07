/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_HANDLER_H
#define ANALYSIS_DVVP_PROFILER_MSPROF_CALLBACK_HANDLER_H

#include <cstdint>
#include "data_dumper.h"
#include "prof_callback.h"
#include "utils/utils.h"

namespace Msprof {
namespace Engine {
class MsprofCallbackHandler {
public:
    MsprofCallbackHandler();
    explicit MsprofCallbackHandler(const std::string module);
    ~MsprofCallbackHandler();

public:
    int HandleMsprofRequest(uint32_t type, VOID_PTR data, uint32_t len);
    void ForceFlush(const std::string &devId);
    int SendData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk);

public:
    static void InitReporters();

public:
    static std::map<uint32_t, MsprofCallbackHandler> reporters_;

private:
    int ReportData(CONST_VOID_PTR data, uint32_t len) const;
    int FlushData();
    int StartReporter();
    int StopReporter();
    int GetDataMaxLen(VOID_PTR data, uint32_t len);
    int GetHashId(VOID_PTR data, uint32_t len) const;

private:
    std::string module_;
    SHARED_PTR_ALIA<Msprof::Engine::DataDumper> reporter_;
};

void FlushAllModule(const std::string &devId = "");
void FlushModule(const std::string &devId);
int SendAiCpuData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk);
}  // namespace Engine
}  // namespace Msprof

#endif
