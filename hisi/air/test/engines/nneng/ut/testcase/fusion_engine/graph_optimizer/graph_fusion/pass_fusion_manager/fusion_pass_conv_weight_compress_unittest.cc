/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "common/util/compress/compress.h"
#include "common/configuration.h"
#include "common/util/platform_info.h"
#include "fusion_manager/fusion_manager.h"
#include "ops_store/ops_kernel_manager.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/conv_weight_compress_fusion_pass.h"
#include "common/pass_manager.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

class fusion_pass_conv_weight_compress_ut : public testing::Test
{
public:
  FEGraphOptimizerPtr graph_optimizer_ptr;
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr;
protected:
    void SetUp()
    {
      OpStoreAdapterManagerPtr op_store_adapter_manager = make_shared<OpStoreAdapterManager>();
      ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager, AI_CORE_NAME);
      FEOpsStoreInfo tbe_custom {
              2,
              "tbe-custom",
              EN_IMPL_CUSTOM_TBE,
              "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
              "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
              false,
              false};

      vector<FEOpsStoreInfo> store_info = {tbe_custom};
      Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);
      OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
      map<string, string> options;
      ops_kernel_info_store_ptr->Initialize(options);
      graph_optimizer_ptr = make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr, op_store_adapter_manager, AI_CORE_NAME);
    }
    void TearDown()
    {

    }

