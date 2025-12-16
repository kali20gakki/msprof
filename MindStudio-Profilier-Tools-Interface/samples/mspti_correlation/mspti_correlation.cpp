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
#include <vector>
#include <thread>
#include <cstring>

// Acl headers
#include "acl/acl.h"
#include "aclnnop/aclnn_add.h"

// MSPTI headers
#include "common/helper_mspti.h"
#include "common/util_acl.h"
#include "mspti.h"

// MSTX headers
#include "mstx/ms_tools_ext.h"

static std::unordered_map<uint64_t, msptiActivity*> g_CorrelationMap;
static std::unordered_map<uint64_t, msptiActivityApi*> g_ApiConnectionMap;
namespace {
int64_t GetShapeSize(const std::vector<int64_t>& shape)
{
    int64_t shapeSize = 1;
    for (auto i : shape) {
        shapeSize *= i;
    }
    return shapeSize;
}

template <typename T>
int CreateAclTensor(const std::vector<T>& hostData, const std::vector<int64_t>& shape, void** deviceAddr,
                    aclDataType dataType, aclTensor** tensor)
{
    auto size = GetShapeSize(shape) * sizeof(T);
    ACL_CALL(aclrtMalloc(deviceAddr, size, ACL_MEM_MALLOC_HUGE_FIRST));
    ACL_CALL(aclrtMemcpy(*deviceAddr, size, hostData.data(), size, ACL_MEMCPY_HOST_TO_DEVICE));

    std::vector<int64_t> strides(shape.size(), 1);
    for (int64_t i = shape.size() - 2; i >= 0; i--) {
        strides[i] = shape[i + 1] * strides[i + 1];
    }

    // 调用aclCreateTensor接口创建aclTensor
    *tensor = aclCreateTensor(shape.data(), shape.size(), dataType, strides.data(), 0, aclFormat::ACL_FORMAT_ND,
                              shape.data(), shape.size(), *deviceAddr);
    return 0;
}

int DoAclAdd(aclrtContext context, aclrtStream stream)
{
    auto ret = ACL_SUCCESS;

    std::vector<int64_t> selfShape = {4, 2};
    std::vector<int64_t> otherShape = {4, 2};
    std::vector<int64_t> outShape = {4, 2};
    void* selfDeviceAddr = nullptr;
    void* otherDeviceAddr = nullptr;
    void* outDeviceAddr = nullptr;
    aclTensor* self = nullptr;
    aclTensor* other = nullptr;
    aclScalar* alpha = nullptr;
    aclTensor* out = nullptr;
    std::vector<float> selfHostData = {0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<float> otherHostData = {1, 1, 1, 2, 2, 2, 3, 3};
    std::vector<float> outHostData = {0, 0, 0, 0, 0, 0, 0, 0};
    float alphaValue = 1.2f;
    // 创建self aclTensor
    ACL_CALL(CreateAclTensor(selfHostData, selfShape, &selfDeviceAddr, aclDataType::ACL_FLOAT, &self));
    // 创建other aclTensor
    ACL_CALL(CreateAclTensor(otherHostData, otherShape, &otherDeviceAddr, aclDataType::ACL_FLOAT, &other));
    // 创建alpha aclScalar
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);
    CHECK_RET(alpha != nullptr, return ret);
    // 创建out aclTensor
    ACL_CALL(CreateAclTensor(outHostData, outShape, &outDeviceAddr, aclDataType::ACL_FLOAT, &out));

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    // 调用aclnnAdd第一段接口
    ACL_CALL(aclnnAddGetWorkspaceSize(self, other, alpha, out, &workspaceSize, &executor));
    // 根据第一段接口计算出的workspaceSize申请device内存
    void* workspaceAddr = nullptr;
    if (workspaceSize > 0) {
        ACL_CALL(aclrtMalloc(&workspaceAddr, workspaceSize, ACL_MEM_MALLOC_HUGE_FIRST));
    }
    ACL_CALL(aclnnAdd(workspaceAddr, workspaceSize, executor, stream));
    ACL_CALL(aclrtSynchronizeStream(stream));

    auto size = GetShapeSize(outShape);
    std::vector<float> resultData(size, 0);
    ACL_CALL(aclrtMemcpy(resultData.data(), resultData.size() * sizeof(resultData[0]), outDeviceAddr,
                         size * sizeof(float), ACL_MEMCPY_DEVICE_TO_HOST));
    for (int64_t i = 0; i < size; i++) {
        LOG_PRINT("result[%ld] is: %f\n", i, resultData[i]);
    }

    aclDestroyTensor(self);
    aclDestroyTensor(other);
    aclDestroyScalar(alpha);
    aclDestroyTensor(out);

    aclrtFree(selfDeviceAddr);
    aclrtFree(otherDeviceAddr);
    aclrtFree(outDeviceAddr);
    if (workspaceSize > 0) {
        aclrtFree(workspaceAddr);
    }
    return 0;
}

void PrintCorrelationTrace()
{
    LOG_PRINT("========== PrintCorrelation ============\n");
    for (const auto& apiIter: g_ApiConnectionMap) {
        uint64_t correlationId = apiIter.second->correlationId;
        auto it = g_CorrelationMap.find(correlationId);
        if (it == g_CorrelationMap.end()) {
            break;
        }
        LOG_PRINT("API and Activity correlation: correlation: %lu\n", apiIter.second->correlationId);
        PrintActivity(reinterpret_cast<msptiActivity*>(apiIter.second));
        PrintActivity(it->second);
    }
}

void HandleKernelRecord(msptiActivityKernel* record)
{
    auto* copy = reinterpret_cast<msptiActivity*>(malloc(sizeof(msptiActivityKernel)));
    if (copy) {
        if (memset_s(copy, sizeof(msptiActivityKernel), 0, sizeof(msptiActivityKernel)) != EOK) {
            LOG_PRINT("ERROR KernelRecord: memset failed!\n");
            return;
        }
        if (memcpy_s(copy, sizeof(msptiActivityKernel), record, sizeof(msptiActivityKernel)) != EOK) {
            LOG_PRINT("ERROR KernelRecord: memcpy failed!\n");
            return;
        }
        g_CorrelationMap[record->correlationId] = copy;
    }
}

void HandleApiRecord(msptiActivityApi* record)
{
    auto* copy = reinterpret_cast<msptiActivityApi*>(malloc(sizeof(msptiActivityApi)));
    if (copy) {
        if (memset_s(copy, sizeof(msptiActivityApi), 0, sizeof(msptiActivityApi)) != EOK) {
            LOG_PRINT("ERROR ApiRecord: memset failed!\n");
            return;
        }
        if (memcpy_s(copy, sizeof(msptiActivityApi), record, sizeof(msptiActivityApi)) != EOK) {
            LOG_PRINT("ERROR ApiRecord: memcpy failed!\n");
            return;
        }
        g_ApiConnectionMap[record->correlationId] = reinterpret_cast<msptiActivityApi*>(copy);
    }
}


// MSPTI
void MsptiTrace(uint8_t *buffer, size_t size, size_t validSize)
{
    if (validSize <= 0) {
        LOG_PRINT("validSize is invalid");
        return;
    }
    msptiActivity *pRecord = nullptr;
    msptiResult status = MSPTI_SUCCESS;
    do {
        status = msptiActivityGetNextRecord(buffer, validSize, &pRecord);
        if (status == MSPTI_SUCCESS) {
            msptiActivityKind kind = pRecord->kind;
            switch (kind) {
                case MSPTI_ACTIVITY_KIND_KERNEL:
                    HandleKernelRecord(reinterpret_cast<msptiActivityKernel*>(pRecord));
                    break;
                case MSPTI_ACTIVITY_KIND_API:
                    HandleApiRecord(reinterpret_cast<msptiActivityApi*>(pRecord));
                    break;
                default:
                    break;
            }
        } else if (status == MSPTI_ERROR_MAX_LIMIT_REACHED) {
            break;
        } else {
            LOG_PRINT("Consume data fail, error is %s", GetResultCodeString(status));
            break;
        }
    } while (1);
    free(buffer);
}

void SetUpMspti(aclrtContext* context, aclrtStream* stream)
{
    // 订阅mspti
    msptiSubscribe(&subscriber, nullptr, nullptr);
    msptiActivityRegisterCallbacks(UserBufferRequest, MsptiTrace);

    // 开启
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_API);
}
}

int main()
{
    int32_t deviceId = 0;
    aclrtContext context;
    aclrtStream stream;
    ACL_CALL(Init(deviceId, &context, &stream));
    SetUpMspti(&context, &stream);
    DoAclAdd(context, stream);
    DeInit(deviceId, &context, &stream);

    DeInitMspti();

    PrintCorrelationTrace();
    return 0;
}
