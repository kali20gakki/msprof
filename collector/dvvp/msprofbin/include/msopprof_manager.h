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

#ifndef MSOPPROF_MANAGER_H
#define MSOPPROF_MANAGER_H
#include <vector>
#include <string>
#include "utils/utils.h"
#include "config/config.h"
#include "singleton/singleton.h"

namespace Analysis {
namespace Dvvp {
namespace Msopprof {
using CONST_CHAR_PTR = const char *;
using namespace analysis::dvvp::common::config;

class MsopprofManager : public analysis::dvvp::common::singleton::Singleton<MsopprofManager> {
public:
    MsopprofManager();
    ~MsopprofManager() {};
    int MsopprofProcess(int argc, CONST_CHAR_PTR argv[]);
    MmProcess GetMsopprofPid() { return opProcess_; }

private:
    bool CheckInputDataValidity(int argc, CONST_CHAR_PTR argv[]) const;
    bool CheckMsopprofIfExist(int, CONST_CHAR_PTR argv[], std::vector<std::string> &op_argv) const;
    int ExecMsopprof(std::string path, std::vector<std::string> argsVec);
private:
    std::string path_;
    MmProcess opProcess_ = MSVP_MMPROCESS;
};
}
}
}
#endif