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

#ifndef ANALYSIS_INFRASTRUCTURE_SYNC_UTILS_H
#define ANALYSIS_INFRASTRUCTURE_SYNC_UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <mutex>

namespace Analysis {
namespace Infra {

enum class FileCategory {
    MSPROF,
    MSPROF_TX,
    STEP,
    DEFAULT = 3
};

const std::string& GetTimeStampStr();

struct BaseMutex {
    mutable std::mutex fileMutex_;
};

const static std::unordered_map<int, BaseMutex> mutexMap = []() -> std::unordered_map<int, BaseMutex> {
    std::unordered_map<int, BaseMutex> dict;
    for (int i = 0; i < static_cast<int>(FileCategory::DEFAULT); ++i) {
        dict[i];
    }
    return dict;
}();
}
}
#endif // ANALYSIS_INFRASTRUCTURE_SYNC_UTILS_H
