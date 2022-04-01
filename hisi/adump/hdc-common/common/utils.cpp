/**
 * @file utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "common/utils.h"
#include <string>
#include "mmpa_api.h"
#include "ide_common_util.h"
#include "ide_platform_util.h"
#include "config/adx_config_manager.h"
#include "file_utils.h"
using namespace IdeDaemon::Common::Config;
using namespace Adx;
namespace IdeDaemon {
namespace Common {
namespace Utils {
/**
 * @brief replace wave to Home directory
 * @param [in] filePath : file path
 * @return
 *        after replace path
 */
std::string ReplaceWaveToHomeDir(const std::string &filePath)
{
    if (filePath.empty()) {
        return "";
    }

    const int waveBeginPos = 0;
    const int wavePos      = 1;
    if (filePath.find(IDE_HOME_WAVE_DIR) == waveBeginPos) {
        std::string homeDir = IdeGetHomeDir();
        homeDir.append(filePath.substr(wavePos));
        return homeDir;
    } else {
        return filePath;
    }
}

/**
 * @brief Check if the path has sufficient disk space
 * @param [in] path: file path
 * @return
 *        true:   the path has sufficient disk space
 *        false:  have invalid char in dir
 */
bool IsValidDirChar(const std::string &path)
{
    if (path.empty()) {
        IDE_LOGE("invalid parameter");
        return false;
    }

    const std::string pathWhiteList = "-=[];\\,./!@#$%^&*()_+{}:?";
    size_t len = path.length();
    for (size_t i = 0; i < len; i++) {
        if (!std::islower(path[i]) && !std::isupper(path[i]) && !std::isdigit(path[i]) &&
            pathWhiteList.find(path[i]) == std::string::npos) {
            IDE_LOGW("invalid path %s in char : %c", path.c_str(), path[i]);
            return false;
        }
    }

    return true;
}

std::vector<std::string> Split(const std::string &inputStr, bool filterOutEnabled,
    const std::string &filterOut, const std::string &pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    std::string str = inputStr + pattern;
    std::string::size_type size = str.size();
    for (std::string::size_type i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            bool ok = true;

            std::string s = str.substr(i, pos - i);
            if ((filterOutEnabled && (s.compare(filterOut) == 0))) {
                ok = false;
            }

            if (ok) {
                result.push_back(s);
            }

            i = pos + pattern.size() - 1;
        }
    }

    return result;
}

/**
 * @brief clear left trims
 * @param[in] str : check trim string
 * @param[in] trims : trim char
 * @return left no trimed char of string
 */
std::string LeftTrim(const std::string &str, const std::string &trims)
{
    if (str.length() > 0) {
        size_t pos = str.find_first_not_of(trims);
        if (pos != std::string::npos) {
            return str.substr(pos);
        } else {
            return "";
        }
    }

    return str;
}
}}}
