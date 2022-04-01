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
#include <gmock/gmock.h>
#include <gflags/gflags.h>

#define private public
#define protected public
#include "ge_running_env/fake_ops_kernel_builder.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"

#include "init_ge.h"
#include "ge_running_env/path_utils.h"
#include "compiler/offline/main_impl.h"
#include "runtime/rt.h"
#include "framework/executor/ge_executor.h"
#include "framework/generator/ge_generator.h"
#include "single_op/single_op.h"
#include "utils/model_data_builder.h"
#include "single_op/task/tbe_task_builder.h"
#include "utils/tensor_descs.h"
#include "utils/data_buffers.h"
#include "utils/mock_runtime.h"
#include "register/op_tiling_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "hybrid/node_executor/ge_local/ge_local_node_executor.h"
#include "graph/manager/graph_mem_manager.h"
#include "utils/bench_env.h"
#include "utils/taskdef_builder.h"
#include "hybrid/model/hybrid_model_builder.h"

#include "ge_running_env/ge_running_env_faker.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"
#include "depends/runtime/src/runtime_stub.h"
#include "external/graph/operator_reg.h"
#include "single_op/single_op_manager.h"

#undef private
#undef protected

DECLARE_string(output);
DECLARE_string(singleop);
DECLARE_string(log);
DECLARE_string(soc_version);
USING_FAKE_NS
namespace ge {
constexpr int64_t kMemtypeHostCompileIndependent = 2;
REG_OP(FakeOpForSingleOp)
    .INPUT(x1, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32, DT_INT8}))
    .INPUT(x2, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32, DT_INT8}))
    .OPTIONAL_INPUT(bias, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32}))
    .OUTPUT(y, TensorType({DT_FLOAT, DT_FLOAT16, DT_INT32}))
    .OPTIONAL_INPUT(offset_w, TensorType({DT_INT8}))
    .ATTR(transpose_x1, Bool, false)
    .ATTR(transpose_x2, Bool, false)
    .ATTR(offset_x, Int, 0)
    .OP_END_FACTORY_REG(FakeOpForSingleOp);

namespace {
using GenerateTaskFun = std::function<Status(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks)>;
constexpr size_t kHostMemInputIndex = 0U;
void ResetFlags() {
  FLAGS_output.clear();
  FLAGS_singleop.clear();
  FLAGS_log = "null";
  FLAGS_soc_version.clear();
}

Status CompileSingleOpByAtc(const std::string &json_path) {
  auto test_dir = PathJoin(GetRunPath().c_str(), "st_run_data/single_op_output");
  Mkdir(test_dir.c_str());
  std::string json_path_arg = "--singleop=" + json_path;
  std::string output_arg = "--output=" + test_dir;
  const char *argv[] = {"atc",
                        "--soc_version=Ascend310",
                        "--log=error",
                        json_path_arg.c_str(),
                        output_arg.c_str(),
  };
  auto ret = main_impl(sizeof(argv) / sizeof(argv[0]), (char **) argv);
  ResetFlags();
  return ret;
}

class MockOpsKernelBuilder : public FakeOpsKernelBuilder {
 public:
  MOCK_METHOD3(GenerateTask, Status(const Node &, RunContext &, vector<domi::TaskDef> &));
  MOCK_METHOD0(GetWorkspaces, std::vector<int64_t>());

  Status CalcOpRunningParam(Node &node) override {
    GE_CHK_STATUS_RET_NOLOG(FakeOpsKernelBuilder::CalcOpRunningParam(node));
    auto workspaces = GetWorkspaces();
    node.GetOpDesc()->SetWorkspace(workspaces);
    return SUCCESS;
  }
};

Status GenerateTaskForAicpu(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != NEG) {
    return SUCCESS;
  }

  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(0));
  return SUCCESS;
}

Status GenerateTaskForTfKernel(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != NEG) {
    return SUCCESS;
  }

  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(0));
  return SUCCESS;
}

