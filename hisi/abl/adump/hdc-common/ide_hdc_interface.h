/**
 * @file ide_hdc_interface.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_HDC_INTERFACE_H
#define IDE_HDC_INTERFACE_H
#include "ascend_hal.h"
#include "ide_tlv.h"
#include "common/extra_config.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int32_t AdxHdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len);
extern int32_t AdxGetVfIdBySession(HDC_SESSION session, IdeI32Pt vfId);
#ifdef __cplusplus
}
#endif
#endif //IDE_HDC_INTERFACE_H