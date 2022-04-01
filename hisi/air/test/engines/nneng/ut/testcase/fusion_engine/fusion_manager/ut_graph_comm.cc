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
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "common/graph_comm.h"
#include "common/fusion_op_comm.h"
#include "graph/utils/graph_utils.h"
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/util/constants.h"
#include "graph/op_kernel_bin.h"
#include "common/graph/fe_graph_utils.h"

#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class graph_comm_ut: public testing::Test
{
public:

protected:

  void SetUp() {
  }

  void TearDown()
  {

  }

  static ComputeGraphPtr CreateEmptyOriginGraph() {
      ComputeGraphPtr graph = std::make_shared<ComputeGraph>("empty_graph");
      return graph;
  }

  static ComputeGraphPtr CreateOriginGraph() {
      ComputeGraphPtr graph = std::make_shared<ComputeGraph>("origin_graph");
      std::string session_graph_id = "123456";
      //session_graph_id
      ge::AttrUtils::SetStr(graph, "session_graph_id", session_graph_id);
      // Node
      OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
      OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

      // add descriptor
      vector<int64_t> dims = {1,2,3,4};
      GeShape shape(dims);

      GeTensorDesc in_desc1(shape);
      in_desc1.SetOriginFormat(FORMAT_NCHW);
      in_desc1.SetFormat(FORMAT_NCHW);
      in_desc1.SetDataType(DT_FLOAT16);
      relu_op->AddInputDesc("x", in_desc1);

      GeTensorDesc out_desc1(shape);
      out_desc1.SetOriginFormat(FORMAT_HWCN);
      out_desc1.SetFormat(FORMAT_HWCN);
      out_desc1.SetDataType(DT_FLOAT16);
      relu_op->AddOutputDesc("y", out_desc1);


      GeTensorDesc in_desc2(shape);
      in_desc2.SetOriginFormat(FORMAT_FRACTAL_Z);
      in_desc2.SetFormat(FORMAT_FRACTAL_Z);
      in_desc2.SetDataType(DT_FLOAT16);
      bn_op->AddInputDesc("x", in_desc2);

      GeTensorDesc out_desc2(shape);
      out_desc2.SetOriginFormat(FORMAT_NHWC);
      out_desc2.SetFormat(FORMAT_NHWC);
      out_desc2.SetDataType(DT_FLOAT16);
      bn_op->AddOutputDesc("y", out_desc2);

      NodePtr bn_node = graph->AddNode(bn_op);
      bn_node->AddSendEventId(123);
      bn_node->AddSendEventId(234);
      bn_node->AddSendEventId(345);
      NodePtr relu_node = graph->AddNode(relu_op);
      relu_node->AddRecvEventId(234);
      relu_node->AddRecvEventId(345);
      ge::GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
      return graph;
  }

  static ComputeGraphPtr CreateOriginGraph2() {
      ComputeGraphPtr graph = std::make_shared<ComputeGraph>("origin_graph");
      std::string session_graph_id = "123456";
      //session_graph_id
      ge::AttrUtils::SetStr(graph, "session_graph_id", session_graph_id);
      // Node
      OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
      OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
      OpDescPtr netout_op = std::make_shared<OpDesc>("netoutput", "NetOutput");

      // add descriptor
      vector<int64_t> dims = {1,2,3,4};
      GeShape shape(dims);

      GeTensorDesc in_desc1(shape);
      in_desc1.SetOriginFormat(FORMAT_NCHW);
      in_desc1.SetFormat(FORMAT_NCHW);
      in_desc1.SetDataType(DT_FLOAT16);
      relu_op->AddInputDesc("x", in_desc1);

      GeTensorDesc out_desc1(shape);
      out_desc1.SetOriginFormat(FORMAT_HWCN);
      out_desc1.SetFormat(FORMAT_HWCN);
      out_desc1.SetDataType(DT_FLOAT16);
      relu_op->AddOutputDesc("y", out_desc1);


      GeTensorDesc in_desc2(shape);
      in_desc2.SetOriginFormat(FORMAT_FRACTAL_Z);
      in_desc2.SetFormat(FORMAT_FRACTAL_Z);
      in_desc2.SetDataType(DT_FLOAT16);
      bn_op->AddInputDesc("x", in_desc2);

      GeTensorDesc out_desc2(shape);
      out_desc2.SetOriginFormat(FORMAT_NHWC);
      out_desc2.SetFormat(FORMAT_NHWC);
      out_desc2.SetDataType(DT_FLOAT16);
      bn_op->AddOutputDesc("y", out_desc2);

      GeTensorDesc in_desc3(shape);
      in_desc3.SetOriginFormat(FORMAT_FRACTAL_Z);
      in_desc3.SetFormat(FORMAT_FRACTAL_Z);
      in_desc3.SetDataType(DT_FLOAT16);
      netout_op->AddInputDesc("x", in_desc3);

      GeTensorDesc out_desc3(shape);
      out_desc3.SetOriginFormat(FORMAT_NHWC);
      out_desc3.SetFormat(FORMAT_NHWC);
      out_desc3.SetDataType(DT_FLOAT16);
      netout_op->AddOutputDesc("y", out_desc3);

      NodePtr bn_node = graph->AddNode(bn_op);
      NodePtr relu_node = graph->AddNode(relu_op);
      NodePtr netout_node = graph->AddNode(netout_op);
      ge::GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
      ge::GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), netout_node->GetInDataAnchor(0));
      return graph;
  }
