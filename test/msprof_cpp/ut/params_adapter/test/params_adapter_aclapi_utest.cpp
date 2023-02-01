/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for acl API params adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */
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
    
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    AclApiParamAdapterMgr->Init();
    MOCKER_CPP(&ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&ParamValidation::MsprofCheckSysDeviceValid)
        .stubs()
        .will(returnValue(true));
    int ret = AclApiParamAdapterMgr->ParamsCheckAclApi();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(ParamsAdapterAclapiUtest, ProfTaskCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclApi> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, ParamsAdapterAclApi);

    struct ProfConfig apiCfg;
    apiCfg.devNums = 1;
    apiCfg.devIdList[0] = {1};
    apiCfg.dataTypeConfig = PROF_TASK_TIME | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API |
                            PROF_AICPU_TRACE | PROF_RUNTIME_API |
                            PROF_HCCL_TRACE | PROF_AICORE_METRICS;
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
    apiCfg.dataTypeConfig = PROF_TASK_TIME | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API |
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
