/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_STREAMIO_COMMON_COMMON_H
#define ANALYSIS_DVVP_STREAMIO_COMMON_COMMON_H

#include <stdio.h>

namespace analysis {
namespace dvvp {
namespace streamio {
namespace common {
enum IO_MODE {
    STREAM_MODE = 0,
    FILE_MODE = 1,
};
}
}  // namespace streamio
}  // namespace dvvp
}  // namespace analysis

#endif
