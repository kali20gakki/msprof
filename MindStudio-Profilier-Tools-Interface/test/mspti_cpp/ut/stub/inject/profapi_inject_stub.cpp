/**
* @file profapi_inject_stub.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "csrc/common/inject/profapi_inject.h"
#include "csrc/common/utils.h"

namespace Mspti {
namespace Inject {
int32_t MsprofRegisterProfileCallback(int32_t callbackType, VOID_PTR callback, uint32_t len)
{
    UNUSED(callbackType);
    UNUSED(callback);
    UNUSED(len);
    return MSPTI_SUCCESS;
}

int32_t profRegReporterCallback(ProfReportHandle reporter)
{
    UNUSED(reporter);
    return MSPTI_SUCCESS;
}

int32_t profRegCtrlCallback(MsprofCtrlHandle handle)
{
    UNUSED(handle);
    return MSPTI_SUCCESS;
}

int32_t profSetProfCommand(VOID_PTR command, uint32_t len)
{
    UNUSED(command);
    UNUSED(len);
    return MSPTI_SUCCESS;
}

int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    UNUSED(moduleId);
    UNUSED(type);
    UNUSED(data);
    UNUSED(len);
    return MSPTI_SUCCESS;
}

uint64_t MsptiGetHashIdImpl(const char* hashInfo, size_t len)
{
    UNUSED(hashInfo);
    UNUSED(len);
    return 0;
}

int8_t MsptiHostFreqIsEnableImpl()
{
    constexpr int8_t TRUE = 1;
    constexpr int8_t FALSE = 0;
    return TRUE;
}

int32_t MsptiApiReporterCallbackImpl(uint32_t agingFlag, const MsprofApi* const data)
{
    UNUSED(agingFlag);
    UNUSED(data);
    return MSPTI_SUCCESS;
}

int32_t MsptiEventReporterCallbackImpl(uint32_t agingFlag, const MsprofEvent* const event)
{
    UNUSED(agingFlag);
    UNUSED(event);
    return MSPTI_SUCCESS;
}

int32_t MsptiCompactInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    UNUSED(agingFlag);
    UNUSED(data);
    UNUSED(length);
    return MSPTI_SUCCESS;
}

int32_t MsptiAddiInfoReporterCallbackImpl(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t length)
{
    UNUSED(agingFlag);
    UNUSED(data);
    UNUSED(length);
    return MSPTI_SUCCESS;
}

int32_t MsptiRegReportTypeInfoImpl(uint16_t level, uint32_t typeId, const char* name, size_t len)
{
    UNUSED(level);
    UNUSED(typeId);
    UNUSED(name);
    UNUSED(len);
    return 0;
}
}
}