/**
* @file mspti_cbid.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPTI_CBID_H
#define MSPTI_CBID_H

/**
 * @brief Definitions of indices for Runtime API functions, unique across entire API
 */
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
