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
#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <fstream>
#include "errno/error_code.h"
#include "msprof_bin.h"
#include "running_mode.h"
#include "msprof_manager.h"
#include "message/prof_params.h"
#include "prof_callback.h"
#include "config/config_manager.h"
#include "params_adapter_msprof.h"
#include "params_adapter.h"
#include "input_parser.h"
#include "platform/platform.h"
#include "env_manager.h"

using namespace Analysis::Dvvp::App;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::ParamsAdapter;
class MSPROF_BIN_UTEST : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

extern int LltMain(int argc, const char **argv, const char **envp);

TEST_F(MSPROF_BIN_UTEST, LltMain) {
    GlobalMockObject::verify();
    char* argv[10];
    argv[0] = "--help";
    argv[1] = "--test";
    char* envp[2];
    envp[0] = "test=a";
    envp[1] = "a=b";
    CHAR_PTR opargv[2];
    opargv[0] = "msopprof";
    opargv[1] = "op";
    MOCKER(&EnvManager::SetEnvList)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, LltMain(1, (const char**)argv, (const char**)envp));
    EXPECT_EQ(PROFILING_FAILED, LltMain(2, (const char**)argv, (const char**)envp));
    EXPECT_EQ(PROFILING_SUCCESS, LltMain(2, (const char**)opargv, (const char**)envp));
    std::ofstream test_file("prof_bin_test");
    test_file << "echo test" << std::endl;
    test_file.close();
    chmod("./prof_bin_test", 0700);
    argv[2] = "--app=./prof_bin_test";
    argv[3] = "--task-time=on";
    EXPECT_EQ(PROFILING_FAILED, LltMain(4, (const char**)argv, (const char**)envp));
    std::remove("prof_bin_test");
    argv[4] = "--sys-devices=0";
    argv[5] = "--output=./msprof_bin_utest";
    argv[6] = "--sys-period=1";
    argv[7] = "--sys-pid-profiling=on";
    EXPECT_EQ(PROFILING_FAILED, LltMain(8, (const char**)argv, (const char**)envp));
}

TEST_F(MSPROF_BIN_UTEST, SetEnvList) {
    GlobalMockObject::verify();
    std::shared_ptr<EnvManager> envManager;
    MSVP_MAKE_SHARED0_BREAK(envManager, EnvManager);
    char* envp[4097];
    char str[] = "a=a";
    for (int i = 0; i < 4097; i++) {
        envp[i] = str;
    }
    envp[4096] = nullptr;
    std::vector<std::string> envpList;
    bool val = envManager->SetEnvList((const char**)envp, envpList);
    EXPECT_EQ(true, val);
}
