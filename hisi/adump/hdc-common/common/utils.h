/**
 * @file common/utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_COMMON_UTILS_H
#define IDE_DAEMON_COMMON_UTILS_H

#include <string>
#include <vector>
#include "mmpa_api.h"
#include "hdc_api.h"
#include "ide_daemon_api.h"

#ifndef M_TRUNC
#define M_TRUNC O_TRUNC
#endif
namespace IdeDaemon {
namespace Common {
namespace Utils {
std::string ReplaceWaveToHomeDir(const std::string &filePath);
bool IsValidDirChar(const std::string &path);
std::vector<std::string> Split(const std::string &inputStr, bool filterOutEnabled = false,
    const std::string &filterOut = "", const std::string &pattern = " ");
std::string LeftTrim(const std::string &str, const std::string &trims);
}
}
}
#endif // __IDE_DAEMON_COMMON_UTILS_H__
