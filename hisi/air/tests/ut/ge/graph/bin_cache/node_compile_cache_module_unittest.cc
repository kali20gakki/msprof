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
#include "graph/ops_stub.h"
#define private public
#include "graph/bin_cache/node_compile_cache_module.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/utils/op_desc_utils.h"

using namespace ge;

class UtestCcm : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestCcm, testAddCompileCache) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node = builder.AddNode("Data", "Data", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  AttrUtils::SetInt(op_desc, "index", 0);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);
}

TEST_F(UtestCcm, testAddSameCompileCache) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node = builder.AddNode("Data", "Data", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  AttrUtils::SetInt(op_desc, "index", 0);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);
  auto same_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(same_item, nullptr);
  ASSERT_EQ(add_item->GetCacheItemId(), same_item->GetCacheItemId());
}

TEST_F(UtestCcm, testAddDifferentCompileCache) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node = builder.AddNode("Data", "Data", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  AttrUtils::SetInt(op_desc, "index", 0);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);

  auto node2 = builder.AddNode("Data", "Data", 1, 1);
  ASSERT_NE(node2, nullptr);
  auto op_desc2 = node2->GetOpDesc();
  ASSERT_NE(op_desc2, nullptr);
  AttrUtils::SetInt(op_desc2, "index", 1);
  auto diff_item = ccm.AddCompileCache(node2, item);
  ASSERT_NE(diff_item, nullptr);
  ASSERT_NE(add_item->GetCacheItemId(), diff_item->GetCacheItemId());
}

TEST_F(UtestCcm, testFindCompileCacheSuccess) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node = builder.AddNode("TestAllAttr", "TestAllAttr", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  op_desc->AddOptionalInputDesc("option_input", GeTensorDesc(GeShape(), FORMAT_RESERVED, DT_UNDEFINED));
  ASSERT_EQ(op_desc->GetAllInputsSize(), 2);
  ASSERT_EQ(op_desc->GetInputsSize(), 1);
  ASSERT_NE(op_desc->MutableInputDesc(0), nullptr);
  ASSERT_EQ(op_desc->MutableInputDesc(1), nullptr);
  AttrUtils::SetInt(op_desc, "test_int", 0);
  AttrUtils::SetStr(op_desc, "test_str", "");
  AttrUtils::SetBool(op_desc, "test_bool", false);
  AttrUtils::SetFloat(op_desc, "test_float", 0.0);
  AttrUtils::SetDataType(op_desc, "test_dt", DT_FLOAT);
  std::vector<DataType> val_list_dt{DT_FLOAT};
  AttrUtils::SetListDataType(op_desc, "test_list_dt", val_list_dt);
  std::vector<bool> val_list_bool{true};
  AttrUtils::SetListBool(op_desc, "test_list_bool", val_list_bool);
  std::vector<int64_t> val_list_int{1,2};
  AttrUtils::SetListInt(op_desc, "test_list_int", val_list_int);
  std::vector<float> val_list_float{1.0, 2.0};
  AttrUtils::SetListFloat(op_desc, "test_list_float", val_list_float);
  std::vector<std::string> val_list_string{"1", "2"};
  AttrUtils::SetListStr(op_desc, "test_list_string", val_list_string);
  std::vector<std::vector<int64_t>> val_list_list_int{{1,2}};
  AttrUtils::SetListListInt(op_desc, "test_list_list_int", val_list_list_int);

  NamedAttrs name_attr;
  AttrUtils::SetNamedAttrs(op_desc, "test_name_attr", name_attr);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);
  auto find_item = ccm.FindCompileCache(node);
  ASSERT_NE(find_item, nullptr);
  ASSERT_EQ(add_item->GetCacheItemId(), find_item->GetCacheItemId());
}

TEST_F(UtestCcm, testFindCompileCacheSuccessForFusionOp) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  builder.graph_->SetGraphID(1);
  builder.graph_->SetSessionID(1);
  auto node = builder.AddNode("fusion_op", "fusion_op", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  ASSERT_EQ(ge::AttrUtils::SetGraph(op_desc, "_original_fusion_graph", builder.graph_), true);

  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);
  auto find_item = ccm.FindCompileCache(node);
  ASSERT_NE(find_item, nullptr);
  ASSERT_EQ(add_item->GetCacheItemId(), find_item->GetCacheItemId());
}

TEST_F(UtestCcm, testFindCompileCacheFailBecauseGraphIdForFusionOp) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  builder.graph_->SetGraphID(1);
  builder.graph_->SetSessionID(1);
  auto node = builder.AddNode("fusion_op", "fusion_op", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  ASSERT_EQ(ge::AttrUtils::SetGraph(op_desc, "_original_fusion_graph", builder.graph_), true);

  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);

  builder.graph_->SetGraphID(2);
  auto find_item = ccm.FindCompileCache(node);
  ASSERT_EQ(find_item, nullptr);
}

