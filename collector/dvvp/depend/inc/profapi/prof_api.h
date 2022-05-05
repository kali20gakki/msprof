/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: handle perf data
 * Author:
 */

#ifndef PROF_API_H
#define PROF_API_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSVP_PROF_API __declspec(dllexport)
#else
#define MSVP_PROF_API __attribute__((visibility("default")))
#endif

#include "stddef.h"
#include "stdint.h"

/**
 * @name  profCtrlCallbackType
 * @brief ctrl callback request type
 */
enum ProfCtrlCallbackType {
    PROF_CTRL_INIT_ACL_ENV = 0,           // start profiling with acl env
    PROF_CTRL_INIT_ACL_JSON,              // start profiling with acl.json
    PROF_CTRL_INIT_GE_OPTIONS,            // start profiling with ge env and options
    PROF_CTRL_FINALIZE,                   // stop profiling
    PROF_CTRL_INIT_HELPER,                // start profiling in helper device
    PROF_CTRL_INIT_DYNA = 0xFF,           // start profiling for dynamic profiling
};

struct ProfSetDevPara {
    uint32_t chipId;
    uint32_t deviceId;
    bool isOpen;
};

#define PROFAPI_SUCCESS 0
#define PROFAPI_FAILED 0xFFFU

using ProfSetDevPara_t = struct ProfSetDevPara;

using PROFAPI_CONFIG_CONST_PTR = const void *; // const aclprofConfig *;
using PROFAPI_SUBSCRIBECONFIG_CONST_PTR = const void *; // const aclprofSubscribeConfig *;
using PROFAPI_PROF_COMMAND_PTR = void *;
using VOID_PTR = void *;
/**
 * @name  ProfCommandHandle
 * @brief callback to start/stop profiling
 * @param type      [IN] enum call back type
 * @param data      [IN] callback data
 * @param len       [IN] callback data size
 * @return enum MsprofErrorCode
 */
using ProfCommandHandle = int32_t (*)(uint32_t type, void *data, uint32_t len);
using ProfReportHandle = int32_t (*)(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len);
using ProfCtrlHandle = int32_t (*)(uint32_t type, VOID_PTR data, uint32_t len);
using ProfSetDeviceHandle = int32_t (*)(VOID_PTR data, uint32_t len);

MSVP_PROF_API int32_t profRegReporterCallback(ProfReportHandle reporter);
MSVP_PROF_API int32_t profRegCtrlCallback(ProfCtrlHandle hanle);
MSVP_PROF_API int32_t profRegDeviceStateCallback(ProfSetDeviceHandle hanle);
MSVP_PROF_API int32_t profGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId);
MSVP_PROF_API int32_t profSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len);
MSVP_PROF_API int32_t profSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream);

MSVP_PROF_API int32_t MsprofInit(uint32_t moduleId, void *data, uint32_t dataLen);
MSVP_PROF_API int32_t MsprofRegisterCallback(uint32_t moduleId, ProfCommandHandle handle);
MSVP_PROF_API int32_t MsprofReportData(uint32_t moduleId, uint32_t type, void* data, uint32_t len);
MSVP_PROF_API int32_t MsprofSetDeviceIdByGeModelIdx(const uint32_t geModelIdx, const uint32_t deviceId);
MSVP_PROF_API int32_t MsprofUnsetDeviceIdByGeModelIdx(const uint32_t geModelIdx, const uint32_t deviceId);
MSVP_PROF_API int32_t MsprofNotifySetDevice(uint32_t chipId, uint32_t deviceId, bool isOpen);
MSVP_PROF_API int32_t MsprofFinalize();

/* ACL API */
MSVP_PROF_API int32_t profAclInit(uint32_t type, const char *profilerPath, uint32_t len);
MSVP_PROF_API int32_t profAclStart(uint32_t type, PROFAPI_CONFIG_CONST_PTR profilerConfig);
MSVP_PROF_API int32_t profAclStop(uint32_t type, PROFAPI_CONFIG_CONST_PTR profilerConfig);
MSVP_PROF_API int32_t profAclFinalize(uint32_t type);
MSVP_PROF_API int32_t profAclSubscribe(uint32_t type, uint32_t modelId, PROFAPI_SUBSCRIBECONFIG_CONST_PTR config);
MSVP_PROF_API int32_t profAclUnSubscribe(uint32_t type, uint32_t modelId);
MSVP_PROF_API int32_t profAclDrvGetDevNum();
MSVP_PROF_API int64_t profAclGetOpTime(uint32_t type, const void *opInfo, size_t opInfoLen, uint32_t index);
MSVP_PROF_API int32_t profAclGetId(uint32_t type, const void *opInfo, size_t opInfoLen, uint32_t index);
MSVP_PROF_API int32_t profAclGetOpVal(uint32_t type, const void *opInfo, size_t opInfoLen,
                                      uint32_t index, void *data, size_t len);
MSVP_PROF_API uint64_t ProfGetOpExecutionTime(const void *data, uint32_t len, uint32_t index);
MSVP_PROF_API const char *profAclGetOpAttriVal(uint32_t type, const void *opInfo, size_t opInfoLen,
                                               uint32_t index, uint32_t attri);
/**
* @ingroup AscendCL
* @brief create pointer to aclprofstamp
*
*
* @retval aclprofStamp pointer
*/
MSVP_PROF_API void *profAclCreateStamp();

/**
* @ingroup AscendCL
* @brief destory stamp pointer
*
*
* @retval void
*/
MSVP_PROF_API void profAclDestroyStamp(void *stamp);

/**
* @ingroup AscendCL
* @brief Record push timestamp
*
* @retval ACL_SUCCESS The function is successfully executed.
* @retval OtherValues Failure
*/
MSVP_PROF_API int32_t profAclPush(void *stamp);

/**
* @ingroup AscendCL
* @brief Record pop timestamp
*
*
* @retval ACL_SUCCESS The function is successfully executed.
* @retval OtherValues Failure
*/
MSVP_PROF_API int32_t profAclPop();

/**
* @ingroup AscendCL
* @brief Record range start timestamp
*
* @retval ACL_SUCCESS The function is successfully executed.
* @retval OtherValues Failure
*/
MSVP_PROF_API int32_t profAclRangeStart(void *stamp, uint32_t *rangeId);

/**
* @ingroup AscendCL
* @brief Record range end timestamp
*
* @retval ACL_SUCCESS The function is successfully executed.
* @retval OtherValues Failure
*/
MSVP_PROF_API int32_t profAclRangeStop(uint32_t rangeId);

/**
* @ingroup AscendCL
* @brief set message to stamp
*
*
* @retval void
*/
MSVP_PROF_API int32_t profAclSetStampTraceMessage(void *stamp, const char *msg, uint32_t msgLen);

/**
* @ingroup AscendCL
* @brief Record mark timestamp
*
* @retval ACL_SUCCESS The function is successfully executed.
* @retval OtherValues Failure
*/
MSVP_PROF_API int32_t profAclMark(void *stamp);

MSVP_PROF_API int32_t profAclSetCategoryName(uint32_t category, const char *categoryName);

MSVP_PROF_API int32_t profAclSetStampCategory(VOID_PTR stamp, uint32_t category);

MSVP_PROF_API int32_t profAclSetStampPayload(VOID_PTR stamp, const int32_t type, VOID_PTR value);
#ifdef __cplusplus
}
#endif

#endif
