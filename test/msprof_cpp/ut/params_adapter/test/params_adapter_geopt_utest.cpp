/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for ge option params adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "params_adapter_geopt.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::ParamsAdapter;

class ParamsAdapterGeoptUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterGeoptUtest, GeOptParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    int ret = GeOptParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfGeOptionsConfig> geCfg;
    MSVP_MAKE_SHARED0_VOID(geCfg, analysis::dvvp::proto::ProfGeOptionsConfig);
    geCfg->set_storage_limit("200MB");
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
}

TEST_F(ParamsAdapterGeoptUtest, ParamsCheckGeOpt)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    GeOptParamAdapterMgr->Init();
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    for (auto cfg : GeOptParamAdapterMgr->geOptConfig_) {
        GeOptParamAdapterMgr->setConfig_.insert(cfg);
        int ret = GeOptParamAdapterMgr->ParamsCheckGeOpt();
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(ParamsAdapterGeoptUtest, SetGeOptionsContainerDefaultValue)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);

    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS] = "ArithmeticUtilization";
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_METRICS] = "ArithmeticUtilization";
    int ret = GeOptParamAdapterMgr->SetGeOptionsContainerDefaultValue();
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetOutputDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(ParamsAdapterGeoptUtest, GeOptionsSetOutputDir)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);

    std::string output;
    int ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_FAILED, ret);
    output = "./";
    ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    output = "/var";
    ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(ParamsAdapterGeoptUtest, GeGetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfGeOptionsConfig> geCfg = nullptr;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params = nullptr;
    int ret = GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params);
    EXPECT_EQ(PROFILING_FAILED, ret);
    MSVP_MAKE_SHARED0_VOID(geCfg, analysis::dvvp::proto::ProfGeOptionsConfig);
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ret);
}