// AUTO GEN PLEASE DO NOT MODIFY IT
};

TEST_F(graph_comm_ut, clone_graph_case1)
{
    ComputeGraphPtr graph = CreateEmptyOriginGraph();
    ComputeGraphPtr clone_graph = std::make_shared<ComputeGraph>("test");
    Status ret = FeGraphUtils::CloneGraph(*(graph.get()), *(clone_graph.get()));
    EXPECT_EQ(ret, fe::SUCCESS);
    EXPECT_EQ(graph->GetName(), clone_graph->GetName());
    std::string session_graph_id = "";
    ge::AttrUtils::GetStr(graph, "session_graph_id", session_graph_id);
    std::string clone_session_graph_id = "";
    ge::AttrUtils::GetStr(clone_graph, "session_graph_id", clone_session_graph_id);

    EXPECT_EQ(graph->GetDirectNode().size(), clone_graph->GetDirectNode().size());
    for (ge::NodePtr node : graph->GetDirectNode()) {
        std::string node_name = node->GetName();
        ge::NodePtr src_node = clone_graph->FindNode(node_name);
        EXPECT_EQ(node->GetType(), src_node->GetType());
        EXPECT_EQ(node->GetAllOutDataAnchors().size(), src_node->GetAllOutDataAnchors().size());
        if (node->GetAllOutDataAnchors().size() > 0) {
            EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(),
                      src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size());
            if (node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 0) {
                std::string dst_node_name = node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                std::string clone_dst_node_name = src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                EXPECT_EQ(dst_node_name, clone_dst_node_name);
            }
        }
    }
}

