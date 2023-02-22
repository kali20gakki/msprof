/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: handle perf data
 * Author: xp
 * Create: 2019-10-13
 */

#ifndef MSPROFILER_API_PROF_ACL_API_H_
#define MSPROFILER_API_PROF_ACL_API_H_

// Task:0-31
// System:32-47
// Other:48-63
// DataTypeConfig
#define PROF_ACL_API                0x00000001ULL
#define PROF_TASK_TIME              0x00000002ULL
#define PROF_AICORE_METRICS         0x00000004ULL
#define PROF_AICPU_TRACE            0x00000008ULL
#define PROF_L2CACHE                0x00000010ULL
#define PROF_HCCL_TRACE             0x00000020ULL
#define PROF_KEYPOINT_TRACE         0x00000040ULL
#define PROF_MSPROFTX               0x00000080ULL
#define PROF_RUNTIME_API            0x00000100ULL
#define PROF_GE_DYNAMIC_OP_EXECUTE  0x00000200ULL
#define PROF_AIV_METRICS            0x0000020000000ULL
#define PROF_MODEL_EXECUTE          0x0000001000000ULL
#define PROF_RUNTIME_TRACE          0x0000004000000ULL
#define PROF_SCHEDULE_TIMELINE      0x0000008000000ULL
#define PROF_SCHEDULE_TRACE         0x0000010000000ULL
#define PROF_SUBTASK_TIME           0x0000040000000ULL
#define PROF_OP_DETAIL              0x0000080000000ULL

#define PROF_TASK_TRACE             (PROF_MODEL_EXECUTE | PROF_RUNTIME_TRACE | PROF_KEYPOINT_TRACE | \
                                     PROF_HCCL_TRACE | PROF_TASK_TIME)

// System
#define PROF_BIU                     0x000000800000ULL
#define PROF_SYS_USAGE               0x000100000000ULL
#define PROF_SYS_PID_USAGE           0x000200000000ULL
#define PROF_SYS_CPU                 0x000400000000ULL
#define PROF_SYS_HARDWARE_MEM        0x000800000000ULL
#define PROF_SYS_IO                  0x001000000000ULL
#define PROF_SYS_INTERCONNECTION     0x002000000000ULL
#define PROF_DVPP                    0x004000000000ULL
#define PROF_SYS_AICORE_SAMPLE       0x008000000000ULL
#define PROF_AIVECTORCORE_SAMPLE     0x010000000000ULL

// Other
#define PROF_KEYPOINT_TRACE_HELPER   0x4000000000000000ULL
#define PROF_MODEL_LOAD              0x8000000000000000ULL

// ========================================== mask ====================================
// Task mask
#define PROF_ACL_API_MASK                0x00000001ULL
#define PROF_TASK_TIME_MASK              0x00000002ULL
#define PROF_AICORE_METRICS_MASK         0x00000004ULL
#define PROF_AICPU_TRACE_MASK            0x00000008ULL
#define PROF_L2CACHE_MASK                0x00000010ULL
#define PROF_HCCL_TRACE_MASK             0x00000020ULL
#define PROF_KEYPOINT_TRACE_MASK         0x00000040ULL
#define PROF_MSPROFTX_MASK               0x00000080ULL
#define PROF_RUNTIME_API_MASK            0x00000100ULL
#define PROF_GE_DYNAMIC_OP_EXECUTE_MASK  0x00000200ULL
#define PROF_AIV_METRICS_MASK            0x0000020000000ULL
#define PROF_MODEL_EXECUTE_MASK          0x0000001000000ULL
#define PROF_RUNTIME_TRACE_MASK          0x0000004000000ULL
#define PROF_SCHEDULE_TIMELINE_MASK      0x0000008000000ULL
#define PROF_SCHEDULE_TRACE_MASK         0x0000010000000ULL
#define PROF_SUBTASK_TIME_MASK           0x0000040000000ULL
#define PROF_OP_DETAIL_MASK              0x0000080000000ULL

// System mask
#define PROF_BIU_MASK                         0x000000800000ULL
#define PROF_SYS_USAGE_MASK                   0x000100000000ULL
#define PROF_SYS_PID_USAGE_MASK               0x000200000000ULL
#define PROF_SYS_CPU_MASK                     0x000400000000ULL
#define PROF_SYS_HARDWARE_MEM_MASK            0x000800000000ULL
#define PROF_SYS_IO_MASK                      0x001000000000ULL
#define PROF_SYS_INTERCONNECTION_MASK         0x002000000000ULL
#define PROF_DVPP_MASK                        0x004000000000ULL
#define PROF_SYS_AICORE_SAMPLE_MASK           0x008000000000ULL
#define PROF_AIVECTORCORE_SAMPLE_MASK         0x010000000000ULL

