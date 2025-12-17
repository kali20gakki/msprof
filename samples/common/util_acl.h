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

#ifndef UTIL_ACL_H_
#define UTIL_ACL_H_

#include "acl/acl.h"
#include "common/util_mstx.h"

int Init(int32_t deviceId, aclrtContext* context, aclrtStream* stream)
{
    // 初始化系统资源
    ACL_CALL(aclInit(nullptr));
    ACL_CALL(aclrtSetDevice(deviceId));
    ACL_CALL(aclrtCreateContext(context, deviceId));
    ACL_CALL(aclrtSetCurrentContext(*context));
    ACL_CALL(aclrtCreateStream(stream));
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