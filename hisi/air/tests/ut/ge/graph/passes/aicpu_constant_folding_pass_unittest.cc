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

#include <string>
#include <vector>
#include <stdlib.h>
#include <gtest/gtest.h>

#define private public
#define protected public
#include "opskernel_manager/ops_kernel_manager.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "graph/passes/aicpu_constant_folding_pass.h"
#undef protected
#undef private

#include "common/types.h"
#include "common/plugin/ge_util.h"
#include "graph/passes/base_pass.h"
#include "graph_builder_utils.h"
#include "inc/kernel.h"
#include "inc/kernel_factory.h"
#include "graph/utils/constant_utils.h"
#include "init/gelib.h"

using namespace ge;

namespace ge {
// dirty ut, optimize later
namespace {
void SetWeightForConstNode(NodePtr &const_node) {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const_node->GetOpDesc(), 0, tensor);
}

/*
 *
 *      netoutput1
 *         |
 *       shapeNo1
 *        |
 *      addnYes1
 *     /    \
 *   /       \
 * const1   const2
 */
ComputeGraphPtr BuildGraph() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto addn1 = builder.AddNode("addn1", "AddNYes", 2, 1);
  auto shape1 = builder.AddNode("shape1", "ShapeNo", 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  int32_t value = 2;
  GeTensorPtr weight = MakeShared<GeTensor>();
  weight->SetData((uint8_t *)&value, sizeof(int32_t));
  OpDescUtils::SetWeights(const1, {weight});
  OpDescUtils::SetWeights(const2, {weight});

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |
///     addnYes1
///    /    \.
///  /       \.
/// const1   const2
ComputeGraphPtr BuildGraph1() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);
  auto addn1 = builder.AddNode("addn1", ADD, 2, 1);
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}
///   data1  data2
///     \   /
///      add
///       |
///   netoutput
///
ComputeGraphPtr BuildWrongGraph1() {
  auto builder = ut::GraphBuilder("g5");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto add = builder.AddNode("add", ADD, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

/// shape1(pc)  shape2(potential_const)
///         \   /
///          add
///           |
///        netoutput
///
ComputeGraphPtr BuildPotentialConstGraph() {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  auto builder = ut::GraphBuilder("g5");
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape1->GetOpDesc(), {0}, {tensor});
  auto shape2 = builder.AddNode("shape2", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape2->GetOpDesc(), {0}, {tensor});
  auto add = builder.AddNode("add", ADD, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(shape1, 0, add, 0);
  builder.AddDataEdge(shape2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

/// shape1     shape2(potential_const)
///         \   /
///          add(pc)
///           |
///        netoutput
///
ComputeGraphPtr BuildPartitialPotentialConstGraph() {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  auto builder = ut::GraphBuilder("g5");
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto shape2 = builder.AddNode("shape2", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape2->GetOpDesc(), {0}, {tensor});
  auto add = builder.AddNode("add", ADD, 2, 1);
  ConstantUtils::MarkPotentialConst(add->GetOpDesc(), {0}, {tensor});
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(shape1, 0, add, 0);
  builder.AddDataEdge(shape2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

class AICPUOpsKernelInfoStore : public ge::OpsKernelInfoStore {
 public:
  AICPUOpsKernelInfoStore(){};
  ~AICPUOpsKernelInfoStore(){};
  Status Initialize(const map<string, string> &options) { return 0; }
  Status Finalize() { return 0; }
  Status CalcOpRunningParam(Node &node) { return 0; }
  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) { return 0; }
  bool CheckSupported(const ge::OpDescPtr &opDescPtr, std::string &unSupportReason) const { return true; }
  void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const {};
  // opsFlag opsFlag[0] indicates constant folding is supported or not
  void opsFlagCheck(const ge::Node &node, std::string &opsFlag) {
    if (node.GetType() == CONSTANT) {
      return;
    }
    opsFlag = "1"; // support
  };
};

class AICPUOpsKernelBuilder : public ge::OpsKernelBuilder {
 public:
    // initialize OpsKernelBuilder
  Status Initialize(const std::map<std::string, std::string> &options) {return SUCCESS;};

  // finalize OpsKernelBuilder
  Status Finalize() {return SUCCESS;};

  // memory allocation requirement
  Status CalcOpRunningParam(Node &node) {return SUCCESS;};

  // generate task for op
  Status GenerateTask(const Node &node, RunContext &context,
                              std::vector<domi::TaskDef> &tasks) {return SUCCESS;};

  // generate task for op with different mode
  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks,
                              OpsKernelBuilder::Mode) {
                                return SUCCESS;
                              }

  // only call aicpu interface to generate task struct
  Status GenSingleOpRunTask(const NodePtr &node, STR_FWK_OP_KERNEL &task, std::string &task_info) {
    task_info = "123";
    return SUCCESS;
  }

  // only call aicpu interface to generate task struct
  Status GenMemCopyTask(const uint64_t count, STR_FWK_OP_KERNEL &task, std::string &task_info) {
     task_info = "123";
     return FAILED; // tmp solution
  }
};

class AICPUOpsKernelBuilder2 : public ge::OpsKernelBuilder {
 public:
    // initialize OpsKernelBuilder
  Status Initialize(const std::map<std::string, std::string> &options) {return SUCCESS;};

  // finalize OpsKernelBuilder
  Status Finalize() {return SUCCESS;};

  // memory allocation requirement
  Status CalcOpRunningParam(Node &node) {return SUCCESS;};

  // generate task for op
  Status GenerateTask(const Node &node, RunContext &context,
                              std::vector<domi::TaskDef> &tasks) {return SUCCESS;};

  // generate task for op with different mode
  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks,
                              OpsKernelBuilder::Mode) {
                                return SUCCESS;
                              }

  // only call aicpu interface to generate task struct
  Status GenSingleOpRunTask(const NodePtr &node, STR_FWK_OP_KERNEL &task, std::string &task_info) {
    task_info = "123";
    return SUCCESS;
  }

  // only call aicpu interface to generate task struct
  Status GenMemCopyTask(const uint64_t count, STR_FWK_OP_KERNEL &task, std::string &task_info) {
     task_info = "123";
     return SUCCESS;
  }
};
} // namespace

class UtestAicpuConstantFoldingPass : public testing::Test {
 public:
  static void SetUpTestCase() {
    auto instance = GELib::GetInstance();
    map<string, string> options;
    instance->Initialize(options);
    OpsKernelManager &ops_kernel_manager = instance->OpsKernelManagerObj();
    // init opsKernelStore
    auto aicpu_kernel_store = MakeShared<AICPUOpsKernelInfoStore>();
    ops_kernel_manager.ops_kernel_store_.emplace(std::pair<string, OpsKernelInfoStorePtr>("aicpu_tf_kernel", aicpu_kernel_store));

    // init opskernel builder
    auto builder = std::make_shared<AICPUOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_["aicpu_tf_kernel"] = builder;
  }
  static void TearDownTestCase() {
    auto instance = GELib::GetInstance();
    instance->Finalize();
  }

 protected:
  UtestAicpuConstantFoldingPass() = default;
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestAicpuConstantFoldingPass, TestNeedIgnorePass) {
  auto graph = BuildWrongGraph1();
  auto add_node = graph->FindNode("add");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;

  GEPass pass(graph);
  aicpu_constant_folding_pass.NeedIgnorePass(add_node);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), NOT_CHANGED);
}

