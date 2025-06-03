/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fusion_op_data.h
 * Description        : 处理fusion_op表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_FUSION_OP_DATA_H
#define ANALYSIS_DOMAIN_FUSION_OP_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct FusionOpInfo : public BasicData {
    uint32_t fusionOpNums = UINT32_MAX;
    uint32_t modelId = UINT32_MAX;
    std::string fusionName;
    std::string memoryInput;
    std::string memoryOutput;
    std::string memoryWeight;
    std::string memoryWorkspace;
    std::string memoryTotal;
    std::string opNames;
};
}
}

#endif // ANALYSIS_DOMAIN_FUSION_OP_DATA_H