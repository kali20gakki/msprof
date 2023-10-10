/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: test msopprof cpp
 * Author: zyb
 * Create: 2023-09-18
 */
#include <string>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "msopprof_manager.h"
#include "param_validation.h"
#include "utils/utils.h"
#include "errno/error_code.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Msopprof;
using namespace analysis::dvvp::common::validation;
class MsopprofManagerUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(MsopprofManagerUtest, MsopprofManagerCheckInputDataValidity)
{
    GlobalMockObject::verify();
    CHAR_PTR opargv[2];
    opargv[0] = "msprof";
    opargv[1] = "--op=on";
    std::vector<std::string> argsVec;
    EXPECT_EQ(true, MsopprofManager::instance()->CheckInputDataValidity(2, (const char**)opargv));
    EXPECT_EQ(false, MsopprofManager::instance()->CheckInputDataValidity(513, (const char**)opargv));
}

TEST_F(MsopprofManagerUtest, MsopprofManagerCheckMsopprofIfExist)
{
    GlobalMockObject::verify();
    std::vector<std::string> op_argv;
    CHAR_PTR opargv[3];
    const int index = 2;
    opargv[0] = "msprof";
    opargv[1] = "--mode=onboard";
    EXPECT_EQ(false, MsopprofManager::instance()->CheckMsopprofIfExist(2, (const char**)opargv, op_argv));
    opargv[index] = "--op=on";
    op_argv.clear();
    EXPECT_EQ(true, MsopprofManager::instance()->CheckMsopprofIfExist(3, (const char**)opargv, op_argv));
}

TEST_F(MsopprofManagerUtest, MsopprofManagerPrintHelp)
{
    GlobalMockObject::verify();
    std::string path = "/home";
    MOCKER_CPP(&ParamValidation::CheckMsopprofBinValid)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&MsopprofManager::ExecMsopprof)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MsopprofManager::instance()->path_ = "";
    MsopprofManager::instance()->PrintHelp();
    MsopprofManager::instance()->path_ = path;
    MsopprofManager::instance()->PrintHelp();
}

TEST_F(MsopprofManagerUtest, MsopprofManagerExecMsopprof)
{
    GlobalMockObject::verify();
    CONST_CHAR_PTR path = "cmd";
    std::vector<std::string> argsVec;
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsopprofManager::instance()->ExecMsopprof(path, argsVec));
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::WaitProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsopprofManager::instance()->ExecMsopprof(path, argsVec));
    EXPECT_EQ(PROFILING_SUCCESS, MsopprofManager::instance()->ExecMsopprof(path, argsVec));
}

