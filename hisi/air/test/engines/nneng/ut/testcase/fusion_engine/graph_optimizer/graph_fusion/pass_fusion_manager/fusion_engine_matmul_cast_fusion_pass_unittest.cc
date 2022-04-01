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
//#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/matmul_cast_fusion_pass.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph/graph.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "common/configuration.h"
#undef protected
#undef private

using namespace std;
using namespace fe;

using FEOpsKernelInfoStorePtr = std::shared_ptr<fe::FEOpsKernelInfoStore>;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

namespace fe {
  static const char *TF_MATMUL = "MatMul";
  static const char *SUB = "Sub";

  bool CheckTbeCastSupportedStub(te::TbeOpInfo &info, te::CheckSupportedResult &is_support,
      string &reason) {
      is_support = te::FULLY_SUPPORTED;
      return true;
  }

static const uint8_t MATMUL_INPUT_NUM = 2;
static const uint8_t MATMUL_OUTPUT_NUM = 1;
static const uint8_t CAST_INPUT_NUM = 1;
static const uint8_t CAST_OUTPUT_NUM = 1;
static const uint8_t SUB_INPUT_NUM = 2;
static const uint8_t SUB_OUTPUT_NUM = 1;
static const int32_t TENSORFLOW_DATATYPE_FLOAT32 = 0;
static const int32_t TENSORFLOW_DATATYPE_FLOAT16 = 19;
static const string MATMUL_DATATYPE_ATTR_KEY = "T";
static const string CAST_DATATYPE_DES_ATTR_KEY = "DstT";
class UTEST_fusion_engine_matmul_cast_fusion_unittest :public testing::Test {
protected:
  FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr;
    void SetUp()
    {
        OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
        TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
        tbe_adapter_ptr->CheckTbeSupported = CheckTbeCastSupportedStub;
        op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

        fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
        FEOpsStoreInfo tbe_builtin {
                0,
                "tbe-builtin",
                EN_IMPL_HW_TBE,
                "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
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

    }

protected:
    ge::NodePtr AddNode(ge::ComputeGraphPtr graph,
                        const string &name,
                        const string &type,
                        unsigned int in_anchor_num,
                        unsigned int out_anchor_num,
                        ge::DataType input_type,
                        ge::DataType output_type)
    {
        ge::GeTensorDesc input_tensor_desc(ge::GeShape({1, 2, 3}), ge::FORMAT_NHWC, input_type);
        ge::GeTensorDesc output_tensor_desc(ge::GeShape({1, 2, 3}), ge::FORMAT_NHWC, output_type);
        ge::OpDescPtr op_desc = make_shared<ge::OpDesc>(name, type);
        for (unsigned int i = 0; i < in_anchor_num; ++i) {
            op_desc->AddInputDesc(input_tensor_desc);
        }
        for (unsigned int i = 0; i < out_anchor_num; ++i) {
            op_desc->AddOutputDesc(output_tensor_desc);
        }
        ge::NodePtr node = graph->AddNode(op_desc);
        return node;
    }

    ge::ComputeGraphPtr CreateMatmulCastGraph()
    {
        // create compute graph
        ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("test_matmul_cast_fusion_graph");
        // create matmul node
        ge::NodePtr matmul_node = AddNode(graph, "matmul", TF_MATMUL, MATMUL_INPUT_NUM, MATMUL_OUTPUT_NUM,
                                         ge::DataType::DT_FLOAT16,
                                         ge::DataType::DT_FLOAT16);
        // create cast node
        ge::NodePtr cast_node = AddNode(graph, "cast", fe::CAST, CAST_INPUT_NUM, CAST_OUTPUT_NUM,
                                       ge::DataType::DT_FLOAT16,
                                       ge::DataType::DT_FLOAT);
        // create sub node
        ge::NodePtr sub_node = AddNode(graph, "sub", SUB, SUB_INPUT_NUM, SUB_OUTPUT_NUM,
                                      ge::DataType::DT_FLOAT,
                                      ge::DataType::DT_FLOAT);
        // link matmul node and cast node
        ge::GraphUtils::AddEdge(matmul_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
        // linkd cast node and sub node
        ge::GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), sub_node->GetInDataAnchor(0));
        return graph;
    }

