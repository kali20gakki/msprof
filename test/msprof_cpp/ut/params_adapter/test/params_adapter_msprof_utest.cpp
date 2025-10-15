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
#include <unistd.h>

#include "params_adapter_msprof.h"
#include "param_validation.h"
#include "errno/error_code.h"
#include "input_parser.h"
#include "utils/utils.h"


using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace analysis::dvvp::common::utils;

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

TEST_F(ParamsAdapterMsprofUtest, ParamsCheckWillReturnFailWhenRunFail)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    // ParamsCheckMsprof fail
    MsprofParamAdapterMgr->msprofConfig_.push_back(INPUT_CFG_COM_AI_CORE);
    MsprofParamAdapterMgr->setConfig_.insert(INPUT_CFG_COM_AI_CORE);
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "xxx";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheck());

    // ComCfgCheck fail
    MsprofParamAdapterMgr->msprofConfig_.clear();
    MsprofParamAdapterMgr->commonConfig_.push_back(INPUT_CFG_COM_ASCENDCL);
    MsprofParamAdapterMgr->setConfig_.clear();
    MsprofParamAdapterMgr->setConfig_.insert(INPUT_CFG_COM_ASCENDCL);
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_ASCENDCL] = "xxx";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheck());

    // ParamsCheckDynProf fail
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] = "on";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DELAY] = "1";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_ASCENDCL] = "on";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheck());

    // CheckAnalysisParams fail
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] = "off";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_PARSE] = "xxx";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheck());

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_PARSE].clear();
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->ParamsCheck());
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
        INPUT_CFG_HOST_SYS_USAGE,
        INPUT_CFG_MSPROF_DELAY,
        INPUT_CFG_MSPROF_DURATION
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
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysUsageValid)
            .stubs()
            .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckDelayAndDurationValid)
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
    std::shared_ptr<ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, ProfileParams);
    MSVP_MAKE_SHARED0_VOID(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->params_ = params;
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

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] = "on";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DELAY] = "1";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheckDynProf());

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DYNAMIC] = "on";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MSPROF_DURATION] = "1";
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->ParamsCheckDynProf());
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsApp)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->SetDefaultParamsApp();
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsAppWillSetAicoreWhenSetTaskTime)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME] = "on";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("on", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME] = "l1";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("on", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME] = "l0";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME] = "off";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE] = "on";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME] = "off";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("on", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_CORE]);
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsAppWillSetAicModeWhenSetOrNotSetAicMode)
{
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("task-based", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_MODE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_MODE] = "sample-based";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("sample-based", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_MODE]);
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsAppWillSetAicMetricsWhenSetOrNotSetAicMetrics)
{
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->platformType_ == PlatformType::CHIP_V4_1_0;
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("PipeUtilization", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS] = "Memory";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("Memory", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS]);
}

TEST_F(ParamsAdapterMsprofUtest, SetDefaultParamsAppWillSetAivModeWhenSetOrNotSetAivMode)
{
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("task-based", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_MODE]);

    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_MODE] = "sample-based";
    MsprofParamAdapterMgr->SetDefaultParamsApp();
    EXPECT_EQ("sample-based", MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_MODE]);
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

TEST_F(ParamsAdapterMsprofUtest, SplitAppPath)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::string appParam;
    MsprofParamAdapterMgr->SplitAppPath(appParam);
    appParam = "./main 1";
    MsprofParamAdapterMgr->SplitAppPath(appParam);
    appParam = "/bin/bash test.sh 1";
    MsprofParamAdapterMgr->SplitAppPath(appParam);
}

TEST_F(ParamsAdapterMsprofUtest, CheckMsprofModeWillReturnFailWhenNotSetMsprofMode)
{
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->CheckMsprofMode(argvMap));
}

TEST_F(ParamsAdapterMsprofUtest, CreateCfgMap)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    MsprofParamAdapterMgr->CreateCfgMap();
}

TEST_F(ParamsAdapterMsprofUtest, SetAnalysisParams)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MsprofParamAdapterMgr->params_ = params;

    // not set paramcontainer
    MsprofParamAdapterMgr->SetAnalysisParams();
    EXPECT_EQ("", MsprofParamAdapterMgr->params_->result_dir);

    // set paramcontainer
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_OUTPUT] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_PYTHON_PATH] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_PARSE] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_ANALYZE] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_RULE] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_QUERY] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_EXPORT] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_EXPORT_TYPE] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_CLEAR] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_ITERATION_ID] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_MODEL_ID] = "x";
    MsprofParamAdapterMgr->paramContainer_[INPUT_CFG_COM_REPORTS] = "x";
    MsprofParamAdapterMgr->SetAnalysisParams();
    EXPECT_EQ("x", MsprofParamAdapterMgr->params_->result_dir);
}

