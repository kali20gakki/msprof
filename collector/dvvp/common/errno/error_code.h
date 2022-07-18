/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_ERRNO_ERROR_CODE_H
#define ANALYSIS_DVVP_COMMON_ERRNO_ERROR_CODE_H

namespace analysis {
namespace dvvp {
namespace common {
namespace error {
const int PROFILING_SUCCESS = 0;
const int PROFILING_FAILED = -1;
const int PROFILING_NOTSUPPORT = -2;
const int PROFILING_ERROR = 1;
const int PROFILING_INVALID_PARAM = -3;
}  // namespace error
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
