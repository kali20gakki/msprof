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

#include "gtest/gtest.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"

#define private public
#define protected public
#include "framework/executor/ge_executor.h"
#include "framework/common/helper/om_file_helper.h"
#include "framework/common/helper/model_helper.h"
#include "graph/execute/model_executor.h"
#include "test_tools_task_info.h"
#include "external/ge/ge_api.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling/command_handle.h"
#include "common/profiling/profiling_properties.h"
#include "depends/mmpa/src/mmpa_stub.h"

using namespace std;
using namespace testing;

namespace ge {
namespace{
  int32_t OnlineInferTestCallBack(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
    return 0;
  }
}

class OnlineInferTest : public testing::Test {
 protected:
  void SetUp() {
    GeExecutor::Initialize({});
    ProfilingManager::Instance().SetMsprofReporterCallback(OnlineInferTestCallBack);
  }
  void TearDown() {
    GeExecutor::FinalizeEx();
    ProfilingManager::Instance().SetMsprofReporterCallback(nullptr);
  }
};

static void BuildSampleGraph(ComputeGraphPtr &graph, ComputeGraphPtr &case1, ComputeGraphPtr &case2, uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Combine sink and no_sink in one graph.
  DEF_GRAPH(g0) {
    const auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 0);
    const auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 1);
    const auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_INDEX, 2);
    const auto output = OP_CFG(NETOUTPUT).Attr(ATTR_ALL_GEARS_INFO, ";2;4;8").Attr(ATTR_GETNEXT_SINK_DYNMAIC, false);
    CHAIN(NODE("ascend_mbatch_shape_data", data_2)->NODE("ascend_mbatch_shape_mapindex", "MapIndex")->
          NODE("ascend_mbatch_shape_case", CASE)->NODE(NODE_NAME_NET_OUTPUT, output));
    CHAIN(NODE("ascend_mbatch_shape_const", CONSTANT)->EDGE(0, 1)->NODE("ascend_mbatch_shape_mapindex"));
    CHAIN(NODE("_arg_ascend_mbatch_batch_0", data_0)->EDGE(0, 1)->NODE("ascend_mbatch_shape_case"));
    CHAIN(NODE("_arg_ascend_mbatch_batch_1", data_1)->EDGE(0, 2)->NODE("ascend_mbatch_shape_case"));
    CHAIN(NODE("ascend_mbatch_shape_data")->NODE("ascend_mbatch_get_dynamic_dims_node", GETDYNAMICDIMS)->NODE(NODE_NAME_NET_OUTPUT));
  };
  graph = ToComputeGraph(g0);
  SetUnknownOpKernel(graph, mem_offset, true);
  const auto mbatch_case_node = graph->FindNode("ascend_mbatch_shape_case");
  EXPECT_NE(mbatch_case_node, nullptr);
  const auto case_desc = mbatch_case_node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetBool(case_desc, ATTR_INSERT_BY_MBATCH, true));
  EXPECT_TRUE(AttrUtils::SetInt(case_desc, ATTR_NAME_BATCH_NUM, 2));

  {
    const auto mbatch_batch_node = graph->FindNode("_arg_ascend_mbatch_batch_0");
    EXPECT_NE(mbatch_batch_node, nullptr);
    GeTensorDesc tensor_desc = mbatch_batch_node->GetOpDesc()->GetOutputDesc(0U);
    tensor_desc.SetShape(GeShape({2, -1, 8}));
    mbatch_batch_node->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
    mbatch_batch_node->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
  }

  {
    const auto mbatch_output_node = graph->FindNode(NODE_NAME_NET_OUTPUT);
    EXPECT_NE(mbatch_output_node, nullptr);
    const auto &op_desc = mbatch_output_node->GetOpDesc();
    const size_t dynamic_dims_index = op_desc->GetAllInputsSize() - 1U;
    GeTensorDesc tensor_desc = op_desc->GetInputDesc(dynamic_dims_index);
    tensor_desc.SetShape(GeShape({2, 4, 8}));
    op_desc->UpdateInputDesc(dynamic_dims_index, tensor_desc);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    auto label_switch = OP_CFG(LABELSWITCHBYINDEX).Attr(ATTR_NAME_LABEL_SWITCH_LIST, std::vector<int64_t>{0, 1});
    auto label_set_l0 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 0);
    auto label_gotoex = OP_CFG(LABELGOTOEX).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 2);
    auto data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    CHAIN(NODE("g1/_arg_0", data_1)->EDGE(0, 0)->NODE("g1/add", ADD)->NODE("g1/output_ascend_mbatch_batch_0", NETOUTPUT));
    CHAIN(NODE("g1/_arg_1", data_2)->EDGE(0, 1)->NODE("g1/add"));

    CHAIN(NODE("g1/_index", data_0)->NODE("g1/switch", label_switch)->CTRL_EDGE()->NODE("label_set_l0", label_set_l0)->CTRL_EDGE()->NODE("g1/_arg_0"));
    CHAIN(NODE("g1/output_ascend_mbatch_batch_0")->CTRL_EDGE()->NODE("label_gotoex", label_gotoex));
  };
  case1 = ToComputeGraph(g1);
  SetUnknownOpKernel(case1, mem_offset);
  AddCaseBranch(graph, "ascend_mbatch_shape_case", case1);
  const auto batch1_output = case1->FindNode("g1/output_ascend_mbatch_batch_0");
  EXPECT_NE(batch1_output, nullptr);
  EXPECT_TRUE(AttrUtils::SetStr(batch1_output->GetOpDesc(), ATTR_NAME_BATCH_LABEL, "batch_label_0"));

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g2) {
    auto label_set_l1 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 1);
    auto label_set_l2 = OP_CFG(LABELSET).Attr(ATTR_NAME_LABEL_SWITCH_INDEX, 2);
    auto data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
    auto data_2 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 2);
    CHAIN(NODE("g2/_arg_0", data_1)->EDGE(0, 0)->NODE("g2/add", ADD)->NODE("g2/output_ascend_mbatch_batch_1", NETOUTPUT));
    CHAIN(NODE("g2/_arg_1", data_2)->EDGE(0, 1)->NODE("g2/add"));

    CHAIN(NODE("label_set_l1", label_set_l1)->CTRL_EDGE()->NODE("g2/_arg_0"));
    CHAIN(NODE("g2/output_ascend_mbatch_batch_1")->CTRL_EDGE()->NODE("label_set_l2", label_set_l2));
  };
  case2 = ToComputeGraph(g2);
  SetUnknownOpKernel(case2, mem_offset);
  AddCaseBranch(graph, "ascend_mbatch_shape_case", case2);
  const auto batch2_output = case2->FindNode("g2/output_ascend_mbatch_batch_1");
  EXPECT_NE(batch2_output, nullptr);
  EXPECT_TRUE(AttrUtils::SetStr(batch2_output->GetOpDesc(), ATTR_NAME_BATCH_LABEL, "batch_label_1"));
}