TEST_F(ParamsAdapterMsprofUtest, SystemToolsIsExist)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);

    // not set host_sys
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->SystemToolsIsExist());

    // set host_sys
    MsprofParamAdapterMgr->setConfig_.insert(INPUT_CFG_HOST_SYS);
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysToolsExit)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->SystemToolsIsExist());
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->SystemToolsIsExist());
}

TEST_F(ParamsAdapterMsprofUtest, GenMsprofContainerWillReturnFailWhenSetInvalidSwitch)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);

    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_EXPORT] = "on";
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap = {
        {ARGS_EXPORT, std::make_pair(cmdInfo, "--export=on")},
    };
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::BlackSwitchCheck)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GenMsprofContainer(argvMap));
}

TEST_F(ParamsAdapterMsprofUtest, GenMsprofContainerWillReturnFailWhenInputEmptySwitchValue)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);

    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_EXPORT] = "\0";
    cmdInfo.args[ARGS_OUTPUT] = "xx";
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap = {
        {ARGS_EXPORT, std::make_pair(cmdInfo, "--export=\0")},
        {ARGS_OUTPUT, std::make_pair(cmdInfo, "--output=xx")},
    };
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::BlackSwitchCheck)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GenMsprofContainer(argvMap));
}

TEST_F(ParamsAdapterMsprofUtest, GenMsprofContainerWillReturnSuccWhenInputValidSwitch)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);

    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_EXPORT] = "on";
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap = {
        {ARGS_EXPORT, std::make_pair(cmdInfo, "--export=on")},
    };
    std::string emptyPath = "";
    std::string notEmptyPath = "path";
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::BlackSwitchCheck)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(emptyPath))
        .then(returnValue(notEmptyPath));

    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GenMsprofContainer(argvMap));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GenMsprofContainer(argvMap));
}

TEST_F(ParamsAdapterMsprofUtest, CheckAnalysisConfig)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::vector<MsprofArgsType> types = {
        ARGS_OUTPUT, ARGS_REPORTS, ARGS_PYTHON_PATH, ARGS_EXPORT_TYPE,
        ARGS_SUMMARY_FORMAT, ARGS_ANALYZE, ARGS_RULE, ARGS_PARSE, ARGS_QUERY,
        ARGS_EXPORT, ARGS_CLEAR, ARGS_EXPORT_ITERATION_ID, ARGS_EXPORT_MODEL_ID,
    };
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckAnalysisOutputIsPathValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckReportsJsonIsPathValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckPythonPathIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportTypeIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportSummaryFormatIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidInputCfgSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidAnalyzeRuleSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportIdIsValid)
        .stubs()
        .will(returnValue(true));
    for (auto &type : types) {
        EXPECT_EQ(true, MsprofParamAdapterMgr->CheckAnalysisConfig(type, "x"));
    }
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportIdIsValid)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(false, MsprofParamAdapterMgr->CheckAnalysisConfig(ARGS_EXPORT_MODEL_ID, "x"));
}

TEST_F(ParamsAdapterMsprofUtest, GetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, nullptr));

    // check msprof mode fail
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));

    struct MsprofCmdInfo exportCmdInfo = {{nullptr}};
    exportCmdInfo.args[ARGS_EXPORT] = "on";
    argvMap.insert(std::make_pair(ARGS_EXPORT, std::make_pair(exportCmdInfo, "--export=on")));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));

    struct MsprofCmdInfo parseCmdInfo = {{nullptr}};
    parseCmdInfo.args[ARGS_PARSE] = "on";
    argvMap.clear();
    argvMap.insert(std::make_pair(ARGS_PARSE, std::make_pair(parseCmdInfo, "--parse=on")));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));

    struct MsprofCmdInfo queryCmdInfo = {{nullptr}};
    queryCmdInfo.args[ARGS_QUERY] = "on";
    argvMap.clear();
    argvMap.insert(std::make_pair(ARGS_QUERY, std::make_pair(queryCmdInfo, "--parse=on")));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));

    struct MsprofCmdInfo analyzeCmdInfo = {{nullptr}};
    analyzeCmdInfo.args[ARGS_ANALYZE] = "on";
    argvMap.clear();
    argvMap.insert(std::make_pair(ARGS_ANALYZE, std::make_pair(analyzeCmdInfo, "--parse=on")));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
}

