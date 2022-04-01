/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "ge_graph_dsl/graph_dsl.h"

#include "register/ffts_plus_update_manager.h"
#include "register/op_tiling_registry.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "test_tools_task_info.h"
#include "framework/executor/ge_executor.h"
#include "depends/runtime/src/runtime_stub.h"
#include "graph/execute/model_executor.h"
#include "test_tools_hook_func.h"
#include "register/op_tiling.h"

using namespace std;
using namespace testing;
using namespace optiling;

namespace ge {
class FFTSPlusTaskUpdateStub : public FFTSPlusTaskUpdate {
 public:
  Status GetAutoThreadParam(const NodePtr &node, const std::vector<optiling::utils::OpRunInfo> &op_run_info,
                            AutoThreadParam &auto_thread_param) {
    auto_thread_param.thread_dim = 32U;
    auto_thread_param.input_output_num = node->GetAllInDataAnchorsSize() + node->GetAllOutDataAnchorsSize();

    std::vector<int64_t> workspaces = node->GetOpDesc()->GetWorkspaceBytes();
    auto_thread_param.input_output_num += workspaces.size();

    auto_thread_param.task_addr_offset.resize(auto_thread_param.input_output_num, 32U);

    std::string core_type;
    (void)AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
    if (core_type == "AICPU") {
      auto_thread_param.args_size = 64U;
      auto_thread_param.extinfo_size = 32U;
    }
    return SUCCESS;
  }

  Status UpdateSubTaskAndCache(const NodePtr &node, const AutoThreadSubTaskFlush &sub_task_flush,
                               rtFftsPlusTaskInfo_t &ffts_plus_task_info) {
    return SUCCESS;
  }
};

graphStatus OpFftsCalculateStub1(const Node &node, std::vector<OpRunInfoV2> &op_run_info) {
  std::string cube_vector_core_type;
  AttrUtils::GetStr(node.GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, cube_vector_core_type);

  // Same TilingKey, 0 for all.
  if (cube_vector_core_type == "AIC") {
    op_run_info.resize(2, OpRunInfoV2(64U, false, 0U));
  } else if (cube_vector_core_type == "MIX_AIV") {
    op_run_info.resize(4, OpRunInfoV2(64U, false, 0U));
  }

  return GRAPH_SUCCESS;
}

graphStatus OpFftsCalculateStub2(const Node &node, std::vector<OpRunInfoV2> &op_run_info) {
  std::string cube_vector_core_type;
  AttrUtils::GetStr(node.GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, cube_vector_core_type);

  if (cube_vector_core_type == "AIC") {
    op_run_info.resize(2, OpRunInfoV2(64U, false, 0U));
    op_run_info[1].SetTilingKey(1);

    const std::vector<char> tiling_data(126, 0);
    op_run_info[0].AddTilingData(tiling_data.data(), tiling_data.size());
    op_run_info[0].SetWorkspaces({100, 100});
  } else if (cube_vector_core_type == "MIX_AIV") {
    op_run_info.resize(4, OpRunInfoV2(64U, false, 0U));
    op_run_info[1].SetTilingKey(1);
    op_run_info[3].SetTilingKey(1);
  }

  return GRAPH_SUCCESS;
}

class FftsPlusTest : public testing::Test {
 protected:
  void SetUp() {
    GeExecutor::Initialize({});
    RTS_STUB_SETUP();

    // Register from FE, set stub here.
    const std::string kCoreTypeAIC = "AIC";     // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    const std::string kCoreTypeAIV = "MIX_AIV"; // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    const std::string kCoreTypeCPU = "AICPU";   // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIC, FFTSPlusTaskUpdateStub);
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIV, FFTSPlusTaskUpdateStub);
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeCPU, FFTSPlusTaskUpdateStub);

    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(CONV2D);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(CONV2D);
    const auto op_tiling_func_aic = [](const Operator &op, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
      static uint32_t tiling_key = 0;
      op_run_info.SetTilingKey(tiling_key % 2U);
      tiling_key++;

      const std::vector<char> tiling_data(126, 0);
      op_run_info.AddTilingData(tiling_data.data(), tiling_data.size());
      op_run_info.SetWorkspaces({100, 100});
      return true;
    };
    REGISTER_OP_TILING_UNIQ_V2(Conv2D, op_tiling_func_aic, 201);
    OpTilingRegistryInterf_V2(CONV2D, op_tiling_func_aic);

    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(RELU);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(RELU);
    const auto op_tiling_func_mix = [](const Operator &op, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
      static uint32_t tiling_key = 0;
      op_run_info.SetTilingKey(tiling_key % 4U);
      tiling_key++;

      op_run_info.SetWorkspaces({100, 128, 96});
      return true;
    };
    REGISTER_OP_TILING_UNIQ_V2(ReLU, op_tiling_func_mix, 201);
    OpTilingRegistryInterf_V2(RELU, op_tiling_func_mix);
  }

  void TearDown() {
    TearDownHooks();
    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(CONV2D);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(CONV2D);
    OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(RELU);
    OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(RELU);

    RTS_STUB_TEARDOWN();
    GeExecutor::FinalizeEx();
  }
};

