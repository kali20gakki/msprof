/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: msopprof manager
 * Author: zyb
 * Create: 2023-09-18
 */

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