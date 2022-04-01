/**
 * @file adx_dsmi.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_DSMI_H
#define ADX_DSMI_H
#include <cstdint>
#include "common/extra_config.h"
int32_t IdeGetPhyDevList(IdeU32Pt devNum, IdeU32Pt devs, uint32_t len);
int32_t IdeGetLogIdByPhyId(uint32_t desPhyId, IdeU32Pt logId);
int32_t AdxGetLogIdByPhyId(uint32_t desPhyId, IdeU32Pt logId);
#endif
