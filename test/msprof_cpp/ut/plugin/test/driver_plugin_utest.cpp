/**
* @file driver_plugin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
*/
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "driver_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

class DriverPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(DriverPluginUtest, LoadDriverSo)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::OpenPlugin)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    driverPlugin->LoadDriverSo();
    driverPlugin->LoadDriverSo();
}

TEST_F(DriverPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true));
    std::string funcName = "halHdcRecv";
    EXPECT_EQ(true, driverPlugin->IsFuncExist(funcName));
}

TEST_F(DriverPluginUtest, MsprofHalHdcRecv)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true));
    std::string funcName = "halHdcRecv";
    EXPECT_EQ(true, driverPlugin->IsFuncExist(funcName));
}