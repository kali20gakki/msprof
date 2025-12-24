/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_CONVERT_SOCLOG_CONVERT_H
#define MSPTI_CONVERT_SOCLOG_CONVERT_H

#include <cstdlib>
#include <cstdint>
#include <memory>
#include <vector>
#include "csrc/activity/ascend/entity/soclog.h"
#include "csrc/activity/ascend/channel/basic_convert.h"

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