Status GenerateTaskForTfDependCompute(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != NEG) {
    return SUCCESS;
  }

  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(4));
  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(0));
  AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_UNKNOWN_SHAPE_TYPE, 4);
  return SUCCESS;
}

Status GenerateTaskForAiCpuDependCompute(const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != NEG) {
    return SUCCESS;
  }

  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(4));
  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(0));
  AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_UNKNOWN_SHAPE_TYPE, 4);
  return SUCCESS;
}

Status GenerateTaskForAiCore(const Node &node,
                             RunContext &run_context,
                             std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != RELU) {
    return SUCCESS;
  }

  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTask());
  return SUCCESS;
}

Status GenerateTaskForDynamicAiCore(const Node &node,
                                    RunContext &run_context,
                                    std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != RELU) {
    return SUCCESS;
  }

  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTask(true));
  return SUCCESS;
}

Status GenerateTaskForDynamicAiCoreV2(const Node &node,
                                      RunContext &run_context,
                                      std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != RELU) {
    return SUCCESS;
  }

  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTaskWithHandle());
  return SUCCESS;
}

Status GenerateTaskForDynamicAiCoreWithAtomicAddrClean(const Node &node,
                                                       RunContext &run_context,
                                                       std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != RELU) {
    return SUCCESS;
  }

  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildAtomicAddrCleanTask());
  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTask(true));
  return SUCCESS;
}

Status GenerateTaskForMemcpyAsync(const Node &node,
                                  RunContext &run_context,
                                  std::vector<domi::TaskDef> &tasks) {
  if (node.GetType() != MEMCPYASYNC) {
    return SUCCESS;
  }
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
  auto kernel_def = task_def.mutable_memcpy_async();
  kernel_def->set_op_index(node.GetOpDesc()->GetId());
  kernel_def->set_count(64);
  kernel_def->set_dst_max(64);
  kernel_def->set_src(0);
  kernel_def->set_dst(64);
  kernel_def->set_kind(RT_MEMCPY_ADDR_DEVICE_TO_DEVICE);
  tasks.emplace_back(task_def);
  return SUCCESS;
}
}  // namespace

class SingleOpTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    GEFinalize();
    ReInitGe();
    BenchEnv::Init();
    optiling::OpTilingFuncV2 tilingfun = [](const ge::Operator &op,
        const optiling::OpCompileInfoV2 &compile_info,
        optiling::OpRunInfoV2 &run_info) -> bool {
      run_info.SetWorkspaces({1024});
      return true;
    };

    optiling::OpTilingRegistryInterf_V2(RELU, tilingfun);
    REGISTER_OP_TILING_UNIQ_V2(ReLU, tilingfun, 1);
  }

  void SetUp() {
    rtStreamCreate(&stream_, 0);
  }

  void TearDown() {
    MockRuntime::SetInstance(nullptr);
    GeExecutor::ReleaseSingleOpResource(stream_);
    rtStreamDestroy(stream_);

    GEFinalize();
    ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
    // rollback
    GeRunningEnvFaker env;
    env.InstallDefault();
  }

  uint64_t model_id = 0;
  rtStream_t stream_ = nullptr;
  std::vector<std::unique_ptr<uint8_t[]>> input_buffers_;
  std::vector<std::unique_ptr<uint8_t[]>> output_buffers_;

  static Status BuildSingleOp(OpDescPtr &op_desc, ModelBufferData &model_buffer) {
    vector<GeTensor> inputs;
    vector<GeTensor> outputs;
    std::vector<GeTensorDesc> input_desc;
    std::vector<GeTensorDesc> output_desc;
    for (const auto &tensor_desc : op_desc->GetAllInputsDescPtr()) {
      inputs.emplace_back(GeTensor(*tensor_desc));
      input_desc.emplace_back(*tensor_desc);
    }
    for (const auto &tensor_desc : op_desc->GetAllOutputsDescPtr()) {
      outputs.emplace_back(GeTensor(*tensor_desc));
      output_desc.emplace_back(*tensor_desc);
    }
    GeGenerator generator;
    generator.Initialize({});
    auto ret = generator.BuildSingleOpModel(op_desc, inputs, outputs, ENGINE_SYS, false, model_buffer);
    return ret;
  }

  Status RunStaticTestCast(OpDescPtr &op_desc) {
    ModelBufferData model_buffer;
    EXPECT_EQ(BuildSingleOp(op_desc, model_buffer), SUCCESS);
    ModelData model_data;
    model_data.model_data = model_buffer.data.get();
    model_data.model_len = model_buffer.length;
    SingleOp *single_op = nullptr;
    GeExecutor ge_executor;
    EXPECT_EQ(ge_executor.LoadSingleOpV2("aicore_op", model_data, stream_, &single_op, model_id++), SUCCESS);

    std::vector<DataBuffer> inputs;
    std::vector<DataBuffer> outputs;
    std::vector<GeTensorDesc> input_desc;
    std::vector<GeTensorDesc> output_desc;
    CreateInputsAndOutputs(op_desc, input_desc, inputs, output_desc, outputs);
    return GeExecutor::ExecuteAsync(single_op, inputs, outputs);
  }

  Status LoadDynamicSingleOp(OpDescPtr &op_desc, DynamicSingleOp **single_op) {
    ModelBufferData model_buffer;
    EXPECT_EQ(BuildSingleOp(op_desc, model_buffer), SUCCESS);

    ModelData model_data;
    model_data.model_data = model_buffer.data.get();
    model_data.model_len = model_buffer.length;
    return GeExecutor::LoadDynamicSingleOpV2("dyn_op", model_data, stream_, single_op, model_id++);
  }

  void FillDataForHostMemInput(const GeTensorDescPtr &tensor_desc, DataBuffer &data_buffer) {
    int64_t mem_type = 0;
    AttrUtils::GetInt(tensor_desc, ge::ATTR_NAME_PLACEMENT, mem_type);
    if (mem_type == kMemtypeHostCompileIndependent) {
      data_buffer.placement = kHostMemType;
      uint64_t *data_ptr = PtrToPtr<void, uint64_t>(data_buffer.data);
      for (size_t i = 0; i < data_buffer.length / sizeof(uint64_t); i++) {
        data_ptr[i] = kHostMemInputValue;
      }
    }
  }

  void CreateInputsAndOutputs(OpDescPtr &op_desc,
                              std::vector<GeTensorDesc> &input_desc,
                              std::vector<DataBuffer> &inputs,
                              std::vector<GeTensorDesc> &output_desc,
                              std::vector<DataBuffer> &outputs) {
    for (const auto &tensor_desc : op_desc->GetAllInputsDescPtr()) {
      bool is_const = false;
      AttrUtils::GetBool(tensor_desc, CONST_ATTR_NAME_INPUT, is_const);
      if (is_const) {
        continue;
      }
      input_desc.emplace_back(*tensor_desc);
      int64_t tensor_size = -1;
      TensorUtils::GetTensorSizeInBytes(*tensor_desc, tensor_size);
      EXPECT_GE(tensor_size, 0);
      input_buffers_.emplace_back(MakeUnique<uint8_t[]>(tensor_size));
      DataBuffer data_buffer;
      data_buffer.data = input_buffers_.back().get();
      data_buffer.length = tensor_size;
      FillDataForHostMemInput(tensor_desc, data_buffer);
      inputs.emplace_back(data_buffer);
    }
    for (const auto &tensor_desc : op_desc->GetAllOutputsDescPtr()) {
      output_desc.emplace_back(*tensor_desc);
      int64_t tensor_size = -1;
      TensorUtils::GetTensorSizeInBytes(*tensor_desc, tensor_size);
      EXPECT_GE(tensor_size, 0);
      output_buffers_.emplace_back(MakeUnique<uint8_t[]>(tensor_size));
      DataBuffer data_buffer;
      data_buffer.data = output_buffers_.back().get();
      data_buffer.length = tensor_size;
      outputs.emplace_back(data_buffer);
    }
  }

  Status RunDynamicTestCast(OpDescPtr &op_desc) {
    DynamicSingleOp *single_op = nullptr;
    EXPECT_EQ(LoadDynamicSingleOp(op_desc, &single_op), SUCCESS);

    std::vector<DataBuffer> inputs;
    std::vector<DataBuffer> outputs;
    std::vector<GeTensorDesc> input_desc;
    std::vector<GeTensorDesc> output_desc;
    CreateInputsAndOutputs(op_desc, input_desc, inputs, output_desc, outputs);
    return GeExecutor::ExecuteAsync(single_op, input_desc, inputs, output_desc, outputs);
  }

  OpDescPtr CreateOp(const std::string &op_type) {
    GeShape shape({2, 8});
    GeTensorDesc tensor_desc(shape);
    auto op_desc = std::make_shared<OpDesc>(op_type, op_type);
    op_desc->AddInputDesc("x", tensor_desc);
    op_desc->AddOutputDesc("y", tensor_desc);
    return op_desc;
  }

  OpDescPtr CreateOpWithHostMemInput(const std::string &op_type) {
    auto op_desc = std::make_shared<OpDesc>(op_type, op_type);
    // input 0
    GeShape shape0({4});
    GeTensorDesc tensor_desc0(shape0, FORMAT_ND, DT_UINT64);
    AttrUtils::SetInt(tensor_desc0, ge::ATTR_NAME_PLACEMENT, kMemtypeHostCompileIndependent);
    op_desc->AddInputDesc("x0", tensor_desc0);

    GeShape shape1({1});
    GeTensorDesc tensor_desc1(shape1, FORMAT_ND, DT_UINT64);
    AttrUtils::SetInt(tensor_desc1, ge::ATTR_NAME_PLACEMENT, kMemtypeHostCompileIndependent);
    op_desc->AddInputDesc("x1", tensor_desc1);

    GeShape shape2({2, 8});
    GeTensorDesc tensor_desc2(shape2, FORMAT_ND, DT_UINT64);
    op_desc->AddOutputDesc("y", tensor_desc2);
    return op_desc;
  }

  void MockForGenerateTask(const std::string &name, const GenerateTaskFun &func) {
    auto builder = std::make_shared<MockOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[name] = builder;
    EXPECT_CALL(*builder, GenerateTask).WillRepeatedly(testing::Invoke(func));
  }
};

