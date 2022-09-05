/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: input cmd log
 * Author: zyw
 * Create: 2020-12-10
 */

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