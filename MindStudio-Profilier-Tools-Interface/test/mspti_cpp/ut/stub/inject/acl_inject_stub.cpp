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
#include "csrc/common/inject/acl_inject.h"

AclError aclrtProfTrace(void *userdata, int32_t length, AclrtStream stream)
{
    return 0;
}

AclError aclrtGetDevice(int32_t* devId)
{
    if (devId != nullptr) {
        *devId = 0;
    }
    return 0;
}

AclError aclrtStreamGetId(AclrtStream stm, int32_t *streamId)
{
    if (streamId != nullptr) {
        *streamId = 0;
    }
    return 0;
}
