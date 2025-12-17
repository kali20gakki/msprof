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
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "slog_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

int CheckLogLevelForC(int moduleId, int logLevel)
{
    return PROFILING_SUCCESS;
}

class SlogPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SlogPluginUtest, LoadSlogSo)
{
    GlobalMockObject::verify();
    auto slogPlugin = SlogPlugin::instance();
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    slogPlugin->LoadSlogSo();
    EXPECT_EQ(nullptr, slogPlugin->checkLogLevelForC_);
    slogPlugin->LoadSlogSo();
    EXPECT_EQ(nullptr, slogPlugin->checkLogLevelForC_);
    slogPlugin->LoadSlogSo();
}

TEST_F(SlogPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto slogPlugin = SlogPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true));
    std::string funcName = "CheckLogLevelForC";
    EXPECT_EQ(true, slogPlugin->IsFuncExist(funcName));
}

TEST_F(SlogPluginUtest, MsprofCheckLogLevelForC)
{
    GlobalMockObject::verify();
    int moduleId = 0;
    int logLevel = 0;
    auto slogPlugin = SlogPlugin::instance();
    slogPlugin->checkLogLevelForC_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, slogPlugin->MsprofCheckLogLevelForC(moduleId, logLevel));
    slogPlugin->checkLogLevelForC_ = CheckLogLevelForC;
    EXPECT_EQ(PROFILING_SUCCESS, slogPlugin->MsprofCheckLogLevelForC(moduleId, logLevel));
}
