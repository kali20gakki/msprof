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
#include "init_ge.h"
#include "utils/bench_env.h"
#include "ge_graph_dsl/graph_dsl.h"

#define private public
#define protected public

#include "test_tools_task_info.h"
#include "hybrid/hybrid_davinci_model.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "external/ge/ge_api.h"
#include "framework/executor/ge_executor.h"
#include "graph/execute/model_executor.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_context.h"
#include "graph/graph.h"
#include "graph/manager/graph_var_manager.h"
#include "common/profiling/profiling_manager.h"
#include "common/utils/executor_utils.h"
#include "common/dump/dump_manager.h"
#include "toolchain/prof_acl_api.h"
#include "graph/load/model_manager/model_manager.h"
#include "opskernel_executor/ops_kernel_executor_manager.h"

using namespace std;
using namespace testing;

namespace ge {
class DavinciModelTest : public testing::Test {
 protected:
  void SetUp() override {
    ReInitGe();
    BenchEnv::Init();
    ProfilingManager::Instance().SetMsprofReporterCallback(&ReporterCallback);
  }
  void TearDown() override {
    ProfilingManager::Instance().SetMsprofReporterCallback(nullptr);
    actual_info_type.clear();
  }

 public:
  GeExecutor ge_executor_;
};

static void BuildSampleGraph(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    const auto active_s = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{1});
    CHAIN(NODE("_arg_0", DATA)->NODE("HcomAllreduce", HCOMALLREDUCE)->NODE("Less", LESS));
    CHAIN(NODE("_arg_1", DATA)->NODE("Less")->NODE("Less_Cast", CAST)->CTRL_EDGE()->NODE("Less_StreamActive", active_s));
    CHAIN(NODE("_arg_2", DATA)->NODE("mul", MUL)->EDGE(0, 1)->NODE("add_n", ADDN)->NODE(NODE_NAME_NET_OUTPUT, NETOUTPUT));
    CHAIN(NODE("_arg_3", DATA)->NODE("aipp", AIPP)->NODE("shape", SHAPE)->EDGE(0, 0)->NODE("add_n"));
    CHAIN(NODE("_cst_string", CONSTANTOP)->NODE("shape", SHAPE));
    CHAIN(NODE("_arg_1")->NODE("mul"));

    const auto switch_f = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_NOT_EQUAL))
                                              .Attr(ATTR_NAME_SWITCH_DATA_TYPE, static_cast<int64_t>(RT_SWITCH_INT64))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("Less_Cast")->EDGE(0, 0)->NODE("Less/StreamSwitch_f", switch_f));
    CHAIN(NODE("Less/StreamSwitch_Const_f", CONSTANTOP)->EDGE(0, 1)->NODE("Less/StreamSwitch_f"));
    CHAIN(NODE("Less_StreamActive")->CTRL_EDGE()->NODE("Less/StreamSwitch_f"));

    const auto switch_t = OP_CFG(STREAMSWITCH).Attr(ATTR_NAME_STREAM_SWITCH_COND, static_cast<uint32_t>(RT_EQUAL))
                                              .Attr(ATTR_NAME_SWITCH_DATA_TYPE, static_cast<int64_t>(RT_SWITCH_INT64))
                                              .Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("Less_Cast")->EDGE(0, 0)->NODE("Less/StreamSwitch_t", switch_t));
    CHAIN(NODE("Less/StreamSwitch_Const_t", CONSTANTOP)->EDGE(0, 1)->NODE("Less/StreamSwitch_t"));
    CHAIN(NODE("Less_StreamActive")->CTRL_EDGE()->NODE("Less/StreamSwitch_t"));

    const auto active_0 = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("_arg_0")->EDGE(0, 0)->NODE("cond/pow", POW)->NODE("cond/sub", SUB)->
          NODE("merge_input_0_memcpy", MEMCPYASYNC)->CTRL_EDGE()->
          NODE("merge_input_0_active", active_0)->CTRL_EDGE()->
          NODE("cond/merge", STREAMMERGE)->EDGE(0, 2)->
          NODE("add_n"));
    CHAIN(NODE("merge_input_0_memcpy")->EDGE(0, 0)->NODE("cond/merge"));
    CHAIN(NODE("_arg_1")->EDGE(0, 1)->NODE("cond/pow"));
    CHAIN(NODE("Less/StreamSwitch_f")->CTRL_EDGE()->NODE("cond/pow"));
    CHAIN(NODE("_arg_1")->EDGE(0, 0)->NODE("cond/realdiv", REALDIV)->NODE("cond/sub"));
    CHAIN(NODE("_arg_2")->EDGE(0, 1)->NODE("cond/realdiv"));
    CHAIN(NODE("Less/StreamSwitch_f")->CTRL_EDGE()->NODE("cond/realdiv"));

    const auto active_1 = OP_CFG(STREAMACTIVE).Attr(ATTR_NAME_ACTIVE_STREAM_LIST, std::vector<int64_t>{2});
    CHAIN(NODE("_arg_1")->EDGE(0, 0)->NODE("cond/mul", MUL)->NODE("cond/add", ADD)->
          NODE("merge_input_1_memcpy", MEMCPYASYNC)->CTRL_EDGE()->
          NODE("merge_input_1_active", active_1)->CTRL_EDGE()->
          NODE("cond/merge"));
    CHAIN(NODE("merge_input_1_memcpy")->EDGE(0, 1)->NODE("cond/merge"));
    CHAIN(NODE("_arg_2")->EDGE(0, 1)->NODE("cond/mul"));
    CHAIN(NODE("Less/StreamSwitch_t")->CTRL_EDGE()->NODE("cond/mul"));
    CHAIN(NODE("_arg_0")->EDGE(0, 0)->NODE("cond/square", SQUARE)->NODE("cond/add"));
    CHAIN(NODE("Less/StreamSwitch_t")->CTRL_EDGE()->NODE("cond/square"));
  };
  graph = ToComputeGraph(g1);
  graph->SetSessionID(10086);
  graph->SetGraphID(20061);
  SetUnknownOpKernel(graph, mem_offset, true);

  {
    const auto &node = graph->FindNode(NODE_NAME_NET_OUTPUT);
    EXPECT_NE(node, nullptr);
    GeTensorDesc input_desc(GeShape({2, 4, 8, 2}), FORMAT_FRACTAL_Z, DT_FLOAT);
    node->GetOpDesc()->UpdateInputDesc(0, input_desc);
    mem_offset += (2 * 4 * 8 * 2 * sizeof(float));
  }

  {
    const auto &no = graph->FindNode("cond/pow");
    EXPECT_TRUE(AttrUtils::SetListInt(no->GetOpDesc(), ATTR_NAME_OUTPUT_MEM_TYPE_LIST, { RT_MEMORY_L1 }));
    const auto &ni = graph->FindNode("cond/sub");
    EXPECT_TRUE(AttrUtils::SetListInt(ni->GetOpDesc(), ATTR_NAME_INPUT_MEM_TYPE_LIST, { RT_MEMORY_L1, RT_MEMORY_HBM }));
  }
  {
    const auto &no = graph->FindNode("Less_Cast");
    EXPECT_TRUE(AttrUtils::SetListInt(no->GetOpDesc(), ATTR_NAME_OUTPUT_MEM_TYPE_LIST, { RT_MEMORY_TS }));
    const auto &nt = graph->FindNode("Less/StreamSwitch_t");
    EXPECT_TRUE(AttrUtils::SetListInt(nt->GetOpDesc(), ATTR_NAME_INPUT_MEM_TYPE_LIST, { RT_MEMORY_TS, RT_MEMORY_HBM }));
    const auto &nf = graph->FindNode("Less/StreamSwitch_f");
    EXPECT_TRUE(AttrUtils::SetListInt(nf->GetOpDesc(), ATTR_NAME_INPUT_MEM_TYPE_LIST, { RT_MEMORY_TS, RT_MEMORY_HBM }));
  }
}