TEST_F(SingleOpTest, TestStaticAicpu) {
  MockForGenerateTask("AicpuLib", GenerateTaskForAicpu);
  auto op_desc = CreateOp(NEG);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestDynamicAicpu) {
  MockForGenerateTask("AicpuLib", GenerateTaskForAicpu);
  auto op_desc = CreateOp(NEG);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestStaticTfKernel) {
  MockForGenerateTask("AicpuLib", GenerateTaskForTfKernel);
  auto op_desc = CreateOp(NEG);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

//  static single op, aicore host mem input
TEST_F(SingleOpTest, TestStaticAicoreHostMem) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForAiCore);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(RELU);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

//  static single op, aicpu host mem input
TEST_F(SingleOpTest, TestStaticAicpuKernelHostMem) {
  MockForGenerateTask("AicpuLib", GenerateTaskForAicpu);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(NEG);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

//  static single op, aicpu host mem input
TEST_F(SingleOpTest, TestStaticTfAicpuKernelHostMem) {
  MockForGenerateTask("AicpuLib", GenerateTaskForTfKernel);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(NEG);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

// dynamic single op, aicore host mem input
TEST_F(SingleOpTest, TestDynamicAicoreWithHostMem) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForDynamicAiCore);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(RELU);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

// dynamic single op, tf aicpu host mem input
TEST_F(SingleOpTest, TestDynamicTfAicpuWithHostMem) {
  MockForGenerateTask("AicpuLib", GenerateTaskForTfDependCompute);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(NEG);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

// dynamic single op, aicpu host mem input
TEST_F(SingleOpTest, TestDynamicAicpuWithHostMem) {
  MockForGenerateTask("AicpuLib", GenerateTaskForAiCpuDependCompute);
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  auto op_desc = CreateOpWithHostMemInput(NEG);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, test_delete_resource) {
  int32_t fake_stream = 0;
  uint64_t op_id = 999;
  auto stream = (rtStream_t)fake_stream;
  auto &instance = SingleOpManager::GetInstance();
  uintptr_t resource_id = 0U;
  ASSERT_EQ(instance.GetResourceId(stream, resource_id), SUCCESS);
  auto res = instance.GetResource(resource_id, stream);
  ASSERT_NE(res, nullptr);
  auto new_op = MakeUnique<SingleOp>(res, &res->stream_mu_, res->stream_);
  res->op_map_[op_id] = std::move(new_op);
  auto new_dynamic_op = MakeUnique<DynamicSingleOp>(&res->tensor_pool_, res->resource_id_, &res->stream_mu_, res->stream_);
  res->dynamic_op_map_[op_id] = std::move(new_dynamic_op);
  ASSERT_EQ(ge::GeExecutor::UnloadSingleOp(op_id), SUCCESS);
  ASSERT_EQ(ge::GeExecutor::UnloadDynamicSingleOp(op_id), SUCCESS);
  ASSERT_EQ(instance.ReleaseResource(stream), SUCCESS);
}

TEST_F(SingleOpTest, TestStaticAiCore) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForAiCore);
  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestDynamicAiCore) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForDynamicAiCore);
  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestDynamicAiCoreV2) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForDynamicAiCoreV2);
  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestDynamicAiCoreWithAtomicAddrClean) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForDynamicAiCoreWithAtomicAddrClean);
  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestMemcpyAsync) {
  MockForGenerateTask("RTSLib", GenerateTaskForMemcpyAsync);
  auto op_desc = CreateOp(MEMCPYASYNC);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestStaticAiCoreWithDump) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForAiCore);

  GeExecutor ge_executor;
  ge::DumpConfig dump_cfg;
  dump_cfg.dump_path = "./dump/";
  dump_cfg.dump_mode = "all";
  dump_cfg.dump_status = "on";
  dump_cfg.dump_op_switch = "on";
  EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);

  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);

  dump_cfg.dump_status = "off";
  dump_cfg.dump_op_switch = "off";
  EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
}

