/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_calculator.h
 * Description        : calculator of hccl.
 * Author             : msprof team
 * Creation Date      : 2024/11/1
 * *****************************************************************************
*/

#ifndef MSPTI_PARSER_HCCL_CALCULATOR_H
#define MSPTI_PARSER_HCCL_CALCULATOR_H

#include <cstdint>
#include <memory>
#include "activity/ascend/entity/hccl_op_desc.h"

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
