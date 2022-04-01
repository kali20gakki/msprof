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

// To test the execution of dynamic data flow ops (Stack series)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#define protected public
#include "ge_running_env/fake_ops_kernel_builder.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#undef private
#undef protected

#include <dlfcn.h>
#include "register/op_tiling_registry.h"
#include "hccl/base.h"
#include "hccl/hccl_types.h"
#include "framework/executor/ge_executor.h"
#include "graph/utils/graph_utils.h"
#include "ge_running_env/fake_op.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "common/plugin/ge_util.h"
#include "external/ge/ge_api.h"
#include "session/session_manager.h"
#include "init_ge.h"
#include "utils/bench_env.h"
#include "utils/taskdef_builder.h"
#include "depends/runtime/src/runtime_stub.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "graph/manager/graph_mem_manager.h"

using namespace std;
using namespace testing;

namespace ge {
namespace {
void *mock_handle = nullptr;
void *mock_method = nullptr;

HcclResult HcomExecEnqueueOperation(HcomOperation opInfo, std::function<void(HcclResult status)> callback) {
  callback(HCCL_SUCCESS);
  return HCCL_SUCCESS;
}

HcclResult HcomExecEnqueueRemoteAccess(const std::string& remoteAccessType,
                                       const std::vector<HcomRemoteAccessAddrInfo>& addrInfos,
                                       std::function<void(HcclResult status)> callback) {
  callback(HCCL_SUCCESS);
  return HCCL_SUCCESS;
}

HcclResult HcomExecEnqueueAllToAllV(HcomAllToAllVParams params, std::function<void(HcclResult status)> callback) {
  callback(HCCL_SUCCESS);
  return HCCL_SUCCESS;
}

HcclResult HcomExecEnqueueGatherAllToAllV(HcomGatherAllToAllVParams params,
                                          std::function<void(HcclResult status)> callback) {
  callback(HCCL_SUCCESS);
  return HCCL_SUCCESS;
}

HcclResult HcomExecInitialize() {
  return HCCL_SUCCESS;
}

HcclResult HcomExecFinalize() {
  return HCCL_SUCCESS;
}

class MockMmpa : public MmpaStubApi {
 public:
  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "HcomExecEnqueueOperation") {
      return (void *) &HcomExecEnqueueOperation;
    } else if (std::string(func_name) == "HcomExecEnqueueRemoteAccess") {
      return (void *) &HcomExecEnqueueRemoteAccess;
    } else if (std::string(func_name) == "HcomExecEnqueueAllToAllV") {
      return (void *) &HcomExecEnqueueAllToAllV;
    } else if (std::string(func_name) == "HcomExecEnqueueGatherAllToAllV") {
      return (void *) &HcomExecEnqueueGatherAllToAllV;
    } else if (std::string(func_name) == "HcomExecInitialize") {
      return (void *) &HcomExecInitialize;
    } else if (std::string(func_name) == "HcomExecFinalize") {
      return (void *) &HcomExecFinalize;
    }
    return dlsym(handle, func_name);
  }
  int32_t RealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen) override {
    (void)realpath(path, realPath);
    return EN_OK;
  }
  void *DlOpen(const char *fileName, int32_t mode) override {
    return (void *)mock_handle;
  }
  int32_t DlClose(void *handle) override {
    return 0L;
  }
};

class MockOpsKernelBuilder : public FakeOpsKernelBuilder {
 public:
  MOCK_METHOD3(GenerateTask, Status(const Node &, RunContext &, vector<domi::TaskDef> &));
};

using GenerateTaskFun = std::function<Status(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks)>;

void MockForGenerateTask(const std::string &name, const GenerateTaskFun &func) {
  auto builder = std::make_shared<MockOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[name] = builder;
  EXPECT_CALL(*builder, GenerateTask).WillRepeatedly(testing::Invoke(func));
}

Status GenerateTaskForAiCore(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildAtomicAddrCleanTask());
  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTask(true));
  return SUCCESS;
}

Status GenerateTaskForTaskWithHandle(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTaskWithHandle());
  return SUCCESS;
}

Status GenerateTaskForStaticAicore(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AiCoreTaskDefBuilder(node).BuildTask());
  return SUCCESS;
}

