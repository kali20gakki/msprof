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
#include "param_validation.h"
#include "utils.h"

using namespace Collector::Dvvp::ParamsAdapter;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;

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

    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, analysis::dvvp::proto::ProfAclConfig);
    aclCfg->set_storage_limit("200MB");
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
}
TEST_F(ParamsAdapterAcljsonUtest, GenAclJsonContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapterAclJson> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, ParamsAdapterAclJson);
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, analysis::dvvp::proto::ProfAclConfig);
    aclCfg->set_storage_limit("200MB");
    aclCfg->set_l2("on");
    AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
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