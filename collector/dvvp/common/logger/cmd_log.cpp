/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "cmd_log.h"
#include <cstdarg>
#include "securec.h"

namespace Collector {
namespace Dvvp {
namespace Msprofbin {
const int MSPROF_BIN_MAX_LOG_SIZE  = 1024; // 1024 : 1k

CmdLog::CmdLog() {}

CmdLog::~CmdLog() {}

void CmdLog::CmdErrorLog(CONST_CHAR_PTR format, ...) const
{
    va_list args;
    char buffer[MSPROF_BIN_MAX_LOG_SIZE + 1] = {0};
    va_start(args, format);
    int ret = vsnprintf_truncated_s(buffer, sizeof(buffer), format, args);
    if (ret > 0) {
        std::cout << "[ERROR] " << buffer << std::endl;
    }
    va_end(args);
}

void CmdLog::CmdInfoLog(CONST_CHAR_PTR format, ...) const
{
    va_list args;
    char buffer[MSPROF_BIN_MAX_LOG_SIZE + 1] = {0};
    va_start(args, format);
    int ret = vsnprintf_truncated_s(buffer, sizeof(buffer), format, args);
    if (ret > 0) {
        std::cout << "[INFO] " << buffer << std::endl;
    }
    va_end(args);
}

void CmdLog::CmdWarningLog(CONST_CHAR_PTR format, ...) const
{
    va_list args;
    char buffer[MSPROF_BIN_MAX_LOG_SIZE + 1] = {0};
    va_start(args, format);
    int ret = vsnprintf_truncated_s(buffer, sizeof(buffer), format, args);
    if (ret > 0) {
        std::cout << "[WARNING] " << buffer << std::endl;
    }
    va_end(args);
}
}
}
}