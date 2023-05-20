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
using namespace analysis::dvvp::proto;
const std::map<uint16_t, std::map<uint32_t, std::string>> DEFAULT_TYPE_INFO = {
    { MSPROF_REPORT_NODE_LEVEL, {
        {MSPROF_REPORT_NODE_BASIC_INFO_TYPE, "node_basic_info"},
        {MSPROF_REPORT_NODE_TENSOR_INFO_TYPE, "tensor_info"},
        {MSPROF_REPORT_NODE_FUSION_OP_INFO_TYPE, "fusion_op_info"},
        {MSPROF_REPORT_NODE_CONTEXT_ID_INFO_TYPE, "context_id_info"},
        {MSPROF_REPORT_NODE_LAUNCH_TYPE, "launch"}
    }},
    { MSPROF_REPORT_MODEL_LEVEL, {
        {MSPROF_REPORT_MODEL_GRAPH_ID_MAP_TYPE, "graph_id_map"}
    }},
    { MSPROF_REPORT_HCCL_NODE_LEVEL, {
        {MSPROF_REPORT_HCCL_MASTER_TYPE, "master"},
        {MSPROF_REPORT_HCCL_SLAVE_TYPE, "slave"}
    }}
};

class MsprofReporterMgr : public analysis::dvvp::common::singleton::Singleton<MsprofReporterMgr> {
public:
    MsprofReporterMgr();
    ~MsprofReporterMgr();
public:
    int32_t StartReporters();
    int32_t StopReporters();
    void FlushAllReporter(const std::string &devId = "");
    int32_t ReportData(uint32_t agingFlag, const MsprofApi &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofEvent &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofCompactInfo &data);
    int32_t ReportData(uint32_t agingFlag, const MsprofAdditionalInfo &data);
    int32_t SendAdditinalData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk);
    int32_t RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string &typeName);
    std::string& GetRegReportTypeInfo(uint16_t level, uint32_t typeId);
    uint64_t GetHashId(const std::string &info);
    int32_t SendAdditionalInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk);

private:
    void FillFileChunkData(const std::string &saveHashData, SHARED_PTR_ALIA<FileChunkReq> fileChunk) const;
    void SaveTypeInfo();

private:
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> reportTypeInfoMap_;
    std::vector<MsprofCallbackHandler> reporters_;
    std::mutex regTypeInfoMtx_;
    std::mutex startMtx_;
    bool isStarted_;
};
}
}
#endif