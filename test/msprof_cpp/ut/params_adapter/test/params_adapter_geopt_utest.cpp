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
#include "message/prof_json_config.h"
#include "param_validation.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::message;

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

TEST_F(ParamsAdapterGeoptUtest, InitFailed)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    MOCKER_CPP(&ParamsAdapterGeOpt::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->Init());
}

TEST_F(ParamsAdapterGeoptUtest, GeOptParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    int ret = GeOptParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg;
    MSVP_MAKE_SHARED0_VOID(geCfg, ProfGeOptionsConfig);
    geCfg->storageLimit = "200MB";
    geCfg->instrProfilingFreq = 1;
    geCfg->sysHardwareMemFreq = 0;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]);
    geCfg->sysHardwareMemFreq = 1;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("1", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]);
    geCfg->sysIoSamplingFreq = 0;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ]);
    geCfg->sysIoSamplingFreq = 1;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("1", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ]);
    geCfg->sysInterconnectionFreq = 0;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]);
    geCfg->sysInterconnectionFreq = 1;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("1", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]);
    geCfg->dvppFreq = 0;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_DVPP_FREQ]);
    geCfg->dvppFreq = 1;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("1", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_DVPP_FREQ]);
    geCfg->hostSysUsageFreq = 0;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ]);
    geCfg->hostSysUsageFreq = 1;
    GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    EXPECT_EQ("1", GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ]);
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
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::CheckHostSysGeOptValid)
        .stubs()
        .will(returnValue(true));
    for (auto cfg : GeOptParamAdapterMgr->geOptConfig_) {
        GeOptParamAdapterMgr->setConfig_.insert(cfg);
        int ret = GeOptParamAdapterMgr->ParamsCheckGeOpt();
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(ParamsAdapterGeoptUtest, SetGeOptionsContainerDefaultValueWillFailWhenSetOutputDirFailInHelperHostSide)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);

    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS] = "ArithmeticUtilization";
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_METRICS] = "ArithmeticUtilization";
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetOutputDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->SetGeOptionsContainerDefaultValue());
}

TEST_F(ParamsAdapterGeoptUtest, SetGeOptionsContainerDefaultValueWillSuccWhenSetOutputDirSuccInHelperHostSide)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
 
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS] = "ArithmeticUtilization";
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_METRICS] = "ArithmeticUtilization";
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::SetDefaultAivParams)
        .stubs();
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetGeOptContainerSysValue)
        .stubs();
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::SetDefaultLlcMode)
        .stubs();
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetOutputDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, GeOptParamAdapterMgr->SetGeOptionsContainerDefaultValue());
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_INSTR_PROFILING_FREQ] = "x";
    EXPECT_EQ(PROFILING_SUCCESS, GeOptParamAdapterMgr->SetGeOptionsContainerDefaultValue());
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

TEST_F(ParamsAdapterGeoptUtest, CheckInstrAndTaskParamBothSet)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg = nullptr;
    MSVP_MAKE_SHARED0_VOID(geCfg, ProfGeOptionsConfig);
    geCfg->instrProfilingFreq = 1;
    geCfg->taskTime = "on";
    EXPECT_EQ(true, GeOptParamAdapterMgr->CheckInstrAndTaskParamBothSet(geCfg));
    geCfg->taskTime = "off";
    EXPECT_EQ(false, GeOptParamAdapterMgr->CheckInstrAndTaskParamBothSet(geCfg));
}

TEST_F(ParamsAdapterGeoptUtest, GeGetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);
    SHARED_PTR_ALIA<ProfGeOptionsConfig> geCfg = nullptr;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params = nullptr;
    int ret = GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params);
    EXPECT_EQ(PROFILING_FAILED, ret);
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    ret = GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params);
    EXPECT_EQ(PROFILING_FAILED, ret);
    MSVP_MAKE_SHARED0_VOID(geCfg, ProfGeOptionsConfig);

    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::CheckInstrAndTaskParamBothSet)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::ComCfgCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterGeOpt::TransToParam)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_FAILED, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
    EXPECT_EQ(PROFILING_SUCCESS, GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params));
}

TEST_F(ParamsAdapterGeoptUtest, CheckHostSysGeOptValid)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterGeOpt> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, ParamsAdapterGeOpt);

    std::string cfg;
    EXPECT_EQ(false, GeOptParamAdapterMgr->CheckHostSysGeOptValid(cfg));

    cfg = "xxx";
    EXPECT_EQ(false, GeOptParamAdapterMgr->CheckHostSysGeOptValid(cfg));

    cfg = "cpu,mem";
    EXPECT_EQ(true, GeOptParamAdapterMgr->CheckHostSysGeOptValid(cfg));
}