TEST_F(UtestCcm, testFindCompileCacheFailBecauseSessionIdForFusionOp) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  builder.graph_->SetGraphID(1);
  builder.graph_->SetSessionID(1);
  auto node = builder.AddNode("fusion_op", "fusion_op", 1, 1);
  ASSERT_NE(node, nullptr);
  auto op_desc = node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);
  ASSERT_EQ(ge::AttrUtils::SetGraph(op_desc, "_original_fusion_graph", builder.graph_), true);

  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);

  builder.graph_->SetSessionID(2);
  auto find_item = ccm.FindCompileCache(node);
  ASSERT_EQ(find_item, nullptr);
}

TEST_F(UtestCcm, testFindCompileCacheSuccessForConstInput) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1);
  ASSERT_NE(data, nullptr);
  auto const_node = builder.AddNode("const", "Const", 0, 1);
  ASSERT_NE(const_node, nullptr);
  auto const_opdesc = const_node->GetOpDesc();
  
  uint8_t val = 1;
  auto const_tensor = std::make_shared<GeTensor>(GeTensorDesc(), &val, sizeof(val));
  ge::AttrUtils::SetTensor(const_opdesc, "value", const_tensor);
  auto add_node = builder.AddNode("add", "Add", 2, 1);
  ASSERT_NE(add_node, nullptr);
  builder.AddDataEdge(data, 0, add_node, 0);
  builder.AddDataEdge(const_node, 0, add_node, 1);
  auto op_desc = add_node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);

  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(add_node, item);
  ASSERT_NE(add_item, nullptr);

  auto find_item = ccm.FindCompileCache(add_node);
  ASSERT_NE(find_item, nullptr);
  ASSERT_EQ(find_item, add_item);
}

TEST_F(UtestCcm, testFindCompileCacheFailBecauseConstInput) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1);
  ASSERT_NE(data, nullptr);
  auto const_node = builder.AddNode("const", "Const", 0, 1);
  ASSERT_NE(const_node, nullptr);
  auto const_opdesc = const_node->GetOpDesc();
  ASSERT_NE(const_opdesc, nullptr);
  
  uint8_t val = 1;
  auto const_tensor = std::make_shared<GeTensor>(GeTensorDesc(), &val, sizeof(val));
  ge::AttrUtils::SetTensor(const_opdesc, "value", const_tensor);
  auto add_node = builder.AddNode("add", "Add", 2, 1);
  ASSERT_NE(add_node, nullptr);
  builder.AddDataEdge(data, 0, add_node, 0);
  builder.AddDataEdge(const_node, 0, add_node, 1);
  auto op_desc = add_node->GetOpDesc();
  ASSERT_NE(op_desc, nullptr);

  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto add_item = ccm.AddCompileCache(add_node, item);
  ASSERT_NE(add_item, nullptr);

  uint8_t val_another = 2;
  auto const_tensor_another = std::make_shared<GeTensor>(GeTensorDesc(), &val_another, sizeof(val_another));
  ge::AttrUtils::SetTensor(const_opdesc, "value", const_tensor_another);
  auto find_item = ccm.FindCompileCache(add_node);
  ASSERT_EQ(find_item, nullptr);
}

TEST_F(UtestCcm, testFindCompileCacheFail) {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto node = builder.AddNode("Data", "Data", 1, 1);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto ret = ccm.FindCompileCache(node);
  ASSERT_EQ(ret, nullptr);
}

TEST_F(UtestCcm, testGetAttrSizeStr) {
  std::string str_value("1234");
  auto any_value = ge::GeAttrValue::CreateFrom<std::string>(str_value);
  NodeCompileCacheModule ccm;
  ASSERT_EQ(ccm.GetAttrSize(any_value), 4);
}

TEST_F(UtestCcm, testGetAttrSizeBool) {
  bool val = false;
  auto any_value = ge::GeAttrValue::CreateFrom<bool>(val);
  NodeCompileCacheModule ccm;
  ASSERT_EQ(ccm.GetAttrSize(any_value), sizeof(val));
}

TEST_F(UtestCcm, testGetAttrSizeInt) {
  int64_t val = 0;
  auto any_value = ge::GeAttrValue::CreateFrom<int64_t>(val);
  NodeCompileCacheModule ccm;
  ASSERT_EQ(ccm.GetAttrSize(any_value), sizeof(val));
}

TEST_F(UtestCcm, testGetAttrSizeFloat) {
  float val = 0;
  auto any_value = ge::GeAttrValue::CreateFrom<float>(val);
  NodeCompileCacheModule ccm;
  ASSERT_EQ(ccm.GetAttrSize(any_value), sizeof(val));
}

TEST_F(UtestCcm, testGetAttrSizeDatatype) {
  DataType val = DT_INT8;
  auto any_value = ge::GeAttrValue::CreateFrom<DataType>(val);
  NodeCompileCacheModule ccm;
  ASSERT_EQ(ccm.GetAttrSize(any_value), sizeof(val));
}

