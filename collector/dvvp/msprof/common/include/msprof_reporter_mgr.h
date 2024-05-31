/**
* @file msprof_reporter_mgr.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef MSPROF_ENGINE_MSPROF_REPORTER_MGR_H
#define MSPROF_ENGINE_MSPROF_REPORTER_MGR_H
#include <mutex>
#include "message.h"
#include "singleton/singleton.h"
#include "msprof_callback_handler.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::utils;
const std::map<uint16_t, std::map<uint32_t, std::string>> DEFAULT_TYPE_INFO = {
    { MSPROF_REPORT_NODE_LEVEL, {
        {MSPROF_REPORT_NODE_BASIC_INFO_TYPE, "node_basic_info"},
        {MSPROF_REPORT_NODE_TENSOR_INFO_TYPE, "tensor_info"},
        {MSPROF_REPORT_NODE_ATTR_INFO_TYPE, "node_attr_info"},
        {MSPROF_REPORT_NODE_FUSION_OP_INFO_TYPE, "fusion_op_info"},
        {MSPROF_REPORT_NODE_CONTEXT_ID_INFO_TYPE, "context_id_info"},
        {MSPROF_REPORT_NODE_LAUNCH_TYPE, "launch"},
        {MSPROF_REPORT_NODE_TASK_MEMORY_TYPE, "task_memory_info"},
        {MSPROF_REPORT_NODE_HCCL_OP_INFO_TYPE, "hccl_op_info"},
        {MSPROF_REPORT_NODE_STATIC_OP_MEM_TYPE, "static_op_mem"}
    }},
    { MSPROF_REPORT_MODEL_LEVEL, {
        {MSPROF_REPORT_MODEL_GRAPH_ID_MAP_TYPE, "graph_id_map"},
        {MSPROF_REPORT_MODEL_EXEOM_TYPE, "model_exeom"},
        {MSPROF_REPORT_MODEL_LOGIC_STREAM_TYPE, "logic_stream_info"}
    }},
    { MSPROF_REPORT_HCCL_NODE_LEVEL, {
        {MSPROF_REPORT_HCCL_MASTER_TYPE, "master"},
        {MSPROF_REPORT_HCCL_SLAVE_TYPE, "slave"}
    }}
};

class MsprofReporterMgr : public analysis::dvvp::common::singleton::Singleton<MsprofReporterMgr>,
                          public analysis::dvvp::common::thread::Thread {
public:
    MsprofReporterMgr();
    ~MsprofReporterMgr();
public:
    int Start() override;
    int Stop() override;
    void Run(const struct error_message::Context &errorContext) override;

    int32_t StartReporters();
    int32_t StopReporters();
    void FlushAllReporter(const std::string &devId = "");
    int32_t ReportData(uint32_t agingFlag, const MsprofApi &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofEvent &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofCompactInfo &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofAdditionalInfo &data);
    int32_t SendAdditinalData(SHARED_PTR_ALIA<ProfileFileChunk> fileChunk);
    int32_t RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string &typeName);
    std::string& GetRegReportTypeInfo(uint16_t level, uint32_t typeId);
    uint64_t GetHashId(const std::string &info);
    int32_t SendAdditionalInfo(SHARED_PTR_ALIA<ProfileFileChunk> fileChunk);
    void NotifyQuit();
    void SetSyncReporter();

private:
    void FillFileChunkData(const std::string &saveHashData, SHARED_PTR_ALIA<ProfileFileChunk> fileChunk,
                           bool isLastChunk) const;
    void SaveTypeInfo(bool isLastChunk);

private:
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> reportTypeInfoMap_;
    std::unordered_map<uint16_t, std::vector<std::pair<uint32_t, std::string>>> reportTypeInfoMapVec_;
    std::unordered_map<uint16_t, uint32_t> indexMap_;
    std::vector<MsprofCallbackHandler> reporters_;
    std::mutex regTypeInfoMtx_;
    std::mutex startMtx_;
    std::mutex notifyMtx_;
    std::condition_variable cv_;
    bool isStarted_;
    bool isUploadStarted_;
    bool isSyncReporter_;
};
}
}
#endif
