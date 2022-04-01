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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/psroipooling_fusion_pass.h"
#include "graph/compute_graph.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "common/graph/fe_graph_utils.h"
#include "common/configuration.h"
#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;
static const string ATTR_DATA_TYPE = "T";

namespace fe {
  static const std::string PSROIPOOLING = "PSROIPooling";

using FEOpsKernelInfoStorePtr = std::shared_ptr<fe::FEOpsKernelInfoStore>;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

bool CheckTbeSupportedStubPSROI(te::TbeOpInfo &info, te::CheckSupportedResult &is_support,
    string &reason) {
  is_support = te::FULLY_SUPPORTED;
  return true;
}

class UTEST_fusion_engine_psroipooling_pass : public testing::Test
{
public:
  FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
protected:
    void SetUp()
    {
        op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
        TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
        tbe_adapter_ptr->CheckTbeSupported = CheckTbeSupportedStubPSROI;
        op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

        fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
        FEOpsStoreInfo tbe_builtin {
                0,
                "tbe-builtin",
                EN_IMPL_HW_TBE,
                "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/tbe_opinfo",
                "",
                false,
                false};
        vector<FEOpsStoreInfo> store_info;
        store_info.emplace_back(tbe_builtin);
        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
        std::map<std::string, std::string> options;
        fe_ops_kernel_info_store_ptr->Initialize(options);

    }

    void TearDown()
    {
        fe_ops_kernel_info_store_ptr->Finalize();

    }

    static NodePtr CreateConstNode(string name, GeTensorDescPtr out_desc_ptr, ComputeGraphPtr graph)
    {
        OpDescPtr constant = std::make_shared<OpDesc>(name, CONSTANT);
        //set OpDesc
        AttrUtils::SetStr(out_desc_ptr, "name", name + "Out0");
        constant->AddOutputDesc(out_desc_ptr->Clone());
        // set attr
        AttrUtils::SetInt(constant, ATTR_DATA_TYPE, DT_FLOAT16);
        NodePtr node_const = graph->AddNode(constant);

        return node_const;
    }

  static NodePtr CreateDataNode(string name, GeTensorDescPtr out_desc_ptr, ComputeGraphPtr graph)
  {
    OpDescPtr data = std::make_shared<OpDesc>(name, DATA);
    data->AddInputDesc(out_desc_ptr->Clone());
    //set OpDesc
    data->AddOutputDesc(out_desc_ptr->Clone());
    // set attr
    NodePtr node_const = graph->AddNode(data);

    return node_const;
  }

    static NodePtr CreateOtherNode(string name, GeTensorDescPtr tensor_desc_ptr, ComputeGraphPtr graph)
    {
        OpDescPtr other_desc_ptr = std::make_shared<OpDesc>(name, "otherNode");
        //set OpDesc
        auto local_tensor_desc = tensor_desc_ptr->Clone();
        // add two input desc
        for (int i = 0; i < 2; ++i) {
            AttrUtils::SetStr(local_tensor_desc, "name", name + "In" + std::to_string(i));
            other_desc_ptr->AddInputDesc(local_tensor_desc);
        }
        // add two output desc
        for (int i = 0; i < 2; ++i) {
            AttrUtils::SetStr(local_tensor_desc, "name", name + "Out" + std::to_string(i));
            other_desc_ptr->AddOutputDesc(local_tensor_desc);
        }
        // add node from other_desc_ptr to graph
        // set attr
        AttrUtils::SetInt(other_desc_ptr, ATTR_DATA_TYPE, DT_FLOAT);
        NodePtr node_other = graph->AddNode(other_desc_ptr);

        return node_other;
    }

    static NodePtr CreatePsroiPoolingNode(string name,
                                          GeTensorDescPtr tensor_desc_ptr,
                                          GeTensorDescPtr roi_tensor_desc_ptr,
                                          ComputeGraphPtr graph,
                                          int64_t output_dim,
                                          int64_t group_size)
    {
        OpDescPtr desc_ptr = std::make_shared<OpDesc>(name, PSROIPOOLING);
        //set OpDesc
        auto local_tensor_desc = tensor_desc_ptr->Clone();
        auto roi_local_tensor_desc = roi_tensor_desc_ptr->Clone();
        // add two input desc

        AttrUtils::SetStr(local_tensor_desc, "name", name + "In0");
        desc_ptr->AddInputDesc(local_tensor_desc);

        AttrUtils::SetStr(roi_local_tensor_desc, "name", name + "In1");
        desc_ptr->AddInputDesc(roi_local_tensor_desc);

        // add 1 output desc
        for (int i = 0; i < 1; ++i) {
            AttrUtils::SetStr(local_tensor_desc, "name", name + "Out" + std::to_string(i));
            desc_ptr->AddOutputDesc(local_tensor_desc);
        }
        // add node from desc_ptr to graph
        // set attr
        AttrUtils::SetInt(desc_ptr, ATTR_DATA_TYPE, DT_FLOAT);
        if (output_dim != -1) {
            AttrUtils::SetInt(desc_ptr, "output_dim", output_dim);
        }
        if (group_size != -1) {
            AttrUtils::SetInt(desc_ptr, "group_size", group_size);
        }
        NodePtr psroi_node_prt = graph->AddNode(desc_ptr);

        return psroi_node_prt;
    }

