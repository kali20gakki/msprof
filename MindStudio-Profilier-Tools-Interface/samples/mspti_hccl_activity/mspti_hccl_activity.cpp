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

// System headers
#include <cstring>
#include <vector>
#include <memory>
#include <thread>
#include <unistd.h>

// Acl header
#include "acl/acl.h"
#include "aclnnop/aclnn_add.h"

// Hccl header
#include "hccl/hccl.h"
#include "hccl/hccl_types.h"

// Mspti header
#include "mspti/mspti.h"
#include "common/helper_mspti.h"
#include "common/util_acl.h"

struct ThreadContext {
    HcclComm comm;
    int32_t device;
};

int HcclSample(void *arg)
{
    ThreadContext* ctx = (ThreadContext *)arg;
    ACL_CALL(aclrtSetDevice(ctx->device));
    aclrtStream stream;
    ACL_CALL(aclrtCreateStream(&stream));
    void* sendBuff;
    void* recvBuff;
    void* hostBuff;

    uint64_t count = 8;
    int mallocSize = count * sizeof(float);
    ACL_CALL(aclrtMallocHost(static_cast<void**>(&hostBuff), mallocSize));
    float* tmpHostBuff = static_cast<float*>(hostBuff);
    for (uint32_t i = 0; i < count; ++i) {
        tmpHostBuff[i] = i;
    }
    ACL_CALL(aclrtMalloc((void**)&sendBuff, mallocSize, ACL_MEM_MALLOC_HUGE_FIRST));
    ACL_CALL(aclrtMemcpy((void*)sendBuff, mallocSize, (void*)hostBuff, mallocSize, ACL_MEMCPY_HOST_TO_DEVICE));
    ACL_CALL(aclrtMalloc((void**)&recvBuff, mallocSize, ACL_MEM_MALLOC_HUGE_FIRST));

    HCCL_CHECK(HcclAllReduce((void *)sendBuff, (void*)recvBuff, count, HCCL_DATA_TYPE_FP32, HCCL_REDUCE_SUM, ctx->comm,
                             stream));
    ACL_CALL(aclrtSynchronizeStream(stream));

    // 8为单机device卡数
    if (ctx->device < 8) {
        void* resultBuff;
        ACL_CALL(aclrtMallocHost(static_cast<void**>(&resultBuff), mallocSize));
        ACL_CALL(aclrtMemcpy((void*)resultBuff, mallocSize, (void*)recvBuff, mallocSize, ACL_MEMCPY_DEVICE_TO_HOST));
        float* tmpResBuff = static_cast<float*>(resultBuff);
        for (uint32_t i = 0; i < count; ++i) {
            LOG_PRINT("rankId: %u, i: %f\n", ctx->device, tmpResBuff[i]);
        }
        ACL_CALL(aclrtFreeHost(resultBuff));
    }
    // 释放通信用sendBuf、recvBuf内存，stream等资源
    ACL_CALL(aclrtFree(recvBuff));
    ACL_CALL(aclrtFree(sendBuff));
    ACL_CALL(aclrtFreeHost(hostBuff));
    ACL_CALL(aclrtDestroyStream(stream));
    ACL_CALL(aclrtResetDevice(ctx->device));
    return 0;
}

void SetUpMspti()
{
    InitMspti(nullptr, nullptr);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_HCCL);
}

int main(int argc, char*argv[])
{
    SetUpMspti();

    ACL_CALL(aclInit(nullptr));
    uint32_t ndev = 8;
    int32_t devices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    HcclComm comms[ndev];
    for (uint32_t i = 0; i < ndev; i++) {
        ACL_CALL(aclrtSetDevice(devices[i]));
    }
    HCCL_CHECK(HcclCommInitAll(ndev, devices, comms));
    std::vector<std::unique_ptr<std::thread>> threads(ndev);
    struct ThreadContext args[ndev];
    for (uint32_t i = 0; i < ndev; i++) {
        args[i].device = i;
        args[i].comm = comms[i];
        threads[i].reset(new (std::nothrow) std::thread(&HcclSample, (void *)&args[i]));
        std::chrono::seconds duration(6); // 设置6秒睡眠，保证线程正常运行
        std::this_thread::sleep_for(duration);
    }
    for (uint32_t i = 0; i < ndev; i++) {
        if (threads[i] && threads[i]->joinable()) {
            threads[i]->join();
        }
    }
    for (uint32_t i = 0; i < ndev; i++) {
        HCCL_CHECK(HcclCommDestroy(comms[i]));
    }
    ACL_CALL(aclFinalize());

    DeInitMspti();
    return 0;
}

