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

bool EnvManager::SetEnvList(analysis::dvvp::common::utils::CONST_CHAR_PTR_PTR envp, std::vector<std::string> &envpList)
{
    if (envp == nullptr) {
        return false;
    }
    bool isParaNumExceed = false;
    uint32_t envpLen = 0;
    constexpr uint32_t maxEnvpLen = 4096;
    while (*envp) {
        envpList.emplace_back(*envp++);
        envpLen++;
        if (envpLen >= maxEnvpLen) {
            isParaNumExceed = true;
            break;
        }
    }
    return isParaNumExceed;
}
}
}
}