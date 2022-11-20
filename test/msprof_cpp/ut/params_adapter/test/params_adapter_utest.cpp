/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for params adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "params_adapter.h"
#include "errno/error_code.h"
#include "param_validation.h"

using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::error;

class ParamsAdapterUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterUtest, CheckListInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    int ret = ParamsAdapterMgr->CheckListInit();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(PlatformType::MINI_TYPE))
        .then(returnValue(PlatformType::CLOUD_TYPE))
        .then(returnValue(PlatformType::MDC_TYPE))
        .then(returnValue(PlatformType::LHISI_TYPE))
        .then(returnValue(PlatformType::DC_TYPE))
        .then(returnValue(PlatformType::CHIP_V4_1_0));
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterUtest, TransToParam)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    ParamsAdapterMgr->CheckListInit();
    std::array<std::string, INPUT_CFG_MAX> paramContainer;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    paramContainer[INPUT_CFG_COM_TASK_TRACE] = "on";
    paramContainer[INPUT_CFG_COM_TRAINING_TRACE] = "on";
    paramContainer[INPUT_CFG_COM_HCCL] = "on";
    paramContainer[INPUT_CFG_COM_SYS_USAGE] = "on";
    paramContainer[INPUT_CFG_COM_SYS_CPU] = "on";
    paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM] = "on";
    paramContainer[INPUT_CFG_COM_SYS_IO] = "on";
    paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION] = "on";
    paramContainer[INPUT_CFG_COM_DVPP] = "on";
    paramContainer[INPUT_CFG_COM_POWER] = "on";
    paramContainer[INPUT_CFG_COM_BIU] = "on";
    paramContainer[INPUT_CFG_HOST_SYS] = "cpu,mem,disk,osrt,network";
    paramContainer[INPUT_CFG_HOST_SYS_USAGE] = "cpu,mem";
    int ret = ParamsAdapterMgr->TransToParam(paramContainer, params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterUtest, ComCfgCheck2)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    std::vector<std::pair<InputCfg, std::string>> cfgErrList;
    std::vector<InputCfg> cfgList = {
        INPUT_CFG_COM_SYS_USAGE_FREQ,
        INPUT_CFG_COM_LLC_MODE,
        INPUT_CFG_HOST_SYS_USAGE
    };
    std::string cfgValue = "on";
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckLlcModeIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysUsageValid)
        .stubs()
        .will(returnValue(true));
    for (auto cfg : cfgList) {
        bool ret = ParamsAdapterMgr->ComCfgCheck2(cfg, cfgValue);
        EXPECT_EQ(true, ret);
    }
    InputCfg  errCfg = INPUT_CFG_COM_TASK_TIME;
    bool ret = ParamsAdapterMgr->ComCfgCheck2(errCfg, cfgValue);
    EXPECT_EQ(false, ret);
}

TEST_F(ParamsAdapterUtest, CheckFreqValid)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    std::string freq = "20";
    InputCfg freqOpt = INPUT_CFG_COM_AIC_FREQ;
    bool ret = ParamsAdapterMgr->CheckFreqValid(freq, freqOpt);
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckFreqIsValid)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(true, ret);
}