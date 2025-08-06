/**
* @file util_mstx.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef UTIL_MSTX_H_
#define UTIL_MSTX_H_

#include <cstdio>

#define CHECK_RET(cond, return_expr) \
    do {                             \
        if (!(cond)) {               \
            return_expr;             \
        }                            \
    } while (0)

#define LOG_PRINT(message, ...)         \
    do {                                \
        printf(message, ##__VA_ARGS__); \
    } while (0)

#define ACL_CALL(acl_func_call)                                                 \
    do {                                                                        \
        auto ret = acl_func_call;                                               \
        if (ret != ACL_SUCCESS) {                                               \
            LOG_PRINT("%s call failed, error code: %d\n", #acl_func_call, ret); \
            return ret;                                                         \
        }                                                                       \
    } while (0)

#endif // UTIL_MSTX_H_