/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
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
using namespace ge;

class fusion_rule_manager_uttest : public testing::Test
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
                "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_rule_manager/tbe_builtin_info",
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
FusionRuleAnchorPtr CreateAnchor(int anchor_idx,
                                    const string &anchor_name,
                                    FusionRuleNodePtr owner_node,
                                    const vector<FusionRuleAnchorPtr> &peer_anchors)
{
    FusionRuleAnchorPtr anchor = make_shared<fe::FusionRuleAnchor>();
    anchor->anchor_idx_ = anchor_idx;
    anchor->anchor_name_ = anchor_name;
    anchor->owner_node_ = owner_node;
    for (size_t i = 0; i < peer_anchors.size(); ++i) {
        anchor->peer_anchors_.emplace_back(peer_anchors[i]);
    }
    for (size_t i = 0; i < anchor->peer_anchors_.size(); ++i) {
        auto peer_anchor = anchor->peer_anchors_[i].lock();
        peer_anchor->peer_anchors_.emplace_back(anchor);
    }
    return anchor;
}
FusionRuleNodePtr CreateFusionRuleNode(const string &node_name,
                                        const vector<string> &node_types,
                                        vector<int> inputs_anchor_indxs,
                                        vector<int> output_anchor_indexs,
                                        const map<string, FusionRuleAttrValuePtr> &attributes)
{
    FusionRuleNodePtr node = make_shared<fe::FusionRuleNode>();
    node->node_name_ = node_name;
    node->node_type_ = node_types;
    for (size_t i = 0; i < inputs_anchor_indxs.size(); ++i) {
        int index = inputs_anchor_indxs[i];
        string anchor_name = node_name + "_input_" + to_string(index);
        auto input_anchor = CreateAnchor(index, anchor_name, node, {});
        node->input_data_anchors_.push_back(input_anchor);
    }
    for (size_t i = 0; i < output_anchor_indexs.size(); ++i) {
        int index = output_anchor_indexs[i];
        string anchor_name = node_name + "_output_" + to_string(index);
        auto output_anchor = CreateAnchor(index, anchor_name, node, {});
        node->output_data_anchors_.push_back(output_anchor);
    }
    node->attributes_ = attributes;
    return node;
}

