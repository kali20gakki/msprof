/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_APP_APPLICATION_H
#define ANALYSIS_DVVP_APP_APPLICATION_H

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "message/prof_params.h"
#include "transport.h"

namespace analysis {
namespace dvvp {
namespace app {
class Application {
public:
    static int LaunchApp(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
                         mmProcess &appProcess);

private:
    static int PrepareAppEnvs(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        std::vector<std::string> &envsV);
    static int PrepareLaunchAppCmd(std::stringstream &ssPerfCmdApp,
                                   SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

    static void PrepareAppArgs(const std::vector<std::string> &params, std::vector<std::string> &argsV);

    static int PrepareAclEnvs(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
                        std::vector<std::string> &envsV);

    static void SetAppEnv(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        std::vector<std::string> &envsV);
    static void SourceEnv(std::vector<std::string> &argsVec);
    static std::string GetAppPath(std::vector<std::string> paramsCmd);
    static std::string GetCmdString(const std::string paramsName);
    static int CanonicalizeAppParam(std::vector<std::string> &paramsCmd);
};
}  // namespace app
}  // namespace dvvp
}  // namespace analysis

#endif
