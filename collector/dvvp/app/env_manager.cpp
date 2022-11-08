/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: ly
 * Create: 2020-12-21
 */
#include "env_manager.h"
namespace Analysis {
namespace Dvvp {
namespace App {
void EnvManager::SetGlobalEnv(std::vector<std::string> &envList)
{
    envList_ = envList;
}

std::vector<std::string> EnvManager::GetGlobalEnv()
{
    return envList_;
}

int EnvManager::SetEnvList(CONST_CHAR_PTR_PTR envp, std::vector<std::string> &envpList)
{
    uint32_t envpLen = 0;
    constexpr uint32_t maxEnvpLen = 4096;
    while (*envp) {
        envpList.emplace_back(*envp++);
        envpLen++;
        if (envpLen >= maxEnvpLen) {
            return -1;
        }
    }
    return 0;
}
}
}
}