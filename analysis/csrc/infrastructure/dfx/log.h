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

#ifndef ANALYSIS_DFX_LOG_H
#define ANALYSIS_DFX_LOG_H

#include <mutex>
#include <vector>
#include <sstream>
#include <unistd.h>

#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/singleton.h"

namespace Analysis {
#define DEBUG(format, ...)

#define INFO(format, ...)                                                                                           \
    do {                                                                                                            \
        Log::GetInstance().LogMsg(Format(format, ##__VA_ARGS__), "[INFO]", Log::GetFileName(__FILE__), __LINE__);   \
    } while (0)

#define WARN(format, ...)                                                                                           \
    do {                                                                                                            \
        Log::GetInstance().LogMsg(Format(format, ##__VA_ARGS__), "[WARN]", Log::GetFileName(__FILE__), __LINE__);   \
    } while (0)

#define ERROR(format, ...)                                                                                          \
    do {                                                                                                            \
        Log::GetInstance().LogMsg(Format(format, ##__VA_ARGS__), "[ERROR]", Log::GetFileName(__FILE__), __LINE__);  \
    } while (0)

#define PRINT_INFO(format, ...)                                                                                     \
    do {                                                                                                            \
        Log::GetInstance().PrintMsg(Format(format, ##__VA_ARGS__), "[INFO]", Log::GetFileName(__FILE__));           \
    } while (0)

#define PRINT_WARN(format, ...)                                                                                     \
    do {                                                                                                            \
        Log::GetInstance().PrintMsg(Format(format, ##__VA_ARGS__), "[WARN]", Log::GetFileName(__FILE__));           \
    } while (0)

#define PRINT_ERROR(format, ...)                                                                                    \
    do {                                                                                                            \
        Log::GetInstance().PrintMsg(Format(format, ##__VA_ARGS__), "[ERROR]", Log::GetFileName(__FILE__));          \
    } while (0)

// 该类主要用于日志处理
class Log : public Utils::Singleton<Log> {
public:
    void LogMsg(const std::string& message, const std::string &level,
                const std::string &fileName, const uint32_t &line);
    void PrintMsg(const std::string& message, const std::string &level, const std::string &fileName) const;
    int Init(const std::string &logDir);
    static std::string GetTime();
    static std::string GetFileName(const std::string &path);

private:
    Utils::FileWriter logWriter_;
    std::mutex logMutex_;
    pid_t pid_ = getpid();
};  // class Log

template <class T>
std::string ToString(const T &val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

template <class ...Args>
std::string Format(const std::string &fmt, const Args& ...args)
{
    if (sizeof...(args) == 0) {
        return fmt;
    }
    const std::vector<std::string> vs = {ToString(args)...};
    std::ostringstream res;
    size_t argIdx = 0;
    size_t idx = 0;
    while (idx < fmt.size()) {
        if (fmt[idx] != '%') {  // 当前字符不是'%'时，直接输出
            res << fmt[idx];
        } else if (idx + 1 < fmt.size() && fmt[idx + 1] == '%') {  // 连续的两个"%%"时，输出'%'
            ++idx;
            res << fmt[idx];
        } else if (argIdx < vs.size()) {  // 单独的%时，替换参数
            res << vs[argIdx];
            ++argIdx;
        }
        ++idx;
    }
    return res.str();
}
}  // namespace Analysis

#endif // ANALYSIS_DFX_LOG_H
