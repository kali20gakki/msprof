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

#include "prof_common.h"

namespace Analysis {
namespace Viewer {
namespace Database {
const std::string HOST = "host";
const std::string DEVICE_PREFIX = "device_";
const std::string SQLITE = "sqlite";

// table name
const std::string TABLE_NAME_STRING_IDS = "STRING_IDS";
const std::string TABLE_NAME_TARGET_INFO_SESSION_TIME = "TARGET_INFO_SESSION_TIME";
const std::string TABLE_NAME_TARGET_INFO_NPU = "TARGET_INFO_NPU";
const std::string TABLE_NAME_ENUM_API_LEVEL = "ENUM_API_LEVEL";
const std::string TABLE_NAME_TASK = "TASK";
const std::string TABLE_NAME_COMPUTE_TASK_INFO = "COMPUTE_TASK_INFO";
const std::string TABLE_NAME_COMMUNICATION_TASK_INFO = "COMMUNICATION_TASK_INFO";
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

// 单位换算常量
const uint64_t NANO_SECOND = 1000000000;
const uint64_t MICRO_SECOND = 1000000;
const uint64_t MILLI_SECOND = 1000;

}  // Database
}  // Viewer
}  // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_CONSTANT_H
