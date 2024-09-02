/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mc2_comm_info_data.h
 * Description        : mc2类算子大小算子映射关系
 * Author             : MSPROF TEAM
 * Creation Date      : 2024/9/1
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MC2_COMM_INFO_H
#define ANALYSIS_DOMAIN_MC2_COMM_INFO_H

namespace Analysis {
namespace Domain {
struct MC2CommInfoData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t aiCpuKfcStreamId = UINT16_MAX;   // 通信大算子所在stream
    std::vector<uint16_t> commStreamIds{};    // 通信小算子所在stream
};
}
}

#endif // ANALYSIS_DOMAIN_MC2_COMM_INFO_H
