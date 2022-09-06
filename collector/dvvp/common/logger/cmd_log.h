/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: input cmd log
 * Author: zyw
 * Create: 2020-12-10
 */

#ifndef COLLECTOR_DVVP_MSPROFBIN_CMD_LOG_H
#define COLLECTOR_DVVP_MSPROFBIN_CMD_LOG_H
#include <iostream>
#include "singleton/singleton.h"

namespace Collector {
namespace Dvvp {
namespace Msprofbin {
using CONST_CHAR_PTR = const char *;

class CmdLog : public analysis::dvvp::common::singleton::Singleton<CmdLog> {
public:
    CmdLog();
    ~CmdLog() override;
    void CmdErrorLog(CONST_CHAR_PTR format, ...) const;
    void CmdInfoLog(CONST_CHAR_PTR format, ...) const;
    void CmdWarningLog(CONST_CHAR_PTR format, ...) const;
};
}
}
}
#endif