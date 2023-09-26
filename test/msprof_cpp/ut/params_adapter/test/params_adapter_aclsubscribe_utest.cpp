/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Description: UT for acl Subscribe params adapter
 * Author: Huawei Technologies Co., Ltd.
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "params_adapter_aclsubscribe.h"
#include "errno/error_code.h"

using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::ParamsAdapter;

class ParamsAdapterAclSubscribeUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(ParamsAdapterAclSubscribeUtest, AclApiParamAdapterInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclSubscribe> AclSubscribeParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclSubscribeParamAdapterMgr, ParamsAdapterAclSubscribe);
    int ret = AclSubscribeParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterAclSubscribeUtest, GetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclSubscribe> AclSubscribeParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclSubscribeParamAdapterMgr, ParamsAdapterAclSubscribe);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    ProfSubscribeConfig config;
    config.aicoreMetrics = PROF_AICORE_PIPE_UTILIZATION;
    uint64_t dataTypeConfig = PROF_TASK_TIME_L1_MASK | PROF_KEYPOINT_TRACE_MASK | PROF_L2CACHE_MASK |
        PROF_AICORE_METRICS_MASK | PROF_AIV_METRICS_MASK;
    EXPECT_EQ(PROFILING_FAILED, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(&config, dataTypeConfig, nullptr));
    EXPECT_EQ(PROFILING_FAILED, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(nullptr, dataTypeConfig, params));
    MOCKER_CPP(&ParamsAdapterAclSubscribe::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapterAclSubscribe::GenAclSubscribeContainer)
        .stubs();
    MOCKER_CPP(&ParamsAdapter::PlatformAdapterInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamsAdapter::TransToParam)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(&config, dataTypeConfig, params));
    EXPECT_EQ(PROFILING_FAILED, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(&config, dataTypeConfig, params));
    EXPECT_EQ(PROFILING_FAILED, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(&config, dataTypeConfig, params));
    EXPECT_EQ(PROFILING_SUCCESS, AclSubscribeParamAdapterMgr->GetParamFromInputCfg(&config, dataTypeConfig, params));
}