      ge::ComputeGraphPtr CreateMatmulCastGraph2()
      {
          // create compute graph
          ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("test_matmul_cast_fusion_graph");
          // create matmul node
          ge::NodePtr matmul_node = AddNode(graph, "matmul", TF_MATMUL, MATMUL_INPUT_NUM, MATMUL_OUTPUT_NUM,
                                           ge::DataType::DT_FLOAT16,
                                           ge::DataType::DT_FLOAT16);
          // create cast node
          ge::NodePtr cast_node = AddNode(graph, "cast", fe::CAST, CAST_INPUT_NUM, CAST_OUTPUT_NUM,
                                         ge::DataType::DT_FLOAT16,
                                         ge::DataType::DT_FLOAT);
          // create sub node
          ge::NodePtr sub_node = AddNode(graph, "sub", SUB, SUB_INPUT_NUM, SUB_OUTPUT_NUM,
                                        ge::DataType::DT_FLOAT,
                                        ge::DataType::DT_FLOAT);
          // link matmul node and cast node
          ge::GraphUtils::AddEdge(matmul_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
          // linkd cast node and sub node
          ge::GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), sub_node->GetInDataAnchor(0));
          return graph;
      }
};

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_first)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph();
    auto matmul_node = graph->FindNode("matmul");
    auto cast_node = graph->FindNode("cast");
    shared_ptr<ge::OpDesc> matmul_op = matmul_node->GetOpDesc();
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    ge::AttrUtils::SetDataType(matmul_op, MATMUL_DATATYPE_ATTR_KEY, ge::DT_FLOAT16);
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT);
//    MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
//    EXPECT_EQ(fe::SUCCESS, status);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 2);
//    uint32_t out_t = 1;
//    ge::AttrUtils::GetInt(graph->FindNode("matmul")->GetOpDesc(), "out_T", out_t);
//    EXPECT_EQ(out_t, 0);
//    bool data_type_fixed = false;
//    ge::AttrUtils::GetBool(graph->FindNode("matmul")->GetOpDesc(), "DataTypeFixed", data_type_fixed);
//    EXPECT_EQ(data_type_fixed, true);
//    ge::ConstGeTensorDescPtr tensor_desc = matmul_op->GetOutputDescPtr(0);
//    EXPECT_EQ(tensor_desc->GetDataType(), ge::DataType::DT_FLOAT);
}

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_matmul)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph2();
    auto matmul_node = graph->FindNode("matmul");
    auto cast_node = graph->FindNode("cast");
    shared_ptr<ge::OpDesc> matmul_op = matmul_node->GetOpDesc();
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    ge::AttrUtils::SetDataType(matmul_op, MATMUL_DATATYPE_ATTR_KEY, ge::DT_FLOAT16);
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT);
//    MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph, fe_ops_kernel_info_store_ptr);
//    EXPECT_EQ(fe::SUCCESS, status);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 2);
//    uint32_t out_t = 1;
//    ge::AttrUtils::GetInt(graph->FindNode("matmul")->GetOpDesc(), "out_T", out_t);
//    EXPECT_EQ(out_t, 0);
//    bool data_type_fixed = false;
//    ge::AttrUtils::GetBool(graph->FindNode("matmul")->GetOpDesc(), "DataTypeFixed", data_type_fixed);
//    EXPECT_EQ(data_type_fixed, true);
//    ge::ConstGeTensorDescPtr tensor_desc = matmul_op->GetOutputDescPtr(0);
//    EXPECT_EQ(tensor_desc->GetDataType(), ge::DataType::DT_FLOAT);
}

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_second)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph();
    auto matmul_node = graph->FindNode("matmul");
    auto cast_node = graph->FindNode("cast");
    shared_ptr<ge::OpDesc> matmul_op = matmul_node->GetOpDesc();
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    ge::AttrUtils::SetDataType(matmul_op, MATMUL_DATATYPE_ATTR_KEY, ge::DT_FLOAT);
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT);
//    fe::MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 3);
}

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_third)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph();
    auto cast_node = graph->FindNode("cast");
    auto matmul_node = graph->FindNode("matmul");
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    shared_ptr<ge::OpDesc> matmul_op = matmul_node->GetOpDesc();
    ge::AttrUtils::SetDataType(matmul_op, MATMUL_DATATYPE_ATTR_KEY, ge::DT_FLOAT16);
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT16);
//    fe::MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 3);
}

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_four)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph();
    auto cast_node = graph->FindNode("cast");
    auto matmul_node = graph->FindNode("matmul");
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT16);
//    fe::MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 3);
}

TEST_F(UTEST_fusion_engine_matmul_cast_fusion_unittest, fusion_success_five)
{
    ge::ComputeGraphPtr graph = CreateMatmulCastGraph();
    auto matmul_node = graph->FindNode("matmul");
    auto sub_node = graph->FindNode("sub");
    auto cast_node = graph->FindNode("cast");
    shared_ptr<ge::OpDesc> cast_op = cast_node->GetOpDesc();
    shared_ptr<ge::OpDesc> matmul_op = matmul_node->GetOpDesc();
    ge::AttrUtils::SetDataType(matmul_op, MATMUL_DATATYPE_ATTR_KEY, ge::DT_FLOAT16);
    ge::AttrUtils::SetDataType(cast_op, CAST_DATATYPE_DES_ATTR_KEY, ge::DT_FLOAT);
    ge::GraphUtils::AddEdge(matmul_node->GetOutDataAnchor(0), sub_node->GetInDataAnchor(1));
//    fe::MatmulCastFusionPass pass;
//    fe::Status status = pass.Run(*graph);
//    size_t node_num = graph->GetDirectNode().size();
//    EXPECT_EQ(node_num, 3);
}

}