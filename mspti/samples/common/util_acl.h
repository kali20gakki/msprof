/**
* @file util_acl.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef UTIL_ACL_H_
#define UTIL_ACL_H_

#include "acl/acl.h"
#include "common/util_mspti.h"

int Init(int32_t deviceId, aclrtContext* context, aclrtStream* stream)
{
    // 固定写法，acl初始化
    ACL_CALL(aclrtSetDevice(deviceId));
    ACL_CALL(aclrtCreateContext(context, deviceId));
    ACL_CALL(aclrtSetCurrentContext(*context));
    ACL_CALL(aclrtCreateStream(stream));
    ACL_CALL(aclInit(nullptr));
    return 0;
}

int DeInit(int32_t deviceId, aclrtContext* context, aclrtStream* stream)
{
    ACL_CALL(aclrtDestroyStream(*stream));
    ACL_CALL(aclrtDestroyContext(*context));
    ACL_CALL(aclrtResetDevice(deviceId));
    ACL_CALL(aclFinalize());
    return 0;
}

# endif // UTIL_ACL_H_