/**
* @file callback_mstx.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* Sample demonstration on how to associate CANN APIs with activities on devices
* through the correlationId field in activity data
*/

// System headers
#include <vector>
#include <thread>
#include <cstring>
#include <unordered_map>

// Acl headers
#include "acl/acl.h"
#include "aclnnop/aclnn_add.h"

// MSPTI headers
#include "common/helper_mspti.h"
#include "common/util_acl.h"
#include "mspti.h"

// MSTX headers
#include "mstx/ms_tools_ext.h"

static std::unordered_map<uint64_t, std::vector<uint64_t>> s_externalCorrelationMap;

// Enum mapping the external id to the different phases in the vector addition it correlates to.
enum class ExternalId {
    INITIALIZATION_EXTERNAL_ID = 0,
    EXECUTION_EXTERNAL_ID = 1,
    CLEANUP_EXTERNAL_ID = 2
};

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
    uint64_t id = 0;

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

    msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0,
                                           static_cast<uint64_t>(ExternalId::INITIALIZATION_EXTERNAL_ID));
    // 创建self aclTensor
    ACL_CALL(CreateAclTensor(selfHostData, selfShape, &selfDeviceAddr, aclDataType::ACL_FLOAT, &self));
    // 创建other aclTensor
    ACL_CALL(CreateAclTensor(otherHostData, otherShape, &otherDeviceAddr, aclDataType::ACL_FLOAT, &other));
    // 创建alpha aclScalar
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);
    CHECK_RET(alpha != nullptr, return ret);
    // 创建out aclTensor
    ACL_CALL(CreateAclTensor(outHostData, outShape, &outDeviceAddr, aclDataType::ACL_FLOAT, &out));
    msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, &id);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;

    // 调用aclnnAdd第一段接口
    msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0,
                                           static_cast<uint64_t>(ExternalId::EXECUTION_EXTERNAL_ID));
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
    msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, &id);
    for (int64_t i = 0; i < size; i++) {
        LOG_PRINT("result[%ld] is: %f\n", i, resultData[i]);
    }

    msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0,
                                           static_cast<uint64_t>(ExternalId::CLEANUP_EXTERNAL_ID));
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
    msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, &id);
    return 0;
}

void PrintExternalCorrelationTrace()
{
    LOG_PRINT("========== PrintCorrelation ============\n");
    for (auto externalIdIt : s_externalCorrelationMap) {
        LOG_PRINT("External id : %lu: ", externalIdIt.first);
        ExternalId id = ExternalId(externalIdIt.first);
        switch (id) {
            case ExternalId::INITIALIZATION_EXTERNAL_ID:
                LOG_PRINT("INITIALIZATION_EXTERNAL_ID \n");
                break;
            case ExternalId::EXECUTION_EXTERNAL_ID:
                LOG_PRINT("EXECUTION_EXTERNAL_ID \n");
                break;
            case ExternalId::CLEANUP_EXTERNAL_ID:
                LOG_PRINT("CLEANUP_EXTERNAL_ID \n");
                break;
            default:
                break;
        }
        auto correlationIds = externalIdIt.second;
        for (auto correlationId : correlationIds) {
            LOG_PRINT("%lu, ", correlationId);
        }
        LOG_PRINT("\n");
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
            PrintActivity(pRecord);
            switch (kind) {
                case MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION: {
                    msptiActivityExternalCorrelation *pExternalCorrelationRecord =
                            reinterpret_cast<msptiActivityExternalCorrelation *>(pRecord);
                    uint64_t externalId = pExternalCorrelationRecord->externalId;
                    uint64_t correlationId = pExternalCorrelationRecord->correlationId;
                    s_externalCorrelationMap[externalId].push_back(correlationId);
                    break;
                }
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
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION);
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

    PrintExternalCorrelationTrace();
    return 0;
}
