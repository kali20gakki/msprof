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
#include "ops_store/ops_kernel_manager.h"
#include "fusion_rule_manager/fusion_rule_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "fusion_rule_manager/fusion_rule_parser/fusion_rule_parser_utils.h"
#include "common/fe_log.h"
#include "common/configuration.h"
#undef private
#undef protected

using namespace std;
using namespace fe;

class fusion_rule_manager_stest : public testing::Test
{
protected:
    void SetUp()
    {
        op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
        ops_kernel_info_store_ptr_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
        TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
        op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
        std::map<std::string, std::string> options;
        ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
        FEOpsStoreInfo tbe_custom {
                6,
                "tbe-custom",
                EN_IMPL_HW_TBE,
                "./air/test/engines/nneng/st/testcase/fusion_rule_manager/tbe_builtin_info",
                ""};
        vector<FEOpsStoreInfo> store_info;
        store_info.emplace_back(tbe_custom);
        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
        OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

        OpsKernelManager::Instance(AI_CORE_NAME).Initialize();
        ops_kernel_info_store_ptr_->Initialize(options);
    }

    void TearDown()
    {
    }
    FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
};

TEST_F(fusion_rule_manager_stest, get_fusion_rules_by_rule_type_failed_not_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    RuleType rule_type;
    rule_type = RuleType::CUSTOM_GRAPH_RULE;
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->GetFusionRulesByRuleType(rule_type, out_rule_vector);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(fusion_rule_manager_stest, fusion_rule_manager_01)
{
    std::cout << "=======================================================================" << std::endl;
    string file_path = "./air/test/engines/nneng/st/testcase/fusion_rule_manager/test.json";
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->LoadFusionRule(file_path, out_rule_vector, ops_kernel_info_store_ptr_);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_stest, attr_assignment_expression_succ_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"no_attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_02)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", {"attr1"}}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_03)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", ""}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_04)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_05)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", {"="}}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, attr_assignment_expression_fail_06)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "-"}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_succ_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"no_attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_02)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", {"attr1"}}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_03)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", ""}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_04)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_05)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", {"="}}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonAttr_fail_06)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "-"}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonEdge_succ_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = {{"src", "input1"}, {"dst", "node1"}};
    auto fusion_rule_edge = std::make_shared<FusionRuleJsonEdge>();
    Status ret = fusion_rule_edge->ParseToJsonEdge(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonNode_succ_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {"min"}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_stest, ParseToJsonOuter_succ_01)
{
    std::cout << "=======================================================================" << std::endl;
    nlohmann::json json_object = {{"name", "out"}, {"src", "node1"}};
    auto fusion_rule_out = std::make_shared<FusionRuleJsonOuter>();
    Status ret = fusion_rule_out->ParseToJsonOuter(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_stest, initialize_success)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    frm->init_flag_ = true;
    Status ret = frm->Initialize(fe::AI_CORE_NAME);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_stest, finalize_success_not_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    Status ret = frm->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_stest, finalize_success_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    frm->init_flag_ = true;
    Status ret = frm->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_stest, get_fusion_rules_by_rule_type_failed_not_init1)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    RuleType rule_type;
    rule_type = RuleType::CUSTOM_GRAPH_RULE;
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->GetFusionRulesByRuleType(rule_type, out_rule_vector);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(fusion_rule_manager_stest, get_fusion_rules_by_rule_type_failed_not_init2)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    frm->init_flag_ = false;
    RuleType rule_type;
    std::string rule_name = "rule_name";
    Status ret = frm->RunGraphFusionRuleByType(*graph, rule_type, rule_name);
    EXPECT_EQ(fe::FAILED, ret);
}