Status GenerateTaskForAicpuDependRange(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(3));
  return SUCCESS;
}

}
class DynamicHcclTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    GEFinalize();
    ReInitGe();
    BenchEnv::Init();
  }
  void SetUp() {
    auto infer_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      *op_desc->MutableOutputDesc(0) = *op_desc->GetInputDescPtr(0);
      return GRAPH_SUCCESS;
    };
    auto infer_depend1_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      *op_desc->MutableOutputDesc(0) = *op_desc->GetInputDescPtr(0);
      op_desc->SetOpInferDepends({"remote", "local", "local_offset"});
      return GRAPH_SUCCESS;
    };
    auto infer_depend2_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      *op_desc->MutableOutputDesc(0) = *op_desc->GetInputDescPtr(0);
      op_desc->SetOpInferDepends({"remote"});
      return GRAPH_SUCCESS;
    };

    auto unique_infer_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      auto output_desc = op_desc->MutableOutputDesc(0);
      output_desc->SetShape(GeShape({-1}));
      output_desc->SetShapeRange({{1, 16}});
      return GRAPH_SUCCESS;
    };

    GeRunningEnvFaker()
        .Reset()
        .Install(FakeEngine("DNN_VM_GE_LOCAL").KernelInfoStore("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
        .Install(FakeEngine("DNN_HCCL").KernelInfoStore("DNN_HCCL"))
        .Install(FakeEngine("DNN_HCCL").KernelInfoStore(kEngineNameHccl))
        .Install(FakeEngine("DNN_VM_RTS").KernelInfoStore("DNN_VM_RTS"))
        .Install(FakeEngine("DNN_VM_RTS").KernelInfoStore("DNN_VM_RTS_OP_STORE"))
        .Install(FakeEngine(kEngineNameAiCpu).KernelInfoStore(kEngineNameAiCpu))
        .Install(FakeOp(STREAMACTIVE).InfoStoreAndBuilder("DNN_VM_RTS").InferShape(infer_fun))
        .Install(FakeOp(SEND).InfoStoreAndBuilder("DNN_VM_RTS").InferShape(infer_fun))
        .Install(FakeOp(RECV).InfoStoreAndBuilder("DNN_VM_RTS").InferShape(infer_fun))
        .Install(FakeOp(IDENTITY).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE").InferShape(infer_fun))
        .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(NEG).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(HCOMALLREDUCE).InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_fun))
        .Install(FakeOp(HCOMREMOTEWRITE).Inputs({"remote", "local", "local_offset"})
                                                .InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_depend1_fun))
        .Install(FakeOp(HCOMREMOTEREFREAD).Inputs({"remote"})
                                                  .InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_depend2_fun))
        .Install(FakeOp(HCOMREMOTEREAD).Inputs({"remote"})
                     .InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_depend2_fun))
        .Install(FakeOp(HCOMALLTOALLV).InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_fun))
        .Install(FakeOp(HCOMGATHERALLTOALLV).InfoStoreAndBuilder(kEngineNameHccl).InferShape(infer_fun))
        .Install(FakeOp(DATA).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE").InferShape(infer_fun))
        .Install(FakeOp(VARIABLE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(CONSTANTOP).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE").InferShape(infer_fun))
        .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(IF).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(PARTITIONEDCALL).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp("Unique").InfoStoreAndBuilder("aicpu_ascend_kernel").InferShape(unique_infer_fun))
        .Install(FakeOp("UniqueV2").InfoStoreAndBuilder("AIcoreEngine").InferShape(unique_infer_fun))
        .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"));
  }
  void TearDown() {
    MmpaStub::GetInstance().Reset();
    GEFinalize();
    ReInitGe();
  }
};

/*******************************************************************************

******************************************************************************/
static void BuildHcclAllReduceGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  DEF_GRAPH(g0) {
    auto data0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto add = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto hcom_allreduce = OP_CFG(HCOMALLREDUCE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {});

    CHAIN(NODE("_arg_0", data0)->NODE("add0", add)->NODE("allreduce", hcom_allreduce)
              ->NODE("Node_Output", net_output));
//    CHAIN(NODE("const_0", const_0)->NODE("Node_Output", net_output));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_offline_graph");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
  ge_graph = ToGeGraph(g0);
}

