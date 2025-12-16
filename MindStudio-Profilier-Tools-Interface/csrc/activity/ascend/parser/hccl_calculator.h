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

#ifndef MSPTI_PARSER_HCCL_CALCULATOR_H
#define MSPTI_PARSER_HCCL_CALCULATOR_H

#include <cstdint>
#include <memory>
#include "csrc/activity/ascend/entity/hccl_op_desc.h"

namespace Mspti {
namespace Parser {
class HcclCalculator {
public:
    static msptiResult CalculateBandWidth(HcclOpDesc* activityHccl);
private:
    HcclCalculator() = delete;
    HcclCalculator(const HcclCalculator&) = delete;
    HcclCalculator* operator=(const HcclCalculator&) = delete;
};
}
}

#endif // MSPTI_PARSER_HCCL_CALCULATOR_H