protected:
  static ComputeGraphPtr CreateGraphWithOneConv(DataType data_type, int32_t type=0, bool is_dynamic=false)
  {
      ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
      OpDescPtr op_desc_data = std::make_shared<OpDesc>("data", "Data");
      OpDescPtr op_desc_const1 = std::make_shared<OpDesc>("const1", "Const");
      OpDescPtr op_desc_const2 = std::make_shared<OpDesc>("const2", "Const");
      OpDescPtr op_desc_conv = std::make_shared<OpDesc>("conv", "Conv2D");
      OpDescPtr op_desc_relu = std::make_shared<OpDesc>("relu", "Relu");

      //add descriptor
      vector<int64_t> dim(4, 4);
      if (is_dynamic) {
        dim[1] = -1;
      }
      GeShape shape(dim);
      GeTensorDesc out_desc(shape);
      out_desc.SetFormat(FORMAT_NCHW);
      out_desc.SetOriginFormat(FORMAT_NCHW);
      out_desc.SetDataType(data_type);
      out_desc.SetOriginDataType(data_type);

      op_desc_data->AddOutputDesc(out_desc);
      op_desc_const1->AddOutputDesc(out_desc);
      op_desc_const2->AddOutputDesc(out_desc);
      op_desc_conv->AddInputDesc(out_desc);
      op_desc_conv->AddInputDesc(out_desc);
      op_desc_conv->AddInputDesc(out_desc);
      op_desc_conv->AddOutputDesc(out_desc);
      op_desc_relu->AddInputDesc(out_desc);
      op_desc_relu->AddOutputDesc(out_desc);

      NodePtr node_data = graph->AddNode(op_desc_data);
      NodePtr node_const1 = graph->AddNode(op_desc_const1);
      NodePtr node_const2 = graph->AddNode(op_desc_const2);
      NodePtr node_conv = graph->AddNode(op_desc_conv);
      NodePtr node_relu = graph->AddNode(op_desc_relu);

      if (type == 1) {
        AttrUtils::SetBool(node_conv->GetOpDesc(), ATTR_NAME_COMPRESS_WEIGHT, true);
        GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_conv->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const1->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const2->GetOutDataAnchor(0), node_conv->GetInDataAnchor(2));
        GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_conv->GetInDataAnchor(1));
      } if (type == 2) {
        ge::AttrUtils::SetInt(op_desc_conv, ATTR_NAME_GROUPS, 2);
        GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_conv->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const1->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const2->GetOutDataAnchor(0), node_conv->GetInDataAnchor(2));
        GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_conv->GetInDataAnchor(1));
      } if (type == 3) {
        AttrUtils::SetBool(node_conv->GetOpDesc(), ATTR_NAME_COMPRESS_WEIGHT, true);
        GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_conv->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const1->GetOutDataAnchor(0), node_conv->GetInDataAnchor(1));
        GraphUtils::AddEdge(node_const2->GetOutDataAnchor(0), node_conv->GetInDataAnchor(2));
        GraphUtils::AddEdge(node_conv->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_data->GetOutControlAnchor(), node_conv->GetInControlAnchor());
        GraphUtils::AddEdge(node_conv->GetOutControlAnchor(), node_relu->GetInControlAnchor());
      } else {
        AttrUtils::SetBool(node_conv->GetOpDesc(), ATTR_NAME_COMPRESS_WEIGHT, true);
        GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_conv->GetInDataAnchor(0));
        GraphUtils::AddEdge(node_const1->GetOutDataAnchor(0), node_conv->GetInDataAnchor(1));
        GraphUtils::AddEdge(node_const2->GetOutDataAnchor(0), node_conv->GetInDataAnchor(2));
        GraphUtils::AddEdge(node_conv->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
      }
      return graph;
  }

  static ComputeGraphPtr CreateGraphWithOneConvCompress()
  {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    OpDescPtr op_desc_data = std::make_shared<OpDesc>("data", "Data");
    OpDescPtr op_desc_quant = std::make_shared<OpDesc>("quant", "AscendQuant");
    OpDescPtr op_desc_conv = std::make_shared<OpDesc>("conv_compress", "Conv2DCompress");
    OpDescPtr op_desc_dequant = std::make_shared<OpDesc>("dequant", "AscendDequant");
    OpDescPtr op_desc_relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr op_desc_const = std::make_shared<OpDesc>("const", "Const");

    //add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    out_desc.SetFormat(FORMAT_NCHW);
    out_desc.SetOriginFormat(FORMAT_NCHW);
    out_desc.SetDataType(DT_INT8);
    out_desc.SetOriginDataType(DT_INT8);

    op_desc_data->AddOutputDesc(out_desc);
    op_desc_quant->AddInputDesc(out_desc);
    op_desc_quant->AddOutputDesc(out_desc);
    op_desc_conv->AddInputDesc(out_desc);
    op_desc_conv->AddInputDesc(out_desc);
    op_desc_conv->AddInputDesc(out_desc);
    op_desc_conv->AddInputDesc(out_desc);
    op_desc_conv->AddOutputDesc(out_desc);
    op_desc_dequant->AddInputDesc(out_desc);
    op_desc_dequant->AddOutputDesc(out_desc);
    op_desc_relu->AddInputDesc(out_desc);
    op_desc_const->AddOutputDesc(out_desc);

    vector<string> input_name_vec;
    input_name_vec.push_back("x");
    input_name_vec.push_back("filter");
    input_name_vec.push_back("bias");
    op_desc_conv->SetInputName(input_name_vec);

    NodePtr node_data = graph->AddNode(op_desc_data);
    NodePtr node_quant = graph->AddNode(op_desc_quant);
    NodePtr node_conv = graph->AddNode(op_desc_conv);
    NodePtr node_dequant = graph->AddNode(op_desc_dequant);
    NodePtr node_relu = graph->AddNode(op_desc_relu);
    NodePtr node_const = graph->AddNode(op_desc_const);

    AttrUtils::SetBool(node_conv->GetOpDesc(), ATTR_NAME_COMPRESS_WEIGHT, true);

    GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_quant->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_quant->GetOutDataAnchor(0), node_conv->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_const->GetOutDataAnchor(0), node_conv->GetInDataAnchor(1));
    GraphUtils::AddEdge(node_conv->GetOutDataAnchor(0), node_dequant->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_dequant->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));

    return graph;
  }

};

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case1)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_FLOAT);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case2)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case3)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8, 0, true);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case4)
{
  PlatformInfo platform_info;
  platform_info.soc_info.ai_core_cnt = 32;
  std::string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatformInfoManager::Instance().platform_info_map_[soc_version] = platform_info;
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};

  Status status = PassManager::Run(*graph, passes, ops_kernel_info_store_ptr);
  for (const auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "WeightCompressHost") {
      float compress_ratio = 0.0;
      ge::OpDescPtr op_desc = node->GetOpDesc();
      (void)ge::AttrUtils::GetFloat(op_desc, "compress_ratio_threshold", compress_ratio);
      EXPECT_EQ(compress_ratio, 0.8f);
    }
  }
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case5)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8, 1);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};

  Status status = PassManager::Run(*graph, passes, ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case6)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8, 2);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};

  Status status = PassManager::Run(*graph, passes, ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::NOT_CHANGED, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case7)
{
  ComputeGraphPtr graph = CreateGraphWithOneConv(ge::DT_INT8, 3);
  ConvWeightCompressFusionPass pass;
  vector<GraphPass*> passes = {&pass};

  Status status = PassManager::Run(*graph, passes, ops_kernel_info_store_ptr);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(fusion_pass_conv_weight_compress_ut, fusion_success_case17)
{
  ComputeGraphPtr graph = CreateGraphWithOneConvCompress();
  size_t size_before = graph->GetDirectNode().size();
  FE_LOGD("The number of nodes before is %zu.", size_before);

  // insert compress node
  Status status = graph_optimizer_ptr->InsertCompressOP(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  bool find_compress = false;
  for (NodePtr node : graph->GetDirectNode()) {
    if (node->GetType() == "Compress") {
      find_compress = true;
      OpDescPtr op_desc = node->GetOpDesc();
      EXPECT_EQ(op_desc->GetInputNameByIndex(0), "weight");
      EXPECT_EQ(op_desc->GetOutputNameByIndex(0), "weight_compress");
      EXPECT_EQ(op_desc->GetOutputNameByIndex(1), "compress_index");
    }
    if (node->GetType() == "Conv2DCompress") {
      vector<string> input_name_vec = node->GetOpDesc()->GetInputName();
      EXPECT_EQ(input_name_vec.size(), 4);
    }
  }
  EXPECT_EQ(find_compress, true);
}