/**
* @file adx_api.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ADX_LOG_API_H
#define ADX_LOG_API_H
#include "common/extra_config.h"
#ifdef __cplusplus
extern "C" {
#endif
    /**
     * @brief set or get device log level
     * @param [in] uint16_t devId: device id
     * @param [in] IdeString logLevel: device log level(debug/info/warning/error/null etc.)
     * @param [in]logLevelResult: log level result
     * @param [in]logOperatonType: log operaton type
     * @return 0: success; other : failed
     */
    int AdxOperateDeviceLogLevel(unsigned short devId,
                                 IdeString logLevel,
                                 char *logLevelResult,
                                 int logLevelResultLength,
                                 int logOperatonType);
#ifdef __cplusplus
}
#endif
#endif