/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.h
 * Description        : Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_UTILS_H
#define MSPTI_COMMON_UTILS_H

#include <stdint.h>

constexpr uint32_t SECTONSEC = 1000000000UL;

namespace Mspti {
namespace Common {

class Utils {
public:
    static uint64_t GetClockMonotonicRawNs();
};

}  // Common
}  // Mspti

#endif
