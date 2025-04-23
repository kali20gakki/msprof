/**
* @file util_mspti.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef UTIL_MSPTI_H_
#define UTIL_MSPTI_H_

#include <cstdio>

#define CHECK_RET(cond, return_expr) \
    do {                               \
        if (!(cond)) {                   \
            return_expr;                   \
        }                                \
    } while (0)

#define LOG_PRINT(message, ...)     \
    do {                              \
        printf(message, ##__VA_ARGS__); \
    } while (0)

#define ACL_CALL(acl_func_call)   \
    do {                                       \
        auto ret = acl_func_call;              \
        if (ret != ACL_SUCCESS) { \
            LOG_PRINT("%s call failed, error code: %d\n", #acl_func_call, ret); \
            return ret; \
        }                         \
    } while (0)

#define HCCL_CHECK(func) \
    do { \
        auto ret = func; \
        if (ret != HCCL_SUCCESS) \
        { \
            LOG_PRINT("hccl interface return errreturn err %s:%d, retcode: %d \n", __FILE__, __LINE__, ret); \
            return ret; \
        } \
    } while (0)
    

#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                                 \
    (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))

#endif // UTIL_MSPTI_H_