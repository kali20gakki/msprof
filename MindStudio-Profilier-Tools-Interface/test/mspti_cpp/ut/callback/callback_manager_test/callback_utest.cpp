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
 * -------------------------------------------------------------------------
*/
#include "gtest/gtest.h"

#include "mspti.h"
#include "csrc/callback/callback_manager.h"

class CallbackUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

static void UserCallback(void *pUserData, msptiCallbackDomain domain,
    msptiCallbackId callbackId, const msptiCallbackData *pCallbackInfo)
{
    if (pCallbackInfo->callbackSite == MSPTI_API_ENTER) {
        printf("Enter: %s\n", pCallbackInfo->functionName);
    } else if (pCallbackInfo->callbackSite == MSPTI_API_EXIT) {
        printf("Exit: %s\n", pCallbackInfo->functionName);
    }
    if (domain == MSPTI_CB_DOMAIN_RUNTIME && callbackId == MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX) {
        printf("Domain MSPTI_CB_DOMAIN_RUNTIME, CallbackID MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX\n");
    }
}

TEST_F(CallbackUtest, CallbackExternalApiTestWithRuntimeDomain)
{
    msptiSubscriberHandle subscriber;
    EXPECT_EQ(MSPTI_SUCCESS, msptiSubscribe(&subscriber, UserCallback, nullptr));
    EXPECT_EQ(MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED, msptiSubscribe(&subscriber, UserCallback, nullptr));
    EXPECT_EQ(MSPTI_SUCCESS, msptiEnableCallback(1, subscriber, MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX));
    Mspti::Callback::CallbackManager::GetInstance()->ExecuteCallback(MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, MSPTI_API_ENTER, "rtCtxCreateEx");
    Mspti::Callback::CallbackManager::GetInstance()->ExecuteCallback(MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, MSPTI_API_EXIT, "rtCtxCreateEx");
    EXPECT_EQ(MSPTI_SUCCESS, msptiEnableCallback(0, subscriber, MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX));

    EXPECT_EQ(MSPTI_SUCCESS, msptiEnableDomain(1, subscriber, MSPTI_CB_DOMAIN_RUNTIME));
    Mspti::Callback::CallbackManager::GetInstance()->ExecuteCallback(MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, MSPTI_API_ENTER, "rtCtxCreateEx");
    Mspti::Callback::CallbackManager::GetInstance()->ExecuteCallback(MSPTI_CB_DOMAIN_RUNTIME,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX, MSPTI_API_EXIT, "rtCtxCreateEx");
    EXPECT_EQ(MSPTI_SUCCESS, msptiEnableDomain(0, subscriber, MSPTI_CB_DOMAIN_RUNTIME));
    EXPECT_EQ(MSPTI_SUCCESS, msptiUnsubscribe(subscriber));
    msptiSubscriberHandle subscriberNul{nullptr};
    EXPECT_EQ(MSPTI_SUCCESS, msptiSubscribe(&subscriberNul, nullptr, nullptr));
    EXPECT_EQ(MSPTI_SUCCESS, msptiUnsubscribe(subscriberNul));
}
