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
 * -------------------------------------------------------------------------
*/

#include "csrc/include/mspti.h"

msptiResult msptiSubscribe(msptiSubscriberHandle *subscriber, msptiCallbackFunc callback, void *userdata)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiUnsubscribe(msptiSubscriberHandle subscriber)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityRegisterCallbacks(
    msptiBuffersCallbackRequestFunc funcBufferRequested, msptiBuffersCallbackCompleteFunc funcBufferCompleted)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityEnable(msptiActivityKind kind)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityDisable(msptiActivityKind kind)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityGetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityFlushAll(uint32_t flag)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityFlushPeriod(uint32_t time)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityDisableMarkerDomain(const char* name)
{
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityEnableMarkerDomain(const char* name)
{
    return MSPTI_SUCCESS;
}