static void BuildSampleNoTilingGraph(ComputeGraphPtr &graph, uint32_t &mem_offset) {
  BuildSampleGraph(graph, mem_offset);
  {
    const auto &node = graph->FindNode("_arg_0");
    EXPECT_NE(node, nullptr);
    GeTensorDesc input_desc(GeShape({2, 4, 8, 2}), FORMAT_FRACTAL_Z, DT_FLOAT);
    auto tensor1 = node->GetOpDesc()->MutableOutputDesc(0);
    (void)AttrUtils::SetBool(tensor1, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
  }
}

void BuildGraphModel(ComputeGraphPtr &graph, GeModelPtr &ge_model, uint32_t mem_offset) {
  TBEKernelStore tbe_kernel_store;
  CustAICPUKernelStore cpu_kernel_store;

  const std::string cst_string("Hello guys. every thing will be ok.");
  InitConstantNode(graph, "_cst_string", GeTensorDesc(GeShape({1}), FORMAT_ND, DT_STRING), cst_string);
  InitConstantNode(graph, "Less/StreamSwitch_Const_f", 0);
  InitConstantNode(graph, "Less/StreamSwitch_Const_t", 1);

  const auto model_task_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef(graph, *model_task_def, "aipp");
  InitKernelTaskDef(graph, *model_task_def, "shape");

  InitKernelTaskDef_TE(graph, *model_task_def, "Less", tbe_kernel_store);
  InitKernelTaskDef_CUSTOM(graph, *model_task_def, "Less_Cast");
  InitKernelTaskDef_CUST_CPU(graph, *model_task_def, "mul", cpu_kernel_store);

  InitStreamActiveDef(graph, *model_task_def, "Less_StreamActive");
  InitStreamSwitchDef(graph, *model_task_def, "Less/StreamSwitch_f");
  InitStreamSwitchDef(graph, *model_task_def, "Less/StreamSwitch_t");

  InitKernelExTaskDef(graph, *model_task_def, "cond/pow");
  InitKernelExTaskDef_AllShape(graph, *model_task_def, "cond/sub");
  InitKernelExTaskDef_Blocking(graph, *model_task_def, "cond/realdiv");

  InitMemcpyAsyncDef(graph, *model_task_def, "merge_input_0_memcpy");
  InitStreamActiveDef(graph, *model_task_def, "merge_input_0_active");
  InitMemcpyAsyncDef(graph, *model_task_def, "merge_input_1_memcpy");
  InitStreamActiveDef(graph, *model_task_def, "merge_input_1_active");

  InitKernelTaskDef_AI_CPU(graph, *model_task_def, "cond/mul");
  InitKernelTaskDef_CPU_AllShape(graph, *model_task_def, "cond/add");
  InitKernelTaskDef_CPU_Blocking(graph, *model_task_def, "cond/square");

  InitMemcpyAddrAsyncDef(graph, *model_task_def, "cond/merge");
  InitKernelTaskDef(graph, *model_task_def, "add_n");

  InitEventTaskDef(graph, *model_task_def);
  InitFusionTaskDef(graph, *model_task_def);
  InitEndGraphDef(graph, *model_task_def, NODE_NAME_NET_OUTPUT);

  InitHcclTaskDef(graph, *model_task_def, "HcomAllreduce");
  InitProfilerTaskDef(graph, *model_task_def);
  InitModelExitTaskDef(graph, *model_task_def);
  InitCmoTaskDef(graph, *model_task_def);
  InitCmoBarrierTaskDef(graph, *model_task_def);

  const size_t logic_var_base = VarManager::Instance(graph->GetSessionID())->GetVarMemLogicBase();
  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 32));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 32));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_TASK_GEN_VAR_ADDR, logic_var_base));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, logic_var_base));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));

  EXPECT_TRUE(AttrUtils::SetListInt(ge_model, ATTR_MODEL_HUGE_STREAM_LIST, {2}));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_P2P_MEMORY_SIZE, 256));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_SESSION_SCOPE_MEMORY_SIZE, 256));

  // Serialization GeModel for Save offline model.
  EXPECT_TRUE(tbe_kernel_store.Build());
  ge_model->SetTBEKernelStore(tbe_kernel_store);
  EXPECT_TRUE(cpu_kernel_store.Build());
  ge_model->SetCustAICPUKernelStore(cpu_kernel_store);
  EXPECT_TRUE(AttrUtils::SetListStr(ge_model, "needCheckCpu", { "aicpu_optype_01", "aicpu_optype_02" }));
  EXPECT_TRUE(AttrUtils::SetListStr(ge_model, "needCheckTf", { "aicpu_tf_optype_01", "aicpu_tf_optype_02" }));

  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_SESSION_ID, graph->GetSessionID()));
}

static void ProfileCommandInit(GeExecutor &ge_executor) {
  actual_info_type.clear();
  ProfilingProperties::Instance().ClearProperties();
  {
    Command command{"prof_init", {}, PROF_MODEL_LOAD_MASK | PROF_TRAINING_TRACE};
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }
}

static void ProfileCommandProf(GeExecutor &ge_executor, const uint32_t model_id) {
  {
    Command command{.cmd_type = "prof_start",
                    .cmd_params = { "devNums", "1", "devIdList", "0", PROFILE_MODEL_ID, std::to_string(model_id) },
                    .module_index = PROF_TRAINING_TRACE | PROF_OP_DETAIL_MASK };
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }

  {
    Command command{.cmd_type = "prof_model_subscribe",
                    .cmd_params = { PROFILE_MODEL_ID, std::to_string(model_id) },
                    .module_index = PROF_MODEL_LOAD_MASK | PROF_TRAINING_TRACE | PROF_OP_DETAIL_MASK };
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }
}

static void ProfileCommandFini(GeExecutor &ge_executor, const uint32_t model_id) {
  {
    Command command{.cmd_type = "prof_stop",
                    .cmd_params = { "devNums", "1", "devIdList", "0", PROFILE_MODEL_ID, std::to_string(model_id) },
                    .module_index = PROF_MODEL_LOAD_MASK | PROF_TRAINING_TRACE | PROF_OP_DETAIL_MASK };
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }

  {
    Command command{.cmd_type = "prof_model_cancel_subscribe",
                    .cmd_params = { PROFILE_MODEL_ID, std::to_string(model_id) }};
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }

  {
    Command command{.cmd_type = "prof_finalize"};
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }
}

static void DumpCommandInit(GeExecutor &ge_executor) {
  {
    DumpConfig dump_cfg;
    dump_cfg.dump_path = "./dump/";
    dump_cfg.dump_mode = "output";
    dump_cfg.dump_status = "on";
    dump_cfg.dump_op_switch = "on";
    ModelDumpConfig model_dump_config;
    model_dump_config.model_name = "g1_om";
    model_dump_config.layers.emplace_back("Less_Cast");
    model_dump_config.layers.emplace_back("cond/add");
    model_dump_config.layers.emplace_back("cond/mul");
    dump_cfg.dump_list.emplace_back(model_dump_config);
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
  }
}

