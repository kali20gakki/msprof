/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_assembler.cpp
 * Description        : 组合CANN层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/cann_assembler.h"
#include <algorithm>
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const uint32_t SORT_INDEX = 7;
const std::string CANN_ASSEMBLER = "CANN";
const std::string CANN_LABEL = "CPU";
const std::string PROCESS_API = "Api";
const std::unordered_map<uint16_t, std::string> LEVEL_MAP{
    {MSPROF_REPORT_ACL_LEVEL, "AscendCL"},
    {MSPROF_REPORT_RUNTIME_LEVEL, "Runtime"},
    {MSPROF_REPORT_MODEL_LEVEL, "Model"},
    {MSPROF_REPORT_NODE_LEVEL, "Node"},
    {MSPROF_REPORT_HCCL_NODE_LEVEL, "Hccl"}
};
}

CannAssembler::CannAssembler() : JsonAssembler(CANN_ASSEMBLER, {{"msprof", FileCategory::MSPROF}}) {}

void GenerateMetaData(std::vector<ApiData> &apiData, int pid, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::set<uint32_t> tidSet;
    std::transform(apiData.begin(), apiData.end(), std::inserter(tidSet, tidSet.end()),
                   [](ApiData &data) { return data.threadId; });
    std::shared_ptr<MetaDataNameEvent> processName;
    MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, pid, DEFAULT_TID, META_DATA_PROCESS_NAME, CANN_ASSEMBLER);
    std::shared_ptr<MetaDataLabelEvent> processLabel;
    MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, pid, DEFAULT_TID, META_DATA_PROCESS_LABEL, CANN_LABEL);
    std::shared_ptr<MetaDataIndexEvent> processIndex;
    MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, pid, DEFAULT_TID, META_DATA_PROCESS_INDEX, SORT_INDEX);
    res.push_back(processName);
    res.push_back(processLabel);
    res.push_back(processIndex);
    for (const auto &id : tidSet) {
        int tid = static_cast<int>(id);
        std::string argName{"Thread " + std::to_string(tid)};
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, pid, tid, META_DATA_THREAD_NAME, argName);
        res.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, pid, tid, META_DATA_THREAD_INDEX, tid);
        res.push_back(threadIndex);
    }
}

std::string GetLevel(uint16_t level)
{
    for (const auto &node : API_LEVEL_TABLE) {
        if (node.second == level) {
            return node.first;
        }
    }
    return "";
}

std::string GetTraceName(uint16_t level, std::string &name)
{
    auto it = LEVEL_MAP.find(level);
    if (it != LEVEL_MAP.end()) {
        return it->second + "@" + name;
    }
    return PROCESS_API + "@" + name;
}

void GenerateApiTrace(std::vector<ApiData> &apiData, std::vector<std::shared_ptr<TraceEvent>> &res, int pid)
{
    std::string levelStr;
    std::string traceName;
    double dur;
    for (auto &data : apiData) {
        levelStr = GetLevel(data.level);
        traceName = GetTraceName(data.level, data.apiName);
        dur = static_cast<double>(data.end - data.start) / NS_TO_US;
        std::shared_ptr<ApiTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, ApiTraceEvent, pid, data.threadId, dur, std::to_string(data.start / NS_TO_US),
                                traceName, data.threadId, data.connectionId, data.structType, levelStr, data.id,
                                data.itemId);
        res.push_back(event);
    }
}
void GenerateConnectionTrace(std::vector<ApiData> &apiData, int pid, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::string connId;
    std::string name;
    for (auto &data : apiData) {
        if (MSPROF_REPORT_NODE_LEVEL == data.level) {
            connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
            name = HOST_TO_DEVICE + connId;
            std::shared_ptr<FlowEvent> start;
            MAKE_SHARED_RETURN_VOID(start, FlowEvent, pid, data.threadId, std::to_string(data.start / NS_TO_US),
                                    HOST_TO_DEVICE, connId, name, FLOW_START);
            res.push_back(start);
        }
    }
}

uint8_t CannAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData == nullptr) {
        WARN("Can't get apiData from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    int formatPid = static_cast<int>(JsonAssembler::GetFormatPid(pid, SORT_INDEX));
    GenerateMetaData(*apiData, formatPid, res_);
    GenerateApiTrace(*apiData, res_, formatPid);
    GenerateConnectionTrace(*apiData, formatPid, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any CANN process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}
}
