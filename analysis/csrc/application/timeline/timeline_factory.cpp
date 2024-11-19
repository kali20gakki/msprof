/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_processor_factory.cpp
 * Description        : data_processor_factory，processor工厂类
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/timeline_factory.h"
#include "analysis/csrc/application/timeline/acc_pmu_assembler.h"
#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/application/timeline/cann_assembler.h"
#include "analysis/csrc/application/timeline/chip_trans_assembler.h"
#include "analysis/csrc/application/timeline/ddr_assembler.h"
#include "analysis/csrc/application/timeline/hbm_assembler.h"
#include "analysis/csrc/application/timeline/hccl_assembler.h"
#include "analysis/csrc/application/timeline/hccs_assembler.h"
#include "analysis/csrc/application/timeline/host_usage_assembler.h"
#include "analysis/csrc/application/timeline/msprof_tx_assembler.h"
#include "analysis/csrc/application/timeline/npu_mem_assembler.h"
#include "analysis/csrc/application/timeline/overlap_analysis_assembler.h"
#include "analysis/csrc/application/timeline/pcie_assembler.h"
#include "analysis/csrc/application/timeline/sio_assembler.h"
#include "analysis/csrc/application/timeline/stars_soc_assembler.h"
#include "analysis/csrc/application/timeline/step_trace_assembler.h"
#include "analysis/csrc/application/timeline/aicore_freq_assembler.h"
#include "analysis/csrc/application/timeline/llc_assembler.h"
#include "analysis/csrc/application/timeline/sys_io_assembler.h"
#include "analysis/csrc/application/timeline/qos_assembler.h"
#include "analysis/csrc/application/timeline/device_tx_assembler.h"

namespace Analysis {
namespace Application {
std::unordered_map<std::string, AssemblerCreator> TimelineFactory::assemblerTable_{
    {PROCESS_ACC_PMU, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, AccPmuAssembler);}},
    {PROCESS_TASK, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, AscendHardwareAssembler);}},
    {PROCESS_API, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, CannAssembler);}},
    {PROCESS_DDR, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, DDRAssembler);}},
    {PROCESS_STARS_CHIP_TRANS, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, ChipTransAssembler);}},
    {PROCESS_HBM, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, HBMAssembler);}},
    {PROCESS_HCCL, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, HcclAssembler);}},
    {PROCESS_HCCS, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, HCCSAssembler);}},
    {PROCESS_NETWORK_USAGE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, NetworkUsageAssembler);}},
    {PROCESS_DISK_USAGE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, DiskUsageAssembler);}},
    {PROCESS_MEMORY_USAGE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, MemUsageAssembler);}},
    {PROCESS_CPU_USAGE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, CpuUsageAssembler);}},
    {PROCESS_MSPROFTX, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, MsprofTxAssembler);}},
    {PROCESS_NPU_MEM, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, NpuMemAssembler);}},
    {PROCESS_OVERLAP_ANALYSE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, OverlapAnalysisAssembler);}},
    {PROCESS_PCIE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, PCIeAssembler);}},
    {PROCESS_SIO, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, SioAssembler);}},
    {PROCESS_STARS_SOC, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, StarsSocAssembler);}},
    {PROCESS_STEP_TRACE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, StepTraceAssembler);}},
    {PROCESS_AI_CORE_FREQ, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, AicoreFreqAssembler);}},
    {PROCESS_LLC, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, LLcAssembler);}},
    {PROCESS_NIC, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, NicAssembler);}},
    {PROCESS_ROCE, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, RoCEAssembler);}},
    {PROCESS_QOS, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, QosAssembler);}},
    {PROCESS_DEVICE_TX, [](std::shared_ptr<JsonAssembler> &assembler) {
        MAKE_SHARED0_NO_OPERATION(assembler, DeviceTxAssembler);}}
};

std::shared_ptr<JsonAssembler> TimelineFactory::GetAssemblerByName(const std::string& processName)
{
    std::shared_ptr<JsonAssembler> assembler{nullptr};
    auto it = assemblerTable_.find(processName);
    if (it != assemblerTable_.end()) {
        it->second(assembler);
    }
    return assembler;
}
}
}
