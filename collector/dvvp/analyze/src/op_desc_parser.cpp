/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: parse op desc
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "op_desc_parser.h"

#include "data_struct.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "prof_acl_mgr.h"
#include "toolchain/prof_acl_api.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
#define CHECK_DATA_RET0(data, len)                  \
    do {                                            \
        int checkDataRet = CheckData(data, len);  \
        if (checkDataRet != ACL_SUCCESS) {        \
            return 0;                               \
        }                                           \
    } while (0)

#define CHECK_DATA_RET(data, len)                   \
    do {                                            \
        int checkDataRet = CheckData(data, len);  \
        if (checkDataRet != ACL_SUCCESS) {        \
            return checkDataRet;                  \
        }                                           \
    } while (0)

#define CHECK_INDEX_RET0(index, len)                                    \
    do {                                                                \
        if (((index) + 1 == 0) ||                                       \
            (((index) + 1) * GetOpDescSize() > (len))) {                \
            MSPROF_LOGE("Index %u is out of range %u", (index), (len)); \
            return 0;                                                   \
        }                                                               \
    } while (0)

#define CHECK_INDEX_RET(index, len)                                     \
    do {                                                                \
        if (((index) + 1 == 0) ||                                       \
            (((index) + 1) * GetOpDescSize() > (len))) {                \
            MSPROF_LOGE("Index %u is out of range %u", (index), (len)); \
            return ACL_ERROR_INVALID_PARAM;                             \
        }                                                               \
    } while (0)

std::string g_aclprofSubscribeOpAttriValue;

using namespace Msprofiler::Api;

OpDescParser::OpDescParser() : opIndex_(0) {}

uint32_t OpDescParser::GetOpDescSize()
{
    return sizeof(ProfOpDesc);
}

int32_t OpDescParser::GetOpNum(CONST_VOID_PTR data, uint32_t len, UINT32_T_PTR opNum)
{
    if (data == nullptr || opNum == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpNum");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    *opNum = len / GetOpDescSize();
    return ACL_SUCCESS;
}

int32_t OpDescParser::GetModelId(CONST_VOID_PTR data, uint32_t len, uint32_t index, UINT32_T_PTR modelId)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetModelId");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    CHECK_INDEX_RET(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    *modelId = opDesc->modelId;
    return ACL_SUCCESS;
}

uint64_t OpDescParser::SetOpTypeAndOpName(const std::string &opType, const std::string &opName)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (opIndex_ + 1 != 0) {
        opIndex_++;
        opTypes_.insert(std::make_pair(opIndex_, opType));
        opNames_.insert(std::make_pair(opIndex_, opName));
        return opIndex_;
    }
    MSPROF_LOGE("Op name and type full, failed to set %s:%s", opType.c_str(), opName.c_str());
    return 0;
}

