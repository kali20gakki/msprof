/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for msprof params adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "params_adapter_msprof.h"
#include "param_validation.h"
#include "errno/error_code.h"
#include "input_parser.h"


using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;

class ParamsAdapterMsprofUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterMsprofUtest, MsprofParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    int ret = MsprofParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckAppValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckEnvValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterMsprof::ParamsCheckMsprofV1)
        .stubs()
        .will(returnValue(true));
    for (auto setCfg : MsprofParamAdapterMgr->msprofConfig_) {
        MsprofParamAdapterMgr->setConfig_.insert(setCfg);
        ret = MsprofParamAdapterMgr->ParamsCheckMsprof();
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(ParamsAdapterMsprofUtest, ParamsCheckMsprofV1)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    int ret;
    std::vector<InputCfg> inputCfgList = {
        INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIC_MODE,
        INPUT_CFG_COM_INSTR_PROFILING_FREQ,
        INPUT_CFG_COM_SYS_DEVICES,
        INPUT_CFG_COM_SYS_PERIOD,
        INPUT_CFG_HOST_SYS,
        INPUT_CFG_HOST_SYS_PID,
    };
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckAiModeValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckSysDeviceValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckSysPeriodValid)
            .stubs()
            .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckHostSysValid)
            .stubs()
            .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysPidValid)
        .stubs()
        .will(returnValue(true));
    for (auto inputCfg : inputCfgList) {
        ret = MsprofParamAdapterMgr->ParamsCheckMsprofV1(inputCfg, " ");
        EXPECT_EQ(true, ret);
    }
}

TEST_F(ParamsAdapterMsprofUtest, ParamsCheckDynProf)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_VOID(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    int ret = MsprofParamAdapterMgr->ParamsCheckDynProf();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] = "on";
    ret = MsprofParamAdapterMgr->ParamsCheckDynProf();
    EXPECT_EQ(PROFILING_FAILED, ret);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC_PID] = "1234";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_APPLICATION] = "main";
    ret = MsprofParamAdapterMgr->ParamsCheckDynProf();
    EXPECT_EQ(PROFILING_FAILED, ret);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_APPLICATION] = "";
    ret = MsprofParamAdapterMgr->ParamsCheckDynProf();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsApp)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->SetDefaultParamsApp();
}

TEST_F(ParamsAdapterMsprofUtest, MsprofSetDefaultParams)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->SetDefaultParamsSystem();
    MsprofParamAdapterMgr->SetDefaultParamsParse();
    MsprofParamAdapterMgr->SetDefaultParamsQuery();
    MsprofParamAdapterMgr->SetDefaultParamsExport();
}

TEST_F(ParamsAdapterMsprofUtest, SpliteAppPath)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::string appParam;
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
    appParam = "./main 1";
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
    appParam = "/bin/bash test.sh 1";
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
}

TEST_F(ParamsAdapterMsprofUtest, SetParamsSelf)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MsprofParamAdapterMgr->params_ = params;
    MsprofParamAdapterMgr->SetParamsSelf();
}

TEST_F(ParamsAdapterMsprofUtest, CreateCfgMap)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->CreateCfgMap();
}

TEST_F(ParamsAdapterMsprofUtest, SetModeDefaultParams)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::vector<MsprofMode> modeTypeList = {
        MsprofMode::MSPROF_MODE_APP,
        MsprofMode::MSPROF_MODE_SYSTEM,
        MsprofMode::MSPROF_MODE_PARSE,
        MsprofMode::MSPROF_MODE_QUERY,
        MsprofMode::MSPROF_MODE_EXPORT
    };
    int ret;
    for (auto modeType : modeTypeList) {
        ret = MsprofParamAdapterMgr->SetModeDefaultParams(modeType);
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(ParamsAdapterMsprofUtest, AnalysisParamsAdapt)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_EXPORT] = "on";
    cmdInfo.args[ARGS_OUTPUT] = "./result";
    cmdInfo.args[ARGS_EXPORT_ITERATION_ID] = "1";
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "csv";
    const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argsMap = {
        {ARGS_EXPORT, std::make_pair(cmdInfo, "--export=on")},
        {ARGS_OUTPUT, std::make_pair(cmdInfo, "--output=./result")},
        {ARGS_EXPORT_ITERATION_ID, std::make_pair(cmdInfo, "--iteration-id=1")},
        {ARGS_SUMMARY_FORMAT, std::make_pair(cmdInfo, "--summary-format=csv")}
    };
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MsprofParamAdapterMgr->params_ = params;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->AnalysisParamsAdapt(argsMap));
}