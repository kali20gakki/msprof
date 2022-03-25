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
class EnvManager : public analysis::dvvp::common::singleton::Singleton<EnvManager> {
public:
    void SetGlobalEnv(std::vector<std::string> &envList);
    const std::vector<std::string> GetGlobalEnv();
private:
    std::vector<std::string> envList_;
};
}
}
}
#endif