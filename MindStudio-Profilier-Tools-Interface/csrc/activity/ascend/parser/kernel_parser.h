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

#ifndef MSPTI_PARSER_KERNEL_PARSER_H
#define MSPTI_PARSER_KERNEL_PARSER_H

#include <memory>
#include "csrc/include/mspti_result.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/common/inject/profapi_inject.h"
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/activity/ascend/entity/soclog.h"

namespace Mspti {
namespace Parser {
class KernelParser {
public:
    static KernelParser &GetInstance();
    msptiResult ReportRtTaskTrack(uint32_t agingFlag, const MsprofCompactInfo *data);
    msptiResult ReportStarsSocLog(uint32_t deviceId, const HalLogData& originData);
private:
    KernelParser();
    ~KernelParser();
    class KernelParserImpl;
    std::unique_ptr<KernelParserImpl> pImpl;
};
}
}

#endif // MSPTI_PARSER_KERNEL_PARSER_H