void BuildGraphModel(const ComputeGraphPtr &graph, const ComputeGraphPtr &case1, const ComputeGraphPtr &case2,
                     uint32_t mem_offset, GeModelPtr &ge_model) {
  TBEKernelStore tbe_kernel_store;
  InitConstantNode(graph, "ascend_mbatch_shape_const", 0);

  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef(graph, *model_task_def, "ascend_mbatch_shape_mapindex");

  const std::vector<uint32_t> label_idx_list{0, 1};
  InitLabelSwitchDef(case1, *model_task_def, "g1/switch");
  InitLabelSetDef(case1, *model_task_def, "label_set_l0");
  InitKernelTaskDef(case1, *model_task_def, "g1/add");
  InitLabelGotoDef(case1, *model_task_def, "label_gotoex");

  InitLabelSetDef(case2, *model_task_def, "label_set_l1");
  InitKernelTaskDef(case2, *model_task_def, "g2/add");
  InitLabelSetDef(case2, *model_task_def, "label_set_l2");

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 0));

  EXPECT_TRUE(tbe_kernel_store.Build());
  ge_model->SetTBEKernelStore(tbe_kernel_store);
}

using DynamicAttribute = std::function<void(const uint32_t model_id)>;
static void TestDynamicBatchSize(const uint32_t model_id) {
  GeExecutor ge_executor;
  {
    std::vector<uint64_t> dynamic_dims; std::vector<uint64_t> cur_dynamic_dims;
    EXPECT_EQ(ge_executor.GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims), SUCCESS);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); uint64_t batch_size = 4U;
    EXPECT_EQ(ge_executor.SetDynamicBatchSize(model_id, &dynamic_input_addr, length, batch_size), SUCCESS);
  }

  {
    std::vector<int64_t> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetCurShape(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<TensorDesc> input_desc; std::vector<TensorDesc> output_desc; bool new_model_desc = false;
    EXPECT_EQ(ge_executor.GetModelDescInfo(model_id, input_desc, output_desc, new_model_desc), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetDynamicBatchInfo(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info;
    EXPECT_EQ(ge_executor.GetCombinedDynamicDims(model_id, batch_info), SUCCESS);
  }

  {
    std::vector<std::string> user_designate_shape_order;
    EXPECT_EQ(ge_executor.GetUserDesignateShapeOrder(model_id, user_designate_shape_order), SUCCESS);
  }

  {
    std::string op_name; std::string attr_name; std::string attr_value;
    EXPECT_EQ(ge_executor.GetOpAttr(model_id, op_name, attr_name, attr_value), SUCCESS);
  }

  {
    std::vector<std::string> dynamic_output_shape_info;
    EXPECT_EQ(ge_executor.GetModelAttr(model_id, dynamic_output_shape_info), SUCCESS);
  }

  {
    uint32_t max_size = 0U;
    EXPECT_EQ(ge_executor.GetMaxUsedMemory(model_id, max_size), SUCCESS);
  }

  {
    uint32_t device_id = 0U;
    EXPECT_EQ(GeExecutor::GetDeviceIdByModelId(model_id, device_id), SUCCESS);
  }

  {
    size_t shape_count = 0U;
    EXPECT_EQ(ge_executor.GetBatchInfoSize(model_id, shape_count), SUCCESS);
  }

  {
    uint32_t device_id = 0U; uint32_t stream_id = 0U; uint32_t task_id = 0U; OpDescInfo op_desc_info;
    EXPECT_EQ(ge_executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info), SUCCESS);
  }

  //{
  //  uint64_t dynamic_input_addr = 0U; uint64_t length = 0U; std::vector<kAippDynamicBatchPara> aipp_batch_para; kAippDynamicPara aipp_parms;
  //  EXPECT_EQ(ge_executor.SetDynamicAippData(model_id, &dynamic_input_addr, length, aipp_batch_para, aipp_parms), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; AippConfigInfo aipp_info;
  //  EXPECT_EQ(ge_executor.GetAIPPInfo(model_id, index, aipp_info), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; InputAippType type; size_t aipp_index = 0U;
  //  EXPECT_EQ(ge_executor.GetAippType(model_id, index, type, aipp_index), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; std::vector<InputOutputDims> input_dims; std::vector<InputOutputDims> output_dims;
  //  EXPECT_EQ(ge_executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; OriginInputInfo orig_input_info;
  //  EXPECT_EQ(ge_executor.GetOrigInputInfo(model_id, index, orig_input_info), SUCCESS);
  //}

  {
    MsprofCommandHandle_t command{};
    command.profSwitch = 0xFFFFFFFFFFFFFFFF;
    command.modelId = model_id;
    command.type = 0;
    EXPECT_EQ(ProfCtrlHandle(RT_PROF_CTRL_SWITCH, &command, sizeof(command)), RT_ERROR_NONE);
  }

  {
    MsprofCommandHandle_t command{};
    command.profSwitch = 0xFFFFFFFFFFFFFFFF;
    command.modelId = model_id;
    command.type = 4;
    EXPECT_EQ(ProfCtrlHandle(RT_PROF_CTRL_SWITCH, &command, sizeof(command)), RT_ERROR_NONE);
  }

  {
    MsprofCommandHandle_t command{};
    command.profSwitch = 0xFFFFFFFFFFFFFFFF;
    command.modelId = model_id;
    command.type = 5;
    EXPECT_EQ(ProfCtrlHandle(RT_PROF_CTRL_SWITCH, &command, sizeof(command)), RT_ERROR_NONE);
  }

  {
    MsprofCommandHandle_t command{};
    command.profSwitch = 0xFFFFFFFFFFFFFFFF;
    command.modelId = model_id;
    command.type = 3;
    EXPECT_EQ(ProfCtrlHandle(RT_PROF_CTRL_SWITCH, &command, sizeof(command)), RT_ERROR_NONE);
    EXPECT_FALSE(ProfilingProperties::Instance().is_load_profiling_);
    EXPECT_FALSE(ProfilingProperties::Instance().is_training_trace_);
  }
}

static void TestDynamicImageSize(const uint32_t model_id) {
  GeExecutor ge_executor;
  {
    std::vector<uint64_t> dynamic_dims; std::vector<uint64_t> cur_dynamic_dims;
    EXPECT_EQ(ge_executor.GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims), SUCCESS);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); uint64_t image_height = 340U; uint64_t image_width = 560U;
    EXPECT_EQ(ge_executor.SetDynamicImageSize(model_id, &dynamic_input_addr, length, image_height, image_width), SUCCESS);
  }

  {
    std::vector<int64_t> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetCurShape(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<TensorDesc> input_desc; std::vector<TensorDesc> output_desc; bool new_model_desc = false;
    EXPECT_EQ(ge_executor.GetModelDescInfo(model_id, input_desc, output_desc, new_model_desc), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetDynamicBatchInfo(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info;
    EXPECT_EQ(ge_executor.GetCombinedDynamicDims(model_id, batch_info), SUCCESS);
  }

  {
    std::vector<std::string> user_designate_shape_order;
    EXPECT_EQ(ge_executor.GetUserDesignateShapeOrder(model_id, user_designate_shape_order), SUCCESS);
  }

  {
    std::string op_name; std::string attr_name; std::string attr_value;
    EXPECT_EQ(ge_executor.GetOpAttr(model_id, op_name, attr_name, attr_value), SUCCESS);
  }

  {
    std::vector<std::string> dynamic_output_shape_info;
    EXPECT_EQ(ge_executor.GetModelAttr(model_id, dynamic_output_shape_info), SUCCESS);
  }

  {
    uint32_t max_size = 0U;
    EXPECT_EQ(ge_executor.GetMaxUsedMemory(model_id, max_size), SUCCESS);
  }

  {
    uint32_t device_id = 0U;
    EXPECT_EQ(GeExecutor::GetDeviceIdByModelId(model_id, device_id), SUCCESS);
  }

  {
    size_t shape_count = 0U;
    EXPECT_EQ(ge_executor.GetBatchInfoSize(model_id, shape_count), SUCCESS);
  }

  {
    uint32_t device_id = 0U; uint32_t stream_id = 0U; uint32_t task_id = 0U; OpDescInfo op_desc_info;
    EXPECT_EQ(ge_executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info), SUCCESS);
  }

  //{
  //  uint64_t dynamic_input_addr = 0U; uint64_t length = 0U; std::vector<kAippDynamicBatchPara> aipp_batch_para; kAippDynamicPara aipp_parms;
  //  EXPECT_EQ(ge_executor.SetDynamicAippData(model_id, &dynamic_input_addr, length, aipp_batch_para, aipp_parms), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; AippConfigInfo aipp_info;
  //  EXPECT_EQ(ge_executor.GetAIPPInfo(model_id, index, aipp_info), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; InputAippType type; size_t aipp_index = 0U;
  //  EXPECT_EQ(ge_executor.GetAippType(model_id, index, type, aipp_index), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; std::vector<InputOutputDims> input_dims; std::vector<InputOutputDims> output_dims;
  //  EXPECT_EQ(ge_executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; OriginInputInfo orig_input_info;
  //  EXPECT_EQ(ge_executor.GetOrigInputInfo(model_id, index, orig_input_info), SUCCESS);
  //}
}

static void TestDynamicDimsSize(const uint32_t model_id) {
  GeExecutor ge_executor;
  {
    std::vector<uint64_t> dynamic_dims{2,4,8}; std::vector<uint64_t> cur_dynamic_dims;
    EXPECT_EQ(ge_executor.GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims), SUCCESS);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); std::vector<uint64_t> dynamic_dims{2,4,8};
    EXPECT_EQ(ge_executor.SetDynamicDims(model_id, &dynamic_input_addr, length, dynamic_dims), SUCCESS);
  }

  {
    std::vector<int64_t> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetCurShape(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<TensorDesc> input_desc; std::vector<TensorDesc> output_desc; bool new_model_desc = false;
    EXPECT_EQ(ge_executor.GetModelDescInfo(model_id, input_desc, output_desc, new_model_desc), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info; int32_t dynamic_type = 0U;
    EXPECT_EQ(ge_executor.GetDynamicBatchInfo(model_id, batch_info, dynamic_type), SUCCESS);
  }

  {
    std::vector<std::vector<int64_t>> batch_info;
    EXPECT_EQ(ge_executor.GetCombinedDynamicDims(model_id, batch_info), SUCCESS);
  }

  {
    std::vector<std::string> user_designate_shape_order;
    EXPECT_EQ(ge_executor.GetUserDesignateShapeOrder(model_id, user_designate_shape_order), SUCCESS);
  }

  {
    std::string op_name; std::string attr_name; std::string attr_value;
    EXPECT_EQ(ge_executor.GetOpAttr(model_id, op_name, attr_name, attr_value), SUCCESS);
  }

  {
    std::vector<std::string> dynamic_output_shape_info;
    EXPECT_EQ(ge_executor.GetModelAttr(model_id, dynamic_output_shape_info), SUCCESS);
  }

  {
    uint32_t max_size = 0U;
    EXPECT_EQ(ge_executor.GetMaxUsedMemory(model_id, max_size), SUCCESS);
  }

  {
    uint32_t device_id = 0U;
    EXPECT_EQ(GeExecutor::GetDeviceIdByModelId(model_id, device_id), SUCCESS);
  }

  {
    size_t shape_count = 0U;
    EXPECT_EQ(ge_executor.GetBatchInfoSize(model_id, shape_count), SUCCESS);
  }

  {
    uint32_t device_id = 0U; uint32_t stream_id = 0U; uint32_t task_id = 0U; OpDescInfo op_desc_info;
    EXPECT_EQ(ge_executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info), SUCCESS);
  }

  //{
  //  uint64_t dynamic_input_addr = 0U; uint64_t length = 0U; std::vector<kAippDynamicBatchPara> aipp_batch_para; kAippDynamicPara aipp_parms;
  //  EXPECT_EQ(ge_executor.SetDynamicAippData(model_id, &dynamic_input_addr, length, aipp_batch_para, aipp_parms), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; AippConfigInfo aipp_info;
  //  EXPECT_EQ(ge_executor.GetAIPPInfo(model_id, index, aipp_info), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; InputAippType type; size_t aipp_index = 0U;
  //  EXPECT_EQ(ge_executor.GetAippType(model_id, index, type, aipp_index), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; std::vector<InputOutputDims> input_dims; std::vector<InputOutputDims> output_dims;
  //  EXPECT_EQ(ge_executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims), SUCCESS);
  //}

  //{
  //  uint32_t index = 0U; OriginInputInfo orig_input_info;
  //  EXPECT_EQ(ge_executor.GetOrigInputInfo(model_id, index, orig_input_info), SUCCESS);
  //}
}

