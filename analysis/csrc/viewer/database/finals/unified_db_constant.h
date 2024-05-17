/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_constant.h
 * Description        : unified_db_constant，定义统一db需要的各类常量
 * Author             : msprof team
 * Creation Date      : 2023/12/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_CONSTANT_H
#define ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_CONSTANT_H

#include <string>
#include <unordered_map>
#include <map>
#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Viewer {
namespace Database {
const std::string HOST = "host";
const std::string DEVICE_PREFIX = "device_";
const std::string SQLITE = "sqlite";

// db name
const std::string DB_NAME_MSPROF_DB = "msprof";

// processor name & table name
const std::string PROCESSOR_NAME_STRING_IDS = "STRING_IDS";
const std::string TABLE_NAME_STRING_IDS = "STRING_IDS";

const std::string PROCESSOR_NAME_SESSION_TIME_INFO = "SESSION_TIME_INFO";
const std::string TABLE_NAME_SESSION_TIME_INFO = "SESSION_TIME_INFO";

const std::string PROCESSOR_NAME_NPU_INFO = "NPU_INFO";
const std::string TABLE_NAME_NPU_INFO = "NPU_INFO";

const std::string PROCESSOR_NAME_HOST_INFO = "HOST_INFO";
const std::string TABLE_NAME_HOST_INFO = "HOST_INFO";

const std::string PROCESSOR_NAME_TASK = "TASK";
const std::string TABLE_NAME_TASK = "TASK";

const std::string PROCESSOR_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";
const std::string TABLE_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";

const std::string PROCESSOR_NAME_COMMUNICATION = "COMMUNICATION";
const std::string TABLE_NAME_COMMUNICATION_TASK_INFO = "COMMUNICATION_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_OP = "COMMUNICATION_OP";

const std::string PROCESSOR_NAME_API = "API";
const std::string TABLE_NAME_CANN_API = "CANN_API";

const std::string PROCESSOR_NAME_NPU_MEM = "NPU_MEM";
const std::string TABLE_NAME_NPU_MEM = "NPU_MEM";

const std::string PROCESSOR_NAME_NPU_MODULE_MEM = "NPU_MODULE_MEM";
const std::string TABLE_NAME_NPU_MODULE_MEM = "NPU_MODULE_MEM";

const std::string PROCESSOR_NAME_NPU_OP_MEM = "NPU_OP_MEM";
const std::string TABLE_NAME_NPU_OP_MEM = "NPU_OP_MEM";

const std::string PROCESSOR_NAME_ENUM = "ENUM";
const std::string TABLE_NAME_ENUM_API_TYPE = "ENUM_API_TYPE";
const std::string TABLE_NAME_ENUM_MODULE = "ENUM_MODULE";

const std::string PROCESSOR_NAME_NIC = "NIC";
const std::string TABLE_NAME_NIC = "NIC";

const std::string PROCESSOR_NAME_ROCE = "ROCE";
const std::string TABLE_NAME_ROCE = "ROCE";

const std::string PROCESSOR_NAME_HBM = "HBM";
const std::string TABLE_NAME_HBM = "HBM";

const std::string PROCESSOR_NAME_DDR = "DDR";
const std::string TABLE_NAME_DDR = "DDR";

const std::string PROCESSOR_NAME_LLC = "LLC";
const std::string TABLE_NAME_LLC = "LLC";

const std::string PROCESSOR_NAME_PMU = "PMU";
const std::string TABLE_NAME_TASK_PMU_INFO = "TASK_PMU_INFO";
const std::string TABLE_NAME_SAMPLE_PMU_TIMELINE = "SAMPLE_PMU_TIMELINE";
const std::string TABLE_NAME_SAMPLE_PMU_SUMMARY = "SAMPLE_PMU_SUMMARY";

const std::string PROCESSOR_NAME_PCIE = "PCIE";
const std::string TABLE_NAME_PCIE = "PCIE";

const std::string PROCESSOR_NAME_HCCS = "HCCS";
const std::string TABLE_NAME_HCCS = "HCCS";

const std::string PROCESSOR_NAME_ACC_PMU = "ACC_PMU";
const std::string TABLE_NAME_ACC_PMU = "ACC_PMU";

const std::string PROCESSOR_NAME_SOC = "SOC";
const std::string TABLE_NAME_SOC = "SOC_BANDWIDTH_LEVEL";

const std::string PROCESSOR_NAME_META_DATA = "META_DATA";
const std::string TABLE_NAME_META_DATA = "META_DATA";

const std::string PROCESSOR_NAME_AICORE_FREQ = "AICORE_FREQ";
const std::string TABLE_NAME_AICORE_FREQ = "AICORE_FREQ";


// api level
const std::unordered_map<std::string, uint16_t> API_LEVEL_TABLE = {
    {"acl", MSPROF_REPORT_ACL_LEVEL},
    {"model", MSPROF_REPORT_MODEL_LEVEL},
    {"node", MSPROF_REPORT_NODE_LEVEL},
    {"hccl",  MSPROF_REPORT_HCCL_NODE_LEVEL},
    {"runtime",  MSPROF_REPORT_RUNTIME_LEVEL}
};

// npu module name
const std::unordered_map<std::string, uint16_t> MODULE_NAME_TABLE = {
    {"SLOG", 0},
    {"IDEDD", 1},
    {"IDEDH", 2},
    {"HCCL", 3},
    {"FMK", 4},
    {"HIAIENGINE",  5},
    {"DVPP",  6},
    {"RUNTIME", 7},
    {"CCE", 8},
    {"HDC", 9},
    {"DRV", 10},
    {"MDCFUSION", 11},
    {"MDCLOCATION",  12},
    {"MDCPERCEPTION",  13},
    {"MDCFSM", 14},
    {"MDCCOMMON", 15},
    {"MDCMONITOR", 16},
    {"MDCBSWP", 17},
    {"MDCDEFAULT", 18},
    {"MDCSC",  19},
    {"MDCPNC",  20},
    {"MLL", 21},
    {"DEVMM", 22},
    {"KERNEL", 23},
    {"LIBMEDIA", 24},
    {"CCECPU", 25},
    {"ASCENDDK",  26},
    {"ROS",  27},
    {"HCCP", 28},
    {"ROCE", 29},
    {"TEFUSION", 30},
    {"PROFILING", 31},
    {"DP", 32},
    {"APP",  33},
    {"TS",  34},
    {"TSDUMP", 35},
    {"AICPU", 36},
    {"LP", 37},
    {"TDT", 38},
    {"FE", 39},
    {"MD",  40},
    {"MB",  41},
    {"ME", 42},
    {"IMU", 43},
    {"IMP", 44},
    {"GE", 45},
    {"MDCFUSA", 46},
    {"CAMERA",  47},
    {"ASCENDCL",  48},
    {"TEEOS", 49},
    {"ISP", 50},
    {"SIS", 51},
    {"HSM", 52},
    {"DSS", 53},
    {"PROCMGR",  54},
    {"BBOX",  55},
    {"AIVECTOR", 56},
    {"TBE", 57},
    {"FV", 58},
    {"MDCMAP", 59},
    {"TUNE", 60},
    {"HSS",  61},
    {"FFTS",  62},
    {"OP",  63},
    {"UDF",  64},
    {"HICAID", 65},
    {"TSYNC", 66},
    {"AUDIO", 67},
    {"TPRT", 68},
    {"ASCENDCKERNEL", 69},
    {"ASYS",  70},
    {"ATRACE",  71},
    {"RTC", 72},
    {"SYSMONITOR", 73},
    {"MBUFF", 74},
    {"CUSTOM", 75}
};

// TS为stars时芯片的sqetype
const std::map<std::string, std::string> STARS_SQE_TYPE_TABLE{
    {"0", "AI_CORE"},
    {"1", "AI_CPU"},
    {"2", "AIV_SQE"},
    {"3", "PLACE_HOLDER_SQE"},
    {"4", "EVENT_RECORD_SQE"},
    {"5", "EVENT_WAIT_SQE"},
    {"6", "NOTIFY_RECORD_SQE"},
    {"7", "NOTIFY_WAIT_SQE"},
    {"8", "WRITE_VALUE_SQE"},
    {"9", "VQ6_SQE"},
    {"10", "TOF_SQE"},
    {"11", "SDMA_SQE"},
    {"12", "VPC_SQE"},
    {"13", "JPEGE_SQE"},
    {"14", "JPEGD_SQE"},
    {"15", "DSA_SQE"},
    {"16", "ROCCE_SQE"},
    {"17", "PCIE_DMA_SQE"},
    {"18", "HOST_CPU_SQE"},
    {"19", "CDQM_SQE"},
    {"20", "C_CORE_SQE"}
};

// TS为hwts时芯片的sqetype
const std::map<std::string, std::string> HW_SQE_TYPE_TABLE{
    {"0", "AI_CORE"},
    {"1", "AI_CPU"},
    {"2", "AIV_SQE"},
    {"3", "PLACE_HOLDER_SQE"},
    {"4", "EVENT_RECORD_SQE"},
    {"5", "EVENT_WAIT_SQE"},
    {"6", "NOTIFY_RECORD_SQE"},
    {"7", "NOTIFY_WAIT_SQE"},
    {"8", "WRITE_VALUE_SQE"},
    {"9", "SDMA_SQE"},
    {"10", "MAX_SQE"}
};

// 单位换算常量
const uint64_t NANO_SECOND = 1000000000;
const uint64_t MICRO_SECOND = 1000000;
const uint64_t MILLI_SECOND = 1000;
constexpr const uint64_t MAX_DB_BYTES = 10ULL * 1024 * 1024 * 1024;
const uint16_t BYTE_SIZE = 1024;
const uint16_t PERCENTAGE = 100;

}  // Database
}  // Viewer
}  // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_CONSTANT_H
