
/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_assembler.cpp
 * Description        : 组合host usage 相关数据
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */
#include "analysis/csrc/application/timeline/host_usage_assembler.h"

#include <unordered_map>

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Parser::Environment;
namespace {
const std::string USAGE = "Usage(%)";
}

NetworkUsageAssembler::NetworkUsageAssembler() : HostUsageAssembler(PROCESS_NETWORK_USAGE) {}
uint8_t NetworkUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<NetWorkUsageData>>();
    if (usageData == nullptr) {
        WARN("Can't get host network data from dataInventory.");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData) {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 std::to_string(data.start / NS_TO_US), "Network Usage");
        event->SetSeriesValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

DiskUsageAssembler::DiskUsageAssembler() : HostUsageAssembler(PROCESS_DISK_USAGE) {}
uint8_t DiskUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<DiskUsageData>>();
    if (usageData == nullptr) {
        WARN("Can't get host disk data from dataInventory.");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData) {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 std::to_string(data.start / NS_TO_US), "Disk Usage");
        event->SetSeriesValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

MemUsageAssembler::MemUsageAssembler() : HostUsageAssembler(PROCESS_MEMORY_USAGE) {}
uint8_t MemUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<MemUsageData>>();
    if (usageData == nullptr) {
        WARN("Can't get host memory data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData) {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 std::to_string(data.start / NS_TO_US), "Memory Usage");
        event->SetSeriesValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}

CpuUsageAssembler::CpuUsageAssembler() : HostUsageAssembler(PROCESS_CPU_USAGE) {}
uint8_t CpuUsageAssembler::GenerateDataTrace(DataInventory &dataInventory, uint32_t pid)
{
    auto usageData = dataInventory.GetPtr<std::vector<CpuUsageData>>();
    if (usageData == nullptr) {
        WARN("Can't get host cpu data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : *usageData) {
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, ASSEMBLE_FAILED, pid, DEFAULT_TID,
                                 std::to_string(data.start / NS_TO_US), "CPU " + data.cpuNo);
        event->SetSeriesValue(USAGE, data.usage);
        res_.push_back(event);
    }
    return ASSEMBLE_SUCCESS;
}


HostUsageAssembler::HostUsageAssembler(const std::string &assembleName)
    :JsonAssembler(assembleName, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

uint8_t HostUsageAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    INFO("Begin to assemble % data.", processorName_);
    auto layerInfo = GetLayerInfo(processorName_);
    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    auto formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex);
    auto generateFlag = GenerateDataTrace(dataInventory, formatPid);
    if (generateFlag != ASSEMBLE_SUCCESS) {
        return generateFlag;
    }
    if (res_.empty()) {
        ERROR("Can't Generate any % data.", processorName_);
        return ASSEMBLE_FAILED;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap = {{HOST_ID, formatPid}};
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    INFO("Assemble % data success.", processorName_);
    return ASSEMBLE_SUCCESS;
}
}
}