// ModelExecutor::RunThread -> ExecuteGraphAsync -> AsyncExecuteModel -> DataInputTensor -> DavinciModel::Push -> Run
Status OnlineInferDynamic(const ComputeGraphPtr &graph, const GeModelPtr &ge_model, const OmeContext &ome_context,
                          const DynamicAttribute &dynamic_callback, const bool sink_dynamic = false) {
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);
  graph_node->SetOmeContext(ome_context);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  int64_t value_0 = 127;
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  int64_t value_1 = 100;
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  std::vector<Tensor> input_tensors{tensor_0, tensor_1};
  if (sink_dynamic) { // GETDYNAMICDIMS on output for sink dynamic.
    int64_t value_2 = 300;
    Tensor tensor_2(tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
    input_tensors.emplace_back(tensor_2);
  }

  std::mutex run_mutex;
  std::condition_variable model_run_cv;
  Status run_status = FAILED;
  std::vector<Tensor> run_outputs;
  const auto callback = [&](Status status, std::vector<Tensor> &outputs) {
    std::unique_lock<std::mutex> lock(run_mutex);
    run_status = status;
    run_outputs.swap(outputs);
    model_run_cv.notify_one();
  };

  GEThreadLocalContext context;
  error_message::Context error_context;
  graph_node->Lock();
  RunArgs run_args{graph_node, graph_id, 2001, error_context, input_tensors, flow_root_model, context, callback};
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  std::unique_lock<std::mutex> lock(run_mutex);
  EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
  EXPECT_EQ(run_status, SUCCESS);
  EXPECT_EQ(run_outputs.size(), sink_dynamic ? 1U : 2U);  // GETDYNAMICDIMS no output.

  dynamic_callback(ge_root_model->GetModelId());
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  return run_status;
}

