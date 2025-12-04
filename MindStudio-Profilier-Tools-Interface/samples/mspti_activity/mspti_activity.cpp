/**
* @file mspti_activity.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* Sample app to demonstrate use of CUPTI library to collect kernel, memory, api data
* Sample demonstrates subscribing to the MSPTI interface, enabling the activity data interface,
* and consuming activity data streams
*/

// System headers
#include <vector>

// ACL Header
#include "acl/acl.h"
#include "acl/acl_prof.h"

// ACLNN Header
#include "aclnnop/aclnn_add.h"

// MSPTI header
#include "common/helper_mspti.h"
#include "common/util_acl.h"
#include "mspti.h"

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

void SetUpMspti()
{
    // 初始化订阅mspti
    InitMspti(nullptr, nullptr);

    // 开启mspti数据采集开关
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_API);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MEMCPY);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MEMORY);
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MEMSET);
}

int main()
{
    SetUpMspti();

    int32_t deviceId = 0;
    aclrtContext context;
    aclrtStream stream;
    Init(deviceId, &context, &stream);
    DoAclAdd(context, stream);
    DeInit(deviceId, &context, &stream);

    DeInitMspti();
    return 0;
}
