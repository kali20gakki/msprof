/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soclog_convert.h
 * Description        : 转换驱动数据为通用SocLog
 * Author             : msprof team
 * Creation Date      : 2025/07/29
 * *****************************************************************************
 */

#ifndef MSPTI_CONVERT_SOCLOG_CONVERT_H
#define MSPTI_CONVERT_SOCLOG_CONVERT_H

#include <cstdlib>
#include <cstdint>
#include <memory>
#include <vector>
#include "csrc/activity/ascend/entity/soclog.h"
#include "basic_convert.h"

namespace Mspti {
namespace Convert {
class SocLogConvert : public BasicConvert<SocLogConvert, HalLogData> {
public:
    static SocLogConvert& GetInstance();
    ~SocLogConvert() = default;
    size_t GetStructSize(uint32_t deviceId, Common::PlatformType chipType) const;
    TransFunc GetTransFunc(uint32_t deviceId, Common::PlatformType chipType) const;

private:
    SocLogConvert() = default;
};
}
}

#endif // MSPTI_CONVERT_SOCLOG_CONVERT_H