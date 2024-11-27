/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_stub.cpp
 * Description        : Mspti stub for python interface.
 * Author             : msprof team
 * Creation Date      : 2024/11/19
 * *****************************************************************************
*/

#include "external/mspti.h"

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
