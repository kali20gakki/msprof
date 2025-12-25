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

#ifndef ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H
#define ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H

#include "analysis/csrc/application/timeline/json_constant.h"
namespace Analysis {
namespace Application {
enum class ConnectionCategory {
    GENERAL,
    MSPROF_TX,
    INVALID = 2
};

class ConnectionIdPool {
public:
    static std::string GetConnectionId(uint64_t connId, ConnectionCategory type)
    {
        if (ConnectionCategory::GENERAL == type) {
            return std::to_string(connId << CONN_OFFSET);
        } else {
            return std::to_string(connId);
        }
    }
};
}
}
#endif // ANALYSIS_APPLICATION_CONNECTION_ID_POOL_H
