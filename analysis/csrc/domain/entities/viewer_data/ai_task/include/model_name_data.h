/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_name_data.h
 * Description        : 处理model_name表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2025/6/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MODEL_NAME_DATA_H
#define ANALYSIS_DOMAIN_MODEL_NAME_DATA_H

#include <string>

namespace Analysis {
namespace Domain {
struct ModelName {
    uint32_t modelId;
    std::string modelName;
};
}
}

#endif // ANALYSIS_DOMAIN_MODEL_NAME_DATA_H