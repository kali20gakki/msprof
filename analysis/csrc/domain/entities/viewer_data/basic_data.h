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

#ifndef ANALYSIS_DOMAIN_BASIC_DATA_H
#define ANALYSIS_DOMAIN_BASIC_DATA_H

#include <cstdint>

namespace Analysis {
namespace Domain {
struct BasicData {
    uint64_t timestamp = UINT64_MAX; // 通常意义指代数据的开始时间 ns

public:
    BasicData() = default;
    BasicData(uint64_t timestamp) : timestamp(timestamp) {}
};
}
}

#endif // ANALYSIS_DOMAIN_BASIC_DATA_H