TEST_F(SingleOpTest, TestDynamicWithDump) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForDynamicAiCore);

  GeExecutor ge_executor;
  ge::DumpConfig dump_cfg;
  dump_cfg.dump_path = "./dump/";
  dump_cfg.dump_mode = "all";
  dump_cfg.dump_status = "on";
  dump_cfg.dump_op_switch = "on";
  EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);

  auto op_desc = CreateOp(RELU);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);

  dump_cfg.dump_status = "off";
  dump_cfg.dump_op_switch = "off";
  EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
}

TEST_F(SingleOpTest, TestMultipleSingleOp) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForAiCore);

  {
    auto op_desc = CreateOp(RELU);
    EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
  }

  {
    auto op_desc = CreateOp(RELU);
    EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
  }
}

TEST_F(SingleOpTest, TestSingleOpWithConstant) {
  MockForGenerateTask("AiCoreLib", GenerateTaskForAiCore);

  auto op_desc = CreateOp(RELU);
  auto tensor = MakeShared<GeTensor>(*op_desc->MutableInputDesc(0));
  AttrUtils::SetBool(op_desc->MutableInputDesc(0), CONST_ATTR_NAME_INPUT, true);
  AttrUtils::SetTensor(op_desc->MutableInputDesc(0), ATTR_NAME_WEIGHTS, tensor);
  EXPECT_EQ(RunStaticTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestTfDependCompute) {
  MockForGenerateTask("AicpuLib", GenerateTaskForTfDependCompute);

  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchEx).WillRepeatedly(testing::Invoke(MockRtKernelLaunchEx));
  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(MockRtMemcpy));

  auto op_desc = CreateOp(NEG);
  AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, static_cast<int64_t>(DEPEND_COMPUTE));

  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestTfDependRange) {
  auto fun = [](const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) -> Status {
    if (node.GetType() != NEG) {
      return SUCCESS;
    }

    tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(3));
    return SUCCESS;
  };
  MockForGenerateTask("AicpuLib", fun);

  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchEx).WillRepeatedly(testing::Invoke(MockRtKernelLaunchEx));
  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(MockRtMemcpy));

  auto op_desc = CreateOp(NEG);
  AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, static_cast<int64_t>(DEPEND_SHAPE_RANGE));

  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestAiCpuDependCompute) {
  MockForGenerateTask("AicpuLib", GenerateTaskForAiCpuDependCompute);

  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchEx).WillRepeatedly(testing::Invoke(MockRtKernelLaunchEx));
  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(MockRtMemcpy));

  auto op_desc = CreateOp(NEG);
  AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, static_cast<int64_t>(DEPEND_COMPUTE));

  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestAiCpuDependRange) {
  auto fun = [](const Node &node, RunContext &run_context, std::vector<domi::TaskDef> &tasks) -> Status {
    if (node.GetType() != NEG) {
      return SUCCESS;
    }

    tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(3));
    return SUCCESS;
  };
  MockForGenerateTask("AicpuLib", fun);

  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchEx).WillRepeatedly(testing::Invoke(MockRtKernelLaunchEx));
  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(MockRtMemcpy));

  auto op_desc = CreateOp(NEG);
  AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, static_cast<int64_t>(DEPEND_SHAPE_RANGE));
  AttrUtils::SetBool(op_desc, ATTR_NAME_IS_BLOCKING_OP, true);
  EXPECT_EQ(RunDynamicTestCast(op_desc), SUCCESS);
}

