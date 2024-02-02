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

#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Viewer {
namespace Database {
const std::string HOST = "host";
const std::string DEVICE_PREFIX = "device_";
const std::string SQLITE = "sqlite";

// db name
const std::string DB_NAME_REPORT_DB = "report";

// table name
const std::string TABLE_NAME_STRING_IDS = "STRING_IDS";
const std::string TABLE_NAME_TARGET_INFO_SESSION_TIME = "TARGET_INFO_SESSION_TIME";
const std::string TABLE_NAME_TARGET_INFO_NPU = "TARGET_INFO_NPU";
const std::string TABLE_NAME_ENUM_API_LEVEL = "ENUM_API_LEVEL";
const std::string TABLE_NAME_TASK = "TASK";
const std::string TABLE_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_TASK_INFO = "COMMUNICATION_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_OP = "COMMUNICATION_OP";
const std::string TABLE_NAME_API = "API";

// api level
const std::unordered_map<std::string, uint16_t> API_LEVEL_TABLE = {
    {"pytorch", MSPROF_REPORT_PYTORCH_LEVEL},
    {"pta", MSPROF_REPORT_PTA_LEVEL},
    {"acl", MSPROF_REPORT_ACL_LEVEL},
    {"model", MSPROF_REPORT_MODEL_LEVEL},
    {"node", MSPROF_REPORT_NODE_LEVEL},
    {"hccl",  MSPROF_REPORT_HCCL_NODE_LEVEL},
    {"runtime",  MSPROF_REPORT_RUNTIME_LEVEL}
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

}  // Database
}  // Viewer
}  // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_CONSTANT_H
