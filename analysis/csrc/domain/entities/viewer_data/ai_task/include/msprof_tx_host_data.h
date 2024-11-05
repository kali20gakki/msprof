/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprof_tx_host_data.h
 * Description        : msprofTx数据格式化后的数据结构
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H
#define ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct MsprofTxHostData {
    uint16_t eventType = UINT16_MAX;
    uint32_t pid = UINT32_MAX;
    uint32_t tid = UINT32_MAX;
    uint32_t category = UINT32_MAX;
    uint32_t payloadType = UINT32_MAX;
    uint32_t messageType = UINT32_MAX;
    uint64_t payloadValue = UINT64_MAX;
    uint64_t start = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    std::string message;
};
}
}
#endif // ANALYSIS_DOMAIN_MSPROF_TX_HOST_DATA_H