/***********************************************************************************************************************
 *                                      Data             Data
 * Data           Data                   |                 |
 *   \             /                     |                 |
 *    \           /                  TransData         TransData
 *     \         /                       \                 /                    Data      Data
 *      \       /                         \               /                       \        /
 *   PartitionedCall                       \             /                         \      /
 *          |                               \           /                           Conv2D
 *          |                              PartitionedCall                             |
 *          |                                     |                                    |
 *          |                                     |                                  Relu
 *      NetOutput                                 |                                    |
 *                                                |                                    |
 *                                            TransData                            NetOutput
 *                                                |
 *                                                |
 *                                            NetOutput
***********************************************************************************************************************/
static void BuildFftsDynamicGraph(ComputeGraphPtr &root_graph, ComputeGraphPtr &dsp_graph, ComputeGraphPtr &ffts_graph, uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  root_graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(root_graph, mem_offset, true);
  const auto root_call_0 = root_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(root_call_0, nullptr);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g2) {
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    CHAIN(NODE("dsp_graph/_arg_0", data_0)->EDGE(0, 0)->
          NODE("dsp_graph/trans_TransData_0", IDENTITY)->EDGE(0, 0)->
          NODE("dsp_graph/PartitionedCall_0", PARTITIONEDCALL)->EDGE(0, 0)->
          NODE("dsp_graph/trans_TransData_2", IDENTITY)->EDGE(0, 0)->
          NODE("dsp_graph/Node_Output", NETOUTPUT)
    );
    CHAIN(NODE("dsp_graph/_arg_1", data_1)->EDGE(0, 0)->
          NODE("dsp_graph/trans_TransData_1", IDENTITY)->EDGE(0, 1)->
          NODE("dsp_graph/PartitionedCall_0")
    );
  };
  dsp_graph = ToComputeGraph(g2);
  dsp_graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(dsp_graph, mem_offset);
  AddPartitionedCall(root_graph, "PartitionedCall_0", dsp_graph);

  const auto ffts_call_node = dsp_graph->FindNode("dsp_graph/PartitionedCall_0");
  EXPECT_NE(ffts_call_node, nullptr);
  AttrUtils::SetBool(ffts_call_node->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3) {
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
                                .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                                .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    auto relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIV")
                              .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                              .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    auto gather = OP_CFG(GATHERV2).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AICPU")
                                  .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU))
                                  .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    CHAIN(NODE("sgt_graph/_arg_0", data_0)->EDGE(0, 0)->
          NODE("sgt_graph/Conv2D", conv_0)->EDGE(0, 0)->
          NODE("sgt_graph/Relu", relu_0)->EDGE(0, 0)->
          NODE("sgt_graph/Gather", gather)->EDGE(0, 0)->
          NODE("sgt_graph/Node_Output", NETOUTPUT)
    );
    CHAIN(NODE("sgt_graph/_arg_1", data_1)->EDGE(0, 1)->
          NODE("sgt_graph/Conv2D", conv_0)
    );
  };
  ffts_graph = ToComputeGraph(g3);
  ffts_graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(ffts_graph, mem_offset);
  AddPartitionedCall(dsp_graph, "dsp_graph/PartitionedCall_0", ffts_graph);

  const auto ffts_conv0 = ffts_graph->FindNode("sgt_graph/Conv2D");
  EXPECT_NE(ffts_conv0, nullptr);
  const auto ffts_relu0 = ffts_graph->FindNode("sgt_graph/Relu");
  EXPECT_NE(ffts_relu0, nullptr);

  InitFftsThreadSliceMap(ffts_conv0->GetOpDesc());
  InitFftsThreadSliceMap(ffts_relu0->GetOpDesc());

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<char> conv_bin(64, '\0');
  TBEKernelPtr conv_kernel = MakeShared<ge::OpKernelBin>("sgt/conv", std::move(conv_bin));
  ffts_conv0->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, conv_kernel);
  AttrUtils::SetStr(ffts_conv0->GetOpDesc(), ffts_conv0->GetName() + "_kernelname", "sgt/conv");
  AttrUtils::SetInt(ffts_conv0->GetOpDesc(), "op_para_size", 2); // for tiling
  AttrUtils::SetBool(ffts_conv0->GetOpDesc(), "_kernel_list_first_name", true); // for call rtallkernel register
  std::vector<char> relu_bin(64, '\0');
  TBEKernelPtr relu_kernel = MakeShared<ge::OpKernelBin>("sgt/relu", std::move(relu_bin));
  ffts_relu0->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, relu_kernel);
  AttrUtils::SetStr(ffts_relu0->GetOpDesc(), ffts_relu0->GetName() + "_kernelname", "sgt/relu");
  AttrUtils::SetInt(ffts_relu0->GetOpDesc(), "op_para_size", 2);
  AttrUtils::SetBool(ffts_relu0->GetOpDesc(), "_kernel_list_first_name", true);
}

