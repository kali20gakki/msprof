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

#ifndef ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H
#define ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H

#include <cstdint>
#include <string>
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Domain {
const std::string MEMCPY_ASYNC = "MEMCPY_ASYNC";
const uint16_t VALID_MEMCPY_OPERATION = 7;
struct MemcpyInfoData {
    TaskId taskId;
    uint64_t dataSize = UINT64_MAX;
    uint16_t memcpyOperation = UINT16_MAX;
};
}
}

#endif // ANALYSIS_DOMAIN_MEMCPY_INFO_DATA_H
