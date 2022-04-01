/**
 * @log_dump.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_DUMP_H
#define LOG_DUMP_H
#include <stdbool.h>
#include "dlog_error_code.h"
#include "log_sys_package.h"
#ifdef __cplusplus
extern "C" {
#endif

void CreateCoreDumpThread(void);
#ifdef __cplusplus
}
#endif
#endif /* LOG_DUMP_H */