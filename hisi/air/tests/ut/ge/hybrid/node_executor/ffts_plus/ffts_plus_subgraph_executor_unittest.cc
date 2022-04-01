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

#define private public
#define protected public
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/model/hybrid_model.h"
#include "register/ffts_plus_update_manager.h"
#include "register/op_tiling_registry.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "common/tbe_handle_store/tbe_handle_store.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "framework/common/types.h"
#include "common/kernel_store.h"
#include "common/model/ge_root_model.h"
#include "hybrid/hybrid_davinci_model.h"

using namespace std;
using namespace testing;
using namespace optiling;

namespace ge {
using namespace hybrid;
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

class UtestFftsPlusSubgraphExecutor : public testing::Test {
 protected:
  void SetUp() {
    auto &engine_mapping = NodeExecutorManager::GetInstance().engine_mapping_;
    engine_mapping.emplace("DNN_VM_RTS_OP_STORE", NodeExecutorManager::ExecutorType::RTS);
    engine_mapping.emplace("DNN_VM_GE_LOCAL_OP_STORE", NodeExecutorManager::ExecutorType::GE_LOCAL);

    // Register from FE, set stub here.
    const std::string kCoreTypeAIC = "AIC";     // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    const std::string kCoreTypeAIV = "MIX_AIV"; // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    const std::string kCoreTypeCPU = "AICPU";   // FftsPlusUpdateManager::FftsPlusUpdateRegistrar
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIC, FFTSPlusTaskUpdateStub);
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeAIV, FFTSPlusTaskUpdateStub);
    REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeCPU, FFTSPlusTaskUpdateStub);

    OpTilingFuncV2 op_tiling_func = [](const Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &) -> bool {return true;};
    REGISTER_OP_TILING_UNIQ_V2(ReLU, op_tiling_func, 101);
    OpTilingRegistryInterf_V2("ReLU", op_tiling_func);
    REGISTER_OP_TILING_UNIQ_V2(Conv2D, op_tiling_func, 101);
    OpTilingRegistryInterf_V2("Conv2D", op_tiling_func);
  }

  void TearDown() {
    TBEHandleStore::GetInstance().kernels_.clear();
    FftsPlusUpdateManager::Instance().creators_.clear();
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
static void BuildFftsDynamicGraph(ComputeGraphPtr &root_graph, ComputeGraphPtr &dsp_graph, ComputeGraphPtr &ffts_graph) {
  uint32_t mem_offset = 0U;
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };
  root_graph = ToComputeGraph(g1);
  root_graph->SetGraphUnknownFlag(true);
  SetKnownOpKernel(root_graph, mem_offset);
  const auto root_call_0 = root_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(root_call_0, nullptr);
  AttrUtils::SetInt(root_graph, "globleworkspace_status", 1);
  AttrUtils::SetInt(root_graph, "globleworkspace_status_bytes", 1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g2) {
    const auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    const auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
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
  SetKnownOpKernel(dsp_graph, mem_offset);
  SetPartitionedCall(root_call_0, dsp_graph);

  const auto ffts_call_node = dsp_graph->FindNode("dsp_graph/PartitionedCall_0");
  EXPECT_NE(ffts_call_node, nullptr);
  AttrUtils::SetBool(ffts_call_node->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g3) {
    const auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    const auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    const auto conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
                                      .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                                      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    const auto relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIV")
                                    .Attr(ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))
                                    .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    const auto gather = OP_CFG(GATHERV2).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AICPU")
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
  SetKnownOpKernel(ffts_graph, mem_offset);
  SetPartitionedCall(ffts_call_node, ffts_graph);

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


  std::vector<char> relu_bin(64, '\0');
  TBEKernelPtr relu_kernel = MakeShared<ge::OpKernelBin>("sgt/relu", std::move(relu_bin));
  ffts_relu0->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, relu_kernel);
  AttrUtils::SetStr(ffts_relu0->GetOpDesc(), ffts_relu0->GetName() + "_kernelname", "sgt/relu");

  const auto &sgt_nodes = ffts_graph->GetDirectNode();
  std::for_each(sgt_nodes.begin(), sgt_nodes.end(), [](const NodePtr &n) {
    (void)AttrUtils::SetBool(n->GetOpDesc(), "OwnerGraphIsUnknown", true);
    (void)AttrUtils::SetBool(n->GetOpDesc(), "_kernel_list_first_name", true);
    (void)AttrUtils::SetInt(n->GetOpDesc(), "op_para_size", 2);
  });
}