/*******************************************************************************

******************************************************************************/
static void BuildHcclRefReadGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  uint64_t data_raw[3] = {10, 11, 12};
  std::vector<uint8_t> data(sizeof(data_raw));
  std::vector<int64_t> shape{3};
  (void)memcpy_s(data.data(), sizeof(data_raw), data_raw, sizeof(data_raw));
  tensor.MutableTensorDesc().SetShape(GeShape(shape));
  tensor.SetData(data);
  DEF_GRAPH(g0) {
    auto const0 = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    auto hcom_remote_refread = OP_CFG(HCOMREMOTEREFREAD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    CHAIN(NODE("_arg_0", const0)->NODE("refread", hcom_remote_refread)->NODE("Node_Output", net_output));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_offline_graph");
//  unsetenv("DUMP_GE_GRAPH");
//  unsetenv("DUMP_GRAPH_LEVEL");
  ge_graph = ToGeGraph(g0);
}

static void BuildHcclReadGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  uint64_t data_raw[] = {10, 11, 25, 10, 11, 25, 10, 11, 25};
  std::vector<uint8_t> data(sizeof(data_raw));
  std::vector<int64_t> shape{3, 3};
  (void)memcpy_s(data.data(), sizeof(data_raw), data_raw, sizeof(data_raw));
  tensor.MutableTensorDesc().SetShape(GeShape(shape));
  tensor.SetData(data);
  DEF_GRAPH(g0) {
    auto const0 = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3, 3});

    auto hcom_remote_read = OP_CFG(HCOMREMOTEREAD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3, 3})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3, 3});

    CHAIN(NODE("_arg_0", const0)->NODE("refread", hcom_remote_read)->NODE("Node_Output", net_output));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_offline_graph");
//  unsetenv("DUMP_GE_GRAPH");
//  unsetenv("DUMP_GRAPH_LEVEL");
  ge_graph = ToGeGraph(g0);
}

static void BuildHcclGatherAlltoAllGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  DEF_GRAPH(g0) {
    auto data0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto add1 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add2 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add3 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add4 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add5 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto hcom_all_to_allv = OP_CFG(HCOMGATHERALLTOALLV)
        .InCnt(5)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {});

    CHAIN(NODE("_arg_0", data0)->NODE("add1", add1)->NODE("all_to_all", hcom_all_to_allv)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_0", data0)->NODE("add2", add2)->NODE("all_to_all", hcom_all_to_allv)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_0", data0)->NODE("add3", add3)->NODE("all_to_all", hcom_all_to_allv));
    CHAIN(NODE("_arg_0", data0)->NODE("add4", add4)->NODE("all_to_all", hcom_all_to_allv));
    CHAIN(NODE("_arg_0", data0)->NODE("add5", add5)->NODE("all_to_all", hcom_all_to_allv));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_offline_graph");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
  ge_graph = ToGeGraph(g0);
}

/*******************************************************************************

******************************************************************************/
static void BuildHcclAlltoAllGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  DEF_GRAPH(g0) {
    auto data0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto add1 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add2 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add3 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add4 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto add5 = OP_CFG(ADD)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto hcom_all_to_allv = OP_CFG(HCOMALLTOALLV)
        .InCnt(5)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {});

    CHAIN(NODE("_arg_0", data0)->NODE("add1", add1)->NODE("all_to_all", hcom_all_to_allv)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_0", data0)->NODE("add2", add2)->NODE("all_to_all", hcom_all_to_allv));
    CHAIN(NODE("_arg_0", data0)->NODE("add3", add3)->NODE("all_to_all", hcom_all_to_allv));
    CHAIN(NODE("_arg_0", data0)->NODE("add4", add4)->NODE("all_to_all", hcom_all_to_allv));
    CHAIN(NODE("_arg_0", data0)->NODE("add5", add5)->NODE("all_to_all", hcom_all_to_allv));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  setenv("DUMP_GE_GRAPH", "1", 1);
  setenv("DUMP_GRAPH_LEVEL", "1", 1);
  GE_DUMP(graph, "sample_offline_graph");
  unsetenv("DUMP_GE_GRAPH");
  unsetenv("DUMP_GRAPH_LEVEL");
  ge_graph = ToGeGraph(g0);
}