static void DumpCommandFini(GeExecutor &ge_executor) {
  {
    DumpConfig dump_cfg;
    dump_cfg.dump_path = "./dump/";
    dump_cfg.dump_mode = "output";
    dump_cfg.dump_status = "off";
    dump_cfg.dump_op_switch = "off";
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
  }
}

static void ModelDumpInitCmd(GeExecutor &ge_executor) {
  {
    Command command{.cmd_type = "dump",
                    .cmd_params = {DUMP_STATUS, "on", DUMP_MODEL, "g1_om", DUMP_FILE_PATH, "/tmp", DUMP_MODE, "all"} };
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }
}

static void ModelDumpFiniCmd(GeExecutor &ge_executor) {
  {
    Command command{.cmd_type = "dump",
                    .cmd_params = {DUMP_STATUS, "off", DUMP_MODEL, "g1_om", DUMP_FILE_PATH, "/tmp", DUMP_MODE, "all"} };
    EXPECT_EQ(ge_executor.CommandHandle(command), SUCCESS);
  }
}

static void OfflineModelCommand(GeExecutor &ge_executor, const uint32_t model_id) {
  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); uint64_t batch_size = 0U;
    ge_executor.SetDynamicBatchSize(model_id, &dynamic_input_addr, length, batch_size);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); uint64_t image_height = 0U; uint64_t image_width = 0U;
    ge_executor.SetDynamicImageSize(model_id, &dynamic_input_addr, length, image_height, image_width);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = sizeof(uint64_t); std::vector<uint64_t> dynamic_dims;
    ge_executor.SetDynamicDims(model_id, &dynamic_input_addr, length, dynamic_dims);
  }

  {
    std::vector<uint64_t> dynamic_dims; std::vector<uint64_t> cur_dynamic_dims;
    ge_executor.GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims);
  }

  {
    std::vector<int64_t> batch_info; int32_t dynamic_type = 0U;
    ge_executor.GetCurShape(model_id, batch_info, dynamic_type);
  }

  {
    uint64_t dynamic_input_addr = 0U; uint64_t length = 0U; std::vector<kAippDynamicBatchPara> aipp_batch_para; kAippDynamicPara aipp_parms;
    ge_executor.SetDynamicAippData(model_id, &dynamic_input_addr, length, aipp_batch_para, aipp_parms);
  }

  {
    std::vector<TensorDesc> input_desc; std::vector<TensorDesc> output_desc; bool new_model_desc = false;
    ge_executor.GetModelDescInfo(model_id, input_desc, output_desc, new_model_desc);
  }

  {
    std::vector<std::vector<int64_t>> batch_info; int32_t dynamic_type = 0U;
    ge_executor.GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  }

  {
    std::vector<std::vector<int64_t>> batch_info;
    ge_executor.GetCombinedDynamicDims(model_id, batch_info);
  }

  {
    std::vector<std::string> user_designate_shape_order;
    ge_executor.GetUserDesignateShapeOrder(model_id, user_designate_shape_order);
  }

  {
    uint32_t index = 0U; AippConfigInfo aipp_info;
    ge_executor.GetAIPPInfo(model_id, index, aipp_info);
  }

  {
    uint32_t index = 0U; InputAippType type; size_t aipp_index = 0U;
    ge_executor.GetAippType(model_id, index, type, aipp_index);
  }

  {
    std::string op_name; std::string attr_name; std::string attr_value;
    ge_executor.GetOpAttr(model_id, op_name, attr_name, attr_value);
  }

  {
    std::vector<std::string> dynamic_output_shape_info;
    ge_executor.GetModelAttr(model_id, dynamic_output_shape_info);
  }

  {
    uint32_t max_size = 0U;
    ge_executor.GetMaxUsedMemory(model_id, max_size);
  }

  {
    uint32_t device_id = 0U;
    GeExecutor::GetDeviceIdByModelId(model_id, device_id) ;
  }

  {
    size_t shape_count = 0U;
    ge_executor.GetBatchInfoSize(model_id, shape_count);
  }

  {
    uint32_t index = 0U; OriginInputInfo orig_input_info;
    ge_executor.GetOrigInputInfo(model_id, index, orig_input_info);
  }

  {
    uint32_t index = 0U; std::vector<InputOutputDims> input_dims; std::vector<InputOutputDims> output_dims;
    ge_executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims);
  }

  {
    uint32_t device_id = 0U; uint32_t stream_id = 0U; uint32_t task_id = 0U; OpDescInfo op_desc_info;
    ge_executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  }
}

