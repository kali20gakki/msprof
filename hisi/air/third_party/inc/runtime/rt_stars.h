/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Description: the definition of stars
 */

#ifndef __CCE_RUNTIME_RT_STARS_H
#define __CCE_RUNTIME_RT_STARS_H

#include "base.h"
#include "rt_stars_define.h"
#if defined(__cplusplus)
extern "C" {
#endif

RTS_API rtError_t rtCmoTaskLaunch(rtCmoTaskInfo_t *taskInfo, rtStream_t stm, uint32_t flag);

RTS_API rtError_t rtBarrierTaskLaunch(rtBarrierTaskInfo_t *taskInfo, rtStream_t stm, uint32_t flag);

#if defined(__cplusplus)
}
#endif
#endif
