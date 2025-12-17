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
                         MmProcess &appProcess);

private:
    static int LaunchShellApp(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
                              MmProcess &appProcess);
    static int PrepareAppEnvs(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        std::vector<std::string> &envsV);
    static int PrepareLaunchAppCmd(std::stringstream &ssPerfCmdApp,
                                   SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

    static void PrepareAppArgs(const std::vector<std::string> &params, std::vector<std::string> &argsV);

    static void SetAppEnv(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        std::vector<std::string> &envsV);
    static std::string GetCmdString(const std::string paramsName);
};
}  // namespace app
}  // namespace dvvp
}  // namespace analysis

#endif