TEST_F(DavinciModelTest, sample_davinci_model_static_memory_no_tiling) {
  uint32_t mem_offset = 0U;
  ComputeGraphPtr graph;
  BuildSampleNoTilingGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);

  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_davinci_model_static_memory");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");

  GeModelPtr ge_model;
  BuildGraphModel(graph, ge_model, mem_offset);
  EXPECT_NE(ge_model, nullptr);

  int64_t value_0 = 127;
  int64_t value_1 = 100;
  int64_t value_2 = 258;
  int64_t value_3 = 512;
  // Tensor for input.
  std::vector<int64_t> dims = {1};
  TensorDesc tensor_desc(Shape(dims), FORMAT_ND, DT_INT64);
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  Tensor tensor_2(tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  Tensor tensor_3(tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<Tensor> input_tensors{ tensor_0, tensor_1, tensor_2, tensor_3 };

  // Tensor for input.
  GeTensorDesc sync_tensor_desc(GeShape(dims), FORMAT_ND, DT_INT64);
  GeTensor sync_tensor_0(sync_tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  GeTensor sync_tensor_1(sync_tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  GeTensor sync_tensor_2(sync_tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  GeTensor sync_tensor_3(sync_tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<GeTensor> sync_inputs{ sync_tensor_0, sync_tensor_1, sync_tensor_2, sync_tensor_3 };

  std::vector<uint32_t> model_ids;
  DumpCommandInit(ge_executor_);
  setenv(kEnvGeuseStaticMemory, "1", 1);
  {
    // Test LoadModelOnline: RunAsyncListener
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    flow_root_model->AddSubModel(ge_root_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->IncreaseLoadCount();

    // Callback for execute.
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

    // RunArgs of Graph.
    GEThreadLocalContext context;
    context.SetGraphOption(
        {{OPTION_EXEC_DYNAMIC_EXECUTE_MODE, "lazy_recompile"}, {OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR, "1"}});
    error_message::Context error_context;
    graph_node->Lock();
    RunArgs run_args{
        graph_node, graph->GetGraphID(), graph->GetSessionID(), error_context, input_tensors, flow_root_model, context,
        callback};

    // Load and execute.
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

    // Wait for execute.
    std::unique_lock<std::mutex> lock(run_mutex);
    EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
    EXPECT_EQ(run_status, SUCCESS);
    EXPECT_EQ(run_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }
}

TEST_F(DavinciModelTest, sample_davinci_model_static_memory) {
  uint32_t mem_offset = 0U;
  ComputeGraphPtr graph;
  BuildSampleGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);

  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_davinci_model_static_memory");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");

  GeModelPtr ge_model;
  BuildGraphModel(graph, ge_model, mem_offset);
  EXPECT_NE(ge_model, nullptr);

  int64_t value_0 = 127;
  int64_t value_1 = 100;
  int64_t value_2 = 258;
  int64_t value_3 = 512;
  // Tensor for input.
  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  Tensor tensor_2(tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  Tensor tensor_3(tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<Tensor> input_tensors{ tensor_0, tensor_1, tensor_2, tensor_3 };

  // Tensor for input.
  GeTensorDesc sync_tensor_desc(GeShape(), FORMAT_ND, DT_INT64);
  GeTensor sync_tensor_0(sync_tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  GeTensor sync_tensor_1(sync_tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  GeTensor sync_tensor_2(sync_tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  GeTensor sync_tensor_3(sync_tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<GeTensor> sync_inputs{ sync_tensor_0, sync_tensor_1, sync_tensor_2, sync_tensor_3 };

  std::vector<uint32_t> model_ids;
  DumpCommandInit(ge_executor_);
  setenv(kEnvGeuseStaticMemory, "1", 1);
  {
    // Test LoadModelOnline: RunAsyncListener
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    flow_root_model->AddSubModel(ge_root_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->IncreaseLoadCount();

    // Callback for execute.
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

    // RunArgs of Graph.
    GEThreadLocalContext context;
    context.SetGraphOption({{OPTION_EXEC_DYNAMIC_EXECUTE_MODE, "lazy_recompile"},
                            {OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR, "1"}});
    error_message::Context error_context;
    graph_node->Lock();
    RunArgs run_args{graph_node, graph->GetGraphID(), graph->GetSessionID(), error_context, input_tensors, flow_root_model, context, callback};

    // Load and execute.
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

    // Wait for execute.
    std::unique_lock<std::mutex> lock(run_mutex);
    EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
    EXPECT_EQ(run_status, SUCCESS);
    EXPECT_EQ(run_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph.
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  // ModelExecutor::RunGraph -> ExecuteGraph -> SyncExecuteModel -> ModelManager::DataInput -> DavinciModel::Push -> Run
  {
    // Test LoadModelOnline: GraphModelListener
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(false);
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 960)); // set max stream

    // Env for load:.ModelManager::CheckAndReleaseStreamEventResource
    GEThreadLocalContext &context = GetThreadLocalContext();
    context.SetGraphOption({{OPTION_EXEC_DYNAMIC_EXECUTE_MODE, "lazy_recompile"},
                            {OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR, "1"}});

    // profiling model subscribe on
    ProfilingManager::Instance().SetSubscribeInfo(0, graph->GetGraphID(), true);

    // Load model of graph
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

    // Execute Synchronous
    std::vector<GeTensor> sync_outputs;
    EXPECT_EQ(model_executor.RunGraph(graph_node, graph->GetGraphID(), sync_inputs, sync_outputs), SUCCESS);
    EXPECT_EQ(sync_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());
    // check reported graph id saved
    EXPECT_TRUE(ProfilingManager::Instance().IsGraphProfReported(graph->GetGraphID()));

    // clear profiling configurations
    ProfilingManager::Instance().subscribe_count_--;
    ProfilingManager::Instance().ProfFinalize();

    // Unload model of graph(leave as max stream model for follow test)
    //EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  {
    // Test LoadModelOnline: RunGraphWithStream
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    ge_root_model->SetIsSpecificStream(true); // For not start DavinciModel thread.
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(true);
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 960));
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 32));

    GeTensorDesc output_desc(GeShape({2, 4, 8, 2}), FORMAT_FRACTAL_Z, DT_FLOAT);
    std::vector<uint8_t> arg_3(512, 0);  // mem_offset += (2 * 4 * 8 * 2 * sizeof(float));
    GeTensor nn_tensor_21(output_desc, arg_3.data(), arg_3.size());
    const std::vector<GeTensor> nn_outputs{ nn_tensor_21 };

    // Load model of graph
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

    // NnExecute with stream.
    rtStream_t run_stream = &model_executor;
    EXPECT_EQ(model_executor.RunGraphWithStream(graph_node, graph->GetGraphID(), run_stream, sync_inputs, nn_outputs), SUCCESS);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph(leave as max event model for follow test).
    //EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  {
    // Test ModelManager::ReleaseResource
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->IncreaseLoadCount();
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 960));
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 960));

    // Callback for execute.
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

    // RunArgs of Graph.
    GEThreadLocalContext context;
    context.SetGraphOption({{OPTION_EXEC_DYNAMIC_EXECUTE_MODE, "lazy_recompile"},
                            {OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR, "1"}});
    error_message::Context error_context;
    graph_node->Lock();
    RunArgs run_args{graph_node, graph->GetGraphID(), graph->GetSessionID(), error_context, input_tensors, flow_root_model, context, callback};

    // Load and execute.
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

    // Wait for execute.
    std::unique_lock<std::mutex> lock(run_mutex);
    EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
    EXPECT_EQ(run_status, SUCCESS);
    EXPECT_EQ(run_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 32));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 32));
  {
    // Test LoadModelOnline: for SuperKernel
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->IncreaseLoadCount();

    // Setup SuperKernel
    EXPECT_TRUE(AttrUtils::SetBool(ge_model, ATTR_NAME_SWITCH_FOR_L1_FUSION, true));
    for (const auto &node : graph->GetAllNodes()) {
      if (node->GetOpDesc()->GetOpKernelLibName() == "AIcoreEngine") {
        EXPECT_TRUE(AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_FUSION_GROUP_KEY, 1));
      }
    }

    // Callback for execute.
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

    // RunArgs of Graph.
    GEThreadLocalContext context;
    error_message::Context error_context;
    graph_node->Lock();
    RunArgs run_args{graph_node, graph->GetGraphID(), graph->GetSessionID(), error_context, input_tensors, flow_root_model, context, callback};

    // Load and execute.
    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, graph->GetSessionID()), SUCCESS);
    EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

    // Wait for execute.
    std::unique_lock<std::mutex> lock(run_mutex);
    EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
    EXPECT_EQ(run_status, SUCCESS);
    EXPECT_EQ(run_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph.
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  DumpCommandFini(ge_executor_);
  for (const auto &id : model_ids) {
    EXPECT_EQ(ge_executor_.UnloadModel(id), ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
  }
  unsetenv(kEnvGeuseStaticMemory);
}

TEST_F(DavinciModelTest, sample_davinci_model_dynamic_memory) {
  shared_ptr<OpsKernelInfoStore> fake_ops_kernel_info_store = std::make_shared<FakeOpsKernelInfoStore>();
  // hccl op goes to AIcoreEngine in this testcase
  OpsKernelExecutorManager::GetInstance().executors_["AIcoreEngine"] = fake_ops_kernel_info_store;
  OpsKernelExecutorManager::GetInstance().executors_[kEngineNameHccl] = fake_ops_kernel_info_store;
  OpsKernelInfoStore *ptr = nullptr;
  EXPECT_EQ(OpsKernelExecutorManager::GetInstance().GetExecutor(kEngineNameHccl, ptr), SUCCESS);
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildSampleGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);

  GeModelPtr ge_model;
  BuildGraphModel(graph, ge_model, mem_offset);
  EXPECT_NE(ge_model, nullptr);

  int64_t value_0 = 127;
  int64_t value_1 = 100;
  int64_t value_2 = 258;
  int64_t value_3 = 512;
  // Tensor for input.
  TensorDesc tensor_desc(Shape(), FORMAT_ND, DT_INT64);
  Tensor tensor_0(tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  Tensor tensor_1(tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  Tensor tensor_2(tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  Tensor tensor_3(tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<Tensor> input_tensors{ tensor_0, tensor_1, tensor_2, tensor_3 };

  // Tensor for input.
  GeTensorDesc sync_tensor_desc(GeShape(), FORMAT_ND, DT_INT64);
  GeTensor sync_tensor_0(sync_tensor_desc, (uint8_t *)&value_0, sizeof(value_0));
  GeTensor sync_tensor_1(sync_tensor_desc, (uint8_t *)&value_1, sizeof(value_1));
  GeTensor sync_tensor_2(sync_tensor_desc, (uint8_t *)&value_2, sizeof(value_2));
  GeTensor sync_tensor_3(sync_tensor_desc, (uint8_t *)&value_3, sizeof(value_3));
  const std::vector<GeTensor> sync_inputs{ sync_tensor_0, sync_tensor_1, sync_tensor_2, sync_tensor_3 };

  RunModelData run_input_data;
  run_input_data.blobs.emplace_back(DataBuffer{&value_0, sizeof(value_0), false, 0});
  run_input_data.blobs.emplace_back(DataBuffer{&value_1, sizeof(value_1), false, 0});
  run_input_data.blobs.emplace_back(DataBuffer{&value_2, sizeof(value_2), false, 0});
  run_input_data.blobs.emplace_back(DataBuffer{&value_3, sizeof(value_3), false, 0});

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  std::vector<uint32_t> model_ids;
  ModelDumpInitCmd(ge_executor_);

  // ModelExecutor::RunGraph -> ExecuteGraph -> SyncExecuteModel -> ModelManager::DataInput -> DavinciModel::Push -> Run
  {
    // Test LoadModelOnline: GraphModelListener
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(false);
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 256 * 1024 * 1024));
    InitAippNodeRelated(graph, "_arg_3", "_arg_2");

    // Load model of graph
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Execute Synchronous
    std::vector<GeTensor> sync_outputs;
    EXPECT_EQ(model_executor.RunGraph(graph_node, graph->GetGraphID(), sync_inputs, sync_outputs), SUCCESS);
    EXPECT_EQ(sync_outputs.size(), 1U);

    // Unload model of graph(leave as max memory model for follow test)
    //EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    CleanAippNodeInfo(graph, "_arg_3");
  }

  {
    // Test LoadModelOnline: RunAsyncListener
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    const auto graph_node = MakeShared<GraphNode>(graph->GetGraphID());
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->IncreaseLoadCount();
    const size_t k512MegaBytes = 512 * 1024 * 1024;
    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, k512MegaBytes)); // Will unload last model.
    InitAippNodeStatic(graph, "_arg_3");

    // Callback for execute.
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

    // RunArgs of Graph.
    GEThreadLocalContext context;
    error_message::Context error_context;
    graph_node->Lock();
    RunArgs run_args{graph_node, graph->GetGraphID(), graph->GetSessionID(), error_context, input_tensors, flow_root_model, context, callback};

    // Load and execute.
    VarManager::Instance(graph->GetSessionID())->UpdateMemoryConfig(k512MegaBytes, k512MegaBytes, k512MegaBytes, k512MegaBytes);
    EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

    // Wait for execute.
    std::unique_lock<std::mutex> lock(run_mutex);
    EXPECT_EQ(model_run_cv.wait_for(lock, std::chrono::seconds(10)), std::cv_status::no_timeout);
    EXPECT_EQ(run_status, SUCCESS);
    EXPECT_EQ(run_outputs.size(), 1U);
    model_ids.emplace_back(ge_root_model->GetModelId());

    // Unload model of graph
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph->GetGraphID()), SUCCESS);
    CleanAippNodeInfo(graph, "_arg_3");
  }
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);

  InitAippNodeDynamic(graph, "_arg_3");
  DelStaticForOffline(graph, mem_offset); // Offline model will set new session_id, static var invalid.
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  {
    // Test LoadModelOffline
    SaveParam save_param;
    ModelHelper model_helper;
    model_helper.SetSaveMode(true);  // Save to file.
    ModelBufferData model_buffer;
    EXPECT_EQ(model_helper.SaveToOmModel(ge_model, save_param, "sample_offline_model.om", model_buffer), SUCCESS);

    size_t model_mem_size = 0U; size_t model_weight_size = 0U;
    EXPECT_EQ(ge_executor_.GetMemAndWeightSize("sample_offline_model.om", model_mem_size, model_weight_size), SUCCESS);

    ModelData model_data;
    EXPECT_EQ(ge_executor_.LoadDataFromFile("sample_offline_model.om", model_data), SUCCESS);
    model_data.om_name = "g1_om";
    {
      size_t mem_size = 0U; size_t weight_size = 0U;
      EXPECT_EQ(ge_executor_.GetMemAndWeightSize(model_data.model_data, model_data.model_len, mem_size, weight_size), SUCCESS);
      EXPECT_EQ(model_mem_size, mem_size);
      EXPECT_EQ(model_weight_size, weight_size);
    }

    std::vector<uint8_t> out_0(512, 0);  // mem_offset += (2 * 4 * 8 * 2 * sizeof(float));
    RunModelData run_output_data;
    run_output_data.blobs.emplace_back(DataBuffer{out_0.data(), out_0.size(), false, 0});
    {
      uint32_t model_id = 0U;
      ProfileCommandInit(ge_executor_);
      EXPECT_EQ(ge_executor_.LoadModelFromData(model_id, model_data, nullptr, 0U, nullptr, 0U), SUCCESS);
      ProfileCommandProf(ge_executor_, model_id);
      model_ids.emplace_back(model_id);

      // Run: Asynchronize
      EXPECT_EQ(ge_executor_.ExecModel(model_id, nullptr, run_input_data, run_output_data, true), SUCCESS);

      EXPECT_EQ(actual_info_type.find("id_map_info"), actual_info_type.end());
      for (auto &info : actual_info_type) {
        const static std::set<std::string> expect_info_type{
           "task_desc_info", "tensor_data_info", "model_load_info", "fusion_op_info", "step_info", "model_time_info"
        };
        EXPECT_NE(expect_info_type.find(info.substr(0, info.find("info") + strlen("info"))), expect_info_type.end());
      }

      ProfileCommandFini(ge_executor_, model_id);
      EXPECT_EQ(ge_executor_.UnloadModel(model_id), SUCCESS);
    }

    {
      unsetenv("GE_PROFILING_TO_STD_OUT");
      uint32_t model_id = 0U;
      ProfileCommandInit(ge_executor_);
      EXPECT_EQ(ge_executor_.LoadModelFromData(model_id, model_data, nullptr, 0U, nullptr, 0U), SUCCESS);
      ProfileCommandProf(ge_executor_, model_id);
      model_ids.emplace_back(model_id);

      // Run: Synchronize, forbidden stream
      EXPECT_EQ(ge_executor_.ExecModel(model_id, nullptr, run_input_data, run_output_data, false), SUCCESS);

      // Run: Synchronize, customer stream
      rtStream_t run_stream = &model_id;
      EXPECT_EQ(ge_executor_.ExecModel(model_id, run_stream, run_input_data, run_output_data, false), SUCCESS);

      EXPECT_EQ(actual_info_type.find("id_map_info"), actual_info_type.end());
      for (auto &info : actual_info_type) {
        const static std::set<std::string> expect_info_type{
            "task_desc_info", "tensor_data_info", "model_load_info", "fusion_op_info", "step_info", "model_time_info"
        };
        EXPECT_NE(expect_info_type.find(info.substr(0, info.find("info") + strlen("info"))), expect_info_type.end());
      }

      ProfileCommandFini(ge_executor_, model_id);
      OfflineModelCommand(ge_executor_, model_id);
      EXPECT_EQ(ge_executor_.UnloadModel(model_id), SUCCESS);
      setenv("GE_PROFILING_TO_STD_OUT", "1", 1); // Reset for it`s set in main.
    }

    {
      // Test LoadModelWithQ
      uint32_t model_id = 0;
      const std::vector<uint32_t> input_queue_ids{ 1001U, 1002U, 1003U, 1004U };
      const std::vector<uint32_t> output_queue_ids{ 2001U };

      EXPECT_EQ(ge_executor_.LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids), SUCCESS);
      model_ids.emplace_back(model_id);

      EXPECT_EQ(ge_executor_.UnloadModel(model_id), SUCCESS);
    }

    delete [] static_cast<uint8_t *>(model_data.model_data);
  }
  ModelDumpFiniCmd(ge_executor_);

  for (const auto &id : model_ids) {
    EXPECT_EQ(ge_executor_.UnloadModel(id), ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
  }
}