static Graph BuildGetNextSinkGraph() {
  DEF_GRAPH(g1) {
    const auto iterator = OP_CFG(FRAMEWORKOP)
        .Attr(ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "IteratorV2")
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, 3, 16, 16})
        .TensorDesc(FORMAT_ND, DT_FLOAT, {})
        .Build("iterator");
    const auto ret_val_0 = OP_CFG(FRAMEWORKOP)
        .Attr(ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "_RetVal")
        .Attr(RETVAL_ATTR_NAME_INDEX, 0)
        .Build("ret_val_0");
    const auto ret_val_1 = OP_CFG(FRAMEWORKOP)
        .Attr(ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "_RetVal")
        .Attr(RETVAL_ATTR_NAME_INDEX, 1)
        .Build("ret_val_1");
    const auto ret_val_2 = OP_CFG(FRAMEWORKOP)
        .Attr(ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "_RetVal")
        .Attr(RETVAL_ATTR_NAME_INDEX, 2)
        .Build("ret_val_2");

    int32_t value = 100;
    GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_INT32);
    GeTensorPtr const_tensor =
        std::make_shared<GeTensor>(tensor_desc, reinterpret_cast<uint8_t *>(&value), sizeof(int32_t));
    const auto const1 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor).Build("const1");
    const auto const2 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor).Build("const2");
    iterator->MutableOutputDesc(0)->SetShape(GeShape({-1, 3, 16, 16}));
    iterator->MutableOutputDesc(1)->SetShape(GeShape());

    CHAIN(NODE(iterator)->EDGE(0, 0)->NODE("add", ADD)->NODE("less", LESS)->NODE(ret_val_0));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE("add", ADD));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE("less", LESS));

    CHAIN(NODE(const1)->EDGE(0, 0)->NODE("add1", ADD)->NODE(ret_val_1));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE("add1", ADD));

    CHAIN(NODE(const1)->EDGE(0, 0)->NODE("div", REALDIV)->NODE(ret_val_2));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE("div", REALDIV));

    CHAIN(NODE(iterator)->EDGE(1, 0)->NODE("identity", IDENTITY));
  };
  const auto graph = ToGeGraph(g1);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  for (const auto &node : compute_graph->GetDirectNode()) {
    if (node->GetType() == ADD || node->GetType() == REALDIV) {
      node->SetHostNode(true);
    }
    if (node->GetType() == REALDIV) {
      (void)AttrUtils::SetBool(node->GetOpDesc(), ATTR_NO_NEED_CONSTANT_FOLDING, true);
    }
  }
  return graph;
}