static void SetAicAivOpKernel(const ComputeGraphPtr &graph, const std::string name) {
  const auto &node = graph->FindNode(name);
  EXPECT_NE(node, nullptr);
  const auto &op_desc = node->GetOpDesc();
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
  std::vector<char> aic_kernel_bin(64, '\0');
  std::vector<char> aiv_kernel_bin(64, '\0');
  std::vector<TBEKernelPtr> tbe_kernel_vec{
      std::make_shared<ge::OpKernelBin>(op_desc->GetName(), std::move(aic_kernel_bin)),
      std::make_shared<ge::OpKernelBin>(op_desc->GetName(), std::move(aiv_kernel_bin))
  };
  op_desc->SetExtAttr(OP_EXTATTR_NAME_THREAD_TBE_KERNEL, tbe_kernel_vec);
  std::vector<string> bin_file_keys{ op_desc->GetName() + "_aic", op_desc->GetName() + "_aiv" };
  (void)AttrUtils::SetListStr(op_desc, "_register_stub_func", bin_file_keys);
  (void)AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName());
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_THREAD_MODE, 1);
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_THREAD_SCOPE_ID, 1);
  // Init Binary Magic
  std::vector<std::string> json_list{ "RT_DEV_BINARY_MAGIC_ELF_AIVEC", "RT_DEV_BINARY_MAGIC_ELF_AICUBE" };
  (void)AttrUtils::SetListStr(op_desc, "_thread_tvm_magic", json_list);
  // Init meta data
  std::vector<std::string> meta_data_list{ "AIVEC_META_DATA", "AICUBE_META_DATA" };
  (void)AttrUtils::SetListStr(op_desc, "_thread_tvm_metadata", meta_data_list);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<string> thread_kernel_names{ "aictest", "aivtest" };
  (void)AttrUtils::SetListStr(op_desc, "_thread_kernelname", thread_kernel_names);
}

/***********************************************************************************************************************
 *
 * Data    Data    Data
 *   \      |       /
 *    \     |      /
 *     \    |     /                                       Data      Data
 *      \   |    /                                          \        /
 *   PartitionedCall                                        \      /
 *          |                                          Data  Conv2D
 *          |                                              \   |
 *          |                                               \  |
 *          |                                                Add
 *          |                                                  |
 *          |                                                Relu
 *      NetOutput                                             |
 *                                                            |
 *                                                        NetOutput
 *
 *
 **********************************************************************************************************************/