// Other mask
#define PROF_KEYPOINT_TRACE_HELPER_MASK       0x4000000000000000ULL
#define PROF_MODEL_LOAD_MASK                  0x8000000000000000ULL

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSVP_PROF_API __declspec(dllexport)
#else
#define MSVP_PROF_API __attribute__((visibility("default")))
#endif

#include <cstdint>
#include <cstddef>

namespace Msprofiler {
namespace Api {
/**
 * @name  ProfGetOpExecutionTime
 * @brief get op execution time of specific part of data
 * @param data  [IN] data read from pipe
 * @param len   [IN] data length
 * @param index [IN] index of part(op)
 * @return op execution time (us)
 */
MSVP_PROF_API uint64_t ProfGetOpExecutionTime(const void *data, uint32_t len, uint32_t index);
}
}

#ifdef __cplusplus
extern "C" {
#endif

MSVP_PROF_API uint64_t ProfGetOpExecutionTime(const void *data, uint32_t len, uint32_t index);

typedef struct aclprofSubscribeConfig aclprofSubscribeConfig1;
/// @ingroup AscendCL
/// @brief subscribe profiling data of graph
/// @param [in] graphId: the graph id subscribed
/// @param [in] profSubscribeConfig: pointer to config of model subscribe
/// @return int result of function
MSVP_PROF_API int aclgrphProfGraphSubscribe(const uint32_t graphId,
    const aclprofSubscribeConfig1 *profSubscribeConfig);

/// @ingroup AscendCL
/// @brief unsubscribe profiling data of graph
/// @param [in] graphId: the graph id subscribed
/// @return int result of function
MSVP_PROF_API int aclgrphProfGraphUnSubscribe(const uint32_t graphId);

/**
 * @ingroup AscendCL
 * @brief get graph id from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 *
 * @retval graph id of subscription data
 * @retval 0 for failed
 */
MSVP_PROF_API size_t aclprofGetGraphId(const void *opInfo, size_t opInfoLen, uint32_t index);

/**
* @ingroup AscendCL
* @brief set stamp pay load
*
*
* @retval void
*/
MSVP_PROF_API int aclprofSetStampPayload(void *stamp, const int32_t type, void *value);

/**
* @ingroup AscendCL
* @brief set stamp tag name
*
*
* @retval void
*/
MSVP_PROF_API int aclprofSetStampTagName(void *stamp, const char *tagName, uint16_t len);

/**
* @ingroup AscendCL
* @brief set category and name
*
*
* @retval void
*/
MSVP_PROF_API int aclprofSetCategoryName(uint32_t category, const char *categoryName);

/**
* @ingroup AscendCL
* @brief set category to stamp
*
*
* @retval void
*/
MSVP_PROF_API int aclprofSetStampCategory(void *stamp, uint32_t category);

/**
* @ingroup AscendCL
* @brief set stamp call trace
*
*
* @retval void
*/
MSVP_PROF_API int aclprofSetStampCallStack(void *stamp, const char *callStack, uint32_t len);

/**
* @ingroup AscendCL
* @brief report stamp data
*
*
* @retval void
*/
MSVP_PROF_API int aclprofReportStamp(const char *tag, uint32_t tagLen, unsigned char *data, uint32_t dataLen);

typedef enum {
    ACL_SUBSCRIBE_OP = 0,
    ACL_SUBSCRIBE_SUBGRAPH = 1,
    ACL_SUBSCRIBE_OP_THREAD = 2,
    ACL_SUBSCRIBE_NONE
} aclprofSubscribeOpFlag;
 
typedef enum {
    ACL_SUBSCRIBE_ATTRI_THREADID = 0,
    ACL_SUBSCRIBE_ATTRI_NONE
} aclprofSubscribeOpAttri;

/**
 * @ingroup AscendCL
 * @brief get op flag from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  index [IN]      index of op array in opInfo
 *
 * @retval op flag
 * @retval ACL_SUBSCRIBE_NONE for failed
 */
MSVP_PROF_API aclprofSubscribeOpFlag aclprofGetOpFlag(const void *opInfo, size_t opInfoLen, uint32_t index);
 
/**
 * @ingroup AscendCL
 * @brief get op flag from subscription data
 *
 * @param  opInfo [IN]     pointer to subscription data
 * @param  opInfoLen [IN]  memory size of subscription data
 * @param  index [IN]      index of op array in opInfo
 * @param  attri [IN]      attribute of op
 *
 * @retval op flag
 * @retval NULL for failed
 */
MSVP_PROF_API const char *aclprofGetOpAttriValue(const void *opInfo, size_t opInfoLen, uint32_t index,
    aclprofSubscribeOpAttri attri);
#ifdef __cplusplus
}
#endif

#endif  // MSPROFILER_API_PROF_ACL_API_H_
