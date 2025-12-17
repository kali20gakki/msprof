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
    opargv[1] = "op";
    std::vector<std::string> argsVec;
    EXPECT_EQ(true, MsopprofManager::instance()->CheckInputDataValidity(2, (const char**)opargv));
    EXPECT_EQ(false, MsopprofManager::instance()->CheckInputDataValidity(1025, (const char**)opargv));
}

TEST_F(MsopprofManagerUtest, MsopprofManagerCheckMsopprofIfExist)
{
    GlobalMockObject::verify();
    std::vector<std::string> op_argv;
    CHAR_PTR opargv[3];
    opargv[0] = "msprof";
    opargv[1] = "--mode=onboard";
    EXPECT_EQ(false, MsopprofManager::instance()->CheckMsopprofIfExist(2, (const char**)opargv, op_argv));
    opargv[1] = "op";
    op_argv.clear();
    EXPECT_EQ(true, MsopprofManager::instance()->CheckMsopprofIfExist(2, (const char**)opargv, op_argv));
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