    static NodePtr CreateConvNode(string node_name, ComputeGraphPtr graph) {
        OpDescPtr conv_o_p = std::make_shared<OpDesc>(node_name, "Conv2D");
        GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
        conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
        conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
        conv_o_p->AddInputDesc(conv_tensor_desc);
        conv_o_p->AddInputDesc(conv_tensor_desc);
        conv_o_p->AddOutputDesc(conv_tensor_desc);
        NodePtr conv_node = graph->AddNode(conv_o_p);
        ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

        return conv_node;
    }

    static ComputeGraphPtr CreateTestGraph(std::string fusion_flag, int64_t output_dim, int64_t group_size)
    {
        ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
         // new a output GeTensorDesc
        GeTensorDescPtr general_ge_tensor_desc = std::make_shared<GeTensorDesc>();
        vector<int64_t> dims{2, 21*7*7, 14, 14};
        GeShape shape(dims);
        general_ge_tensor_desc->SetShape(shape);
        general_ge_tensor_desc->SetOriginShape(shape);
        general_ge_tensor_desc->SetFormat(FORMAT_NCHW);
        general_ge_tensor_desc->SetOriginFormat(FORMAT_NCHW);
        general_ge_tensor_desc->SetDataType(DT_FLOAT16);
        NodePtr data0 = CreateDataNode("test/data0", general_ge_tensor_desc, graph);

        GeTensorDescPtr rois_tensor_desc = std::make_shared<GeTensorDesc>();
        vector<int64_t> roi_dims{5, 3, 1, 1};
        GeShape rois_shape(roi_dims);

        rois_tensor_desc->SetShape(rois_shape);
        rois_tensor_desc->SetOriginShape(rois_shape);
        rois_tensor_desc->SetFormat(FORMAT_NCHW);
        rois_tensor_desc->SetOriginFormat(FORMAT_NCHW);
        rois_tensor_desc->SetDataType(DT_FLOAT16);
        NodePtr data1 = CreateDataNode("test/const0", rois_tensor_desc, graph);

        vector<int64_t> weight{21*7*7, 2, 14, 14};
        GeShape weight_shape(weight);
        general_ge_tensor_desc->SetShape(weight_shape);
        general_ge_tensor_desc->SetOriginShape(weight_shape);
        general_ge_tensor_desc->SetFormat(FORMAT_NCHW);
        general_ge_tensor_desc->SetOriginFormat(FORMAT_NCHW);
        general_ge_tensor_desc->SetDataType(DT_FLOAT16);
        NodePtr data2 = CreateDataNode("test/data0", general_ge_tensor_desc, graph);

        NodePtr psroi_node_ptr = CreatePsroiPoolingNode("test/PSROIPooling_fusion",
                general_ge_tensor_desc, rois_tensor_desc, graph, output_dim, group_size);

        NodePtr conv_node_ptr = CreateConvNode("test/conv2d", graph);

         /* add link of anchors */
        std::vector<OutDataAnchorPtr> srcs;
        std::vector<InDataAnchorPtr> dsts;
        // 0: add link from Const(0) to MatMul(fusion)[0]
        srcs.push_back(data0->GetOutDataAnchor(0));
        dsts.push_back(psroi_node_ptr->GetInDataAnchor(0));
        // 1: add link from Const(1) to MatMul(fusion)[1]
        srcs.push_back(data1->GetOutDataAnchor(0));
        dsts.push_back(psroi_node_ptr->GetInDataAnchor(1));

        // add edges
        if (fusion_flag == "SwapCi") {
          for (int i = 0; i <= 1; ++i)
          {
            GraphUtils::AddEdge(srcs[i], dsts[i]);
          }
        } else {
          GraphUtils::AddEdge(data0->GetOutDataAnchor(0), conv_node_ptr->GetInDataAnchor(0));
          GraphUtils::AddEdge(data2->GetOutDataAnchor(0), conv_node_ptr->GetInDataAnchor(1));
          GraphUtils::AddEdge(conv_node_ptr->GetOutDataAnchor(0), psroi_node_ptr->GetInDataAnchor(0));
          GraphUtils::AddEdge(data1->GetOutDataAnchor(0), psroi_node_ptr->GetInDataAnchor(1));
        }

        return graph;
    }

    static Status DumpGraph(const ComputeGraphPtr graph)
    {
        for(NodePtr node : graph->GetAllNodes()) {
            printf("node name = %s.\n", node->GetName().c_str());
            for (OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
                for (InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
                    printf("    node name = %s[%d], out data node name = %s[%d].\n",
                        node->GetName().c_str(),
                        anchor->GetIdx(),
                        peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                        peer_in_anchor->GetIdx());
                }
            }
            if (node->GetOutControlAnchor() != nullptr) {
                for (InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
                    printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                        peer_in_anchor->GetOwnerNode()->GetName().c_str());
                }
            }
        }
    }
};

