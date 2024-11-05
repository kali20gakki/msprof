/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sio_data.h
 * Description        : sio_processor处理sio表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct SioData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t dieId = UINT16_MAX; // 0 ~ 1
    uint64_t localTime = 0;
    double reqRxBandwidth = 0; // 请求流通道, MB/s
    double rspRxBandwidth = 0; // 回应流通道, MB/s
    double snpRxBandwidth = 0; // 侦听流通道, MB/s
    double datRxBandwidth = 0; // 数据流通道, MB/s
    double reqTxBandwidth = 0;
    double rspTxBandwidth = 0;
    double snpTxBandwidth = 0;
    double datTxBandwidth = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_SIO_FORMAT_DATA_H
