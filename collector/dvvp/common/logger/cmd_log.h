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