TEST_F(SingleOpTest, TestCompileStaticSingleOp) {
  EXPECT_EQ(CompileSingleOpByAtc("st_run_data/json/single_op/add_op.json"), SUCCESS);
  GEFinalize();
  ReInitGe();
  EXPECT_EQ(CompileSingleOpByAtc("st_run_data/json/single_op/matmul_op.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestCompileDynamicSingleOp) {
  EXPECT_EQ(CompileSingleOpByAtc("st_run_data/json/single_op/dynamic_ops.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidOpType) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_op.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidAttrType) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_attr_type.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidAttrName) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_attr_name.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidAttrValue) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_attr_value.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidInputDesc) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_input_desc.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidInputDescDtypeAndFormat) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_input_desc_dtype_and_format.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidOutputDesc) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_output_desc.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidOutputDescDtype) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_output_desc_dtype.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidOutputDescFormat) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_output_desc_format.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidJsonPath) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/_.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidJsonContent) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/broken.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidInputNum) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_input_num_mismatch.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidOutputNum) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_output_num_mismatch.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidUnknownRankShape) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_unknown_rank_shape.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidUnknownRankWithRange) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_unknown_rank_with_range.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidUnknownShapeRange) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_unknown_shape_range_mismatch.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestInvalidUnknownShapeNumRange) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/exception/invalid_unknown_shape_num_range_mismatch.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestOpManyAttrs) {
  EXPECT_EQ(CompileSingleOpByAtc("st_run_data/json/single_op/op_many_attrs.json"), SUCCESS);
}

TEST_F(SingleOpTest, TestOpUnregstAttrs) {
  EXPECT_NE(CompileSingleOpByAtc("st_run_data/json/single_op/op_unregst_oppath_attrs.json"), SUCCESS);
}
}  // namespace ge
