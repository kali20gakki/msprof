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

#ifndef ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H
#define ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H

#include <cstdint>

namespace Analysis {
namespace Domain {
const uint64_t NS_TO_US = 1000;
const uint64_t S_TO_MS = 1000;
const uint64_t MS_TO_US = 1000;
const uint64_t US_TO_MS = 1000;
const uint64_t MS_TO_NS = 1000000;
const uint64_t FREQ_TO_MHz = 1000000;
}
}

#endif // ANALYSIS_DOMAIN_ENTITIES_CONSTANT_TIME_UNIT_CONSTANT_H

