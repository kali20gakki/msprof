/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: handle perf data
 * Author:
 */

#ifndef PROF_API_H
#define PROF_API_H

#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSVP_PROF_API __declspec(dllexport)
#else
#define MSVP_PROF_API __attribute__((visibility("default")))
#endif

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

MSVP_PROF_API int32_t ProfRegReporterCallback(ProfReportHandle reporter);
MSVP_PROF_API int32_t ProfRegCtrlCallback(ProfCtrlHandle hanle);
MSVP_PROF_API int32_t ProfRegDeviceStateCallback(ProfSetDeviceHandle hanle);
MSVP_PROF_API int32_t ProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId);
MSVP_PROF_API int32_t ProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len);
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

// acl define
using aclError = int;
using aclrtStream = void*;
using aclrtEvent = void*;
using aclrtContext = void*;
static const int ACL_ERROR_NONE = 0;
static const int ACL_SUCCESS = 0;

static const int ACL_ERROR_INVALID_PARAM = 100000;
static const int ACL_ERROR_UNINITIALIZE = 100001;
static const int ACL_ERROR_REPEAT_INITIALIZE = 100002;
static const int ACL_ERROR_INVALID_FILE = 100003;
static const int ACL_ERROR_WRITE_FILE = 100004;
static const int ACL_ERROR_INVALID_FILE_SIZE = 100005;
static const int ACL_ERROR_PARSE_FILE = 100006;
static const int ACL_ERROR_FILE_MISSING_ATTR = 100007;
static const int ACL_ERROR_FILE_ATTR_INVALID = 100008;
static const int ACL_ERROR_INVALID_DUMP_CONFIG = 100009;
static const int ACL_ERROR_INVALID_PROFILING_CONFIG = 100010;
static const int ACL_ERROR_INVALID_MODEL_ID = 100011;
static const int ACL_ERROR_DESERIALIZE_MODEL = 100012;
static const int ACL_ERROR_PARSE_MODEL = 100013;
static const int ACL_ERROR_READ_MODEL_FAILURE = 100014;
static const int ACL_ERROR_MODEL_SIZE_INVALID = 100015;
static const int ACL_ERROR_MODEL_MISSING_ATTR = 100016;
static const int ACL_ERROR_MODEL_INPUT_NOT_MATCH = 100017;
static const int ACL_ERROR_MODEL_OUTPUT_NOT_MATCH = 100018;
static const int ACL_ERROR_MODEL_NOT_DYNAMIC = 100019;
static const int ACL_ERROR_OP_TYPE_NOT_MATCH = 100020;
static const int ACL_ERROR_OP_INPUT_NOT_MATCH = 100021;
static const int ACL_ERROR_OP_OUTPUT_NOT_MATCH = 100022;
static const int ACL_ERROR_OP_ATTR_NOT_MATCH = 100023;
static const int ACL_ERROR_OP_NOT_FOUND = 100024;
static const int ACL_ERROR_OP_LOAD_FAILED = 100025;
static const int ACL_ERROR_UNSUPPORTED_DATA_TYPE = 100026;
static const int ACL_ERROR_FORMAT_NOT_MATCH = 100027;
static const int ACL_ERROR_BIN_SELECTOR_NOT_REGISTERED = 100028;
static const int ACL_ERROR_KERNEL_NOT_FOUND = 100029;
static const int ACL_ERROR_BIN_SELECTOR_ALREADY_REGISTERED = 100030;
static const int ACL_ERROR_KERNEL_ALREADY_REGISTERED = 100031;
static const int ACL_ERROR_INVALID_QUEUE_ID = 100032;
static const int ACL_ERROR_REPEAT_SUBSCRIBE = 100033;
static const int ACL_ERROR_STREAM_NOT_SUBSCRIBE = 100034;
static const int ACL_ERROR_THREAD_NOT_SUBSCRIBE = 100035;
static const int ACL_ERROR_WAIT_CALLBACK_TIMEOUT = 100036;
static const int ACL_ERROR_REPEAT_FINALIZE = 100037;
static const int ACL_ERROR_NOT_STATIC_AIPP = 100038;
static const int ACL_ERROR_COMPILING_STUB_MODE = 100039;
static const int ACL_ERROR_GROUP_NOT_SET = 100040;
static const int ACL_ERROR_GROUP_NOT_CREATE = 100041;
static const int ACL_ERROR_PROF_ALREADY_RUN = 100042;
static const int ACL_ERROR_PROF_NOT_RUN = 100043;
static const int ACL_ERROR_DUMP_ALREADY_RUN = 100044;
static const int ACL_ERROR_DUMP_NOT_RUN = 100045;
static const int ACL_ERROR_PROF_REPEAT_SUBSCRIBE = 148046;
static const int ACL_ERROR_PROF_API_CONFLICT = 148047;
static const int ACL_ERROR_INVALID_MAX_OPQUEUE_NUM_CONFIG = 148048;
static const int ACL_ERROR_INVALID_OPP_PATH = 148049;
static const int ACL_ERROR_OP_UNSUPPORTED_DYNAMIC = 148050;
static const int ACL_ERROR_RELATIVE_RESOURCE_NOT_CLEARED = 148051;

