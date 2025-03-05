/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_data.h
 * Description        : api_processor处理api_event表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_API_DATA_H
#define ANALYSIS_DOMAIN_API_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct ApiData : public BasicData {
    uint16_t level = UINT16_MAX;
    uint32_t threadId = UINT32_MAX;
    uint64_t end = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    std::string apiName;
    std::string structType;  // api数据中的structType字段为runtime、model、node层数据的apiName
    std::string id;  // api数据中的id字段为acl层数据的apiName
    std::string itemId; // api数据中hccl层数据的apiName
};
}
}

#endif // ANALYSIS_DOMAIN_API_DATA_H
