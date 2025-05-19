/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_json_enum
 * Description        : export Json枚举
 * Author             : msprof team
 * Creation Date      : 2024/12/07
 * *****************************************************************************
 */
#ifndef ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H
#define ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H

namespace Analysis {
namespace Application {
enum class JsonProcess {
    ASCEND,
    ACC_PMU,
    CANN,
    DDR,
    STARS_CHIP_TRANS,
    HBM,
    COMMUNICATION,
    HCCS,
    OS_RUNTIME_API,
    NETWORK_USAGE,
    DISK_USAGE,
    MEMORY_USAGE,
    CPU_USAGE,
    MSPROFTX,
    NPU_MEM,
    OVERLAP_ANALYSE,
    PCIE,
    SIO,
    STARS_SOC,
    STEP_TRACE,
    FREQ,
    LLC,
    NIC,
    ROCE,
    QOS,
    DEVICE_TX,
};

const std::unordered_map<std::string, JsonProcess> strToJsonProcess = {
    {"ascend",           JsonProcess::ASCEND},
    {"acc_pmu",          JsonProcess::ACC_PMU},
    {"cann",             JsonProcess::CANN},
    {"ddr",              JsonProcess::DDR},
    {"stars_chip_trans", JsonProcess::STARS_CHIP_TRANS},
    {"hbm",              JsonProcess::HBM},
    {"communication",    JsonProcess::COMMUNICATION},
    {"hccs",             JsonProcess::HCCS},
    {"os_runtime_api",   JsonProcess::OS_RUNTIME_API},
    {"network_usage",    JsonProcess::NETWORK_USAGE},
    {"disk_usage",       JsonProcess::DISK_USAGE},
    {"memory_usage",     JsonProcess::MEMORY_USAGE},
    {"cpu_usage",        JsonProcess::CPU_USAGE},
    {"msproftx",         JsonProcess::MSPROFTX},
    {"npu_mem",          JsonProcess::NPU_MEM},
    {"overlap_analyse",  JsonProcess::OVERLAP_ANALYSE},
    {"pcie",             JsonProcess::PCIE},
    {"sio",              JsonProcess::SIO},
    {"stars_soc",        JsonProcess::STARS_SOC},
    {"step_trace",       JsonProcess::STEP_TRACE},
    {"freq",             JsonProcess::FREQ},
    {"llc",              JsonProcess::LLC},
    {"nic",              JsonProcess::NIC},
    {"roce",             JsonProcess::ROCE},
    {"qos",              JsonProcess::QOS},
    {"device_tx",        JsonProcess::DEVICE_TX},
};

const std::vector<JsonProcess> allProcesses{
    JsonProcess::ASCEND, JsonProcess::ACC_PMU, JsonProcess::CANN, JsonProcess::DDR, JsonProcess::STARS_CHIP_TRANS,
    JsonProcess::HBM, JsonProcess::COMMUNICATION, JsonProcess::HCCS, JsonProcess::OS_RUNTIME_API,
    JsonProcess::NETWORK_USAGE, JsonProcess::DISK_USAGE, JsonProcess::MEMORY_USAGE, JsonProcess::CPU_USAGE,
    JsonProcess::MSPROFTX, JsonProcess::NPU_MEM, JsonProcess::OVERLAP_ANALYSE, JsonProcess::PCIE, JsonProcess::SIO,
    JsonProcess::STARS_SOC, JsonProcess::STEP_TRACE, JsonProcess::FREQ, JsonProcess::LLC, JsonProcess::NIC,
    JsonProcess::ROCE, JsonProcess::QOS, JsonProcess::DEVICE_TX
};
}
}

#endif // ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H
