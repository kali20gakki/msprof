/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : runtime_utils.h
 * Description        : Runtime Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/12/4
 * *****************************************************************************
*/

#ifndef MSPTI_RUNTIME_COMMON_UTILS_H
#define MSPTI_RUNTIME_COMMON_UTILS_H

#include <cstdint>
#include "csrc/common/inject/inject_base.h"

namespace Mspti {
namespace Common {

uint32_t GetDeviceId();
uint32_t GetStreamId(RtStreamT stm);

} // Common
} // Mspti

#endif
