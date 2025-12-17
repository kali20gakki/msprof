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

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {
const int FFTS_PLUS = 4;
const int TASK_AIC = 6;
const int TASK_AIV = 7;
const int NORMAL_TASK = 0;
AcceleratorType GetAcceleratorTypeByType(uint16_t subTaskType, uint16_t fftsType);
}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H
