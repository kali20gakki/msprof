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

const std::vector<std::string> EnvManager::GetGlobalEnv()
{
    return envList_;
}
}
}
}