static const int ACL_ERROR_BAD_ALLOC = 200000;
static const int ACL_ERROR_API_NOT_SUPPORT = 200001;
static const int ACL_ERROR_INVALID_DEVICE = 200002;
static const int ACL_ERROR_MEMORY_ADDRESS_UNALIGNED = 200003;
static const int ACL_ERROR_RESOURCE_NOT_MATCH = 200004;
static const int ACL_ERROR_INVALID_RESOURCE_HANDLE = 200005;
static const int ACL_ERROR_FEATURE_UNSUPPORTED = 200006;
static const int ACL_ERROR_PROF_MODULES_UNSUPPORTED = 200007;

static const int ACL_ERROR_STORAGE_OVER_LIMIT = 300000;

static const int ACL_ERROR_INTERNAL_ERROR = 500000;
static const int ACL_ERROR_FAILURE = 500001;
static const int ACL_ERROR_GE_FAILURE = 500002;
static const int ACL_ERROR_RT_FAILURE = 500003;
static const int ACL_ERROR_DRV_FAILURE = 500004;
static const int ACL_ERROR_PROFILING_FAILURE = 500005;

#define ACL_TENSOR_SHAPE_RANGE_NUM 2
#define ACL_TENSOR_VALUE_RANGE_NUM 2
#define ACL_UNKNOWN_RANK 0xFFFFFFFFFFFFFFFE

typedef enum {
    ACL_DT_UNDEFINED = -1,
    ACL_FLOAT = 0,
    ACL_FLOAT16 = 1,
    ACL_INT8 = 2,
    ACL_INT32 = 3,
    ACL_UINT8 = 4,
    ACL_INT16 = 6,
    ACL_UINT16 = 7,
    ACL_UINT32 = 8,
    ACL_INT64 = 9,
    ACL_UINT64 = 10,
    ACL_DOUBLE = 11,
    ACL_BOOL = 12,
    ACL_STRING = 13,
    ACL_COMPLEX64 = 16,
    ACL_COMPLEX128 = 17,
    ACL_BF16 = 27
} aclDataType;

typedef enum {
    ACL_FORMAT_UNDEFINED = -1,
    ACL_FORMAT_NCHW = 0,
    ACL_FORMAT_NHWC = 1,
    ACL_FORMAT_ND = 2,
    ACL_FORMAT_NC1HWC0 = 3,
    ACL_FORMAT_FRACTAL_Z = 4,
    ACL_FORMAT_NC1HWC0_C04 = 12,
    ACL_FORMAT_HWCN = 16,
    ACL_FORMAT_NDHWC = 27,
    ACL_FORMAT_FRACTAL_NZ = 29,
    ACL_FORMAT_NCDHW = 30,
    ACL_FORMAT_NDC1HWC0 = 32,
    ACL_FRACTAL_Z_3D = 33
} aclFormat;

typedef enum {
    ACL_DEBUG = 0,
    ACL_INFO = 1,
    ACL_WARNING = 2,
    ACL_ERROR = 3,
} aclLogLevel;

typedef enum {
    ACL_MEMTYPE_DEVICE = 0,
    ACL_MEMTYPE_HOST = 1,
    ACL_MEMTYPE_HOST_COMPILE_INDEPENDENT = 2
} aclMemType;

// ge define
namespace ge {
using Status = uint32_t;
#define GE_ERRORNO(runtime, type, level, sysid, modid, name, value, desc)                                \
    constexpr Status name = (static_cast<uint32_t>(0xFFU & (static_cast<uint32_t>(runtime))) << 30U) |   \
                              (static_cast<uint32_t>(0xFFU & (static_cast<uint32_t>(type))) << 28U) |    \
                              (static_cast<uint32_t>(0xFFU & (static_cast<uint32_t>(level))) << 25U) |   \
                              (static_cast<uint32_t>(0xFFU & (static_cast<uint32_t>(sysid))) << 17U) |   \
                              (static_cast<uint32_t>(0xFFU & (static_cast<uint32_t>(modid))) << 12U) |   \
                              (static_cast<uint32_t>(0x0FFFU) & (static_cast<uint32_t>(value)))

    GE_ERRORNO(0, 0, 0, 0, 0, SUCCESS, 0, "success");
    GE_ERRORNO(0b11, 0b11, 0b111, 0xFFU, 0b11111, FAILED, 0xFFFU, "failed");
} // namespace ge

#endif
