/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_base.h
 * Description        : Common definition of Mspti.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#ifndef MSPTI_BASE_H
#define MSPTI_BASE_H

#include <cstddef>
#include <cstdint>

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSPTI_API __declspec(dllexport)
#else
#define MSPTI_API __attribute__((visibility("default")))
#endif

typedef enum {
    MSPTI_SUCCESS                                       = 0,
    MSPTI_ERROR_INVALID_PARAMETER                       = 1,
    MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED      = 2,
    MSPTI_ERROR_MAX_LIMIT_REACHED                       = 3,
    MSPTI_ERROR_DEVICE_OFFLINE                          = 4,
    MSPTI_ERROR_MEMORY_MALLOC                           = 5,
    MSPTI_ERROR_DEVICE_PROF_TASK                        = 6,
    MSPTI_ERROR_INNER                                   = 7,
    MSPTI_ERROR_CHANNEL_READER_ERROR                    = 8,
} msptiResult;


#endif
