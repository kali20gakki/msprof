/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : batch_id.h
 * Description        : 补齐batch id算法
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
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
void ModelingComputeBatchIdBinary(HalUniData **task, uint32_t taskNum, HalUniData **flip, uint32_t flipNum);

}

}

#endif /* BATCH_ID_H */
