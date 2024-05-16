/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor_factory.cpp
 * Description        : processor的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/table_processor_factory.h"

#include <unordered_map>

#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/acc_pmu_processor.h"
#include "analysis/csrc/viewer/database/finals/api_processor.h"
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/viewer/database/finals/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/ddr_processor.h"
#include "analysis/csrc/viewer/database/finals/enum_processor.h"
#include "analysis/csrc/viewer/database/finals/hbm_processor.h"
#include "analysis/csrc/viewer/database/finals/hccs_processor.h"
#include "analysis/csrc/viewer/database/finals/host_info_processor.h"
#include "analysis/csrc/viewer/database/finals/llc_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_info_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_module_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/npu_op_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/pcie_processor.h"
#include "analysis/csrc/viewer/database/finals/pmu_processor.h"
#include "analysis/csrc/viewer/database/finals/session_time_info_processor.h"
#include "analysis/csrc/viewer/database/finals/soc_processor.h"
#include "analysis/csrc/viewer/database/finals/string_ids_processor.h"
#include "analysis/csrc/viewer/database/finals/sys_io_processor.h"
#include "analysis/csrc/viewer/database/finals/task_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {

namespace {
    using ProcessorConstructor = std::function<void(const std::string&, const std::set<std::string>&,
                                                    std::shared_ptr<TableProcessor>&)>;
    std::unordered_map<std::string, ProcessorConstructor> processorTable = {
        {PROCESSOR_NAME_STRING_IDS,        [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, StringIdsProcessor, msprofDBPath);}},
        {PROCESSOR_NAME_SESSION_TIME_INFO, [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, SessionTimeInfoProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_NPU_INFO,          [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, NpuInfoProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_HOST_INFO,          [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, HostInfoProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_ENUM,              [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, EnumProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_TASK,              [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, TaskProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_COMPUTE_TASK_INFO, [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, ComputeTaskInfoProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_COMMUNICATION,     [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, CommunicationInfoProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_API,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, ApiProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_NPU_MEM,           [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, NpuMemProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_NPU_MODULE_MEM,    [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, NpuModuleMemProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_NPU_OP_MEM,        [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, NpuOpMemProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_NIC,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, NicProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_ROCE,              [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, RoCEProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_HBM,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, HBMProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_DDR,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, DDRProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_PMU,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, PmuProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_LLC,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, LLCProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_PCIE,              [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, PCIeProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_HCCS,              [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, HCCSProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_ACC_PMU,           [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, AccPmuProcessor, msprofDBPath, profPaths);}},
        {PROCESSOR_NAME_SOC,               [](const std::string& msprofDBPath, const std::set<std::string>& profPaths,
                                              std::shared_ptr<TableProcessor>& processor) {
            MAKE_SHARED_RETURN_VOID(processor, SocProcessor, msprofDBPath, profPaths);}}
    };
}

std::shared_ptr<TableProcessor> TableProcessorFactory::CreateTableProcessor(
    const std::string &processorName,
    const std::string &msprofDBPath,
    const std::set<std::string> &profPaths)
{
    std::shared_ptr<TableProcessor> processor = nullptr;
    auto item = processorTable.find(processorName);
    if (item != processorTable.end()) {
        item->second(msprofDBPath, profPaths, processor);
    }
    return processor;
}


} // Database
} // Viewer
} // Analysis