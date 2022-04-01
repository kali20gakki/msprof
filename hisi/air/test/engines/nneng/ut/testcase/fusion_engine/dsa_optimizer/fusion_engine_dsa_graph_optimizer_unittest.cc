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
#include "graph_optimizer/dsa_optimizer/dsa_graph_optimizer.h"
#include "common/configuration.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph/ge_context.h"
#include "ge/ge_api_types.h"

#include "graph/utils/graph_utils.h"
#include "common/util/op_info_util.h"
#include "common/fe_utils.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/dsa_adapter/dsa_op_store_adapter.h"
#include "ops_store/sub_op_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include "./ge_context.h"
#include "./ge_local_context.h"
#undef protected
#undef private

using namespace testing;
using namespace fe;
using namespace ge;
using DSAOpStoreAdapterPtr = std::shared_ptr<fe::DSAOpStoreAdapter>;
using DSAGraphOptimizerPtr = std::shared_ptr<fe::DSAGraphOptimizer>;
using OpStoreAdapterPtr = std::shared_ptr<fe::OpStoreAdapter>;
class UTEST_fusion_engine_dsa_graph_optimizer : public testing::Test {
 public:
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
  FormatDtypeSetterPtr format_dtype_setter_ptr_;
  OpImplyTypeJudgePtr op_impl_type_judge_ptr_;
  DSAGraphOptimizerPtr dsa_graph_optimizer_;
  DSAOpStoreAdapterPtr dsa_adapter_ptr;
  shared_ptr<fe::SubOpInfoStore> sub_ops_kernel_ptr;
  shared_ptr<fe::SubOpsStore> sub_ops_store_ptr;
 protected:
  void SetUp() {
    dsa_adapter_ptr = std::make_shared<DSAOpStoreAdapter>();
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("dsa_op_adapter", dsa_adapter_ptr));
    ops_kernel_info_store_ptr_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_,
                                                                        fe::kDsaCoreName);
    reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();
    dsa_graph_optimizer_ = make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                          op_store_adapter_manager_ptr_, fe::kDsaCoreName);
    FEOpsStoreInfo TBE_OPINFO_STUB = {
            12,
            "dsa-builtin",
            EN_IMPL_HW_DSA,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/dsa_opinfo",
            ""
    };
    sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("dsa-builtin");
    sub_ops_store_ptr->SetSubStoreInfo(TBE_OPINFO_STUB);
    sub_ops_store_ptr->InitializeSubStore(fe::kDsaCoreName);

    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(TBE_OPINFO_STUB);
    Configuration::Instance(fe::kDsaCoreName).ops_store_info_vector_ = (store_info);

    sub_ops_kernel_ptr = std::make_shared<fe::SubOpInfoStore>(TBE_OPINFO_STUB);
    sub_ops_kernel_ptr->Initialize(fe::kDsaCoreName);
    OpsKernelManager::Instance(fe::kDsaCoreName).sub_ops_kernel_map_.emplace("dsa-builtin", sub_ops_kernel_ptr);

    std::map<std::string, std::string> options;
    options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", SHAPE_GENERALIZED));
    ge::GetThreadLocalContext().SetGlobalOption(options);

    std::map<std::string, std::string> options1;
    OpsKernelManager::Instance(fe::kDsaCoreName).Finalize();
    ops_kernel_info_store_ptr_->Initialize(options1);
  }

  void TearDown() {
    sub_ops_store_ptr->FinalizeSubStore();
    sub_ops_store_ptr.reset();
    sub_ops_kernel_ptr->Finalize();
    sub_ops_kernel_ptr.reset();
    ops_kernel_info_store_ptr_->Finalize();
  }

  static void CreateGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("dsa", "DSAGenBitMask");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr out = std::make_shared<OpDesc>("OUT", fe::NETOUTPUT);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_ND);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetDataType(DT_INT32);
    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_ND);
    in_desc2.SetOriginFormat(FORMAT_ND);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("count", in_desc1);
    bn_op->AddInputDesc("dropout", in_desc2);
    data->AddOutputDesc("y", in_desc1);
    data1->AddOutputDesc("y", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_ND);
    out_desc2.SetOriginFormat(FORMAT_ND);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("out", out_desc2);
    out->AddInputDesc("x", out_desc2);

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr out_node = graph->AddNode(out);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  }

  static void CreateGraphNODSA(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("dsa", "NO_OPS");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr out = std::make_shared<OpDesc>("OUT", fe::NETOUTPUT);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_ND);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetDataType(DT_INT32);
    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_ND);
    in_desc2.SetOriginFormat(FORMAT_ND);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("count", in_desc1);
    bn_op->AddInputDesc("dropout", in_desc2);
    data->AddOutputDesc("y", in_desc1);
    data1->AddOutputDesc("y", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_ND);
    out_desc2.SetOriginFormat(FORMAT_ND);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("out", out_desc2);
    out->AddInputDesc("x", out_desc2);

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr out_node = graph->AddNode(out);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  }

  static void CreateDSAGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("dsa", "DSAGenBitMask");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr out = std::make_shared<OpDesc>("OUT", fe::NETOUTPUT);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_ND);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetDataType(DT_INT32);
    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_ND);
    in_desc2.SetOriginFormat(FORMAT_ND);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("count", in_desc1);
    bn_op->AddInputDesc("dropout", in_desc2);
    data->AddOutputDesc("y", in_desc1);
    data1->AddOutputDesc("y", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_ND);
    out_desc2.SetOriginFormat(FORMAT_ND);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("out", out_desc2);
    (void) ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, EN_IMPL_HW_DSA);
    out->AddInputDesc("x", out_desc2);

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr out_node = graph->AddNode(out);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  }
  static void CreateTbeGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("dsa", "DSAGenBitMask");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr out = std::make_shared<OpDesc>("OUT", fe::NETOUTPUT);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_ND);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetDataType(DT_INT32);
    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_ND);
    in_desc2.SetOriginFormat(FORMAT_ND);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("count", in_desc1);
    bn_op->AddInputDesc("dropout", in_desc2);
    data->AddOutputDesc("y", in_desc1);
    data1->AddOutputDesc("y", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_ND);
    out_desc2.SetOriginFormat(FORMAT_ND);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("out", out_desc2);
    (void) ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    out->AddInputDesc("x", out_desc2);

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr out_node = graph->AddNode(out);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  }
};

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, get_attributes_success)
{
  GraphOptimizerAttribute attrs;
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);
  Status status = dsa_graph_optimizer_->GetAttributes(attrs);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, dsa_op_store_adapter_finalize)
{
  std::map<std::string, std::string> options;
  dsa_adapter_ptr->init_flag = true;
  dsa_adapter_ptr->Initialize(options, kDsaCoreName);
  dsa_adapter_ptr->init_flag = false;
  dsa_adapter_ptr->Finalize();
  dsa_adapter_ptr->init_flag = true;
  dsa_adapter_ptr->Finalize();
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, initialize_success)
{
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);
  std::map<std::string, std::string> options;
  dsa_graph_optimizer_->init_flag_ = true;
  Status status = dsa_graph_optimizer_->Initialize(options, nullptr);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, finalize_success)
{
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);
  dsa_graph_optimizer_->init_flag_ = true;
  Status status = dsa_graph_optimizer_->Finalize();
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, finalize_success1)
{
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);
  dsa_graph_optimizer_->init_flag_ = false;
  Status status = dsa_graph_optimizer_->Finalize();
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_original_graph_init_again)
{
  std::map<std::string, std::string> options;
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  op_store_adapter_manager_ptr_->InitializeAdapter("dsa_op_adapter", options, kDsaCoreName);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);

  dsa_graph_optimizer_->init_flag_ = false;
  Status status = dsa_graph_optimizer_->OptimizeOriginalGraph(*(graph.get()));
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_original_graph_fail1)
{
  std::map<std::string, std::string> options;
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateGraphNODSA(graph);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                  op_store_adapter_manager_ptr_, kDsaCoreName);

  (void)dsa_graph_optimizer_->Initialize(options, nullptr);
  Status status = dsa_graph_optimizer_->OptimizeOriginalGraph(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_original_graph)
{
  std::map<std::string, std::string> options;
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  op_store_adapter_manager_ptr_->InitializeAdapter("dsa_op_adapter", options, kDsaCoreName);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                   op_store_adapter_manager_ptr_, kDsaCoreName);

  (void)dsa_graph_optimizer_->Initialize(options, nullptr);
  Status status = dsa_graph_optimizer_->OptimizeOriginalGraph(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetAllNodes()) {
    if (node->GetType() == "DSAGenBitMask") {
      int64_t fe_impl_type = -1;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), FE_IMPLY_TYPE, fe_impl_type);
      EXPECT_EQ(fe_impl_type, static_cast<OpImplType>(EN_IMPL_HW_DSA));
      std::string op_slice_info;
      (void)ge::AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
      std::cout << "Node DSAGenBitMask slice info is：" << op_slice_info << endl;
    }
  }
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_original_graph1)
{
  std::map<std::string, std::string> options;
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.clear();
  op_store_adapter_manager_ptr_->InitializeAdapter("dsa_op_adapter", options, kDsaCoreName);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                   op_store_adapter_manager_ptr_, kDsaCoreName);

  (void)dsa_graph_optimizer_->Initialize(options, nullptr);
  Status status = dsa_graph_optimizer_->OptimizeOriginalGraph(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetAllNodes()) {
    if (node->GetType() == "DSAGenBitMask") {
      int64_t fe_impl_type = -1;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), FE_IMPLY_TYPE, fe_impl_type);
      EXPECT_EQ(fe_impl_type, static_cast<OpImplType>(EN_IMPL_HW_DSA));
      std::string op_slice_info;
      (void)ge::AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_slice_info);
      std::cout << "Node DSAGenBitMask slice info is：" << op_slice_info << endl;
    }
  }
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_original_graph2)
{
  std::map<std::string, std::string> options;
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTbeGraph(graph);
  auto format_dtype_setter_ptr_ = std::make_shared<FormatDtypeSetter>(kDsaCoreName, op_store_adapter_manager_ptr_);
  Status status = format_dtype_setter_ptr_->SetSupportFormatDtype(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_fused_graph)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateDSAGraph(graph);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                   op_store_adapter_manager_ptr_, kDsaCoreName);

  std::map<std::string, std::string> options;
  (void)dsa_graph_optimizer_->Initialize(options, nullptr);
  Status status = dsa_graph_optimizer_->OptimizeFusedGraph(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetAllNodes()) {
    if (node->GetType() == "DSAGenBitMask") {
      std::vector<int32_t> memory_no_reuse;
      (void)ge::AttrUtils::GetListInt(node->GetOpDesc(), ge::ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, memory_no_reuse);
      std::vector<int32_t> memory_no_reuse1 = {1, 1};
      EXPECT_EQ(memory_no_reuse1, memory_no_reuse);
      std::vector<int64_t> workspace_bytes = {48, 48};
      EXPECT_EQ(node->GetOpDesc()->GetWorkspaceBytes(), workspace_bytes);
    }
  }
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, optimize_fused_graph_failed)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateDSAGraph(graph);
  auto dsa_graph_optimizer_ = std::make_shared<DSAGraphOptimizer>(ops_kernel_info_store_ptr_,
                                                                   op_store_adapter_manager_ptr_, kDsaCoreName);

  std::map<std::string, std::string> options;
  (void)dsa_graph_optimizer_->Initialize(options, nullptr);
  dsa_graph_optimizer_->init_flag_ = false;
  Status status = dsa_graph_optimizer_->OptimizeFusedGraph(*(graph.get()));
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, check_supported)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);

  std::map<std::string, std::string> options;
  (void)ops_kernel_info_store_ptr_->Initialize(options);
  for (auto node : graph->GetAllNodes()) {
    if (node->GetType() == "DSAGenBitMask") {
      std::string reason;
      bool res = ops_kernel_info_store_ptr_->CheckSupported(node->GetOpDesc(), reason);
      EXPECT_EQ(true, res);
    }
  }
}

TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, fe_util_ut1)
{
  std::string engine_name = AI_CORE_NAME;
  OpImplType op_impl_type = EN_IMPL_HW_DSA;
  bool ret = IsInvalidImplType(engine_name, op_impl_type);
  EXPECT_EQ(ret, true);
}
TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, fe_util_ut2)
{
  std::string engine_name = kDsaCoreName;
  OpImplType op_impl_type = EN_IMPL_HW_TBE;
  bool ret = IsInvalidImplType(engine_name, op_impl_type);
  EXPECT_EQ(ret, true);
}
TEST_F(UTEST_fusion_engine_dsa_graph_optimizer, fe_util_ut3)
{
  std::string engine_name = kDsaCoreName;
  OpImplType op_impl_type = EN_IMPL_HW_DSA;
  bool ret = IsInvalidImplType(engine_name, op_impl_type);
  EXPECT_EQ(ret, false);
}