TEST_F(UTEST_fusion_engine_psroipooling_pass, swapci_fusion_success)
{
    ComputeGraphPtr graph = CreateTestGraph("SwapCi", 21, 7);
    PSROIPoolingFusionPass pass;
    pass.group_size_=-1;
    pass.output_dim_=-1;
    fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
    EXPECT_EQ(fe::SUCCESS, status);

    for(auto node : graph->GetDirectNode()) {
        auto op_desc = node->GetOpDesc();
        if (node->GetName().find("SwapCi") != node->GetName().npos) {
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetFormat());
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetOriginFormat());
            vector<int64_t> right_in_shape {2, 21*7*7, 14, 14};
            EXPECT_EQ(right_in_shape, op_desc->GetInputDesc(0).GetShape().GetDims());
            EXPECT_EQ(right_in_shape, op_desc->GetInputDesc(0).GetOriginShape().GetDims());

            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetFormat());
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetOriginFormat());
            vector<int64_t> right_out_shape {2, 1568, 14, 14};
            vector<int64_t> right4_d_shape {2, 32*7*7, 14, 14};
            EXPECT_EQ(right_out_shape, op_desc->GetOutputDesc(0).GetShape().GetDims());
            EXPECT_EQ(right4_d_shape, op_desc->GetOutputDesc(0).GetOriginShape().GetDims());
        }

        if (node->GetName() == "test/PSROIPooling_fusion") {
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetFormat());
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetOriginFormat());
            vector<int64_t> right_in_shape {2, 1568, 14, 14};
            vector<int64_t> right_in4_d_shape {2, 32*7*7, 14, 14};
            EXPECT_EQ(right_in_shape, op_desc->GetInputDesc(0).GetShape().GetDims());
            EXPECT_EQ(right_in4_d_shape, op_desc->GetInputDesc(0).GetOriginShape().GetDims());

            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetFormat());
            EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetOriginFormat());
            vector<int64_t> right_out_shape {1029, 32, 14, 14};
            vector<int64_t> right4_d_shape {1029, 2, 14, 14};
            EXPECT_EQ(right_out_shape, op_desc->GetOutputDesc(0).GetShape().GetDims());
            EXPECT_EQ(right4_d_shape, op_desc->GetOutputDesc(0).GetOriginShape().GetDims());
        }
    }
}

TEST_F(UTEST_fusion_engine_psroipooling_pass, swapci_fusion_no_output_dim)
{
  ComputeGraphPtr graph = CreateTestGraph("SwapCi", -1, 7);
  PSROIPoolingFusionPass pass;
  pass.group_size_=-1;
  pass.output_dim_=-1;
  fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::PARAM_INVALID, status);
}

TEST_F(UTEST_fusion_engine_psroipooling_pass, swapci_fusion_no_group_size)
{
  ComputeGraphPtr graph = CreateTestGraph("SwapCi", 21, -1);
  PSROIPoolingFusionPass pass;
  pass.group_size_=-1;
  pass.output_dim_=-1;
  fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::PARAM_INVALID, status);
}

TEST_F(UTEST_fusion_engine_psroipooling_pass, swapco_fusion_success)
{
  ComputeGraphPtr graph = CreateTestGraph("SwapCo", 21, 7);
  PSROIPoolingFusionPass pass;
  pass.group_size_=-1;
  pass.output_dim_=-1;
  fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::SUCCESS, status);

  for(auto node : graph->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    if (node->GetName().find("SwapCo") != node->GetName().npos) {
      EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetInputDesc(0).GetOriginFormat());
      vector<int64_t> right_in_shape {21*7*7, 2, 14, 14};
      EXPECT_EQ(right_in_shape, op_desc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(right_in_shape, op_desc->GetInputDesc(0).GetOriginShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, op_desc->GetOutputDesc(0).GetOriginFormat());
      vector<int64_t> right_out_shape {32*7*7, 2, 14, 14};
      EXPECT_EQ(right_out_shape, op_desc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(right_out_shape, op_desc->GetOutputDesc(0).GetOriginShape().GetDims());
    }
  }
}

TEST_F(UTEST_fusion_engine_psroipooling_pass, swapci_fusion_conv_input_not_two)
{
  ComputeGraphPtr graph = CreateTestGraph("SwapCo", 21, -1);
  PSROIPoolingFusionPass pass;
  pass.group_size_=-1;
  pass.output_dim_=-1;
  fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::PARAM_INVALID, status);
}


TEST_F(UTEST_fusion_engine_psroipooling_pass, swapci_fusion_psroi_input_not_two)
{
  ComputeGraphPtr graph = CreateTestGraph("SwapCo", 21, -1);
  PSROIPoolingFusionPass pass;
  pass.group_size_=-1;
  pass.output_dim_=-1;
  fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::PARAM_INVALID, status);
}
}
