/**
 * @syslogd.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SYSLOGD_H
#define SYSLOGD_H
#include "log_sys_package.h"
#include "log_common_util.h"
#include "syslogd_common.h"

#ifdef __cplusplus
extern "C" {
#endif

int InitGlobals(void);
void InitModuleDefault(const struct Options *opt);
void ProcSyslogd(void);
#ifdef __cplusplus
}
#endif
#endif
