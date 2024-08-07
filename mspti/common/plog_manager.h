/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log.h
 * Description        : Record log to plog.
 * Author             : msprof team
 * Creation Date      : 2024/05/31
 * *****************************************************************************
*/
#ifndef MSPTI_COMMON_PLOG_MANAGER_H
#define MSPTI_COMMON_PLOG_MANAGER_H

#include <cstdint>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>

#include "common/inject/plog_inject.h"
#include "common/utils.h"
#include "securec.h"

#define PLOG_TYPE_MSPTI 31
#define MAX_LOG_BUF_SIZE 1024
#define FILENAME (strrchr("/" __FILE__, '/') + 1)

namespace Mspti {
namespace Common {
namespace {
const std::unordered_map<int, std::string> level_name = {
    {DLOG_ERROR, "ERROR"},
    {DLOG_INFO, "INFO"},
    {DLOG_WARN, "WARN"},
    {DLOG_DEBUG, "DEBUG"},
};
}

class PlogManager {
public:
    static PlogManager* GetInstance()
    {
        static PlogManager instance;
        return &instance;
    }

    template<typename... T>
    void Log(int level, const char* fileName, int linNo, const char *fmt, T... args)
    {
        if (CheckLogLevelForC(type_, level) == 1) {
            char buf[MAX_LOG_BUF_SIZE] = {0};
            if (sprintf_s(buf, MAX_LOG_BUF_SIZE, "[%s:%d] >>> (tid:%u) %s\n", fileName, linNo,
                Utils::GetTid(), fmt) < 0) {
                return;
            }
            DlogInnerForC(type_, level, buf, args...);
        }
    }

    template<typename... T>
    void Printf(int level, const char* fileName, int linNo, const char *fmt, T... args)
    {
        char buf[MAX_LOG_BUF_SIZE] = {0};
        auto level_ptr = level_name.find(level);
        if (level_ptr == level_name.end()) {
            return;
        }
        if (sprintf_s(buf, MAX_LOG_BUF_SIZE, "[%s] PROFILING(%u,%s) [%s:%d] >>> (tid:%u) %s\n",
            level_ptr->second.c_str(), Utils::GetPid(), Utils::GetProcName(), fileName, linNo,
            Utils::GetTid(), fmt) < 0) {
            return;
        }
        printf(buf, args...);
    }

private:
    PlogManager() = default;
    explicit PlogManager(const PlogManager &obj) = delete;
    PlogManager& operator=(const PlogManager &obj) = delete;
    explicit PlogManager(PlogManager &&obj) = delete;
    PlogManager& operator=(PlogManager &&obj) = delete;

private:
    static constexpr int type_{PLOG_TYPE_MSPTI};
};

#define MSPTI_LOGE(format, ...) do {                                                                        \
    Mspti::Common::PlogManager::GetInstance()->Log(DLOG_ERROR, FILENAME, __LINE__, format, ##__VA_ARGS__);  \
} while (0)

#define MSPTI_LOGI(format, ...) do {                                                                        \
    Mspti::Common::PlogManager::GetInstance()->Log(DLOG_INFO, FILENAME, __LINE__, format, ##__VA_ARGS__);   \
} while (0)

#define MSPTI_LOGW(format, ...) do {                                                                        \
    Mspti::Common::PlogManager::GetInstance()->Log(DLOG_WARN, FILENAME, __LINE__, format, ##__VA_ARGS__);   \
} while (0)

#define MSPTI_LOGD(format, ...) do {                                                                        \
    Mspti::Common::PlogManager::GetInstance()->Log(DLOG_DEBUG, FILENAME, __LINE__, format, ##__VA_ARGS__);  \
} while (0)

#define PRINT_LOGE(format, ...) do {                                                                           \
    Mspti::Common::PlogManager::GetInstance()->Printf(DLOG_ERROR, FILENAME, __LINE__, format, ##__VA_ARGS__);  \
} while (0)

#define PRINT_LOGW(format, ...) do {                                                                           \
    Mspti::Common::PlogManager::GetInstance()->Printf(DLOG_WARN, FILENAME, __LINE__, format, ##__VA_ARGS__);   \
} while (0)


}  // Common
}  // Mspti
#endif