static void BuildAddGraph(ComputeGraphPtr &graph, const std::string &file_constant_name, const bool is_unknown) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  std::vector<int64_t> shape = {2, 2, 2, 2};
  auto file_const_op = OP_CFG(FILECONSTANT).Attr("shape", shape).Attr("dtype", DT_FLOAT).Attr("file_id", "vector_search_bucker_value_bin");

  int64_t dims_size = 1;
  vector<int64_t> data_vec = {2, 2, 2, 2};
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<float> data_value_vec(dims_size, 1);
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                  data_value_vec.size() * sizeof(float));
  std::cout << "davinci_model_execute_with_file_constant" << data_value_vec.size() << std::endl;
  auto const_op = OP_CFG(CONSTANT).Weight(data_tensor);
  auto add = OP_CFG(ADD);
  auto output = OP_CFG(NETOUTPUT);
  DEF_GRAPH(g1) {
    CHAIN(NODE(file_constant_name, file_const_op)->EDGE(0, 0)->NODE("add", add));
    CHAIN(NODE("const_op", const_op)->EDGE(0, 1)->NODE("add", add));
    CHAIN(NODE("add", add)->EDGE(0, 0)->NODE(NODE_NAME_NET_OUTPUT, output));
  };

  graph = ToComputeGraph(g1);
  graph->SetGraphUnknownFlag(is_unknown);
  uint32_t mem_offset = 0;
  SetUnknownOpKernel(graph, mem_offset, true);
}

