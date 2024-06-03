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
const double INVALID_TIME = -1;
const uint64_t DEFAULT_TIMESTAMP = UINT64_MAX;
const std::string SQLITE = "sqlite";
const std::string UNKNOWN_STRING = "UNKNOWN";
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_CONSTANT_DEFAULT_VALUE_CONSTANT_H