static void BuildFftsPlusGraph(ComputeGraphPtr &root_graph, ComputeGraphPtr &ffts_plus_graph) {
  uint32_t mem_offset = 0U;
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
    CHAIN(NODE("_arg_2", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  SetUnknownOpKernel(root_graph, mem_offset, true);

  DEF_GRAPH(g2) {
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    auto conv_0 = OP_CFG(CONV2D)
                      .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                      .Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
                      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    auto relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU));
    auto add_0 = OP_CFG(ADD).Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU));
    CHAIN(NODE("sgt_graph/_arg_0", data_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Conv2D", conv_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Add", add_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Relu", relu_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt_graph/_arg_1", data_1)->EDGE(0, 1)->NODE("sgt_graph/Conv2D", conv_0));
    CHAIN(NODE("sgt_graph/_arg_2", data_2)->EDGE(0, 1)->NODE("sgt_graph/Add", add_0));
  };
  ffts_plus_graph = ToComputeGraph(g2);
  SetUnknownOpKernel(ffts_plus_graph, mem_offset);
  AddPartitionedCall(root_graph, "PartitionedCall_0", ffts_plus_graph);

  SetAicAivOpKernel(ffts_plus_graph, "sgt_graph/Conv2D");
}

static void BuildFftsPlusGraphMixL2(ComputeGraphPtr &root_graph, ComputeGraphPtr &dsp_graph, NodePtr &mix_l2_node,
                                 NodePtr &mix_l2_node_static, uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  root_graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(root_graph, mem_offset, true);
  const auto root_call_0 = root_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(root_call_0, nullptr);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3) {
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto conv_0 = OP_CFG(CONV2D)
                      .Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
                      .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    auto conv_1 = OP_CFG(CONV2D)
        .Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
        .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
        .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    CHAIN(NODE("sgt_graph/_arg_0", data_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Conv2D", conv_0)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Conv2D_static", conv_1)
              ->EDGE(0, 0)
              ->NODE("sgt_graph/Node_Output", NETOUTPUT));
    CHAIN(NODE("sgt_graph/_arg_1", data_1)->EDGE(0, 1)->NODE("sgt_graph/Conv2D", conv_0));
    CHAIN(NODE("sgt_graph/_arg_1", data_1)->EDGE(0, 1)->NODE("sgt_graph/Conv2D_static", conv_1));
  };
  dsp_graph = ToComputeGraph(g3);
  dsp_graph->SetGraphUnknownFlag(true);
  SetUnknownOpKernel(dsp_graph, mem_offset);
  AddPartitionedCall(root_graph, "PartitionedCall_0", dsp_graph);

  auto PrePareForNode = [](const NodePtr& mix_l2_node, bool is_dynamic){
    AttrUtils::SetStr(mix_l2_node->GetOpDesc(), "compile_info_json", "stub_json");
    AttrUtils::SetStr(mix_l2_node->GetOpDesc(), "compile_info_key", "stub_key");
    AttrUtils::SetStr(mix_l2_node->GetOpDesc(), ATTR_NAME_ALIAS_ENGINE_NAME, "mix_l2");
    AttrUtils::SetBool(mix_l2_node->GetOpDesc(), ATTR_NAME_FORCE_UNKNOWN_SHAPE, is_dynamic);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<char> conv_bin(64, '\0');
    TBEKernelPtr conv_kernel = MakeShared<ge::OpKernelBin>("sgt/conv", std::move(conv_bin));
    mix_l2_node->GetOpDesc()->SetExtAttr(std::string("_mix_aic") + OP_EXTATTR_NAME_TBE_KERNEL, conv_kernel);
    AttrUtils::SetStr(mix_l2_node->GetOpDesc(), "_mix_aic" + mix_l2_node->GetName() + "_kernelname", "mixaic_a");
    mix_l2_node->GetOpDesc()->SetExtAttr(std::string("_mix_aiv") + OP_EXTATTR_NAME_TBE_KERNEL, conv_kernel);
    AttrUtils::SetStr(mix_l2_node->GetOpDesc(), "_mix_aiv" + mix_l2_node->GetName() + "_kernelname", "mixaiv_b");
    if (is_dynamic) {
      AttrUtils::SetBool(mix_l2_node->GetOpDesc(), "_mix_aiv_kernel_list_first_name", true); // all kernel register
      AttrUtils::SetInt(mix_l2_node->GetOpDesc(), "op_para_size", 2); //tiling
    } else {
      AttrUtils::SetInt(mix_l2_node->GetOpDesc(), "op_para_size", 0); // no need tiling
    }
  };
  mix_l2_node = dsp_graph->FindNode("sgt_graph/Conv2D");
  EXPECT_NE(mix_l2_node, nullptr);
  PrePareForNode(mix_l2_node, true);
  mix_l2_node_static = dsp_graph->FindNode("sgt_graph/Conv2D_static");
  EXPECT_NE(mix_l2_node_static, nullptr);
  PrePareForNode(mix_l2_node_static, false);
}