static Graph BuildGetNextNotSinkGraph(bool is_placeholder = false) {
  string data_name = is_placeholder ? "data1" : "IteratorGetNext_data1";
  DEF_GRAPH(g1) {
    const auto data1 = OP_CFG(DATA)
        .Attr(ATTR_NAME_INDEX, 0)
        .InCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
        .Build(data_name);
    const auto ret_val_0 = OP_CFG(FRAMEWORKOP)
        .Attr(ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, "_RetVal")
        .Attr(RETVAL_ATTR_NAME_INDEX, 0)
        .Build("ret_val_0");

    int32_t value = 100;
    GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_INT32);
    GeTensorPtr const_tensor =
        std::make_shared<GeTensor>(tensor_desc, reinterpret_cast<uint8_t *>(&value), sizeof(int32_t));
    const auto const1 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor).Build("const1");
    data1->MutableOutputDesc(0)->SetShape(GeShape({-1, 3, 16, 16}));

    auto conv2d1 = OP_CFG(CONV2D).InCnt(2).OutCnt(1).Build("conv2d1");
    auto relu1 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu1");

    CHAIN(NODE(data1)->NODE(conv2d1)->NODE(relu1))->NODE(ret_val_0);
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(conv2d1));
  };
  return ToGeGraph(g1);
}

