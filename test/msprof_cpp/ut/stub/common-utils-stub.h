/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef _ANALYSIS_DVVP_UT_COMMON_UTILS_STUB_H
#define _ANALYSIS_DVVP_UT_COMMON_UTILS_STUB_H
#include <string.h>
#include <stdio.h>
#include <string>
#include "mmpa_api.h"

extern "C" int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...);
std::string GetAdxWorkPath();
#endif

