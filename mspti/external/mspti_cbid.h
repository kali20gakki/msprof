/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_cbid.h
 * Description        : Callback ID.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_CBID_H
#define MSPTI_CBID_H

typedef enum {
    MSPTI_CBID_RUNTIME_INVALID                              = 0,
    // Device
    MSPTI_CBID_RUNTIME_DEVICE_SET                           = 1,
    MSPTI_CBID_RUNTIME_DEVICE_RESET                         = 2,
    MSPTI_CBID_RUNTIME_DEVICE_SET_EX                        = 3,
    // Context
    MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX                   = 4,
    MSPTI_CBID_RUNTIME_CONTEXT_CREATED                      = 5,
    MSPTI_CBID_RUNTIME_CONTEXT_DESTROY                      = 6,
    // Stream
    MSPTI_CBID_RUNTIME_STREAM_CREATED                       = 7,
    MSPTI_CBID_RUNTIME_STREAM_DESTROY                       = 8,
    MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED                  = 9,
    MSPTI_CBID_RUNTIME_SIZE,
    MSPTI_CBID_RUNTIME_FORCE_INT                            = 0x7fffffff
} msptiCallbackIdRuntime;

#endif