TEST_F(UtestAicpuConstantFoldingPass, TestDefaultNeedFold) {
  auto graph = BuildGraph1();
  auto addn_node = graph->FindNode("addn1");
  EXPECT_NE(addn_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;

  GEPass pass(graph);
  EXPECT_EQ(aicpu_constant_folding_pass.NeedFold(), true);
}

TEST_F(UtestAicpuConstantFoldingPass, TestNoNeedFoldPotentialConst) {
  auto graph = BuildPartitialPotentialConstGraph();
  auto add_node = graph->FindNode("add");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), NOT_CHANGED);
  EXPECT_EQ(aicpu_constant_folding_pass.NeedFold(), false);
}


TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);
}


TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtMemcpyFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);
  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestInit) {
  auto graph = BuildPotentialConstGraph();
  auto add_node = graph->FindNode("add");
  EXPECT_NE(add_node, nullptr);
  setenv("AICPU_CONSTANT_FOLDING_ON", "1", true);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  aicpu_constant_folding_pass.Init(0, graph);
  unsetenv("AICPU_CONSTANT_FOLDING_ON");
}

TEST_F(UtestAicpuConstantFoldingPass, TestFrameworkNode) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("g");
  OpDescPtr op_desc = shared_ptr<OpDesc>(new (std::nothrow) OpDesc("fmk_op", ge::FRAMEWORKOP));
  NodePtr fmk_node = graph->AddNode(op_desc);

  EXPECT_NE(fmk_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;

  bool ret = aicpu_constant_folding_pass.NeedIgnorePass(fmk_node);
  EXPECT_EQ(ret, true);
}

