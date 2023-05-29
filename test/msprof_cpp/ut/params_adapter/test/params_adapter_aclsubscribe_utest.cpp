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