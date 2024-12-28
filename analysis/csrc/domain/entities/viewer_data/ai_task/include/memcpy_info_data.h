/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_data.h
 * Description        : memcpy_info_processor处理MemcpyInfo表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H
#define ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
const std::string MEMCPY_ASYNC = "MEMCPY_ASYNC";
const uint16_t VALID_MEMCPY_OPERATION = 7;
struct MemcpyInfoData {
    uint64_t globalTaskId = UINT64_MAX;
    uint64_t dataSize = UINT64_MAX;
    uint16_t memcpyOperation = UINT16_MAX;
};
}
}

#endif // ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H