int32_t OpDescParser::GetOpTypeLen(CONST_VOID_PTR data, uint32_t len, SIZE_T_PTR opTypeLen, uint32_t index)
{
    if (data == nullptr || opTypeLen == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpTypeLen");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    CHECK_INDEX_RET(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = opTypes_.find(opDesc->opIndex);
    if (iter == opTypes_.end()) {
        MSPROF_LOGE("Failed to get op type len of %llu, maybe it has been got already", opDesc->opIndex);
        return ACL_ERROR_INVALID_PARAM;
    }
    *opTypeLen = iter->second.size() + 1;
    return ACL_SUCCESS;
}

int32_t OpDescParser::GetOpType(CONST_VOID_PTR data, uint32_t len, CHAR_PTR opType, uint32_t opTypeLen, uint32_t index)
{
    if (data == nullptr || opType == nullptr || opTypeLen == 0) {
        MSPROF_LOGE("Invalid param of GetOpType");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    CHECK_INDEX_RET(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = opTypes_.find(opDesc->opIndex);
    if (iter == opTypes_.end()) {
        MSPROF_LOGE("Failed to get op type of %llu, maybe it has been got already", opDesc->opIndex);
        return ACL_ERROR_INVALID_PARAM;
    }
    errno_t err = memcpy_s(opType, opTypeLen, iter->second.c_str(), iter->second.size());
    if (err != EOK) {
        return ACL_ERROR_INVALID_PARAM;
    }
    *(opType + opTypeLen - 1) = '\0';
    opTypes_.erase(iter);
    return ACL_SUCCESS;
}

int32_t OpDescParser::GetOpNameLen(CONST_VOID_PTR data, uint32_t len, SIZE_T_PTR opNameLen, uint32_t index)
{
    if (data == nullptr || opNameLen == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpNameLen");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    CHECK_INDEX_RET(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = opNames_.find(opDesc->opIndex);
    if (iter == opNames_.end()) {
        MSPROF_LOGE("Failed to get op name len of %llu, maybe it has been got already", opDesc->opIndex);
        return ACL_ERROR_INVALID_PARAM;
    }
    *opNameLen = iter->second.size() + 1;
    return ACL_SUCCESS;
}

int32_t OpDescParser::GetOpName(CONST_VOID_PTR data, uint32_t len, CHAR_PTR opName, uint32_t opNameLen, uint32_t index)
{
    if (data == nullptr || opName == nullptr || opNameLen == 0) {
        MSPROF_LOGE("Invalid param of GetOpName");
        return ACL_ERROR_INVALID_PARAM;
    }
    CHECK_DATA_RET(data, len);
    CHECK_INDEX_RET(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = opNames_.find(opDesc->opIndex);
    if (iter == opNames_.end()) {
        MSPROF_LOGE("Failed to get op name of %llu, maybe it has been got already", opDesc->opIndex);
        return ACL_ERROR_INVALID_PARAM;
    }
    errno_t err = memcpy_s(opName, opNameLen, iter->second.c_str(), iter->second.size());
    if (err != EOK) {
        MSPROF_LOGE("memcpy_s fail, err=%d opNameLen=%u opName=%s", err, opNameLen, iter->second.c_str());
        return ACL_ERROR_INVALID_PARAM;
    }
    *(opName + opNameLen - 1) = '\0';
    opNames_.erase(iter);
    return ACL_SUCCESS;
}

uint64_t OpDescParser::GetOpStart(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpStart");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->start;
}

uint64_t OpDescParser::GetOpEnd(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpEnd");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->end;
}

uint64_t OpDescParser::GetOpDuration(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpDuration");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->duration;
}

uint64_t OpDescParser::GetOpExecutionTime(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpExecutionTime");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->executionTime;
}

uint64_t OpDescParser::GetOpCubeFops(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpCubeFops");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->cubeFops;
}

uint64_t OpDescParser::GetOpVectorFops(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpVectorFops");
        return 0;
    }
    CHECK_DATA_RET0(data, len);
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->vectorFops;
}

int32_t OpDescParser::CheckData(CONST_VOID_PTR data, uint32_t len)
{
    if (len % GetOpDescSize() != 0) {
        MSPROF_LOGE("Length of data: %u is not [integer multiple] of OpDescSize: %u", len, GetOpDescSize());
        return ACL_ERROR_INVALID_PARAM;
    }
    auto addr = static_cast<const uint8_t *>(data);
    for (uint32_t i = 0; i < len / GetOpDescSize(); i++) {
        uint32_t signature = analysis::dvvp::common::utils::Utils::GenerateSignature(
            addr + i * GetOpDescSize() + sizeof(uint32_t), GetOpDescSize() - sizeof(uint32_t));
        auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + i * GetOpDescSize());
        if (opDesc == nullptr) {
            MSPROF_LOGE("Failed to call reinterpret_cast.");
            return ACL_ERROR_INVALID_RESOURCE_HANDLE;
        }
        if (opDesc->signature != signature) {
            MSPROF_LOGE("Part %u of data is invalid", i);
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}

uint32_t OpDescParser::GetOpFlag(CONST_VOID_PTR data, uint32_t len, uint32_t index)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpFlag, data is null");
        return ACL_SUBSCRIBE_NONE;
    }
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return ACL_ERROR_INVALID_RESOURCE_HANDLE;
    }
    return opDesc->flag;
}

const char *OpDescParser::GetOpAttriValue(CONST_VOID_PTR data, uint32_t len, uint32_t index,
    aclprofSubscribeOpAttri attri)
{
    if (data == nullptr) {
        MSPROF_LOGE("Invalid param of GetOpAttriValue, data is null");
        return nullptr;
    }
    CHECK_INDEX_RET0(index, len);
    auto addr = static_cast<CONST_CHAR_PTR>(data);
    auto opDesc = reinterpret_cast<const ProfOpDesc *>(addr + index * GetOpDescSize());
    if (opDesc == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return nullptr;
    }
    std::map<aclprofSubscribeOpAttri, aclprofSubscribeOpFlag> opFlagAttriMap = {
        {ACL_SUBSCRIBE_ATTRI_THREADID, ACL_SUBSCRIBE_OP_THREAD}
    };
    switch (attri) {
        case ACL_SUBSCRIBE_ATTRI_THREADID:
            if (opFlagAttriMap[attri] != opDesc->flag) {
                MSPROF_LOGE("Invalid param of GetOpAttriValue, curr op flag %u not support attri %u",
                            opDesc->flag, attri);
                return nullptr;
            }
            g_aclprofSubscribeOpAttriValue = std::to_string(opDesc->threadId);
            return g_aclprofSubscribeOpAttriValue.c_str();
        default:
            MSPROF_LOGE("Invalid param of GetOpAttriValue, attri %u", attri);
            return nullptr;
    }
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
