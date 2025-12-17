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
#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_BATCH_ID_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_BATCH_ID_H

#include <cstdint>
#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {

namespace Domain {
/*!
 * 数据建模计算和补齐Batch Id，二分法
 * @param task : 任务数组，存放任务指针
 * @param taskNum : 任务数组个数
 * @param flip : 翻转数组，存放翻转指针
 * @param taskNum : 翻转数组个数
 **/
void ModelingComputeBatchIdBinary(HalUniData **task, uint32_t taskNum, HalUniData **flip, uint16_t flipNum);

}

}

#endif /* BATCH_ID_H */