void BuildAddGraphModel(ComputeGraphPtr &graph, GeModelPtr &ge_model) {
  TBEKernelStore tbe_kernel_store;
  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  EXPECT_TRUE(AttrUtils::SetInt(graph, "globleworkspace_status", 1));
  EXPECT_TRUE(AttrUtils::SetInt(graph, "globleworkspace_status_bytes", 1));
  InitKernelTaskDef_TE(graph, *model_task_def, "add", tbe_kernel_store);

  InitEventTaskDef(graph, *model_task_def);
  InitFusionTaskDef(graph, *model_task_def);
  InitEndGraphDef(graph, *model_task_def, NODE_NAME_NET_OUTPUT);

  InitProfilerTaskDef(graph, *model_task_def);
  InitModelExitTaskDef(graph, *model_task_def);

  const size_t logic_var_base = VarManager::Instance(graph->GetSessionID())->GetVarMemLogicBase();
  ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 3));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_TASK_GEN_VAR_ADDR, logic_var_base));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, logic_var_base));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));

  std::vector<uint64_t> weights_value(100, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));

  EXPECT_TRUE(tbe_kernel_store.Build());
  ge_model->SetTBEKernelStore(tbe_kernel_store);
}

TEST_F(DavinciModelTest, davinci_model_execute_with_file_constant) {
  ComputeGraphPtr graph;
  BuildAddGraph(graph, "file_constant_1", false);
  {
    size_t file_const_size = 64;
    float *float_buf = (float *)malloc(file_const_size);
    if (float_buf == nullptr) {
      return;
    }
    std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
    if (!out1.is_open()) {
      free(float_buf);
      return;
    }
    out1.write((char *)float_buf, file_const_size);
    out1.close();
    free(float_buf);
  }
  GeModelPtr ge_model;
  BuildAddGraphModel(graph, ge_model);
  EXPECT_NE(ge_model, nullptr);

  {
    // Test LoadModelOnline
    GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

    GraphId graph_id = 1001;
    GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(true);


    std::map<std::string, std::string> config;
    std::string a = "ge.exec.value_bins";
    std::string b = "{\"value_bins\":[{\"value_bin_id\":\"vector_search_bucker_value_bin\", \"value_bin_file\":\"./test_copy_one_weight.bin\"}]}";
    config.insert(std::pair<std::string, std::string>(a,b));
    GetThreadLocalContext().SetGraphOption(config);

    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }
  (void)remove("test_copy_one_weight.bin");
}

TEST_F(DavinciModelTest, command_profiling_get_hybrid_model) {
  uint32_t model_id = 999;
  Command cmd;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  auto hybrid_model_ptr = ge::hybrid::HybridDavinciModel::Create(ge_root_model);
  auto shared_model = std::shared_ptr<hybrid::HybridDavinciModel>(hybrid_model_ptr.release());
  shared_model->SetDeviceId(0);
  cmd.cmd_params.push_back("modelId");
  cmd.cmd_params.push_back(to_string(model_id));
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelSubscribeCommand(cmd), FAILED);
  ModelManager::GetInstance().InsertModel(model_id, shared_model);
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelSubscribeCommand(cmd), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelUnsubscribeCommand(cmd), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(model_id), SUCCESS);
}

TEST_F(DavinciModelTest, unknown_shape_execute_with_file_constant_host) {
  ComputeGraphPtr graph;
  BuildAddGraph(graph, "file_constant_2", true);
  {
    size_t file_const_size = 64;
    float *float_buf = (float *)malloc(file_const_size);
    if (float_buf == nullptr) {
      return;
    }
    std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
    if (!out1.is_open()) {
      free(float_buf);
      return;
    }
    out1.write((char *)float_buf, file_const_size);
    out1.close();
    free(float_buf);
  }
  GeModelPtr ge_model;
  BuildAddGraphModel(graph, ge_model);
  EXPECT_NE(ge_model, nullptr);

  // Test LoadModelOnline
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);

  std::map<std::string, std::string> config;
  std::string a = "ge.exec.value_bins";
  std::string b =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_bucker_value_bin\", "
      "\"value_bin_file\":\"./test_copy_one_weight.bin\"}]}";
  config.insert(std::pair<std::string, std::string>(a, b));
  config["ge.exec.placement"] = "HOST";
  GetThreadLocalContext().SetGraphOption(config);

  auto hybrid_model = hybrid::HybridDavinciModel::Create(ge_root_model);
  ASSERT_NE(hybrid_model, nullptr);
  ASSERT_EQ(hybrid_model->Init(), SUCCESS);

  (void)remove("test_copy_one_weight.bin");
}

