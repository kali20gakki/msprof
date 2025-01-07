/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sync_utils.h
 * Description        : 文件写作锁的工具类
 * Author             : msprof team
 * Creation Date      : 2024/7/16
 * *****************************************************************************
 */

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