Status OnlineInferDynCompile(const Graph &graph, const uint32_t graph_id,
                             const map<AscendString, AscendString> &options) {
  Session session(options);
  auto ret = session.AddGraph(graph_id, graph, options);
  EXPECT_EQ(ret, SUCCESS);

  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(graph_id, inputs);
  return ret;
}

TEST_F(OnlineInferTest, online_infer_dynamic_execute) {
  uint32_t mem_offset = 0U;
  ComputeGraphPtr graph, case1, case2;
  BuildSampleGraph(graph, case1, case2, mem_offset);
  EXPECT_NE(graph, nullptr);
  const auto mbatch_case_node = graph->FindNode("ascend_mbatch_shape_case");
  EXPECT_NE(mbatch_case_node, nullptr);
  const auto mbatch_batch_node = graph->FindNode("_arg_ascend_mbatch_batch_0");
  EXPECT_NE(mbatch_batch_node, nullptr);
  const auto mbatch_output_node = graph->FindNode(NODE_NAME_NET_OUTPUT);
  EXPECT_NE(mbatch_output_node, nullptr);

  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "online_infer_dynamic");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");

  GeModelPtr ge_model;
  BuildGraphModel(graph, case1, case2, mem_offset, ge_model);
  EXPECT_NE(ge_model, nullptr);

  const auto case_desc = mbatch_case_node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(case_desc, ATTR_DYNAMIC_TYPE, static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH)));
  EXPECT_TRUE(AttrUtils::SetListStr(case_desc, ATTR_USER_DESIGNEATE_SHAPE_ORDER, {}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_0", {2}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_1", {4}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_0", {2}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_1", {4}));
  {
    // Test LoadModelOnline: Online infer for dynamic DATA.
    OmeContext ome_context;
    ome_context.dynamic_node_type = DATA;
    ome_context.user_input_dims = {{"1", {}}, {"2", {}}};
    ome_context.dynamic_shape_dims = {{}, {2}, {4}, {8}};
    ome_context.data_nodes.emplace_back(mbatch_batch_node);
    ome_context.getnext_nosink_nodes.clear();
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model, ome_context, &TestDynamicBatchSize), SUCCESS);   // ParseInputsDimsForData
  }

  EXPECT_TRUE(AttrUtils::SetInt(case_desc, ATTR_DYNAMIC_TYPE, static_cast<int32_t>(DynamicInputType::DYNAMIC_IMAGE)));
  EXPECT_TRUE(AttrUtils::SetListStr(case_desc, ATTR_USER_DESIGNEATE_SHAPE_ORDER, {}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_0", {295,413}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_1", {340,560}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_0", {0,2}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_1", {1,2}));
  {
    // Test LoadModelOnline: Online infer for dynamic DATA+GetNext(no_sink).
    OmeContext ome_context;
    ome_context.dynamic_node_type = DATA;
    ome_context.user_input_dims = {{"1", {}}};
    ome_context.dynamic_shape_dims = {{}, {2}, {4}, {8}};
    ome_context.data_nodes.emplace_back(mbatch_batch_node);
    ome_context.getnext_nosink_nodes.emplace_back(mbatch_batch_node);
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model, ome_context, &TestDynamicImageSize), SUCCESS); // ParseInputsDimsForGetNextNoSinkAndData
  }

  EXPECT_TRUE(AttrUtils::SetInt(case_desc, ATTR_DYNAMIC_TYPE, static_cast<int32_t>(DynamicInputType::DYNAMIC_DIMS)));
  EXPECT_TRUE(AttrUtils::SetListStr(case_desc, ATTR_USER_DESIGNEATE_SHAPE_ORDER, {"_arg_ascend_mbatch_batch_0"}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_0", {2}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_pred_value_1", {4}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_0", {0,2}));
  EXPECT_TRUE(AttrUtils::SetListInt(case_desc, "_combined_batch_1", {1,2}));
  {
    // Test LoadModelOnline: Online infer for dynamic DATA+GetNext(no_sink).
    OmeContext ome_context;
    ome_context.dynamic_node_type = GETNEXT;
    ome_context.user_input_dims = {{"1", {}}};
    ome_context.dynamic_shape_dims = {{}, {2}, {4}, {8}};
    ome_context.data_nodes.emplace_back(mbatch_batch_node);
    ome_context.getnext_nosink_nodes.emplace_back(mbatch_batch_node);
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model, ome_context, &TestDynamicDimsSize), SUCCESS); // ParseInputsDimsForGetNextNoSinkAndData
  }

  // DavinciModel::GetGetDynamicDimsNodeInfo: is_getnext_sink_dynamic_ = true;
  const auto &op_desc = mbatch_output_node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_GETNEXT_SINK_DYNMAIC, true));
  std::vector<std::string> dynamic_shape_info{ "2,0,2", "1,1,3" };
  EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DYNAMIC_OUTPUT_DIMS, dynamic_shape_info));
  {
    // Test LoadModelOnline: Online infer for dynamic GetNext(no_sink).
    OmeContext ome_context;
    ome_context.dynamic_node_type = GETNEXT;
    ome_context.user_input_dims = {{"1", {}}, {"2", {}}};
    ome_context.dynamic_shape_dims = {{}, {2}, {4}, {8}};
    ome_context.data_nodes.clear();
    ome_context.getnext_nosink_nodes.emplace_back(mbatch_batch_node);
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model, ome_context, &TestDynamicDimsSize, true), SUCCESS); // ParseInputsDimsForData
  }

  {
    // Test LoadModelOnline: Online infer for dynamic GetNext(sink).
    OmeContext ome_context;
    ome_context.dynamic_node_type = GETNEXT;
    ome_context.user_input_dims = {};
    ome_context.dynamic_shape_dims = {{}, {2}, {4}, {8}};
    ome_context.data_nodes.clear();
    ome_context.getnext_nosink_nodes.clear();
    EXPECT_EQ(OnlineInferDynamic(graph, ge_model, ome_context, &TestDynamicDimsSize, true), SUCCESS); // Empty.
  }

  {
    // Test LoadModelOffline
    SaveParam save_param;
    ModelHelper model_helper;
    model_helper.SetSaveMode(false);  // Save to buffer.
    ModelBufferData model_buffer;
    EXPECT_EQ(model_helper.SaveToOmModel(ge_model, save_param, "file_name_prefix", model_buffer), SUCCESS);
    const ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", ""};

    int64_t arg_0 = 127;
    int64_t arg_1 = 100;
    int64_t arg_2 = 512;
    RunModelData run_input_data;
    run_input_data.blobs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
    run_input_data.blobs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});
    run_input_data.blobs.emplace_back(DataBuffer{&arg_2, sizeof(arg_2), false, 0});

    int64_t arg_3 = 111;
    RunModelData run_output_data;
    run_output_data.blobs.emplace_back(DataBuffer{&arg_3, sizeof(arg_3), false, 0});

    uint32_t model_id = 0;
    GeExecutor ge_executor;
    EXPECT_EQ(ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0U, nullptr, 0U), SUCCESS);
    EXPECT_EQ(ge_executor.ExecModel(model_id, nullptr, run_input_data, run_output_data, true), SUCCESS);
    EXPECT_EQ(ge_executor.UnloadModel(model_id), SUCCESS);
  }
}