static void RunFftsPlusDynamicGraph(const FlowModelPtr &flow_model) {
  // Callback for Execute.
  std::mutex run_mutex;
  std::condition_variable model_run_cv;
  Status run_status = FAILED;
  std::vector<Tensor> output_tensors;
  const auto call_back = [&](Status status, std::vector<Tensor> &outputs) {
    std::unique_lock<std::mutex> lock(run_mutex);
    run_status = status;
    output_tensors.swap(outputs);
    model_run_cv.notify_one();
  };

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);
  graph_node->Lock();

  // Load for GraphNode.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_model, graph_node), SUCCESS);

  // Execute for Dynamic Graph.
  int64_t value_0 = 110;
  int64_t value_1 = 110;
  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  const std::vector<Tensor> input_tensors{tensor_0, tensor_1};

  GEThreadLocalContext context;
  error_message::Context error_context;
  RunArgs run_args{graph_node, graph_id, 2001, error_context, input_tensors, flow_model, context, call_back};
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  std::unique_lock<std::mutex> lock(run_mutex);
  EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
  EXPECT_EQ(run_status, SUCCESS);

  // Unload test Graph.
  EXPECT_EQ(model_executor.UnloadGraph(flow_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);

  auto ge_root_model = std::dynamic_pointer_cast<GeRootModel>(flow_model->GetSubmodels().begin()->second);
  if (ge_root_model != nullptr) {
    ge_root_model->ClearAllModelId();
  }
}

TEST_F(FftsPlusTest, ffts_plus_dynamic_graph) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr dsp_graph;
  ComputeGraphPtr sgt_graph;
  uint32_t mem_offset = 0U;
  BuildFftsDynamicGraph(root_graph, dsp_graph, sgt_graph, mem_offset);
  const auto ffts_call_node = dsp_graph->FindNode("dsp_graph/PartitionedCall_0");

  ASSERT_EQ(dsp_graph->TopologicalSorting(), GRAPH_SUCCESS);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(root_graph, "ffts_plus_dynamic");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");

  // Build FftsPlusTaskDef.
  const auto model_def = MakeShared<domi::ModelTaskDef>();
  domi::TaskDef &task_def = *model_def->add_task();
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(ffts_call_node->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_AIV, mutable_aic_aiv_ctx, InitAicAivCtx, sgt_graph->FindNode("sgt_graph/Conv2D"), false);
  ADD_FFTS_PLUS_CTX_MANUAL(RT_CTX_TYPE_AIV, mutable_aic_aiv_ctx, InitAicAivCtx, sgt_graph->FindNode("sgt_graph/Conv2D"), false);
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_MIX_AIC, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, sgt_graph->FindNode("sgt_graph/Relu"), false, false);
  ADD_FFTS_PLUS_CTX_MANUAL(RT_CTX_TYPE_MIX_AIC, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, sgt_graph->FindNode("sgt_graph/Relu"), false, false);
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_MIX_AIV, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, sgt_graph->FindNode("sgt_graph/Relu"), false, false);
  ADD_FFTS_PLUS_CTX_MANUAL(RT_CTX_TYPE_MIX_AIV, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, sgt_graph->FindNode("sgt_graph/Relu"), false, false);
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_NOTIFY_RECORD, mutable_notify_ctx, InitNotifyCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_WRITE_VALUE, mutable_write_value_ctx, InitWriteValueCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_SDMA, mutable_sdma_ctx, InitSdmaCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_FLUSH_DATA, mutable_data_ctx, InitDataCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_INVALIDATE_DATA, mutable_data_ctx, InitDataCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_WRITEBACK_DATA, mutable_data_ctx, InitDataCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_AICPU, mutable_aicpu_ctx, InitAicpuFwkCtxAndExtInfo, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_COND_SWITCH, mutable_cond_switch_ctx, InitCondSwitchCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_CASE_SWITCH, mutable_case_switch_ctx, InitCaseSwitchCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_CASE_SWITCH, mutable_case_default_ctx, InitCaseDefaultCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_AT_START, mutable_at_start_ctx, InitAtStartCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_AT_END, mutable_at_end_ctx, InitAtEndCtx, sgt_graph->FindNode("sgt_graph/Gather"));
  ADD_FFTS_PLUS_CTX(RT_CTX_TYPE_LABEL, mutable_label_ctx, InitLabelCtx, sgt_graph->FindNode("sgt_graph/Gather"));

  // Build GeModel.
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetModelName(root_graph->GetName());
  GeModelPtr ge_sub_model = MakeShared<GeModel>();
  ge_sub_model->SetModelTaskDef(model_def);

  ge_sub_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(dsp_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(dsp_graph->GetName(), ge_sub_model);

  // Test: OpFftsCalculateV2 not Ready.
  RunFftsPlusDynamicGraph(flow_root_model);

  // Same TilingKey [0, 0].
  SetNormalHook(optiling::OpFftsCalculateV2, OpFftsCalculateStub1);
  RunFftsPlusDynamicGraph(flow_root_model);

  // Diff TilingKey [0, 1].
  SetNormalHook(optiling::OpFftsCalculateV2, OpFftsCalculateStub2);
  RunFftsPlusDynamicGraph(flow_root_model);

  // Just delete hook.
  DelNormalHook(optiling::OpFftsCalculateV2);
}

