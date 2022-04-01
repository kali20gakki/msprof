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
#include <iostream>
#include <list>

#define protected public
#define private public
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/util/constants.h"
#include "common/scope_allocator.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/ub_fusion/fusion_graph_merge/fusion_graph_merge.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class fusion_graph_merge_ut : public testing::Test {
public:
protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(fusion_graph_merge_ut, set_Data_dump_ref) {
  L2FusionInfoPtr originl2_info = std::make_shared<TaskL2FusionInfo_t>();
  originl2_info->l2_info.node_name[0] = "test";
  originl2_info->l2_info.node_name[1] = "test1";
  originl2_info->l2_info.node_name[2] = "";

  L2FusionInfoPtr fusion_l2_info = std::make_shared<TaskL2FusionInfo_t>();

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;

  OpDescPtr split = std::make_shared<OpDesc>("split", SPLITD);
  // add descriptor
  ge::GeShape shape1({1, 32, 14, 14});
  ge::GeShape shape2({1, 32, 14, 14});
  GeTensorDesc in_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT16);
  in_desc1.SetOriginFormat(ge::FORMAT_NCHW);
  in_desc1.SetOriginDataType(ge::DT_FLOAT16);
  in_desc1.SetOriginShape(shape1);

  GeTensorDesc out_desc1(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT16);
  out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
  out_desc1.SetOriginDataType(ge::DT_FLOAT16);
  out_desc1.SetOriginShape(shape2);
  ge::AttrUtils::SetStr(in_desc1, fe::ATTR_DATA_DUMP_REF, "1");
  ge::AttrUtils::SetStr(out_desc1, fe::ATTR_DATA_DUMP_REF, "1");
  split->AddInputDesc(in_desc1);
  split->AddOutputDesc(out_desc1);
  // create nodes
  NodePtr split_node = graph->AddNode(split);

  OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
  // add descriptor
  GeTensorDesc in_desc2(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT16);
  in_desc2.SetOriginFormat(ge::FORMAT_NCHW);
  in_desc2.SetOriginDataType(ge::DT_FLOAT16);
  in_desc2.SetOriginShape(shape1);

  GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT16);
  out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
  out_desc2.SetOriginDataType(ge::DT_FLOAT16);
  out_desc2.SetOriginShape(shape2);
  ge::AttrUtils::SetStr(in_desc2, fe::ATTR_DATA_DUMP_REF, "1");
  ge::AttrUtils::SetStr(out_desc2, fe::ATTR_DATA_DUMP_REF, "1");
  concat->AddInputDesc(in_desc2);
  concat->AddOutputDesc(out_desc2);

  // create nodes
  NodePtr concat_node = graph->AddNode(concat);

  ge::OpDescPtr op_desc = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc1 =
      ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::GeTensorDesc tensor_desc2 =
      ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  op_desc->AddInputDesc(tensor_desc1);
  op_desc->AddOutputDesc(tensor_desc2);
  ge::NodePtr fusion_node = graph->AddNode(op_desc);
  ge::GraphUtils::AddEdge(fusion_node->GetOutDataAnchor(0),
                          concat_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                          fusion_node->GetInDataAnchor(0));
  fusion_graph_merge_ptr->fusion_op_name_map_all_["test"].insert(
      std::make_pair(fusion_anchor_idx, fusion_node));
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
  fusion_graph_merge_ptr->SetDataDumpRef(fusion_node, *graph);
  Status ret = fe::SUCCESS;
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_ut, set_l2_name_and_index_for_fus_node) {
  L2FusionInfoPtr originl2_info = std::make_shared<TaskL2FusionInfo_t>();
  originl2_info->l2_info.node_name[0] = "test";
  originl2_info->l2_info.node_name[1] = "test1";
  originl2_info->l2_info.node_name[2] = "";

  L2FusionInfoPtr fusion_l2_info = std::make_shared<TaskL2FusionInfo_t>();

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc =
      ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetInt(tensor_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OUTPUT_INDEX,
                        0);
  op_desc->AddOutputDesc(tensor_desc);
  ge::NodePtr fusion_node = graph->AddNode(op_desc);
  fusion_graph_merge_ptr->fusion_op_name_map_all_["test"].insert(
      std::make_pair(fusion_anchor_idx, fusion_node));
  Status ret = fusion_graph_merge_ptr->SetL2NameAndIndex(originl2_info, fusion_l2_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_ut, set_l2_name_and_index_for_unfus_node) {
  L2FusionInfoPtr originl2_info = std::make_shared<TaskL2FusionInfo_t>();
  originl2_info->l2_info.node_name[0] = "test";
  originl2_info->l2_info.output_index[0] = 0;
  originl2_info->l2_info.node_name[1] = "test1";
  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc =
      ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetInt(tensor_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OUTPUT_INDEX,
                        0);
  op_desc->AddOutputDesc(tensor_desc);
  ge::NodePtr fusion_node = graph->AddNode(op_desc);
  fusion_graph_merge_ptr->fusion_op_name_map_all_["test"].insert(
      std::make_pair(fusion_anchor_idx, fusion_node));
  Status ret =
      fusion_graph_merge_ptr->SetL2NameAndIndexForUnfusNode(originl2_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_ut, update_l1_attr)
{
    const string attr_key = "L1_Attr";
    vector<int64_t> attr_list = {1, 2, 3, 4};
    OpDescPtr op_desc = std::make_shared<OpDesc>("concat", CONCATD);
    ge::AttrUtils::SetListInt(op_desc, attr_key, attr_list);

    uint32_t anchor_index = 1;
    uint32_t tensor_desc_index = 1;
    vector<int64_t> target_vec = {0, 0, 0, 0};

    auto graph_common = std::make_shared<GraphComm>("engineName");
    graph_common->Initialize();
    auto fusion_graph_merge_ptr =
            std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
    fusion_graph_merge_ptr->UpdateL1Attr(op_desc, attr_key, anchor_index, tensor_desc_index, target_vec);
    EXPECT_EQ(2, target_vec[1]);
}

TEST_F(fusion_graph_merge_ut, set_l2_task_info_to_fusion_op) {
  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>("fusion_scope", graph_common);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr node1_op = std::make_shared<ge::OpDesc>("conv0","conv");
  ge::NodePtr node1 = std::make_shared<ge::Node>(node1_op, graph);
  Status ret = fusion_graph_merge_ptr->SetL2TaskInfoToFusionOp(node1);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_ut, fuse_two_nodes) {
  std::shared_ptr<AutomaticBufferFusion> auto_buffer_fusion_ptr = std::make_shared<AutomaticBufferFusion>(nullptr);
  ComputeGraphPtr owner_graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr node1_op = std::make_shared<ge::OpDesc>("conv0","conv");
  ge::OpDescPtr node2_op = std::make_shared<ge::OpDesc>("conv1","conv");
  ge::NodePtr node1 = std::make_shared<ge::Node>(node1_op, owner_graph);
  ge::NodePtr node2 = std::make_shared<ge::Node>(node1_op, owner_graph);
  int64_t producer_scope_id = 0;
  int64_t consumer_scope_id = 0;
  Status ret = auto_buffer_fusion_ptr->FuseTwoNodes(node1, node2, producer_scope_id, consumer_scope_id);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_ut, set_multi_kernel_output_offsets) {
  std::vector<int64_t> buffer_fusion_output_offset;
  std::vector<int64_t> save_pre_output_offset;
  buffer_fusion_output_offset.push_back(1);

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc_src = make_shared<OpDesc>("sourceNode", "Conv");
  ge::OpDescPtr op_desc_fus = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetInt(tensor_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OUTPUT_INDEX,0);
  ge::AttrUtils::SetListInt(op_desc_fus, ge::ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, buffer_fusion_output_offset);
  op_desc_fus->AddOutputDesc(tensor_desc);
  op_desc_src->AddOutputDesc(tensor_desc);
  fusion_graph_merge_ptr->SetMultiKernelOutPutOffsets(op_desc_src, 2,
                                                      op_desc_fus, save_pre_output_offset);
  EXPECT_EQ(buffer_fusion_output_offset[1], 0);
}

TEST_F(fusion_graph_merge_ut, update_output_ref_port_index) {
  std::vector<uint32_t> ref_port_index = {1};

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc_src = make_shared<OpDesc>("sourceNode", "Conv");
  ge::OpDescPtr op_desc_fus = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc_fus =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::GeTensorDesc tensor_desc_src =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetListInt(tensor_desc_src, "ref_port_index", ref_port_index);
  op_desc_fus->AddOutputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  ge::AttrUtils::SetBool(op_desc_fus, ge::ATTR_NAME_REFERENCE, true);

  op_desc_src->AddOutputDesc(tensor_desc_src);
  fusion_graph_merge_ptr->UpdateOutputRefPortIndex(op_desc_src, op_desc_fus, 1);


  ge::AttrUtils::GetListInt(op_desc_src->MutableOutputDesc(0), "ref_port_index", ref_port_index);
  std::cout << "ref_port_index: " << ref_port_index[0] << std::endl;
  EXPECT_EQ(ref_port_index[0], 2);
}