TEST_F(graph_comm_ut, clone_graph_case2)
{
    ComputeGraphPtr graph = CreateOriginGraph();
    ComputeGraphPtr clone_graph = std::make_shared<ComputeGraph>("test");
    Status ret = FeGraphUtils::CloneGraph(*(graph.get()), *(clone_graph.get()));
    EXPECT_EQ(ret, fe::SUCCESS);
    EXPECT_EQ(graph->GetName(), clone_graph->GetName());
    std::string session_graph_id = "";
    ge::AttrUtils::GetStr(graph, "session_graph_id", session_graph_id);
    std::string clone_session_graph_id = "";
    ge::AttrUtils::GetStr(clone_graph, "session_graph_id", clone_session_graph_id);

    EXPECT_EQ(graph->GetDirectNode().size(), clone_graph->GetDirectNode().size());
    for (ge::NodePtr node : graph->GetDirectNode()) {
        std::string node_name = node->GetName();
        ge::NodePtr src_node = clone_graph->FindNode(node_name);
        EXPECT_EQ(node->GetType(), src_node->GetType());
        EXPECT_EQ(node->GetAllOutDataAnchors().size(), src_node->GetAllOutDataAnchors().size());
        if (node->GetAllOutDataAnchors().size() > 0) {
            EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(),
                      src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size());
            if (node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 0) {
                std::string dst_node_name = node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                std::string clone_dst_node_name = src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                EXPECT_EQ(dst_node_name, clone_dst_node_name);
            }
        }
        EXPECT_EQ(node->GetSendEventIdList().size(), src_node->GetSendEventIdList().size());
        EXPECT_EQ(node->GetRecvEventIdList().size(), src_node->GetRecvEventIdList().size());
    }
}

TEST_F(graph_comm_ut, clone_graph_case3)
{
    ComputeGraphPtr graph = CreateOriginGraph2();
    ComputeGraphPtr clone_graph = std::make_shared<ComputeGraph>("test");
    Status ret = FeGraphUtils::CloneGraph(*(graph.get()), *(clone_graph.get()));
    EXPECT_EQ(ret, fe::SUCCESS);
    EXPECT_EQ(graph->GetName(), clone_graph->GetName());
    std::string session_graph_id = "";
    ge::AttrUtils::GetStr(graph, "session_graph_id", session_graph_id);
    std::string clone_session_graph_id = "";
    ge::AttrUtils::GetStr(clone_graph, "session_graph_id", clone_session_graph_id);

    EXPECT_EQ(graph->GetDirectNode().size(), clone_graph->GetDirectNode().size());
    for (ge::NodePtr node : graph->GetDirectNode()) {
        std::string node_name = node->GetName();
        ge::NodePtr src_node = clone_graph->FindNode(node_name);
        EXPECT_EQ(node->GetType(), src_node->GetType());
        EXPECT_EQ(node->GetAllOutDataAnchors().size(), src_node->GetAllOutDataAnchors().size());
        if (node->GetAllOutDataAnchors().size() > 0) {
            EXPECT_EQ(node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size(),
                      src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size());
            if (node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 0) {
                std::string dst_node_name = node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                std::string clone_dst_node_name = src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode()->GetName();
                EXPECT_EQ(dst_node_name, clone_dst_node_name);
            }
        }
        EXPECT_EQ(node->GetSendEventIdList().size(), src_node->GetSendEventIdList().size());
        EXPECT_EQ(node->GetRecvEventIdList().size(), src_node->GetRecvEventIdList().size());
    }
}

TEST_F(graph_comm_ut, set_Batch_bind_only) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr conv_desc = std::make_shared<OpDesc>("conv", "Conv2D");
  ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
  AttrUtils::SetBool(conv_desc, "_is_n_batch_split", true);
  NodePtr conv_node = graph->AddNode(conv_desc);
  const char tbe_bin[] = "tbe_bin";
  vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
  ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
          conv_node->GetName(), std::move(buffer));
  conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  auto fusion_op_comm = std::make_shared<FusionOpComm>();
  vector<ge::NodePtr> fus_nodelist;
  fus_nodelist.push_back(conv_node);
  OpDescPtr new_conv_desc = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
  fusion_op_comm->SetTBEFusionOp(fus_nodelist, new_conv_desc, "engineName");
  bool is_batch = false;
  AttrUtils::GetBool(conv_desc, "_is_n_batch_split", is_batch);
  EXPECT_EQ(is_batch, true);
}