static void BuildDSAGraph(ComputeGraphPtr &root_graph) {
 uint32_t mem_offset = 0U;
 DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
    CHAIN(NODE("_arg_2", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  SetUnknownOpKernel(root_graph, mem_offset, true);
}

TEST_F(FftsPlusTest, dsa_graph) {
  ComputeGraphPtr root_graph;
  BuildDSAGraph(root_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  EXPECT_NE(root_graph, nullptr);

  InitDSATaskDef(root_graph, *model_task_def, "PartitionedCall_0", true);

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, dsa_graph_set_input1_value) {
  ComputeGraphPtr root_graph;
  BuildDSAGraph(root_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  EXPECT_NE(root_graph, nullptr);

  InitDSATaskDef(root_graph, *model_task_def, "PartitionedCall_0", false);

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, ffts_plus_graph_load_success) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_plus_graph;
  BuildFftsPlusGraph(root_graph, ffts_plus_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def= MakeShared<domi::ModelTaskDef>();
  auto &task_def = *model_task_def->add_task();
  InitFftsplusTaskDef(ffts_plus_graph, task_def);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  auto &aic_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusAicCtxDef(ffts_plus_graph, aic_ctx_def, "sgt_graph/Conv2D");
  auto &aicpu_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusAicpuCtxDef(ffts_plus_graph, aicpu_ctx_def, "sgt_graph/Add");
  auto &aicpu_ctx_def_relu = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusAicpuFwkCtxDef(ffts_plus_graph, aicpu_ctx_def_relu, "sgt_graph/Relu");

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, ffts_plus_graph_with_aicpu_load_success) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_plus_graph;
  BuildFftsPlusGraph(root_graph, ffts_plus_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def= MakeShared<domi::ModelTaskDef>();
  auto &task_def = *model_task_def->add_task();
  InitFftsplusTaskDef(ffts_plus_graph, task_def);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  auto &aic_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusAicCtxDef(ffts_plus_graph, aic_ctx_def, "sgt_graph/Conv2D");
  auto &aicpu_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusAicpuCtxDef(ffts_plus_graph, aicpu_ctx_def, "sgt_graph/Add");
  auto &custom_aicpu_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitCustomFftsPlusAicpuCtxDef(ffts_plus_graph, custom_aicpu_ctx_def, "sgt_graph/Relu");

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, ffts_plus_graph_mixl2) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr dsp_graph;
  NodePtr mixl2_node_dynamic;
  NodePtr mixl2_node_static;
  uint32_t mem_offset = 0;
  BuildFftsPlusGraphMixL2(root_graph, dsp_graph, mixl2_node_dynamic, mixl2_node_static,mem_offset);
  ASSERT_EQ(dsp_graph->TopologicalSorting(), GRAPH_SUCCESS);
  // Build FftsPlusTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_def = MakeShared<domi::ModelTaskDef>();
  {
    domi::TaskDef &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
    domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
    ffts_plus_task_def->set_op_index(mixl2_node_dynamic->GetOpDesc()->GetId());
    InitTaskSQEInfo(ffts_plus_task_def);
    InitTaskAdditionalDataInfo(ffts_plus_task_def);
    ADD_FFTS_PLUS_MIX_CTX(RT_CTX_TYPE_MIX_AIC, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, mixl2_node_dynamic, true, false)
    ADD_FFTS_PLUS_MIX_CTX(RT_CTX_TYPE_MIX_AIC, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, mixl2_node_dynamic, false, false)
  }
  {
    domi::TaskDef &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
    domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
    ffts_plus_task_def->set_op_index(mixl2_node_static->GetOpDesc()->GetId());
    InitTaskSQEInfo(ffts_plus_task_def);
    InitTaskAdditionalDataInfo(ffts_plus_task_def);
    int index = 0;
    ADD_FFTS_PLUS_MIX_CTX(RT_CTX_TYPE_MIX_AIC, mutable_mix_aic_aiv_ctx, InitMixAicAivCtx, mixl2_node_static, false)
  }
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  // Build GeModel.
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetModelName(root_graph->GetName());
  GeModelPtr ge_sub_model = MakeShared<GeModel>();
  ge_sub_model->SetModelTaskDef(model_def);

  ge_sub_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(dsp_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(dsp_graph->GetName(), ge_sub_model);

  // For _alias_engine_name Node, workspace checked by optiling::PostProcCalculateV2.
  OpTilingFuncRegistry::RegisteredOpFuncInfo().erase(CONV2D);
  OpTilingRegistryInterf_V2::RegisteredOpInterf().erase(CONV2D);
  const auto op_tiling_func = [](const Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &op_run_info) {
    return true;
  };
  REGISTER_OP_TILING_UNIQ_V2(Conv2D, op_tiling_func, 202);
  OpTilingRegistryInterf_V2(CONV2D, op_tiling_func);

  // Load and run asynchronous for FftsPlus Model.
  RunFftsPlusDynamicGraph(flow_root_model);
}

TEST_F(FftsPlusTest, ffts_plus_graph_init) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_plus_graph;
  BuildFftsPlusGraph(root_graph, ffts_plus_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def= MakeShared<domi::ModelTaskDef>();
  auto &task_def = *model_task_def->add_task();
  InitFftsplusTaskDef(ffts_plus_graph, task_def);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  auto &case_defalut_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusCaseDefaultDef(ffts_plus_graph, case_defalut_ctx_def, "sgt_graph/Conv2D");
  auto &notify_ctx_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusNotifyDef(ffts_plus_graph, notify_ctx_def, "sgt_graph/Add");
  auto &write_value_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitWriteValueDef(ffts_plus_graph, write_value_def, "sgt_graph/Relu");

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, ffts_plus_graph_init_task_def) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_plus_graph;
  BuildFftsPlusGraph(root_graph, ffts_plus_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def= MakeShared<domi::ModelTaskDef>();
  auto &task_def = *model_task_def->add_task();
  InitFftsplusTaskDef(ffts_plus_graph, task_def);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  auto &sdma_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitSdmaDef(ffts_plus_graph, sdma_def, "sgt_graph/Conv2D");
  auto &data_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitDataCtx(ffts_plus_graph, data_def, "sgt_graph/Add");
  auto &switch_def = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitCondSwitchCtx(ffts_plus_graph, switch_def, "sgt_graph/Relu");

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(FftsPlusTest, ffts_plus_graph_cache_persist) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr ffts_plus_graph;
  BuildFftsPlusGraph(root_graph, ffts_plus_graph);

  // Build FftsTaskDef.
  std::shared_ptr<domi::ModelTaskDef> model_task_def= MakeShared<domi::ModelTaskDef>();
  auto &task_def = *model_task_def->add_task();
  InitFftsplusTaskDef(ffts_plus_graph, task_def);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  auto &cache_persist_ctx = *ffts_plus_task_def.add_ffts_plus_ctx();
  InitFftsPlusCachePersistDef(ffts_plus_graph, cache_persist_ctx, "sgt_graph/Conv2D");

  // Build GeModel.
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetName(root_graph->GetName());
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(root_graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  auto flow_root_model = MakeShared<FlowModel>(root_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // Test for Load.
  ModelExecutor model_executor;
  ASSERT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  ASSERT_EQ(model_executor.Finalize(), SUCCESS);
}
} // namespace ge
