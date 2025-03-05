/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : basic_data.h
 * Description        : processor通用格式化数据的基类
 * Author             : msprof team
 * Creation Date      : 2025/3/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_BASIC_DATA_H
#define ANALYSIS_DOMAIN_BASIC_DATA_H

#include <cstdint>

namespace Analysis {
namespace Domain {
struct BasicData {
    uint64_t timestamp = UINT64_MAX; // 通常意义指代数据的开始时间 ns

public:
    BasicData() = default;
    BasicData(uint64_t timestamp) : timestamp(timestamp) {}
};
}
}

#endif // ANALYSIS_DOMAIN_BASIC_DATA_H