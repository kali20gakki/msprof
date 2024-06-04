/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.cpp
 * Description        : Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "common/utils.h"

#include <ctime>
#include <sys/prctl.h>

#include "securec.h"

namespace Mspti {
namespace Common {

uint64_t Utils::GetClockMonotonicRawNs()
{
    struct timespec ts;
    if (memset_s(&ts, sizeof(timespec), 0, sizeof(timespec)) != EOK) {
        return 0;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * SECTONSEC + static_cast<uint64_t>(ts.tv_nsec);
}

std::string Utils::GetProcName()
{
    static char buf[16] = {'\0'};
    prctl(PR_GET_NAME, (unsigned long)buf);
    return std::string(buf);
}
}  // Common
}  // Mspti