TEST_F(fusion_rule_manager_uttest, get_fusion_rules_by_rule_type_failed_not_init)
{
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    RuleType rule_type;
    rule_type = RuleType::CUSTOM_GRAPH_RULE;
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->GetFusionRulesByRuleType(rule_type, out_rule_vector);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(fusion_rule_manager_uttest, fusion_rule_manager_01)
{
    string file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_rule_manager/test.json";
    auto frm = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    vector<FusionRulePatternPtr> out_rule_vector = {};
    Status ret = frm->LoadFusionRule(file_path, out_rule_vector, ops_kernel_info_store_ptr_);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_succ_01)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_01)
{
    nlohmann::json json_object = { {"no_attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_02)
{
    nlohmann::json json_object = { {"attr", {"attr1"}}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_03)
{
    nlohmann::json json_object = { {"attr", ""}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_04)
{
    nlohmann::json json_object = { {"attr", "attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_05)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", {"="}}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_06)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "-"}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, attr_assignment_expression_fail_07)
{
    nlohmann::json json_object = "json_object";
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseJson(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_succ_01)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_01)
{
    nlohmann::json json_object = { {"no_attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_02)
{
    nlohmann::json json_object = { {"attr", {"attr1"}}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_03)
{
    nlohmann::json json_object = { {"attr", ""}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_04)
{
    nlohmann::json json_object = { {"attr", "attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "="}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_05)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", {"="}}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonAttr_fail_06)
{
    nlohmann::json json_object = { {"attr", "node1.attr1"}, {"value",{"node1.attr1", 18.2}}, {"expr", "-"}};
    auto attr_assign = std::make_shared<AttrAssignmentExpression>();
    Status ret = attr_assign->ParseToJsonAttr(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonEdge_succ_01)
{
    nlohmann::json json_object = {{"src", "input1"}, {"dst", "node1"}};
    auto fusion_rule_edge = std::make_shared<FusionRuleJsonEdge>();
    Status ret = fusion_rule_edge->ParseToJsonEdge(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_succ_01)
{
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {"min"}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed1)
{
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode("");
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed2)
{
    nlohmann::json json_object = {{"name", 1}, {"type", {"min"}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed3)
{
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {""}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed4)
{
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed5)
{
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {"min","max"}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonNode_failed6)
{
    nlohmann::json json_object = {{"Not_support_key", "Conv"}, {"type", {"min","max"}}};
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);
    auto fusion_rule_node = std::make_shared<FusionRuleJsonNode>();
    Status ret = fusion_rule_node->ParseToJsonNode(json_object);
    EXPECT_EQ(fe::ILLEGAL_JSON, ret);
}
TEST_F(fusion_rule_manager_uttest, ParseToJsonOuter_succ_01)
{
    nlohmann::json json_object = {{"name", "out"}, {"src", "node1"}};
    auto fusion_rule_out = std::make_shared<FusionRuleJsonOuter>();
    Status ret = fusion_rule_out->ParseToJsonOuter(json_object);
    EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(fusion_rule_manager_uttest, convert_to_attr_value_success)
{
    AttrAssignmentExpression test_object;
    std::map<string, std::vector<string>> node_map;
    FusionRuleAttr tmp_attr;
    tmp_attr.attr_name = "keep_dims";
    tmp_attr.node_name = "node";
    std::vector<string> vec{"formatAgnosticOp", "conv"};
    node_map.insert(pair<string, std::vector<string>>("node", vec));
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);

    test_object.attr_ = tmp_attr;
    test_object.value_ = make_shared<FusionRuleAttrValue>();
    test_object.tmp_value_ = {"1.1", "1.2"};
    test_object.parse_to_attr_success_ = true;
    Status ret = test_object.ConvertToAttrValue(test_object.tmp_value_, ge::GeAttrValue::VT_LIST_FLOAT, test_object.value_);
    EXPECT_EQ(ret, fe::SUCCESS);

}
TEST_F(fusion_rule_manager_uttest, convert_to_attr_value_failed)
{
    AttrAssignmentExpression test_object;
    std::map<string, std::vector<string>> node_map;
    FusionRuleAttr tmp_attr;
    tmp_attr.attr_name = "keep_dims";
    tmp_attr.node_name = "node";
    std::vector<string> vec{"formatAgnosticOp", "conv"};
    node_map.insert(pair<string, std::vector<string>>("node", vec));
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);

    test_object.attr_ = tmp_attr;
    test_object.value_ = make_shared<FusionRuleAttrValue>();
    test_object.tmp_value_ = {"1.1"};
    test_object.parse_to_attr_success_ = true;
    Status ret = test_object.ConvertToAttrValue(test_object.tmp_value_, ge::GeAttrValue::VT_BYTES, test_object.value_);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);

}
TEST_F(fusion_rule_manager_uttest, parse_to_json_attr_failed)
{
    AttrAssignmentExpression test_object;
    Status ret = test_object.ParseToJsonAttr("");
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_to_attr_value_failed)
{
    AttrAssignmentExpression test_object;
    std::map<string, std::vector<string>> node_map;
    ge::GeAttrValue::ValueType tmp_value_type;
    FusionRuleAttr tmp_attr;
    tmp_attr.attr_name = "attr";
    tmp_attr.node_name = "node";
    std::vector<string> vec{"conv", "StridedRead"};
    node_map.insert(pair<string, std::vector<string>>("node", vec));
    FusionRuleParserUtils::Instance()->SetValue(ops_kernel_info_store_ptr_);

    Status ret = test_object.ParseToAttrValue(node_map);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_to_json_pattern_failed)
{
    FusionRuleJsonPattern test_object;
    Status ret = test_object.ParseToJsonPattern("");
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, load_json_failed)
{
    FusionRuleJsonPattern test_object;
    Status ret = test_object.ParseToJsonPattern("");
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_to_origin_graph_failed)
{
    FusionRuleJsonPattern test_object;
    FusionRuleJsonGraphPtr origin_graph = make_shared<FusionRuleJsonGraph>();
    Status ret = test_object.ParseToOriginGraph("", origin_graph);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_to_fusion_graph_failed)
{
    FusionRuleJsonPattern test_object;
    FusionRuleJsonGraphPtr fusion_graph = make_shared<FusionRuleJsonGraph>();
    Status ret = test_object.ParseToFusionGraph("", fusion_graph);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed)
{
    FusionRuleJsonNode test_object;
    Status ret = test_object.ParseJson("");
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed1)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"name", 1}, {"type", {"min"}}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed2)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"name", "Conv"}, {"type", 1}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed3)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {""}}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed4)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {"min","max"}}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed5)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"name", "Conv"}, {"type", {}}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, parse_json_failed6)
{
    FusionRuleJsonNode test_object;
    nlohmann::json json_object = {{"Not_support_key", "Conv"}, {"type", {"min"}}};
    Status ret = test_object.ParseJson(json_object);
    EXPECT_EQ(ret, fe::ILLEGAL_JSON);
}
TEST_F(fusion_rule_manager_uttest, add_input_anchor_failed)
{
    FusionRuleNodeConstructor test_object;
    map<string, FusionRuleAttrValuePtr> attributes;
    FusionRuleAttrValuePtr attrvalueptr = make_shared<FusionRuleAttrValue>();
    attributes.insert(pair<string, FusionRuleAttrValuePtr> ("attr1", attrvalueptr));
    FusionRuleNodePtr test_node = CreateFusionRuleNode("test_node", {"Test"}, {}, {0}, attributes);
    Status ret = test_object.AddAttr(test_node, "attr1", attrvalueptr);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, check_node_validity_failed)
{
    FusionRuleNodeConstructor test_object;
    FusionRuleNodePtr test_node = CreateFusionRuleNode("test_node", {"Test"}, {}, {}, {});
    Status ret = test_object.CheckNodeValidity(test_node);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, check_node_validity_failed1)
{
    FusionRuleNodeConstructor test_object;
    FusionRuleNodePtr test_node = CreateFusionRuleNode("test_node", {"Test"}, {}, {}, {});
    Status ret = test_object.CheckNodeValidity(test_node);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, check_node_validity_failed2)
{
    FusionRuleNodeConstructor test_object;
    FusionRuleNodePtr node = make_shared<fe::FusionRuleNode>();
    vector<string> node_types = {"node_types"};
    node->node_name_ = "node_name";
    node->node_type_ = node_types;
    for (size_t i = 0; i < 2; ++i) {
        int index = 0;
        string anchor_name = "_input_" + to_string(index);
        auto input_anchor = CreateAnchor(index, anchor_name, node, {});
        node->input_data_anchors_.push_back(input_anchor);
    }
    for (size_t i = 0; i < 2; ++i) {
        int index = i;
        string anchor_name = "_output_" + to_string(index);
        auto output_anchor = CreateAnchor(index, anchor_name, node, {});
        node->output_data_anchors_.push_back(output_anchor);
    }
    node->attributes_ = {};
    Status ret = test_object.CheckNodeValidity(node);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, check_node_validity_failed3)
{
    FusionRuleNodeConstructor test_object;
    FusionRuleNodePtr node = make_shared<fe::FusionRuleNode>();
    vector<string> node_types = {"node_types"};
    node->node_name_ = "node_name";
    node->node_type_ = node_types;
    for (size_t i = 0; i < 2; ++i) {
        int index = i;
        string anchor_name = "_input_" + to_string(index);
        auto input_anchor = CreateAnchor(index, anchor_name, node, {});
        node->input_data_anchors_.push_back(input_anchor);
    }
    for (size_t i = 0; i < 2; ++i) {
        int index = 0;
        string anchor_name = "_output_" + to_string(index);
        auto output_anchor = CreateAnchor(index, anchor_name, node, {});
        node->output_data_anchors_.push_back(output_anchor);
    }
    node->attributes_ = {};
    Status ret = test_object.CheckNodeValidity(node);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, construct_failed)
{
    FusionRuleAnchorConstructor test_object;
    FusionRuleAnchorPtr anchor = make_shared<FusionRuleAnchor>();
    Status ret = test_object.Construct(anchor, -3, "name");
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, get_str_from_attr_alue_succ1)
{
    ge::GeAttrValue attr_value_float = GeAttrValue::CreateFrom<float>(1.1);
    auto attr_value = fe::GetStrFromAttrValue(attr_value_float);
    EXPECT_EQ(attr_value, "1.100000");
    ge::GeAttrValue attr_value_datatype = GeAttrValue::CreateFrom<ge::DataType>(ge::DataType::DT_INT4);
    attr_value = fe::GetStrFromAttrValue(attr_value_datatype);
    EXPECT_EQ(attr_value, "");
}
TEST_F(fusion_rule_manager_uttest, check_fusion_rule_pattern_failed)
{
    FusionRulePatternConstructor fusionrule_pattern;
    FusionRulePatternPtr pattern = make_shared<FusionRulePattern>();
    FusionRuleNodePtr test_node = CreateFusionRuleNode("test_node", {"Test"}, {}, {}, {});
    pattern->input_info_.push_back(test_node);
    Status ret = fusionrule_pattern.CheckFusionRulePattern(pattern);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}
TEST_F(fusion_rule_manager_uttest, check_fusion_rule_pattern_failed1)
{
    FusionRulePatternConstructor fusionrule_pattern;
    FusionRulePatternPtr pattern = make_shared<FusionRulePattern>();
    FusionRuleNodePtr test_node = CreateFusionRuleNode("test_node", {"Test"}, {}, {}, {});
    pattern->output_info_.push_back(test_node);
    Status ret = fusionrule_pattern.CheckFusionRulePattern(pattern);
    EXPECT_EQ(ret, fe::ILLEGAL_RULE);
}