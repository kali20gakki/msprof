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
#ifndef ANALYSIS_DVVP_COMMON_ERRNO_ERROR_CODE_H
#define ANALYSIS_DVVP_COMMON_ERRNO_ERROR_CODE_H

namespace analysis {
namespace dvvp {
namespace common {
namespace error {
const int PROFILING_ERROR = 1;
const int PROFILING_SUCCESS = 0;
const int PROFILING_FAILED = -1;
const int PROFILING_NOTSUPPORT = -2;
const int PROFILING_INVALID_PARAM = -3;
const int PROFILING_IN_WARMUP = -4;
const int PROFILING_DOING_NOTHING = -5;
}  // namespace error
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