TEST_F(UtestFftsPlusSubgraphExecutor, simple_ffts_plus_dynamic_graph) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr dsp_graph;
  ComputeGraphPtr ffts_graph;
  BuildFftsDynamicGraph(root_graph, dsp_graph, ffts_graph);

  // Build FftsPlusTaskDef.
  const std::shared_ptr<domi::ModelTaskDef> model_def = MakeShared<domi::ModelTaskDef>();
  domi::TaskDef &task_def = *model_def->add_task();
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(ffts_graph->GetParentNode()->GetOpDesc()->GetId());
  InitTaskSQEInfo(ffts_plus_task_def);
  {
    const auto node = ffts_graph->FindNode("sgt_graph/Conv2D");
    EXPECT_NE(node, nullptr);
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_def->set_op_index(node->GetOpDesc()->GetId());
    ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
    domi::FftsPlusAicAivCtxDef *aic_aiv_def = ctx_def->mutable_aic_aiv_ctx();
    InitAicAivCtx(aic_aiv_def, false);
  }
  {
    const auto node = ffts_graph->FindNode("sgt_graph/Relu");
    EXPECT_NE(node, nullptr);
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_def->set_op_index(node->GetOpDesc()->GetId());
    ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIC));
    domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv = ctx_def->mutable_mix_aic_aiv_ctx();
    InitMixAicAivCtx(mix_aic_aiv, true, false);
  }
  {
    const auto node = ffts_graph->FindNode("sgt_graph/Gather");
    EXPECT_NE(node, nullptr);
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_def->set_op_index(node->GetOpDesc()->GetId());
    ctx_def->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
    domi::FftsPlusAicpuCtxDef *ai_cpu_def = ctx_def->mutable_aicpu_ctx();
    InitAicpuCtxCtx(ai_cpu_def, false);
  }

  // Build GeModel.
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(root_graph);
  ge_root_model->SetModelName(root_graph->GetName());
  GeModelPtr ge_sub_model = MakeShared<GeModel>();
  ge_sub_model->SetModelTaskDef(model_def);

  ge_sub_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(dsp_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(dsp_graph->GetName(), ge_sub_model);

  // Test for Load.
  auto hybrid_model = hybrid::HybridDavinciModel::Create(ge_root_model);
  ASSERT_NE(hybrid_model, nullptr);
  ASSERT_EQ(hybrid_model->Init(), SUCCESS);

  // Test for Execute.
  std::vector<uint8_t> value_0(512, 0);
  DataBuffer in_tensor0(value_0.data(), value_0.size(), false, 0);
  std::vector<uint8_t> value_1(512, 0);
  DataBuffer in_tensor1(value_1.data(), value_1.size(), false, 0);
  const std::vector<DataBuffer> inputs{ in_tensor0, in_tensor1 };

  std::vector<uint8_t> value_2(512, 0);
  DataBuffer out_tensor0(value_2.data(), value_2.size(), false, 0);
  std::vector<DataBuffer> outputs{ out_tensor0 };

  GeTensorDesc input_desc(GeShape({1,4,4,8}), FORMAT_NCHW, DT_FLOAT); // sizeof(float) * 1 * 4 * 4 * 8 = 512
  TensorUtils::SetSize(input_desc, 512);
  const std::vector<GeTensorDesc> input_descs{ input_desc, input_desc };
  std::vector<GeTensorDesc> output_descs;

  ASSERT_EQ(hybrid_model->Execute(inputs, input_descs, outputs, output_descs, nullptr), SUCCESS);
}
} // namespace ge