TEST_F(graph_comm_ut, set_buffer_fusion_info) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr conv_desc = std::make_shared<OpDesc>("conv", "Conv2D");
  ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
  AttrUtils::SetBool(conv_desc, "is_nbatch_spilt", true);
  NodePtr conv_node = graph->AddNode(conv_desc);
  const char tbe_bin[] = "tbe_bin";
  vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
  ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
          conv_node->GetName(), std::move(buffer));
  conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  ge::AttrUtils::SetInt(conv_desc, "l2_fusion_group_id", 1);
  ge::AttrUtils::SetBool(conv_desc, NEED_RE_PRECOMPILE, true);
  ge::AttrUtils::SetStr(conv_desc, ge::ATTR_NAME_FUSION_GROUP_TYPE, "ut_test");
  ge::AttrUtils::SetStr(conv_desc, "continuous_stream_label","L2StreamLabel");
  std::vector<int32_t>  l2_output_offset;
  l2_output_offset.push_back(1);
  ge::AttrUtils::SetListInt(conv_desc, "output_offset_for_buffer_fusion", l2_output_offset);
  ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, 1);
  ge::AttrUtils::SetBool(conv_desc, ge::ATTR_NO_TASK_AND_DUMP_NEEDED, true);
  auto fusion_op_comm = std::make_shared<FusionOpComm>();
  vector<ge::NodePtr> fus_nodelist;
  fus_nodelist.push_back(conv_node);
  OpDescPtr new_conv_desc = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
  fusion_op_comm->SetTBEFusionOp(fus_nodelist, new_conv_desc, "engineName");
  bool l2_optimize = false;
  AttrUtils::GetBool(new_conv_desc, NEED_RE_PRECOMPILE, l2_optimize);
  EXPECT_EQ(l2_optimize, true);
  int32_t output_real_calc_flag = 0;
  AttrUtils::GetInt(new_conv_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag);
  EXPECT_EQ(output_real_calc_flag, 1);
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
}

TEST_F(graph_comm_ut, set_l1_fusion_info) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr conv_desc = std::make_shared<OpDesc>("conv", "Conv2D");
  ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
  AttrUtils::SetBool(conv_desc, "is_nbatch_spilt", true);
  NodePtr conv_node = graph->AddNode(conv_desc);
  const char tbe_bin[] = "tbe_bin";
  vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
  ge::OpKernelBinPtr tbe_kernel_ptr =
      std::make_shared<ge::OpKernelBin>(conv_node->GetName(), std::move(buffer));
  conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  ge::AttrUtils::SetInt(conv_desc, "_l1_fusion_group_id", 1);
  ge::AttrUtils::SetBool(conv_desc, NEED_RE_PRECOMPILE, true);
  ge::AttrUtils::SetStr(conv_desc, ge::ATTR_NAME_FUSION_GROUP_TYPE, "ut_test");
  ge::AttrUtils::SetStr(conv_desc, "continuous_stream_label", "L2StreamLabel");
  std::vector<int32_t> l1_output_offset;
  l1_output_offset.push_back(1);
  ge::AttrUtils::SetListInt(conv_desc, "output_offset_for_buffer_fusion",
                            l1_output_offset);
  ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, 1);
  ge::AttrUtils::SetStr(conv_desc, "_L1_fusion_sub_graph_no", "1");
  auto fusion_op_comm = std::make_shared<FusionOpComm>();
  vector<ge::NodePtr> fus_nodelist;
  fus_nodelist.push_back(conv_node);
  OpDescPtr new_conv_desc = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
  Configuration::Instance(fe::AI_CORE_NAME).buffer_optimize_ = EN_L1_OPTIMIZE;
  fusion_op_comm->SetTBEFusionOp(fus_nodelist, new_conv_desc, "engineName");
  bool l1_optimize = false;
  AttrUtils::GetBool(new_conv_desc, NEED_RE_PRECOMPILE, l1_optimize);
  EXPECT_EQ(l1_optimize, true);
  int32_t output_real_calc_flag = 0;
  AttrUtils::GetInt(new_conv_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE,
                    output_real_calc_flag);
  EXPECT_EQ(output_real_calc_flag, 1);
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ =
      EN_OPTIMIZE_DISABLE;
}
