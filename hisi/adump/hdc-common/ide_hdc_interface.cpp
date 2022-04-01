/**
 * @file ide_hdc_interface.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "hdc_api.h"
#include "ide_hdc_interface.h"

int32_t AdxHdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    return Adx::HdcWrite(session, buf, len);
}

int32_t AdxGetVfIdBySession(HDC_SESSION session, IdeI32Pt vfId)
{
    return Adx::IdeGetVfIdBySession(session, vfId);
}
