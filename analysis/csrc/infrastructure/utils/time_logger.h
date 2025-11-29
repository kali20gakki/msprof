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
#ifndef ANALYSIS_UTILS_TIME_LOGGER_H
#define ANALYSIS_UTILS_TIME_LOGGER_H

#include <string>
#include <chrono>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis {
namespace Utils {

class TimeLogger {
public:
    TimeLogger(const std::string tag)
        : tag_(tag), startTime_(std::chrono::high_resolution_clock::now())
    {
        INFO("% start", tag_);
    }

    ~TimeLogger()
    {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime_).count();
        INFO("% end, cost time = % ms", tag_, dur);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime_;
    std::string tag_;
};
}
}
#endif // ANALYSIS_UTILS_TIME_LOGGER_H