/*******************************************************************************

┌────────┐  (0,0)   ┌──────┐  (0,0)   ┌──────────────┐  (0,0)   ┌─────────┐  (0,0)   ┌───────────┐  (0,0)   ┌───────────┐  (0,0)   ┌─────────────┐
│ _arg_0 │ ───────> │ add0 │ ───────> │ remote_write │ ───────> │ refread │ ───────> │ alltoallv │ ───────> │ allreduce │ ───────> │ Node_Output │
└────────┘          └──────┘          └──────────────┘          └─────────┘          └───────────┘          └───────────┘          └─────────────┘

******************************************************************************/
static void BuildHcclRemoteWriteGraph(Graph &ge_graph, uint32_t &mem_offset) {
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);

  uint64_t data_raw[3] = {10, 11, 12};
  std::vector<uint8_t> data(sizeof(data_raw));
  std::vector<int64_t> shape{3};
  (void)memcpy_s(data.data(), sizeof(data_raw), data_raw, sizeof(data_raw));
  tensor.MutableTensorDesc().SetShape(GeShape(shape));
  tensor.SetData(data);
  DEF_GRAPH(g0) {
    auto const0 = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    auto const1 = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    auto const2 = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    auto hcom_remote_write = OP_CFG(HCOMREMOTEWRITE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3})
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .Attr(HCOM_ATTR_REDUCE_TYPE, "min");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_UINT64, {3});

    CHAIN(NODE("_arg_0", const0)->NODE("remote_write", hcom_remote_write)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_1", const1)->NODE("remote_write", hcom_remote_write)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_2", const2)->NODE("remote_write", hcom_remote_write)->NODE("Node_Output", net_output));
  };
  ComputeGraphPtr graph = ToComputeGraph(g0);
  ge_graph = ToGeGraph(g0);
}


TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingRemoteWrite) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclRemoteWriteGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({3});
  uint8_t buffer[3 * 8];
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}

TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingRefRead) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclRefReadGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
//  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({3});
  uint8_t buffer[3 * 8] = {55,55,55,55};
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}


TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingRead) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(1024);
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclReadGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({3, 3});
  uint8_t buffer[9 * 8] = {55,55,55,55};
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}

TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingAlltoAll) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclAlltoAllGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({16});
  uint8_t buffer[16 * 4];
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  Tensor input_1(tensor_desc);
  Tensor input_2(tensor_desc);
  Tensor input_3(tensor_desc);
  Tensor input_4(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));
  input_1.SetData(buffer, sizeof(buffer));
  input_2.SetData(buffer, sizeof(buffer));
  input_3.SetData(buffer, sizeof(buffer));
  input_4.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0, input_1, input_2, input_3, input_4};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}

TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingGatherAlltoAll) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclGatherAlltoAllGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({16});
  uint8_t buffer[16 * 4];
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  Tensor input_1(tensor_desc);
  Tensor input_2(tensor_desc);
  Tensor input_3(tensor_desc);
  Tensor input_4(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));
  input_1.SetData(buffer, sizeof(buffer));
  input_2.SetData(buffer, sizeof(buffer));
  input_3.SetData(buffer, sizeof(buffer));
  input_4.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0, input_1, input_2, input_3, input_4};
  std::vector<Tensor> outputs;
  outputs.resize(2);
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}

TEST_F(DynamicHcclTest, TestDynamicOnlineTrainingAllReduce) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xffffffff;
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  uint32_t mem_offset = 0;
  Graph graph;
  BuildHcclAllReduceGraph(graph, mem_offset);

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape({16});
  uint8_t buffer[16 * 4];
  TensorDesc tensor_desc(shape);
  Tensor input_0(tensor_desc);
  Tensor input_1(tensor_desc);
  Tensor input_2(tensor_desc);
  input_0.SetData(buffer, sizeof(buffer));
  input_1.SetData(buffer, sizeof(buffer));
  input_2.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{input_0, input_1, input_2};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}

} // namespace ge

