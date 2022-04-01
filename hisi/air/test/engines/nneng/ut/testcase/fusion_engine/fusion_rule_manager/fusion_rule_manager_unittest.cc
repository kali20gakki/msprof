/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#define protected public
#define private public

#include "fusion_rule_manager/fusion_rule_manager.h"
#include "common/fe_log.h"
#include "common/configuration.h"

#undef private
#undef protected

using namespace std;
using namespace fe;

class fusion_rule_manager_unittest : public testing::Test
{
protected:
    void SetUp()
    {
        op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
        ops_kernel_info_store_ptr_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    }

    void TearDown()
    {
    }
    FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
};

TEST_F(fusion_rule_manager_unittest, initialize_success)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    frm->init_flag_ = true;
    Status ret = frm->Initialize(fe::AI_CORE_NAME);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_unittest, finalize_success_not_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    Status ret = frm->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_unittest, finalize_success_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    frm->init_flag_ = true;
    Status ret = frm->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_unittest, get_fusion_rules_by_rule_type_failed_not_init1)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    RuleType rule_type;
    rule_type = RuleType::CUSTOM_GRAPH_RULE;
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->GetFusionRulesByRuleType(rule_type, out_rule_vector);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(fusion_rule_manager_unittest, get_fusion_rules_by_rule_type_failed_not_init2)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    frm->init_flag_ = false;
    RuleType rule_type;
    std::string rule_name = "rule_name";
    Status ret = frm->RunGraphFusionRuleByType(*graph, rule_type, rule_name);
    EXPECT_EQ(fe::FAILED, ret);
}

