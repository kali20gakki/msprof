/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_unit_constant.h
 * Description        : 时间单位转换常量
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H
#define ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H

#include <cstdint>

namespace Analysis {
namespace Domain {
const uint64_t NS_TO_US = 1000;
const uint64_t S_TO_MS = 1000;
const uint64_t MS_TO_US = 1000;
const uint64_t US_TO_MS = 1000;
const uint64_t MS_TO_NS = 1000000;
const uint64_t FREQ_TO_MHz = 1000000;
}
}

#endif // ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H

