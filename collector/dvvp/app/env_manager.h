/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: ly
 * Create: 2020-12-21
 */
#ifndef ANALYSIS_DVVP_ENV_MANAGER_H
#define ANALYSIS_DVVP_ENV_MANAGER_H
#include <vector>
#include <string>
#include "singleton/singleton.h"
namespace Analysis {
namespace Dvvp {
namespace App {
using CONST_CHAR_PTR_PTR = const char **;

class EnvManager : public analysis::dvvp::common::singleton::Singleton<EnvManager> {
public:
    void SetGlobalEnv(std::vector<std::string> &envList);
    std::vector<std::string> GetGlobalEnv();
    static int SetEnvList(CONST_CHAR_PTR_PTR envp, std::vector<std::string> &envpList);
private:
    std::vector<std::string> envList_;
};
}
}
}
#endif