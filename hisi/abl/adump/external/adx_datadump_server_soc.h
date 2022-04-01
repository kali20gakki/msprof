/**
* @file adx_datadump_server_soc.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ADX_DATADUMP_SERVER_SOC_H
#define ADX_DATADUMP_SERVER_SOC_H
namespace Adx {
/**
 * @brief initialize server for soc datadump function.
 * @param [in]  hostPid : app pid
 * @return
 *      IDE_DAEMON_OK:    datadump server init success
 *      IDE_DAEMON_ERROR: datadump server init failed
 */
int32_t AdxSocDataDumpInit(const std::string &hostPid);

/**
 * @brief uninitialize server for soc datadump function.
 * @return
 *      IDE_DAEMON_OK:    datadump server uninit success
 *      IDE_DAEMON_ERROR: datadump server uninit failed
 */
int32_t AdxSocDataDumpUnInit();
}
#endif
