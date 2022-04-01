/**
 * @file adx_dump_soc_api.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "adx_dump_soc_helper.h"

/**
 * @brief dump start api,create a HDC session for dump
 * @param [in] privInfo: remote connect info
 *
 * @return
 *      not NULL: Handle used by hdc
 *      NULL:     dump start failed
 */
extern "C" IDE_SESSION IdeDumpStart(IdeString privInfo)
{
    return Adx::SocDumpStart(privInfo);
}

/**
 * @brief dump data to remote server
 * @param [in] session: HDC session to dump data
 * @param [in] dumpChunk: Dump information
 * @return
 *      IDE_DAEMON_INVALID_PARAM_ERROR: invalid parameter
 *      IDE_DAEMON_UNKNOW_ERROR: write data failed
 *      IDE_DAEMON_NONE_ERROR:   write data succ
 */
extern "C" IdeErrorT IdeDumpData(IDE_SESSION session, const IdeDumpChunk *dumpChunk)
{
    return Adx::SocDumpData(session, dumpChunk);
}

/**
 * @brief send dump end msg
 * @param [in] session: HDC session to dump data
 * @return
 *      IDE_DAEMON_UNKNOW_ERROR: send dump end msg failed
 *      IDE_DAEMON_NONE_ERROR:   send dump end msg success
 */
extern "C" IdeErrorT IdeDumpEnd(IDE_SESSION session)
{
    return Adx::SocDumpEnd(session);
}