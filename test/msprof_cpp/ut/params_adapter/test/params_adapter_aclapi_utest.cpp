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
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "params_adapter_aclapi.h"
#include "errno/error_code.h"
#include "utils.h"
#include "param_validation.h"

using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::validation;

class ParamsAdapterAclapiUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterAclapiUtest, InitFailed)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    MOCKER_CPP(&ParamsAdapterAclApi::CheckListInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->Init());
}

TEST_F(ParamsAdapterAclapiUtest, DevIdToStr)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);

    uint32_t devNum = 2;
    uint32_t devList[devNum] = {
        1, 64
    };
    std::string ret = AclApiParamAdapterMgr->DevIdToStr(devNum, devList);
    std::string result = "1";
    EXPECT_EQ(result, ret);
    devList[0] = 1;
    devList[1] = 2; // 2 test num
    ret = AclApiParamAdapterMgr->DevIdToStr(devNum, devList);
    result = "1,2";
    EXPECT_EQ(result, ret);
}

TEST_F(ParamsAdapterAclapiUtest, AclApiParamAdapterInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    int ret = AclApiParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterAclapiUtest, ParamsCheckAclApi)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    
    AclApiParamAdapterMgr->Init();
    AclApiParamAdapterMgr->setConfig_ = {INPUT_CFG_COM_TRAINING_TRACE, INPUT_CFG_COM_SYS_DEVICES,
                                         INPUT_CFG_HOST_SYS, INPUT_CFG_COM_OUTPUT};
    MOCKER_CPP(&ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&ParamValidation::MsprofCheckSysDeviceValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&ParamsAdapterAclApi::CheckHostSysAclApiValid)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    int ret = AclApiParamAdapterMgr->ParamsCheckAclApi();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->ParamsCheckAclApi());
}

TEST_F(ParamsAdapterAclapiUtest, ProfTaskCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);

    struct ProfConfig apiCfg;
    apiCfg.devNums = 1;
    apiCfg.devIdList[0] = {1};
    apiCfg.dataTypeConfig = PROF_TASK_TIME_L1 | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API | PROF_OP_ATTR |
                            PROF_AICPU_TRACE | PROF_RUNTIME_API |
                            PROF_HCCL_TRACE | PROF_AICORE_METRICS | PROF_OP_ATTR;
    apiCfg.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_STORAGE_LIMIT] = "200MB";
    AclApiParamAdapterMgr->ProfTaskCfgToContainer(&apiCfg, argsArr);
}

TEST_F(ParamsAdapterAclapiUtest, ProfSystemCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    struct ProfConfig apiCfg;
    apiCfg.devNums = 1;
    apiCfg.devIdList[0] = {1};
    apiCfg.dataTypeConfig = PROF_TASK_TIME_L1 | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API | PROF_OP_ATTR |
                            PROF_AICPU_TRACE | PROF_RUNTIME_API |
                            PROF_HCCL_TRACE | PROF_AICORE_METRICS;
    apiCfg.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_SYS_IO_FREQ] = "10";
    argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ] = "10";
    argsArr[ACL_PROF_DVPP_FREQ] = "10";
    argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ] = "10";
    AclApiParamAdapterMgr->ProfSystemCfgToContainer(&apiCfg, argsArr);
    argsArr[ACL_PROF_LLC_MODE] = "read";
    AclApiParamAdapterMgr->ProfSystemCfgToContainer(&apiCfg, argsArr);
}

TEST_F(ParamsAdapterAclapiUtest, CheckHostSysAclApiValid)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    std::string cfg;
    EXPECT_EQ(false, AclApiParamAdapterMgr->CheckHostSysAclApiValid(cfg));

    cfg = "xxx";
    EXPECT_EQ(false, AclApiParamAdapterMgr->CheckHostSysAclApiValid(cfg));

    cfg = "cpu,mem";
    EXPECT_EQ(true, AclApiParamAdapterMgr->CheckHostSysAclApiValid(cfg));
}

TEST_F(ParamsAdapterAclapiUtest, GetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    ProfConfig cfg;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(nullptr, argsArr, params));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, nullptr));
    params->result_dir = "/tmp/test";
    MOCKER_CPP(&ParamsAdapterAclApi::ProfCfgToContainer)
        .stubs();
    MOCKER_CPP(&ParamsAdapterAclApi::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapterAclApi::ParamsCheckAclApi)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapter::ComCfgCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapter::TransToParam)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
    EXPECT_EQ(PROFILING_FAILED, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
    EXPECT_EQ(PROFILING_SUCCESS, AclApiParamAdapterMgr->GetParamFromInputCfg(&cfg, argsArr, params));
}

TEST_F(ParamsAdapterAclapiUtest, ProfCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_VOID(AclApiParamAdapterMgr, ParamsAdapterAclApi);
    ProfConfig cfg;
    cfg.dataTypeConfig = PROF_ACL_API_MASK | PROF_TASK_TIME_L0_MASK;
    cfg.devNums = 0;
    cfg.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ] = "1";
    argsArr[ACL_PROF_AIV_METRICS] = "x";
    argsArr[ACL_PROF_HOST_SYS] = "x";
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    AclApiParamAdapterMgr->ProfCfgToContainer(&cfg, argsArr);
    EXPECT_EQ("on", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_COM_ASCENDCL]);
    EXPECT_EQ("l0", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_COM_TASK_TIME]);
    EXPECT_EQ("on", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AI_VECTOR]);
    EXPECT_EQ("x", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_HOST_SYS]);
    EXPECT_EQ("on", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM]);
    EXPECT_EQ("1", AclApiParamAdapterMgr->paramContainer_[INPUT_CFG_COM_SYS_HARDWARE_MEM_FREQ]);
}