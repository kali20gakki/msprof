/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for acl.json params adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "params_adapter_acljson.h"
#include "errno/error_code.h"
#include "message/prof_json_config.h"
#include "param_validation.h"
#include "utils.h"

using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::message;

class ParamsAdapterAcljsonUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterAcljsonUtest, AclJsonParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    int ret = AclJsonParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    SHARED_PTR_ALIA<ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, ProfAclConfig);
    aclCfg->storageLimit = "200MB";
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
}
TEST_F(ParamsAdapterAcljsonUtest, GenAclJsonContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, ProfAclConfig);
    aclCfg->storageLimit = "200MB";
    aclCfg->l2 = "on";
    aclCfg->instrProfilingFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    aclCfg->sysHardwareMemFreq = 0;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]);
    aclCfg->sysHardwareMemFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("1", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]);
    aclCfg->sysIoSamplingFreq = 0;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ]);
    aclCfg->sysIoSamplingFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("1", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_IO_FREQ]);
    aclCfg->sysInterconnectionFreq = 0;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]);
    aclCfg->sysInterconnectionFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("1", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_INTERCONNECTION_FREQ]);
    aclCfg->dvppFreq = 0;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_DVPP_FREQ]);
    aclCfg->dvppFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("1", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_COM_DVPP_FREQ]);
    aclCfg->hostSysUsageFreq = 0;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ]);
    aclCfg->hostSysUsageFreq = 1;
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ("1", AclJsonParamAdapterMgr->paramContainer_[INPUT_CFG_HOST_SYS_USAGE_FREQ]);
}

TEST_F(ParamsAdapterAcljsonUtest, ParamsCheckAclJson)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    AclJsonParamAdapterMgr->Init();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapterAclJson::CheckHostSysAclJsonValid)
        .stubs()
        .will(returnValue(true));
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    for (auto cfg : AclJsonParamAdapterMgr->aclJsonConfig_) {
        AclJsonParamAdapterMgr->setConfig_.insert(cfg);
        int ret = AclJsonParamAdapterMgr->ParamsCheckAclJson();
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(ParamsAdapterAcljsonUtest, SetAclJsonContainerDefaultValue)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    AclJsonParamAdapterMgr->setConfig_.insert(INPUT_CFG_COM_AI_VECTOR);
    AclJsonParamAdapterMgr->SetAclJsonContainerDefaultValue();
}

TEST_F(ParamsAdapterAcljsonUtest, AclJsonSetOutputDir)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    
    std::string output;
    std::string ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    std::string result1 = analysis::dvvp::common::utils::Utils::GetSelfPath();
    size_t pos = result1.rfind(MSVP_SLASH);
        if (pos != std::string::npos) {
            result1 = result1.substr(0, pos + 1);
        }
    EXPECT_EQ(result1, ret);

    std::string result2 = analysis::dvvp::common::utils::Utils::GetSelfPath();
    result2 = analysis::dvvp::common::utils::Utils::DirName(result2) + "/";
    output = "/var";
    ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(result2, ret);

    Utils::RemoveDir("/tmp/acljsonOutput");
    output = "/tmp/acljsonOutput";
    ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(output, ret);
    Utils::RemoveDir(output);
}

TEST_F(ParamsAdapterAcljsonUtest, CheckHostSysAclJsonValid)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);

    std::string cfg;
    EXPECT_EQ(false, AclJsonParamAdapterMgr->CheckHostSysAclJsonValid(cfg));

    cfg = "xxx";
    EXPECT_EQ(false, AclJsonParamAdapterMgr->CheckHostSysAclJsonValid(cfg));

    cfg = "cpu,mem";
    EXPECT_EQ(true, AclJsonParamAdapterMgr->CheckHostSysAclJsonValid(cfg));
}

TEST_F(ParamsAdapterAcljsonUtest, GetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb;
    MSVP_MAKE_SHARED0_VOID(inputCfgPb, ProfAclConfig);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(nullptr, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, nullptr));
    MOCKER_CPP(&ParamsAdapterAclJson::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapterAclJson::CheckInstrAndTaskParamBothSet)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    MOCKER_CPP(&ParamsAdapterAclJson::GenAclJsonContainer)
        .stubs();
    MOCKER_CPP(&ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapterAclJson::ParamsCheckAclJson)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapter::ComCfgCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapterAclJson::SetAclJsonContainerDefaultValue)
        .stubs();
    MOCKER_CPP(&ParamsAdapter::TransToParam)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_FAILED, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
    EXPECT_EQ(PROFILING_SUCCESS, AclJsonParamAdapterMgr->GetParamFromInputCfg(inputCfgPb, params));
}

TEST_F(ParamsAdapterAcljsonUtest, CheckInstrAndTaskParamBothSetWillReturnFalseWhenNotSetInstrSwitch)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_VOID(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb;
    MSVP_MAKE_SHARED0_VOID(inputCfgPb, ProfAclConfig);
    inputCfgPb->instrProfilingFreq = 0;
    EXPECT_EQ(false, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
}

TEST_F(ParamsAdapterAcljsonUtest, CheckInstrAndTaskParamBothSetWillFalseWhenSetInstrSwitchAndNotSetTaskSwitch)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_VOID(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb;
    MSVP_MAKE_SHARED0_VOID(inputCfgPb, ProfAclConfig);
    inputCfgPb->instrProfilingFreq = 1;
    EXPECT_EQ(false, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
}

TEST_F(ParamsAdapterAcljsonUtest, CheckInstrAndTaskParamBothSetWillReturnTrueWhenSetInstrSwitchAndSetTaskSwitch)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_VOID(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<ProfAclConfig> inputCfgPb;
    MSVP_MAKE_SHARED0_VOID(inputCfgPb, ProfAclConfig);
    inputCfgPb->instrProfilingFreq = 1;
    inputCfgPb->taskTime = "on";
    EXPECT_EQ(true, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
    inputCfgPb->taskTime = "l0";
    EXPECT_EQ(true, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
    inputCfgPb->taskTime = "l1";
    EXPECT_EQ(true, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
    inputCfgPb->taskTime = "off";
    inputCfgPb->aicpu = "on";
    EXPECT_EQ(true, AclJsonParamAdapterMgr->CheckInstrAndTaskParamBothSet(inputCfgPb));
}