TEST_F(DavinciModelTest, unknown_shape_execute_with_file_constant) {
  ComputeGraphPtr graph;
  BuildAddGraph(graph, "file_constant_33333", true);
  {
    size_t file_const_size = 64;
    float *float_buf = (float *)malloc(file_const_size);
    if (float_buf == nullptr) {
      return;
    }
    std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
    if (!out1.is_open()) {
      free(float_buf);
      return;
    }
    out1.write((char *)float_buf, file_const_size);
    out1.close();
    free(float_buf);
  }
  GeModelPtr ge_model;
  BuildAddGraphModel(graph, ge_model);
  EXPECT_NE(ge_model, nullptr);

  // Test LoadModelOnline
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);

  std::map<std::string, std::string> config;
  std::string a = "ge.exec.value_bins";
  std::string b =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_bucker_value_bin\", "
      "\"value_bin_file\":\"./test_copy_one_weight.bin\"}]}";
  config.insert(std::pair<std::string, std::string>(a, b));
  GetThreadLocalContext().SetGraphOption(config);
  auto hybrid_model = hybrid::HybridDavinciModel::Create(ge_root_model);
  ASSERT_NE(hybrid_model, nullptr);
  ASSERT_EQ(hybrid_model->Init(), SUCCESS);

  (void)remove("test_copy_one_weight.bin");
}

TEST_F(DavinciModelTest, davinci_model_execute_no_tiling) {
  const auto SetUnknownOpKernelForNoTiling = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    GeTensorDesc tensor0(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    TensorUtils::SetSize(tensor0, 64);
    AttrUtils::SetBool(tensor0, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor0, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 0);
    std::vector<int64_t> max_shape_list = {1, 10, 224, 224};
    AttrUtils::SetListInt(tensor0, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    GeTensorDesc tensor1(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    TensorUtils::SetSize(tensor1, 64);
    AttrUtils::SetBool(tensor1, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor1, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
    AttrUtils::SetListInt(tensor1, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      if (op_desc->GetType() == DATA) {
        op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
        op_desc->UpdateOutputDesc(0, tensor0);
        op_desc->SetOutputOffset({2048});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else if (op_desc->GetType() == ADD) {
        op_desc->SetOpKernelLibName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor0);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({0, 2048});
        op_desc->SetOutputOffset({2112});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else {
        op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
        op_desc->UpdateInputDesc(0, tensor1);
        op_desc->SetInputOffset({2112});
        op_desc->SetSrcName( { "add" } );
        op_desc->SetSrcIndex({ 0 });
      }
    }
  };

  auto add = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto data = OP_CFG(DATA).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto output = OP_CFG(NETOUTPUT).Attr(ATTR_NAME_OP_NO_TILING, true);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data", data)->EDGE(0, 0)->NODE("add", add));
    CHAIN(NODE("add", add)->EDGE(0, 0)->NODE("output", output));
  };

  auto graph = ToComputeGraph(g1);
  SetUnknownOpKernelForNoTiling(graph->GetDirectNode());
  EXPECT_NE(graph, nullptr);

  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  TBEKernelStore tbe_kernel_store;
  InitKernelTaskDef_TE(graph, *model_task_def, "add", tbe_kernel_store);
  InitEventTaskDef(graph, *model_task_def);
  InitFusionTaskDef(graph, *model_task_def);
  InitEndGraphDef(graph, *model_task_def, "output");
  InitProfilerTaskDef(graph, *model_task_def);
  InitModelExitTaskDef(graph, *model_task_def);

  DumpProperties dump_properties;
  dump_properties.SetDumpMode("all");
  dump_properties.AddPropertyValue(DUMP_ALL_MODEL, {});
  DumpManager::GetInstance().AddDumpProperties(0, dump_properties);
  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));
  EXPECT_NE(ge_model, nullptr);

  {
    // Test LoadModelOnline
    GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

    GraphId graph_id = 1001;
    GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(true);

    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }

  // Serialization GeModel to memory.
  SaveParam save_param;
  ModelHelper model_helper;
  model_helper.SetSaveMode(false);  // Save to buffer.
  ModelBufferData model_buffer;
  EXPECT_TRUE(tbe_kernel_store.Build());
  ge_model->SetTBEKernelStore(tbe_kernel_store);
  EXPECT_EQ(model_helper.SaveToOmModel(ge_model, save_param, "file_name_prefix", model_buffer), SUCCESS);
  const ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", ""};

  // Test LoadModelWithQ
  {
    const std::vector<uint32_t> input_queue_ids{ 1001U };
    const std::vector<uint32_t> output_queue_ids{ 1002U };

    uint32_t model_id = 0;
    GeExecutor ge_executor;
    EXPECT_EQ(ge_executor.LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids), SUCCESS);
    EXPECT_EQ(ge_executor.UnloadModel(model_id), SUCCESS);
  }
}

TEST_F(DavinciModelTest, davinci_model_execute_with_aicpu_queue) {
  std::vector<NamedAttrs> resources(2);
  NamedAttrs &queue_resource = resources[0];
  AttrUtils::SetStr(queue_resource, "resource_type", "RES_QUEUE");
  AttrUtils::SetStr(queue_resource, "queue_name", "some_queue");
  AttrUtils::SetInt(queue_resource, "queue_depth", 2);
  AttrUtils::SetInt(queue_resource, "queue_id_idx", 0);

  NamedAttrs &channel_resource = resources[1];
  AttrUtils::SetStr(channel_resource, "resource_type", "RES_CHANNEL");

  auto n_queue_id = OP_CFG(CONSTANT);
  NamedAttrs op_resource;
  auto n_batch_dequeue = OP_CFG("BatchDequeue").Attr("_resource_list", resources);
  DEF_GRAPH(g1) {
    CHAIN(NODE("queue_id", n_queue_id)->EDGE(0, 0)->NODE("batch_dequeue", n_batch_dequeue));
  };
  const auto graph = ToComputeGraph(g1);
  EXPECT_NE(graph, nullptr);
  const auto node = graph->FindNode("batch_dequeue");
  node->GetOpDesc()->SetInputOffset({0});

  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  InitKernelTaskDef_AI_CPU(graph, *model_task_def, "batch_dequeue");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));
  EXPECT_NE(ge_model, nullptr);

  {
    // Test LoadModelOnline
    GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

    GraphId graph_id = 1001;
    GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(true);

    ModelExecutor model_executor;
    EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
    EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
    EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
    EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  }
}

TEST_F(DavinciModelTest, sample_davinci_model_execute_reuse_zero_copy_memory) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildSampleGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);

  GeModelPtr ge_model;
  BuildGraphModel(graph, ge_model, mem_offset);
  EXPECT_NE(ge_model, nullptr);

  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  GetThreadLocalContext().SetGraphOption(std::map<std::string, std::string>{{OPTION_EXEC_REUSE_ZERO_COPY_MEMORY, "1"}});

  const static std::string kEnabled = "1";
  std::string reuse_zero_copy_memory;
  GetContext().GetOption(OPTION_EXEC_REUSE_ZERO_COPY_MEMORY, reuse_zero_copy_memory);
  EXPECT_EQ(reuse_zero_copy_memory, kEnabled);

  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);

  GetThreadLocalContext().SetGraphOption(std::map<std::string, std::string>{{OPTION_EXEC_REUSE_ZERO_COPY_MEMORY, ""}});
}

