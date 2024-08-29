/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : connection_id_pool.h
 * Description        : 封装connectionId的计算逻辑
 * Author             : msprof team
 * Creation Date      : 2024/8/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H
#define ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H

#include "analysis/csrc/application/timeline/json_constant.h"
namespace Analysis {
namespace Application {
enum class ConnectionCategory {
    GENERAL,
    MSPROF_TX,
    INVALID = 2
};

class ConnectionIdPool {
public:
    static std::string GetConnectionId(uint64_t connId, ConnectionCategory type)
    {
        if (ConnectionCategory::GENERAL == type) {
            return std::to_string(connId << CONN_OFFSET);
        } else {
            return std::to_string(connId);
        }
    }
};
}
}
#endif // ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H