TEST_F(UtestAicpuConstantFoldingPass, LaunchMemCopyTaskFail) {
   // GenMemCopyTask return success
  auto builder = std::make_shared<AICPUOpsKernelBuilder2>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_["aicpu_tf_kernel"] = builder;

  vector<uint64_t> data_infos;
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  Status ret = aicpu_constant_folding_pass.LaunchMemCopyTask(data_infos);
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestAicpuConstantFoldingPass, GenerateDataPtrInfo) {
  aicpu::FWKAdapter::ResultSummary result_summary;
  result_summary.raw_data_size = 10;
  result_summary.shape_data_size = 16;

  vector<AicpuConstantFoldingPass::DataPtrInfo> data_vec;
  vector<uint64_t> data_infos;
  vector<uint64_t> output_addrs;
  output_addrs.emplace_back(reinterpret_cast<uint64_t>(&result_summary));
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  Status ret = aicpu_constant_folding_pass.GenerateDataPtrInfo(output_addrs, data_vec, data_infos);
  for (auto item : data_vec) {
    uint8_t *p = reinterpret_cast<uint8_t *>(item.dst_ptr);
    delete[] p;
  }
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestAicpuConstantFoldingPass, GenerateGeTensor) {
  auto tensor_desc = MakeShared<GeTensorDesc>();
  auto op_desc = MakeShared<OpDesc>();
  for (int i = 0; i < 2; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }

  vector<AicpuConstantFoldingPass::DataPtrInfo> data_vec;
  vector<GeTensorPtr> outputs;
  uint8_t *data_dst_ptr = new uint8_t[10];
  data_dst_ptr[0] = 1;
  data_dst_ptr[1] = 2;

  uint64_t *shape_dst_ptr = new uint64_t[2];
  shape_dst_ptr[0] = 3;
  shape_dst_ptr[1] = 4;

  AicpuConstantFoldingPass::DataPtrInfo raw_data_info;
  raw_data_info.release_flag = 1;
  raw_data_info.data_size = 10;
  raw_data_info.src_ptr = 10;
  raw_data_info.dst_ptr = reinterpret_cast<uint64_t>(data_dst_ptr);
  data_vec.emplace_back(raw_data_info);

  AicpuConstantFoldingPass::DataPtrInfo shape_data_info;
  shape_data_info.release_flag = 1;
  shape_data_info.data_size = 16;
  shape_data_info.src_ptr = 10;
  shape_data_info.dst_ptr = reinterpret_cast<uint64_t>(shape_dst_ptr);
  data_vec.emplace_back(shape_data_info);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  AicpuConstantFoldingPass::DataPtrInfo raw_data_info_copy = raw_data_info;
  AicpuConstantFoldingPass::DataPtrInfo shape_data_info_copy = shape_data_info;
  shape_data_info_copy.data_size = 0;
  data_vec.emplace_back(raw_data_info_copy);
  data_vec.emplace_back(shape_data_info_copy);

  Status ret = aicpu_constant_folding_pass.GenerateGeTensor(op_desc, data_vec, outputs);
  EXPECT_EQ(ret, SUCCESS);
  delete[] data_dst_ptr;
  delete[] shape_dst_ptr;
}

TEST_F(UtestAicpuConstantFoldingPass, GetInputAddrs_Test)
{
  std::vector<ConstGeTensorPtr> weight_vec;
  std::vector<ge::AicpuConstantFoldingPass::AddrAndType> input_addrs;

  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  Status ret = aicpu_constant_folding_pass.GetInputAddrs(weight_vec, input_addrs);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestAicpuConstantFoldingPass, UpdateWorkSpaceAddr_Test)
{
  std::string task_info;
  STR_FWK_OP_KERNEL task;

  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  Status ret = aicpu_constant_folding_pass.UpdateWorkSpaceAddr(task_info, task);
  EXPECT_EQ(ret, FAILED);

  task_info.assign("abc");
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);
  ret = aicpu_constant_folding_pass.UpdateWorkSpaceAddr(task_info, task);
  EXPECT_EQ(ret, FAILED);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, UpdateInputAndOutputAddr_Test)
{
  std::vector<uint64_t> io_addrs{1, 2};
  STR_FWK_OP_KERNEL task;
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);
  Status ret = aicpu_constant_folding_pass.UpdateInputAndOutputAddr(io_addrs, task);
  EXPECT_EQ(ret, FAILED);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, GenerateTaskForLaunch_Test)
{
  void *task_buf = nullptr;
  STR_FWK_OP_KERNEL aicpu_task;
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);
  Status ret = aicpu_constant_folding_pass.GenerateTaskForLaunch(aicpu_task, task_buf);
  EXPECT_EQ(ret, FAILED);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtModelCreateFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_3";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtStreamCreateFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_4";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtModelBindStreamFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_5";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtKernelLaunchExFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_6";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtModelLoadCompleteFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_7";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtModelExecuteFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_8";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

TEST_F(UtestAicpuConstantFoldingPass, TestComputePotentialWeightWithAicpuKernel_rtStreamSynchronizeFailed) {
  auto graph = BuildGraph1();
  auto add_node = graph->FindNode("addn1");
  EXPECT_NE(add_node, nullptr);
  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  const char_t * const kEnvValue = "CONSTANT_FOLDING_PASS_9";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvValue, fail_collect_path.c_str(), 1);

  GEPass pass(graph);
  std::vector<GeTensorPtr> outputs;
  EXPECT_EQ(aicpu_constant_folding_pass.NeedIgnorePass(add_node), false);
  EXPECT_EQ(aicpu_constant_folding_pass.ComputePotentialWeight(add_node, outputs), SUCCESS);

  // 清理环境变量
  mmSetEnv(kEnvValue, "", 1);
}

}  // namespace ge
