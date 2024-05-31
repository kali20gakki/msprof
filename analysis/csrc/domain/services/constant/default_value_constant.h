/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : default_value_constant.h
 * Description        : 存储常量
 * Author             : msprof team
 * Creation Date      : 2024/4/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_CONSTANT_DEFAULT_VALUE_CONSTANT_H
#define ANALYSIS_DOMAIN_SERVICES_CONSTANT_DEFAULT_VALUE_CONSTANT_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Analysis {
namespace Domain {
const uint64_t DEFAULT_MODEL_ID = UINT64_MAX;
const uint64_t DEFAULT_HOST_TASK_TYPE = UINT64_MAX;
const uint32_t DEFAULT_DEVICE_TASK_TYPE = UINT32_MAX;
const int32_t DEFAULT_INDEX_ID = -1;
const int64_t DEFAULT_CONNECTION_ID = -1;
const uint64_t DEFAULT_TIMESTAMP = UINT64_MAX;
const std::string SQLITE = "sqlite";
const std::string UNKNOWN_STRING = "UNKNOWN";
const int CHIP4_DEFAULT_COLUMN_NUM = 6;
const int OTHER_CHIP_DEFAULT_COLUMN_NUM = 5;
const int CHIP4_PMU_TIME_NUM = 2;
const int OTHER_CHIP_PMU_TIME_NUM = 1;
const std::unordered_map<uint32_t, std::string> hostTaskTypeMap {
    {0,  "AI_CORE"},
    {1,  "AI_CPU"},
    {2,  "AI_VECTOR_CORE"},
    {3,  "WRITE_BACK"},
    {4,  "MIX_AIC"},
    {5,  "MIX_AIV"},
    {6,  "FFTS_PLUS"},
    {7,  "DSA_SQE"},
    {8,  "DVPP"},
    {9,  "HCCL"},
    {10, "INVALID"},
    {11, "HCCL_AI_CPU"}
};

const std::unordered_map<uint32_t, std::string> deviceTaskTypeMap {
    {0, "AI_CORE"},
    {1, "AI_CPU"},
    {2, "AIV_SQE"},
    {3, "PLACE_HOLDER_SQE"},
    {4, "EVENT_RECORD_SQE"},
    {5, "EVENT_WAIT_SQE"},
    {6, "NOTIFY_RECORD_SQE"},
    {7, "NOTIFY_WAIT_SQE"},
    {8, "WRITE_VALUE_SQE"},
    {9, "VQ6_SQE"},
    {10, "TOF_SQE"},
    {11, "SDMA_SQE"},
    {12, "VPC_SQE"},
    {13, "JPEGE_SQE"},
    {14, "JPEGD_SQE"},
    {15, "DSA_SQE"},
    {16, "ROCCE_SQE"},
    {17, "PCIE_DMA_SQE"},
    {18, "HOST_CPU_SQE"},
    {19, "CDQM_SQE"},
    {20, "C_CORE_SQE"}
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_CONSTANT_DEFAULT_VALUE_CONSTANT_H
