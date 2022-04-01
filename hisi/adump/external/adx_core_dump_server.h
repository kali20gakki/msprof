/**
* @file adx_core_dump_server.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ADX_CORE_DUMP_SERVER_H
#define ADX_CORE_DUMP_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief initialize server for coredump function.
 * @return
 *      IDE_DAEMON_OK:    coredump server init success
 *      IDE_DAEMON_ERROR: coredump server init failed
 */
int AdxCoreDumpServerInit();
#ifdef __cplusplus
}
#endif
#endif

