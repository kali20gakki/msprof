/**
* @file adx_api.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ADX_API_H
#define ADX_API_H
#include "common/extra_config.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief       hdc send source file to destination file
 * @param [in]  srcFile : source file
 * @param [in]  desFile : destination file
 * @return
 *      0 : success; other : failed
 */
int AdxHdcSendFile(IdeString srcFile, IdeString desFile);

/**
 * @brief       get device log
 * @param [in]  unsigned short devId : device id
 * @param [in]  desPath : store device log path of host
 * @param [in]  logType : device log type(slog, stackcore, bbox, message etc.)
 * @return
 *      0 : success; other : failed
 */
int AdxGetDeviceFile(unsigned short devId, IdeString desPath, IdeString logType);
#ifdef __cplusplus
}
#endif
#endif