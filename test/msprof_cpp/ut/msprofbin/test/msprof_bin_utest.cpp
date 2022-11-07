/**
* @file msprof_bin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
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
extern void SetEnvList(const char **envp, std::vector<std::string> &envpList);

TEST_F(MSPROF_BIN_UTEST, LltMain) {
    GlobalMockObject::verify();
    char* argv[10];
    argv[0] = "--help";
    argv[1] = "--test";
    char* envp[2];
    envp[0] = "test=a";
    envp[1] = "a=b";
    
    MOCKER(&SetEnvList)
        .stubs();
    EXPECT_EQ(PROFILING_FAILED, LltMain(1, (const char**)argv, (const char**)envp));
    EXPECT_EQ(PROFILING_FAILED, LltMain(2, (const char**)argv, (const char**)envp));

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
    char* envp[4097];
    char str[] = "a=a";
    for (int i = 0; i < 4097; i++) {
        envp[i] = str;
    }
    envp[4096] = nullptr;
    std::vector<std::string> envpList;
    SetEnvList((const char**)envp, envpList);
}