TEST_F(ParamsAdapterMsprofUtest, GetParamFromInputCfgWillReturnFailWhenRunFail1)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);

    // Init fail
    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_APPLICATION] = "xxx";
    argvMap.insert(std::make_pair(ARGS_APPLICATION, std::make_pair(cmdInfo, "--application=xxx")));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
    GlobalMockObject::verify();

    // GenMsprofContainer fail
    struct MsprofCmdInfo invalidcmdInfo = {{nullptr}};
    invalidcmdInfo.args[ARGS_APPLICATION] = "\0";
    argvMap.clear();
    argvMap.insert(std::make_pair(ARGS_APPLICATION, std::make_pair(invalidcmdInfo, "--application=\0")));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));

    // PlatformAdapterInit fail
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    argvMap.clear();
    argvMap.insert(std::make_pair(ARGS_APPLICATION, std::make_pair(cmdInfo, "--application=xxx")));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
}

TEST_F(ParamsAdapterMsprofUtest, GetParamFromInputCfgWillReturnFailWhenRunFail2)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);

    // ComCfgCheck fail
    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_APPLICATION] = "xxx";
    argvMap.insert(std::make_pair(ARGS_APPLICATION, std::make_pair(cmdInfo, "--application=xxx")));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::ComCfgCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
}

TEST_F(ParamsAdapterMsprofUtest, GetParamFromInputCfgWillReturnFailWhenRunFail3)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    // SystemToolsIsExist fail
    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_APPLICATION] = "xxx";
    cmdInfo.args[ARGS_HOST_SYS] = "cpu";
    argvMap.insert(std::make_pair(ARGS_APPLICATION, std::make_pair(cmdInfo, "--application=xxx")));
    argvMap.insert(std::make_pair(ARGS_HOST_SYS, std::make_pair(cmdInfo, "--host-sys=cpu")));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::ComCfgCheck)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MsprofParamAdapterMgr->setConfig_.insert(INPUT_CFG_HOST_SYS);
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysToolsExit)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::TransToParam)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->GetParamFromInputCfg(argvMap, params));
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
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->SetModeDefaultParams(MsprofMode::MSPROF_MODE_INVALID));
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
    Utils::CreateDir("./result");
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MsprofParamAdapterMgr->params_ = params;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofParamAdapterMgr->AnalysisParamsAdapt(argsMap));
    Utils::RemoveDir("./result");
}

TEST_F(ParamsAdapterMsprofUtest, AnalysisParamsAdaptSoftLink)
{
    // 测试软链接
    GlobalMockObject::verify();

    const char* symlinkPath = "./result_symlink";
    const char* outputPath = "./result";

    std::shared_ptr<ParamsAdapterMsprof> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, ParamsAdapterMsprof);
    struct MsprofCmdInfo cmdInfo = {{nullptr}};
    cmdInfo.args[ARGS_EXPORT] = "on";
    cmdInfo.args[ARGS_OUTPUT] = "./result_symlink";
    cmdInfo.args[ARGS_EXPORT_ITERATION_ID] = "1";
    cmdInfo.args[ARGS_SUMMARY_FORMAT] = "csv";

    const std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argsMap = {
        {ARGS_EXPORT, std::make_pair(cmdInfo, "--export=on")},
        {ARGS_OUTPUT, std::make_pair(cmdInfo, "--output=./result_symlink")},
        {ARGS_EXPORT_ITERATION_ID, std::make_pair(cmdInfo, "--iteration-id=1")},
        {ARGS_SUMMARY_FORMAT, std::make_pair(cmdInfo, "--summary-format=csv")}
    };
    Utils::CreateDir("./result");

    unlink(symlinkPath);
    symlink(outputPath, symlinkPath); // 创建软链接

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);

    MsprofParamAdapterMgr->params_ = params;
    EXPECT_EQ(PROFILING_FAILED, MsprofParamAdapterMgr->AnalysisParamsAdapt(argsMap));

    unlink(symlinkPath);
    Utils::RemoveDir("./result_symlink");
    Utils::RemoveDir("./result");
}