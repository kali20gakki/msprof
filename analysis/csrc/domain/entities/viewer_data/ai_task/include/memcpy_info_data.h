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
struct MemcpyInfoData {
    uint64_t dataSize = UINT64_MAX;
    uint64_t maxSize = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    std::string memcpyDirection;
};
}
}

#endif // ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H
