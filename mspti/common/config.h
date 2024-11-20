/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : config.h
 * Description        : Common config.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_CONFIG_H
#define MSPTI_COMMON_CONFIG_H

#include <limits>

constexpr int64_t SUPPORT_SYSCNT_DRV_VERSION = 0x071905;
constexpr uint32_t HOST_ID = 64;
constexpr uint32_t MSTONS = 1000000;
constexpr uint32_t MARK_MAX_CACHE_NUM = std::numeric_limits<uint32_t>::max();
constexpr uint32_t MAX_MARK_MSG_LEN = std::numeric_limits<uint8_t>::max();
constexpr uint32_t DEFAULT_PERIOD_FLUSH_TIME = 60000;

#define CHANNEL_PROF_ERROR (-1)
#define CHANNEL_PROF_STOPPED_ALREADY (-4)
#define COMM_NAME_MAX_LENGTH 128
#endif
