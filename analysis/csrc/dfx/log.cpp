/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log.cpp
 * Description        : 日志模块
 * Author             : msprof team
 * Creation Date      : 2023/11/11
 * *****************************************************************************
 */

#include "analysis/csrc/dfx/log.h"

#include <iostream>
#include <sys/syscall.h>
#include <cstring>

#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
using namespace Analysis::Utils;

namespace {
const mode_t LOG_DIR_MODE = 0750;
const size_t TIME_SIZE = 20;
}

int Log::Init(const std::string &logDir)
{
    if (logDir.empty()) {
        PRINT_ERROR("Log path is empty.");
        return ANALYSIS_ERROR;
    }
    if (!File::CreateDir(logDir, LOG_DIR_MODE)) {
        PRINT_ERROR("Create log dir failed.");
        return ANALYSIS_ERROR;
    }
    std::string logName = Format("msprof_analysis_%.log", pid_);
    std::string logFile = File::PathJoin({logDir, logName});
    logWriter_.Open(logFile, std::ios::out | std::ios::app);
    if (!logWriter_.IsOpen()) {
        PRINT_ERROR("Log file open failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

std::string Log::GetFileName(const std::string &path)
{
    return (strrchr(path.c_str(), '/')) ? (strrchr(path.c_str(), '/') + 1) : path;
}

std::string Log::GetTime()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo = *std::localtime(&nowTime);
    char formatTime[TIME_SIZE];
    std::strftime(formatTime, sizeof(formatTime), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return static_cast<std::string>(formatTime);
}

void Log::LogMsg(const std::string& message, const std::string &level,
                 const std::string &fileName, const uint32_t &line)
{
    std::ostringstream oss;
    auto tid = syscall(SYS_gettid);
    oss << GetTime() << " " << level << " [" << pid_ << "]" << " [" << tid << "]";
    oss << " [" << fileName << ":" << line << "] " << message << "\n";
    std::lock_guard<std::mutex> lock(logMutex_);
    logWriter_.WriteText(oss.str());
}

void Log::PrintMsg(const std::string& message, const std::string &level, const std::string &fileName) const
{
    std::ostringstream oss;
    oss << GetTime() << " " << level << " [" << pid_ << "] ";
    oss << fileName << ": " << message << "\n";
    std::cout << oss.str();
}
}  // namespace Analysis