Status BuildGraphNode(GraphId graph_id, GraphNodePtr &graph_node, FlowModelPtr &flow_root_model, GeModelPtr &ge_model) {
  uint32_t mem_offset = 0;
  ComputeGraphPtr graph;
  BuildSampleGraph(graph, mem_offset);
  EXPECT_NE(graph, nullptr);

  BuildGraphModel(graph, ge_model, mem_offset);
  EXPECT_NE(ge_model, nullptr);

  auto ge_root_model = MakeShared<GeRootModel>(graph);
  flow_root_model = MakeShared<FlowModel>(graph);
  flow_root_model->AddSubModel(ge_root_model);
  ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

  graph_node = MakeShared<GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);
  return SUCCESS;
}

// davinci model reuse memory
TEST_F(DavinciModelTest, load_model_with_extending_static_memory) {
  // set and check option
  std::map<std::string, std::string> options_map;
  options_map[kOptionExecGeUseStaticMemory] = "2";
  GetThreadLocalContext().SetGraphOption(options_map);
  EXPECT_TRUE(ExecutorUtils::IsGeUseExtendSizeStaticMemory());

  // first model, malloc memory
  GraphId graph_id1 = 1001;
  GraphNodePtr graph_node1;
  FlowModelPtr flow_root_model1;
  GeModelPtr ge_model1;
  (void)BuildGraphNode(graph_id1, graph_node1, flow_root_model1, ge_model1);
  ModelExecutor model_executor;
  const uint64_t session_id = flow_root_model1->GetRootGraph()->GetSessionID();
  EXPECT_EQ(model_executor.Initialize({}, session_id), SUCCESS);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model1, graph_node1), SUCCESS);

  // second model, reuse memory of model1
  options_map[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "1";
  GetThreadLocalContext().SetGraphOption(options_map);
  GraphId graph_id2 = 1002;
  GraphNodePtr graph_node2;
  FlowModelPtr flow_root_model2;
  GeModelPtr ge_model2;
  VarManagerPool::Instance().RemoveVarManager(session_id);
  (void)BuildGraphNode(graph_id2, graph_node2, flow_root_model2, ge_model2);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model2, graph_node2), SUCCESS);
  auto &model_mgr = ModelManager::GetInstance();
  // Check the number of models loaded
  EXPECT_EQ(model_mgr.model_map_.size(), 2);
  // Check, two davinci model use the same memory base;
  EXPECT_EQ(model_mgr.model_map_.begin()->second->mem_base_, model_mgr.model_map_.rbegin()->second->mem_base_);

  // third model, unload all model, molloc new memory
  GraphId graph_id3 = 1003;
  GraphNodePtr graph_node3;
  FlowModelPtr flow_root_model3;
  GeModelPtr ge_model3;
  VarManagerPool::Instance().RemoveVarManager(session_id);
  (void)BuildGraphNode(graph_id3, graph_node3, flow_root_model3, ge_model3);
  int64_t mem_offset = 0;
  AttrUtils::GetInt(ge_model3, ATTR_MODEL_MEMORY_SIZE, mem_offset);
  mem_offset += 64;
  EXPECT_TRUE(AttrUtils::SetInt(ge_model3, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model3, graph_node3), SUCCESS);
  // Check the number of models loaded
  EXPECT_EQ(model_mgr.model_map_.size(), 1);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model3, graph_id3), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  GetThreadLocalContext().SetGraphOption({});
}

TEST_F(DavinciModelTest, davinci_model_execute_no_tiling_without_q) {
  const auto SetUnknownOpKernelForNoTiling = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    GeTensorDesc tensor0(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    TensorUtils::SetSize(tensor0, 64);
    AttrUtils::SetBool(tensor0, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor0, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 0);
    std::vector<int64_t> max_shape_list = {1, 10, 224, 224};
    AttrUtils::SetListInt(tensor0, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    GeTensorDesc tensor1(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    TensorUtils::SetSize(tensor1, 64);
    AttrUtils::SetBool(tensor1, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor1, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
    AttrUtils::SetListInt(tensor1, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      if (op_desc->GetType() == DATA) {
        op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
        op_desc->UpdateOutputDesc(0, tensor0);
        op_desc->SetOutputOffset({2048});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else if (op_desc->GetType() == ADD) {
        op_desc->SetOpKernelLibName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor0);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({2048});
        op_desc->SetOutputOffset({2112});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else {
        op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
        op_desc->UpdateInputDesc(0, tensor1);
        op_desc->SetInputOffset({2112});
        op_desc->SetSrcName( { "add" } );
        op_desc->SetSrcIndex({ 0 });
      }
    }
  };
  int32_t data_value_vec1[2] = {0, 0};
  GeTensorDesc data_tensor_desc(GeShape({2}), FORMAT_ND, DT_INT32);
  GeTensorPtr data_tensor1 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec1, 2 * sizeof(int32_t));
  auto const1 = OP_CFG(CONSTANT).Weight(data_tensor1).Attr(ATTR_NAME_OP_NO_TILING, true);

  auto add = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto output = OP_CFG(NETOUTPUT).Attr(ATTR_NAME_OP_NO_TILING, true);
  DEF_GRAPH(g1) {
      CHAIN(NODE("const1", const1)->EDGE(0, 0)->NODE("add", add));
      CHAIN(NODE("add", add)->EDGE(0, 0)->NODE("output", output));
  };

  auto graph = ToComputeGraph(g1);
  SetUnknownOpKernelForNoTiling(graph->GetDirectNode());
  EXPECT_NE(graph, nullptr);

  std::shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  TBEKernelStore tbe_kernel_store;
  InitKernelTaskDef_TE(graph, *model_task_def, "add", tbe_kernel_store);
  InitEventTaskDef(graph, *model_task_def);
  InitFusionTaskDef(graph, *model_task_def);
  InitEndGraphDef(graph, *model_task_def, "output");
  InitProfilerTaskDef(graph, *model_task_def);
  InitModelExitTaskDef(graph, *model_task_def);

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetModelTaskDef(model_task_def);
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_BASE_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, 0));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));
  EXPECT_NE(ge_model, nullptr);

  {
    GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
    GeExecutor ge_executor;
    uint32_t model_id = 0;
    EXPECT_NE(ge_executor.LoadModelWithoutQ(model_id, ge_root_model), SUCCESS);
  }

  {
    // Test LoadModelOnline
    GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
    auto flow_root_model = MakeShared<FlowModel>(graph);
    flow_root_model->AddSubModel(ge_root_model);
    ge_root_model->SetSubgraphInstanceNameToModel(graph->GetName(), ge_model);

    GraphId graph_id = 1001;
    GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
    graph_node->SetFlowModel(flow_root_model);
    graph_node->SetLoadFlag(true);
    graph_node->SetAsync(true);

    GeExecutor ge_executor;
    uint32_t model_id = 0;
    EXPECT_NE(ge_executor.LoadModelWithoutQ(model_id, ge_root_model), SUCCESS);
    EXPECT_EQ(ge_executor.UnloadModel(model_id), SUCCESS);
  }
}

} // namespace ge

