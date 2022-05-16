/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: prof subscribe common function
 * Author: wangheng
 * Create: 2021-04-23
 */
#include "prof_api_common.h"
#include "ai_drv_dev_api.h"
#include "prof_acl_mgr.h"
#include "acl/acl_prof.h"
#include "op_desc_parser.h"
#include "msprof_callback_impl.h"
#include "errno/error_code.h"
#include "profapi_plugin.h"

using namespace Analysis::Dvvp::Analyze;
using namespace analysis::dvvp::common::error;

aclprofSubscribeConfig *aclprofCreateSubscribeConfig(
    int8_t opTimeSwitch, aclprofAicoreMetrics aicoreMetrics, VOID_PTR fd)
{
    if (fd == nullptr) {
        MSPROF_LOGE("fd is nullptr");
        return nullptr;
    }

    static const int8_t profTimeInfoSwitchOff = 0;
    static const int8_t profTimeInfoSwitchOn = 1;
    if (opTimeSwitch < profTimeInfoSwitchOff || opTimeSwitch > profTimeInfoSwitchOn) {
        MSPROF_LOGE("opTimeSwitch %d is not in range [%d, %d]", opTimeSwitch,
                    profTimeInfoSwitchOff, profTimeInfoSwitchOn);
        return nullptr;
    }
    aclprofSubscribeConfig *subscribeConfig = new (std::nothrow)aclprofSubscribeConfig;
    if (subscribeConfig == nullptr) {
        MSPROF_LOGE("new aclprofSubscribeConfig failed");
        MSPROF_ENV_ERROR("EK0201", std::vector<std::string>({"buf_size"}),
            std::vector<std::string>({std::to_string(sizeof(aclprofSubscribeConfig))}));
        return nullptr;
    }
    subscribeConfig->config.timeInfo = opTimeSwitch;
    subscribeConfig->config.aicoreMetrics = static_cast<ProfAicoreMetrics>(aicoreMetrics);
    subscribeConfig->config.fd = fd;
    return subscribeConfig;
}

aclError aclprofDestroySubscribeConfig(const aclprofSubscribeConfig *profSubscribeConfig)
{
    if (profSubscribeConfig == nullptr) {
        MSPROF_LOGE("profSubscribeConfig is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }
    delete profSubscribeConfig;
    return ACL_SUCCESS;
}

aclError aclprofGetOpDescSize(SIZE_T_PTR opDescSize)
{
    MSPROF_LOGI("start to execute aclprofGetOpDescSize");
    if (opDescSize == nullptr) {
        MSPROF_LOGE("Invalid param of ProfGetOpDescSize");
        return ACL_ERROR_INVALID_PARAM;
    }
    *opDescSize = OpDescParser::GetOpDescSize();
    return ACL_SUCCESS;
}

aclError aclprofGetOpNum(CONST_VOID_PTR opInfo, size_t opInfoLen, UINT32_T_PTR opNumber)
{
    int32_t ret = OpDescParser::GetOpNum(opInfo, opInfoLen, opNumber);
    RETURN_IF_NOT_SUCCESS(ret);
    return ACL_SUCCESS;
}

aclError aclprofGetOpTypeLen(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index, SIZE_T_PTR opTypeLen)
{
    int32_t ret = OpDescParser::instance()->GetOpTypeLen(opInfo, opInfoLen, opTypeLen, index);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Get op type len failed, prof result = %d", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

aclError aclprofGetOpType(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index, CHAR_PTR opType, size_t opTypeLen)
{
    int32_t ret = OpDescParser::instance()->GetOpType(opInfo, opInfoLen, opType, opTypeLen, index);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Get op type failed, prof result = %d", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

aclError aclprofGetOpNameLen(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index, SIZE_T_PTR opNameLen)
{
    int32_t ret = OpDescParser::instance()->GetOpNameLen(opInfo, opInfoLen, opNameLen, index);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Get op name len failed, prof result = %d", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

aclError aclprofGetOpName(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index, CHAR_PTR opName, size_t opNameLen)
{
    int32_t ret = OpDescParser::instance()->GetOpName(opInfo, opInfoLen, opName, opNameLen, index);
    if (ret != ACL_SUCCESS) {
        MSPROF_LOGE("Get op name failed, prof result = %d", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

uint64_t aclprofGetOpStart(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    uint64_t result = OpDescParser::GetOpStart(opInfo, opInfoLen, index);
    return result;
}

uint64_t aclprofGetOpEnd(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    uint64_t result = OpDescParser::GetOpEnd(opInfo, opInfoLen, index);
    return result;
}

uint64_t aclprofGetOpDuration(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    uint64_t result = OpDescParser::GetOpDuration(opInfo, opInfoLen, index);
    return result;
}

aclprofSubscribeOpFlag aclprofGetOpFlag(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index)
{
    uint32_t result = OpDescParser::GetOpFlag(opInfo, opInfoLen, index);
    return static_cast<aclprofSubscribeOpFlag>(result);
}

const char *aclprofGetOpAttriValue(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index,
    aclprofSubscribeOpAttri attri)
{
    const char *result = OpDescParser::GetOpAttriValue(opInfo, opInfoLen, index, attri);
    return result;
}
