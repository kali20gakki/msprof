/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
* File Name          : number_mapping.h
* Description        : 用于映射数值到字符串的对象的定义
* Author             : msprof team
* Creation Date      : 2023/12/26
* *****************************************************************************
*/

#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
class NumberMapping {
public:
    // 对象类型
    enum class MappingType {
        GE_DATA_TYPE = 0,
        GE_FORMAT,
        GE_TASK_TYPE,
        HCCL_DATA_TYPE,
        HCCL_LINK_TYPE,
        HCCL_TRANSPORT_TYPE,
        HCCL_RDMA_TYPE,
        HCCL_OP_TYPE,
        LEVEL,
        ACL_API_TAG
    };
    static std::string Get(MappingType type, uint32_t key);
};

} // Drafts
} // Database
} // Viewer
} // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_NUMBER_MAPPING_H
