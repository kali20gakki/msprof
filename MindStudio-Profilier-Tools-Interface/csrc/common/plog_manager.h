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
 * -------------------------------------------------------------------------
*/
#ifndef MSPTI_COMMON_PLOG_MANAGER_H
#define MSPTI_COMMON_PLOG_MANAGER_H

#include <cstdint>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>

#include "csrc/common/inject/plog_inject.h"
#include "csrc/common/utils.h"
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
            if (sprintf_s(buf, MAX_LOG_BUF_SIZE, "(mspti)[%s:%d] >>> (tid:%u) %s\n", fileName, linNo,
                Utils::GetTid(), fmt) < 0) {
                return;
            }
            DlogInnerForC(type_, level, buf, args...);
        }
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

}  // Common
}  // Mspti
#endif