TEST_F(OnlineInferTest, online_infer_dynamic_compile) {
  // 1. GetNext sink
  {
    // build graph
    Graph graph = BuildGetNextSinkGraph();
    DUMP_GRAPH_WHEN("PreRunAfterOptimize1");
    // new session & add graph
    map<AscendString, AscendString> options = {
        {"ge.inputShape", "get_next:-1,3,16,16"},
        {"ge.dynamicDims", "2;8;16"},
        {"ge.dynamicNodeType", "0"}
    };
    EXPECT_EQ(OnlineInferDynCompile(graph, 1, options), SUCCESS);
    // check result
    CHECK_GRAPH(PreRunAfterOptimize1) {
      const auto case_node = graph->FindNode("ascend_mbatch_shape_case");
      ASSERT_NE(case_node, nullptr);
      EXPECT_EQ(case_node->GetInNodes().size(), 6);
      EXPECT_EQ(case_node->GetInNodes().at(0)->GetName(), "ascend_mbatch_shape_mapindex");
      EXPECT_EQ(case_node->GetInNodes().at(4)->GetType(), REALDIV);
    };
  }

  // 2. GetNext not sink
  {
    // build graph
    Graph graph = BuildGetNextNotSinkGraph();
    DUMP_GRAPH_WHEN("PreRunAfterOptimize1");
    // new session & add graph
    map<AscendString, AscendString> options = {
        {"ge.inputShape", "get_next:-1,3,16,16"},
        {"ge.dynamicDims", "2;8;16"},
        {"ge.dynamicNodeType", "0"}
    };
    EXPECT_EQ(OnlineInferDynCompile(graph, 2, options), SUCCESS);
    // check result
    CHECK_GRAPH(PreRunAfterOptimize1) {
      const auto case_node = graph->FindNode("ascend_mbatch_shape_case");
      ASSERT_NE(case_node, nullptr);
      EXPECT_EQ(case_node->GetInNodes().size(), 3);
    };
  }

  // 3. placeholder dyn
  {
    // build graph
    Graph graph = BuildGetNextNotSinkGraph(true);
    DUMP_GRAPH_WHEN("PreRunAfterOptimize1");
    // new session & add graph
    map<AscendString, AscendString> options = {
        {"ge.inputShape", "data1:-1,3,16,16"},
        {"ge.dynamicDims", "2;8;16"},
        {"ge.dynamicNodeType", "1"}
    };
    EXPECT_EQ(OnlineInferDynCompile(graph, 3, options), SUCCESS);
    // check result
    CHECK_GRAPH(PreRunAfterOptimize1) {
      const auto case_node = graph->FindNode("ascend_mbatch_shape_case");
      ASSERT_NE(case_node, nullptr);
      EXPECT_EQ(case_node->GetInNodes().size(), 3);
    };
  }
}
} // namespace ge

