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
#include <condition_variable>
#include <mutex>
#include <future>

#define private public
#define protected public
#include "graph/load/model_manager/model_manager.h"
#include "local_engine/engine/host_cpu_engine.h"
#include "ge_running_env/fake_ops_kernel_builder.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "hybrid/common/npu_memory_allocator.h"
#include "graph/bin_cache/node_compile_cache_module.h"
#undef private
#undef protected

#include "external/graph/operator_reg.h"
#include "graph/ge_attr_value.h"
#include "common/dump/dump_manager.h"
#include "register/op_tiling_registry.h"
#include "framework/executor/ge_executor.h"
#include "ge_running_env/fake_op.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "external/ge/ge_api.h"
#include "session/session_manager.h"
#include "graph/utils/tensor_adapter.h"
#include "init_ge.h"
#include "utils/bench_env.h"
#include "utils/taskdef_builder.h"
#include "utils/graph_factory.h"
#include "depends/runtime/src/runtime_stub.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "ge_graph_dsl/assert/check_utils.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "graph/ge_context.h"

namespace ge {
namespace {
void *mock_host_cpu_handle = (void *) 0x12345678;
optiling::OpRunInfoV2 tiling_run_info_;
bool tiling_result_ = true;

auto infer_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      *op_desc->MutableOutputDesc(0) = *op_desc->GetInputDescPtr(0);
      return GRAPH_SUCCESS;
    };

uint32_t RunHostCpuForAssign(void *args) {
  auto *arg_base = reinterpret_cast<uint8_t *>(args);
  auto io_addrs = reinterpret_cast<uintptr_t *>(arg_base + sizeof(aicpu::AicpuParamHead));
  auto *input_1 = reinterpret_cast<int32_t *>(io_addrs[1]);
  auto *output = reinterpret_cast<int32_t *>(io_addrs[2]);
  *output = *input_1;
  return 0;
}

class MockRuntime : public RuntimeStub {
 public:
  MOCK_METHOD6(rtKernelLaunchWithFlag, int32_t(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
      rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flag));
};

class MockMemcpy : public RuntimeStub {
 public:
  MOCK_METHOD5(rtMemcpy, int32_t(void * , uint64_t, const void *, uint64_t, rtMemcpyKind_t));
};

class MockMmpa : public MmpaStubApi {
 public:
  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "RunHostCpuKernel") {
      return (void *) &RunHostCpuForAssign;
    }
    return dlsym(handle, func_name);
  }

  int32_t DlClose(void *handle) override {
    return 0;
  }
};

class MockMalloc : public RuntimeStub {
 public:
  rtError_t rtMalloc(void **dev_ptr, uint64_t size, rtMemType_t type) override {
    total_malloc_size += size;
    if (total_malloc_size > 2000) {
      return -1;
    }

    *dev_ptr = new uint8_t[size];
    memset_s(*dev_ptr, size, 0, size);

    return RT_ERROR_NONE;
  }

  rtError_t rtFree(void *dev_ptr) override {
    total_malloc_size = 0;
    delete[](uint8_t *) dev_ptr;
    return RT_ERROR_NONE;
  }

 private:
  uint64_t total_malloc_size = 0;
};

struct GeneralizedShapeInfo
{
  GeShape shape;
  std::vector<std::pair<int64_t, int64_t>> shape_range;
};

struct FakeFuzzCompilerOpsKernelInfoStore : public FakeOpsKernelInfoStore {
 FakeFuzzCompilerOpsKernelInfoStore(const std::string &kernel_lib_name) : FakeOpsKernelInfoStore(kernel_lib_name) {}
  void ClearCompileOpCount() {
    node_name_2_comile_hits_.clear();
  }
  uint32_t GetNodeFuzzCompileCount(const std::string &node_name) {
    return node_name_2_comile_hits_[node_name];
  }
  Status Initialize(const std::map<std::string, std::string> &options) override {
    return SUCCESS;
  }
  Status Finalize() override {
    return SUCCESS;
  }
  bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
    return true;
  }
  void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override {}

  // fuzz compile interface
  Status FuzzCompileOp(std::vector<ge::NodePtr> &node_vec) override {
    if (AttrUtils::HasAttr(node_vec[0]->GetOpDesc(), "_original_fusion_graph")) {
      ++node_name_2_comile_hits_[node_vec[0]->GetName()];
      // if node is ub fusion node, fuzz compile return failed, to switch origin graph execution
      return FAILED;
    }
    for (auto &node : node_vec) {
      // set compile info on node
      ge::AttrUtils::SetStr(node->GetOpDesc(), "compile_info_key", "op_compile_info_key");
      ge::AttrUtils::SetStr(node->GetOpDesc(), "compile_info_json", "op_compile_info_json");
      ++node_name_2_comile_hits_[node->GetName()];
    }
    return SUCCESS;
  }
  private:
   std::map<std::string, uint32_t> node_name_2_comile_hits_;
};

class FakeFuzzCompileOptimizer : public FakeGraphOptimizer {
  public:
  void ClearStubData() {
    node_2_shape_info_.clear();
  }
  void SetGeneralizedInfoToNode(const std::string &node_name, const GeneralizedShapeInfo &shape_info) {
    node_2_shape_info_[node_name] = shape_info;
  }
  
  // simulate fuzz compile
  Status OptimizeGraphPrepare(ComputeGraph &graph) override {
    std::string build_mode;
    if (ge::GetContext().GetOption("ge.shape_generalized_build_mode", build_mode) != ge::GRAPH_SUCCESS) {
      return SUCCESS;
    }

    if (build_mode != "shape_generalized") {
      return SUCCESS;
    }
    // set generlized shape to nodes on graph, current only support graph without subgraph
    for (const auto &node : graph.GetDirectNode()) {
      const auto node_name = node->GetName();
      auto iter = node_2_shape_info_.find(node_name);
      if (iter == node_2_shape_info_.end()) {
        // stub data is wrong. break the process
        continue;
      }
      auto shape_info = iter->second;
      for (size_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
        auto input_desc = node->GetOpDesc()->MutableInputDesc(i);
        input_desc->SetShape(shape_info.shape);
        input_desc->SetOriginShape(shape_info.shape);
        input_desc->SetShapeRange(shape_info.shape_range);
        input_desc->SetOriginShapeRange(shape_info.shape_range);
      }
      for (size_t i = 0; i < node->GetAllOutDataAnchorsSize(); ++i) {
        auto output_desc = node->GetOpDesc()->MutableOutputDesc(i);
        output_desc->SetShape(shape_info.shape);
        output_desc->SetOriginShape(shape_info.shape);
        output_desc->SetShapeRange(shape_info.shape_range);
        output_desc->SetOriginShapeRange(shape_info.shape_range);
      }
    }
    return SUCCESS;
  }

  private:
  std::map<string, GeneralizedShapeInfo> node_2_shape_info_;
};

void FakeFuzzCompileEngine() {
  auto fuzz_compile_optimzer = MakeShared<FakeFuzzCompileOptimizer>();
  GeneralizedShapeInfo shape_info;
  shape_info.shape = GeShape({2,-1,-1,2});
  shape_info.shape_range = {{2,2},{1,20},{1,20},{2,2}};
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("data1", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("data2", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("conv2d", shape_info);
  auto fuzz_compile_ops_kernel_store = MakeShared<FakeFuzzCompilerOpsKernelInfoStore>("AIcoreEngine");
  GeRunningEnvFaker().Reset()
        .Install(FakeEngine("DNN_VM_GE_LOCAL").KernelInfoStore("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
        .Install(FakeEngine(kEngineNameAiCpu).KernelInfoStore(kEngineNameAiCpu))
        .Install(FakeEngine(kEngineNameAiCpuTf).KernelInfoStore(kEngineNameAiCpuTf))
        .Install(FakeEngine("DNN_VM_HOST_CPU").KernelInfoStore("DNN_VM_HOST_CPU_OP_STORE"))
        .Install(FakeEngine("DNN_VM_RTS").KernelInfoStore("DNN_VM_RTS_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine")
                     .KernelInfoStore(fuzz_compile_ops_kernel_store)
                     .GraphOptimizer("FuzzOptimizer", fuzz_compile_optimzer))
        .Install(FakeOp(CONV2D).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(PARTITIONEDCALL).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(DATA).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(VARIABLE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"));
}

void FakeFuzzCompileEngineForUbFusion() {
  auto fuzz_compile_optimzer = MakeShared<FakeFuzzCompileOptimizer>();
  GeneralizedShapeInfo shape_info;
  shape_info.shape = GeShape({2,-1,-1,2});
  shape_info.shape_range = {{2,2},{1,20},{1,20},{2,2}};
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("data1", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("conv2d", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("relu", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("netoutput_sub", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("data_a", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("fused_conv2d", shape_info);
  fuzz_compile_optimzer->SetGeneralizedInfoToNode("netoutput", shape_info);
  auto fuzz_compile_ops_kernel_store = MakeShared<FakeFuzzCompilerOpsKernelInfoStore>("AIcoreEngine");
  GeRunningEnvFaker().Reset()
        .Install(FakeEngine("DNN_VM_GE_LOCAL").KernelInfoStore("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
        .Install(FakeEngine(kEngineNameAiCpu).KernelInfoStore(kEngineNameAiCpu))
        .Install(FakeEngine(kEngineNameAiCpuTf).KernelInfoStore(kEngineNameAiCpuTf))
        .Install(FakeEngine("DNN_VM_HOST_CPU").KernelInfoStore("DNN_VM_HOST_CPU_OP_STORE"))
        .Install(FakeEngine("DNN_VM_RTS").KernelInfoStore("DNN_VM_RTS_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine")
                     .KernelInfoStore(fuzz_compile_ops_kernel_store)
                     .GraphOptimizer("FuzzOptimizer", fuzz_compile_optimzer))
        .Install(FakeOp(CONV2D).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(PARTITIONEDCALL).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(DATA).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(RELU).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(VARIABLE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp("_RetVal").InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"));
}

Graph BuildFuzzCompileOriginGraph() {
  std::vector<int64_t> shape = {2,2,3,2};  // NCHW

  auto data1 = OP_CFG(DATA)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("data1");
  auto data2 = OP_CFG(DATA)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("data2");
  vector<int64_t> test_int64_list_attr = {1,2,3};
  vector<int32_t> test_int32_list_attr = {1,2,3};
  vector<uint32_t> test_uint32_list_attr = {1,2,3};
  auto conv2d = OP_CFG(CONV2D)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(2)
        .OutCnt(1)
        .Attr("string_attr", "test")
        .Attr("int32_attr", (int32_t)1)
        .Attr("uint32_attr", (uint32_t)1)
        .Attr("test_int64_list_attr", test_int64_list_attr)
        .Attr("test_int32_list_attr", test_int32_list_attr)
        .Attr("test_uint32_list_attr", test_uint32_list_attr)
        .Attr("data_format", "NHWC")  // attr on operator
        .Attr("dilations", test_int64_list_attr)
        .Attr("groups", (int32_t)1)
        .Attr("offset_x", (int32_t)1)
        .Build("conv2d");
  conv2d->SetOpEngineName("AIcoreEngine");
  conv2d->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that?

  auto netoutput = OP_CFG(NETOUTPUT)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("netoutput");

  DEF_GRAPH(g1) {
    CHAIN(NODE(data1)->NODE(conv2d)->NODE(netoutput));
    CHAIN(NODE(data2)->EDGE(0,1)->NODE(conv2d));
  };
  return ToGeGraph(g1);
}

Graph BuildFuzzCompileUnknownRankGraph() {
  std::vector<int64_t> shape = {-2};  // NCHW

  auto data1 = OP_CFG(DATA)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .Build("data1");

  vector<int64_t> test_int64_list_attr = {1,2,3};
  auto conv2d = OP_CFG(CONV2D)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr("data_format", "NHWC")  // attr on operator
        .Attr("dilations", test_int64_list_attr)
        .Attr("groups", (int32_t)1)
        .Attr("offset_x", (int32_t)1)
        .Build("conv2d");
  conv2d->SetOpEngineName("AIcoreEngine");
  conv2d->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that?

  auto netoutput = OP_CFG(NETOUTPUT)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("netoutput");

  DEF_GRAPH(g1) {
    CHAIN(NODE(data1)->NODE(conv2d)->NODE(netoutput));
  };
  return ToGeGraph(g1);
}

Graph BuildFuzzCompileOriginGraphWithUBfusion() {
   std::vector<int64_t> shape = {2,2,3,2};  // NCHW
   std::vector<int64_t> unknown_shape = {2,2,-1,2};  // NCHW

  auto data1 = OP_CFG(DATA)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, unknown_shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_PARENT_NODE_INDEX, 0)
        .Attr("OwnerGraphIsUnknown", true)
        .Build("data1");

  vector<int64_t> test_int64_list_attr = {1,2,3};
  vector<int32_t> test_int32_list_attr = {1,2,3};
  vector<uint32_t> test_uint32_list_attr = {1,2,3};
  auto conv2d = OP_CFG(CONV2D)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, unknown_shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr("string_attr", "test")
        .Attr("int32_attr", (int32_t)1)
        .Attr("uint32_attr", (uint32_t)1)
        .Attr("test_int64_list_attr", test_int64_list_attr)
        .Attr("test_int32_list_attr", test_int32_list_attr)
        .Attr("test_uint32_list_attr", test_uint32_list_attr)
        .Attr("data_format", "NHWC")  // attr on operator
        .Attr("dilations", test_int64_list_attr)
        .Attr("groups", (int32_t)1)
        .Attr("offset_x", (int32_t)1)
        .Build("conv2d");
  conv2d->SetOpEngineName("AIcoreEngine");
  conv2d->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that?

  auto relu = OP_CFG(RELU)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, unknown_shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("relu");
  relu->SetOpEngineName("AIcoreEngine");
  relu->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that? // fe should insure kernel lib name

  auto netoutput_sub = OP_CFG("_RetVal")
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, unknown_shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_PARENT_NODE_INDEX, 0)
        .Build("netoutput_sub");

  DEF_GRAPH(fuse_origin_graph) {
    CHAIN(NODE(data1)->NODE(conv2d)->NODE(relu)->NODE(netoutput_sub));
  };

  auto data_a = OP_CFG(DATA)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .Build("data_a");

  auto conv2d_fused = OP_CFG(CONV2D)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Attr("data_format", "NHWC")  // attr on operator
        .Attr("dilations", test_int64_list_attr)
        .Attr("groups", (int32_t)1)
        .Attr("offset_x", (int32_t)1)
        .Attr("_original_fusion_graph", fuse_origin_graph)
        .Build("conv2d_fused");
  conv2d_fused->SetOpEngineName("AIcoreEngine");
  conv2d_fused->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that?

  DEF_GRAPH(g1) {
    CHAIN(NODE(data_a)->NODE(conv2d_fused)->NODE("netoutput", NETOUTPUT));
  };
  
  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto conv2d_fused_node = compute_graph->FindNode("conv2d_fused");
  auto fused_graph = ToGeGraph(fuse_origin_graph);
  auto fused_compute_graph = GraphUtils::GetComputeGraph(fused_graph);
  fused_compute_graph->SetGraphUnknownFlag(true);
  auto netoutput_sub_node = fused_compute_graph->FindNode("netoutput_sub");
  AttrUtils::SetGraph(conv2d_fused_node->GetOpDesc(), "_original_fusion_graph", fused_compute_graph);
  return graph;
}
}
class DynamicGraphTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    ModelManager::GetInstance().max_model_id_ = 1024;
    GEFinalize();
    ReInitGe();
    BenchEnv::Init();
  }

  void SetUp() {
    tiling_result_ = true;
    tiling_run_info_ = optiling::OpRunInfoV2{};

    auto unique_infer_fun = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      auto output_desc = op_desc->MutableOutputDesc(0);
      output_desc->SetShape(GeShape({-1}));
      output_desc->SetShapeRange({{1, 16}});

      op_desc->MutableOutputDesc(1)->SetDataType(DT_INT32);
      op_desc->MutableOutputDesc(1)->SetShape({});
      return GRAPH_SUCCESS;
    };

    auto type2_infer = [](Operator &op) -> graphStatus {
      auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
      op_desc->SetOpInferDepends({"__input0"});
      Tensor tensor;
      auto output_desc = op_desc->MutableOutputDesc(0);
      if (op.GetInputConstData("__input0", tensor) == GRAPH_SUCCESS) {
        output_desc->SetShape(GeShape({4}));
      } else {
        output_desc->SetShape(GeShape({-1}));
        output_desc->SetShapeRange({{1, 16}});
      }
      return GRAPH_SUCCESS;
    };
    auto ge_dev = GeRunningEnvFaker();
    ge_dev.Reset()
        .Install(FakeEngine("DNN_VM_GE_LOCAL").KernelInfoStore("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeEngine("AIcoreEngine").KernelInfoStore("AIcoreEngine"))
        .Install(FakeEngine(kEngineNameAiCpu).KernelInfoStore(kEngineNameAiCpu))
        .Install(FakeEngine(kEngineNameAiCpuTf).KernelInfoStore(kEngineNameAiCpuTf))
        .Install(FakeEngine("DNN_VM_HOST_CPU").KernelInfoStore("DNN_VM_HOST_CPU_OP_STORE"))
        .Install(FakeEngine("DNN_VM_RTS").KernelInfoStore("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp("MyAdd").InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(RELU).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(CONV2D).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(MUL).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(CAST).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp("MyNeg").InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(NEG).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(TOPKV2).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp("FakeType2Op").InfoStoreAndBuilder("AIcoreEngine").InferShape(type2_infer))
        .Install(FakeOp(DATA).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(VARIABLE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(SHAPE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(IDENTITY).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE").InferShape(infer_fun))
        .Install(FakeOp(CONSTANTOP).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(CONSTANT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(IF).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(CASE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(WHILE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(PARTITIONEDCALL).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp("Unique").InfoStoreAndBuilder("aicpu_ascend_kernel").InferShape(unique_infer_fun))
        .Install(FakeOp("UniqueV2").InfoStoreAndBuilder("AIcoreEngine").InferShape(unique_infer_fun))
        .Install(FakeOp(NETOUTPUT).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"))
        .Install(FakeOp(ASSIGN).InfoStoreAndBuilder("DNN_VM_HOST_CPU_OP_STORE"))
        .Install(FakeOp(SUB).InfoStoreAndBuilder("DNN_VM_HOST_CPU_OP_STORE"))
        .Install(FakeOp(ADD).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(LESS).InfoStoreAndBuilder("AIcoreEngine").InferShape(infer_fun))
        .Install(FakeOp(ENTER).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(MERGE).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(SWITCH).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(LOOPCOND).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(STREAMSWITCH).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(STREAMMERGE).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(STREAMACTIVE).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(NEXTITERATION).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"))
        .Install(FakeOp(EXIT).InfoStoreAndBuilder("DNN_VM_RTS_OP_STORE"));

    optiling::OpTilingFuncV2 tilingfun = [](const ge::Operator &op,
                                            const optiling::OpCompileInfoV2 &compile_info,
                                            optiling::OpRunInfoV2 &run_info) -> bool {
      run_info.SetWorkspaces({1024});
      return true;
    };

    optiling::OpTilingFuncV2 mock_tiling_func = [this](const ge::Operator &op,
                                                       const optiling::OpCompileInfoV2 &compile_info,
                                                       optiling::OpRunInfoV2 &run_info) -> bool {
      run_info = tiling_run_info_;
      return tiling_result_;
    };

    optiling::OpTilingRegistryInterf_V2(RELU, tilingfun);
    REGISTER_OP_TILING_UNIQ_V2(ReLU, tilingfun, 1);

    optiling::OpTilingRegistryInterf_V2("FakeType2Op", tilingfun);
    REGISTER_OP_TILING_UNIQ_V2(FakeType2Op, tilingfun, 1);

    optiling::OpTilingRegistryInterf_V2("MyAdd", mock_tiling_func);
    REGISTER_OP_TILING_UNIQ_V2(MyAdd, mock_tiling_func, 1);

    optiling::OpTilingRegistryInterf_V2("MyNeg", mock_tiling_func);
    REGISTER_OP_TILING_UNIQ_V2(MyNeg, mock_tiling_func, 1);

    optiling::OpTilingRegistryInterf_V2(LESS, mock_tiling_func);
    REGISTER_OP_TILING_UNIQ_V2(Less, mock_tiling_func, 1);

    optiling::OpTilingRegistryInterf_V2(MUL, mock_tiling_func);
    REGISTER_OP_TILING_UNIQ_V2(Mul, mock_tiling_func, 1);

    optiling::OpTilingRegistryInterf_V2(CONV2D, tilingfun);
    REGISTER_OP_TILING_UNIQ_V2(Conv2D, tilingfun, 1);
  }
  void TearDown() {
    MmpaStub::GetInstance().Reset();
    MockRuntime::Reset();
    GEFinalize();
    ReInitGe();
  }
};

namespace {
static void OfflineModelCommand(GeExecutor &ge_executor, const uint32_t model_id) {
  {
    uint64_t dynamic_input_addr = 0U;
    uint64_t length = sizeof(uint64_t);
    uint64_t batch_size = 0U;
    ge_executor.SetDynamicBatchSize(model_id, &dynamic_input_addr, length, batch_size);
  }

  {
    uint64_t dynamic_input_addr = 0U;
    uint64_t length = sizeof(uint64_t);
    uint64_t image_height = 0U;
    uint64_t image_width = 0U;
    ge_executor.SetDynamicImageSize(model_id, &dynamic_input_addr, length, image_height, image_width);
  }

  {
    uint64_t dynamic_input_addr = 0U;
    uint64_t length = sizeof(uint64_t);
    std::vector<uint64_t> dynamic_dims;
    ge_executor.SetDynamicDims(model_id, &dynamic_input_addr, length, dynamic_dims);
  }

  {
    std::vector<uint64_t> dynamic_dims;
    std::vector<uint64_t> cur_dynamic_dims;
    ge_executor.GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims);
  }

  {
    std::vector<int64_t> batch_info;
    int32_t dynamic_type = 0U;
    ge_executor.GetCurShape(model_id, batch_info, dynamic_type);
  }

  {
    uint64_t dynamic_input_addr = 0U;
    uint64_t length = 0U;
    std::vector<kAippDynamicBatchPara> aipp_batch_para;
    kAippDynamicPara aipp_parms;
    ge_executor.SetDynamicAippData(model_id, &dynamic_input_addr, length, aipp_batch_para, aipp_parms);
  }

  {
    std::vector<TensorDesc> input_desc;
    std::vector<TensorDesc> output_desc;
    bool new_model_desc = false;
    ge_executor.GetModelDescInfo(model_id, input_desc, output_desc, new_model_desc);
  }

  {
    std::vector<std::vector<int64_t>> batch_info;
    int32_t dynamic_type = 0U;
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
    uint32_t index = 0U;
    AippConfigInfo aipp_info;
    ge_executor.GetAIPPInfo(model_id, index, aipp_info);
  }

  {
    uint32_t index = 0U;
    InputAippType type;
    size_t aipp_index = 0U;
    ge_executor.GetAippType(model_id, index, type, aipp_index);
  }

  {
    std::string op_name;
    std::string attr_name;
    std::string attr_value;
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
    GeExecutor::GetDeviceIdByModelId(model_id, device_id);
  }

  {
    size_t shape_count = 0U;
    ge_executor.GetBatchInfoSize(model_id, shape_count);
  }

  {
    uint32_t index = 0U;
    OriginInputInfo orig_input_info;
    ge_executor.GetOrigInputInfo(model_id, index, orig_input_info);
  }

  {
    uint32_t index = 0U;
    std::vector<InputOutputDims> input_dims;
    std::vector<InputOutputDims> output_dims;
    ge_executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims);
  }

  {
    uint32_t device_id = 0U;
    uint32_t stream_id = 0U;
    uint32_t task_id = 0U;
    OpDescInfo op_desc_info;
    ge_executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  }
}

Tensor CreateTensor(const TensorDesc &tensor_desc) {
  int64_t tensor_size = -1;
  TensorUtils::GetTensorSizeInBytes(TensorAdapter::TensorDesc2GeTensorDesc(tensor_desc), tensor_size);
  std::vector<uint8_t> tensor_buffer(tensor_size);
  Tensor tensor(tensor_desc);
  tensor.SetData(std::move(tensor_buffer));
  return tensor;
}

Tensor CreateTensor(const std::vector<int64_t> &dims, Format format = FORMAT_ND, DataType data_type = DT_FLOAT) {
  return CreateTensor(TensorDesc(Shape(dims), format, data_type));
}

std::vector<Tensor> CreateInputTensors(const Graph &graph) {
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  std::map<int64_t, GeTensorDescPtr> tensor_descs;
  for (auto &node : compute_graph->GetDirectNode()) {
    if (node->GetType() == DATA) {
      int64_t index = 0;
      AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_INDEX, index);
      tensor_descs[index] = node->GetOpDesc()->MutableOutputDesc(0);
    }
  }

  std::vector<Tensor> tensors(tensor_descs.size());
  for (const auto &it : tensor_descs) {
    tensors[it.first] = CreateTensor(TensorAdapter::GeTensorDesc2TensorDesc(*it.second));
  }

  return tensors;
}

class MockOpsKernelBuilder : public FakeOpsKernelBuilder {
 public:
  MOCK_METHOD3(GenerateTask, Status(const Node &, RunContext &, vector<domi::TaskDef> &));
  MOCK_METHOD0(GetWorkspaces, std::vector<int64_t>());

  Status CalcOpRunningParam(Node &node) override {
    GE_CHK_STATUS_RET_NOLOG(FakeOpsKernelBuilder::CalcOpRunningParam(node));
    auto workspaces = GetWorkspaces();
    node.GetOpDesc()->SetWorkspaceBytes(workspaces);
    return SUCCESS;
  }
};

using GenerateTaskFun = std::function<Status(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks)>;

void MockForGenerateTask(const std::string &name, const GenerateTaskFun &func) {
  auto builder = std::make_shared<MockOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[name] = builder;
  EXPECT_CALL(*builder, GenerateTask).WillRepeatedly(testing::Invoke(func));
}

void MockOnceForOnceSkipGenerateTask(const std::string &name, const GenerateTaskFun &func1, const GenerateTaskFun &func2) {
  auto builder = std::make_shared<MockOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[name] = builder;
  EXPECT_CALL(*builder, GenerateTask).WillOnce(testing::Invoke(func1)).WillRepeatedly(testing::Invoke(func2));
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

Status GenerateTaskForHostCpu(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildHostCpuTask(0));
  return SUCCESS;
}

Status GenerateTaskForAicpuDependRange(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(3));
  return SUCCESS;
}

Status SkipGenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) {
  (void)tasks;
  return SUCCESS;
}

Graph BuildDynamicInputGraph() {
  DEF_GRAPH(dynamic_graph) {
    auto data_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    auto data_1 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    auto add = OP_CFG("MyAdd")
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    CHAIN(NODE("_arg_0", data_0)->NODE("add", add)->NODE("Node_Output", net_output));
    CHAIN(NODE("_arg_1", data_1)->NODE("add", add));
  };

  return ToGeGraph(dynamic_graph);
}

void LoadDynamicOfflineGraph(Graph &graph, uint32_t &model_id) {
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(GEInitialize(options), SUCCESS);
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  std::map<string, string> build_options;
  ModelBufferData model_buffer_data{};
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();

  GeExecutor ge_executor;
  ge_executor.Initialize();
  ModelData model_data;
  model_data.model_data = model_buffer_data.data.get();
  model_data.model_len = model_buffer_data.length;
  EXPECT_EQ(ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0, nullptr, 0), SUCCESS);
}

void BuildAndExecDynamicOfflineModel() {
  auto graph = BuildDynamicInputGraph();
  uint32_t model_id = 0;
  LoadDynamicOfflineGraph(graph, model_id);

  GeExecutor ge_executor;
  ge_executor.Initialize();
  GeShape shape({8, 16});
  GeTensorDesc tensor_desc(shape);
  std::vector<GeTensorDesc> input_desc{tensor_desc, tensor_desc};
  std::vector<GeTensorDesc> output_desc;
  uint8_t buffer[8 * 16 * 4];
  RunModelData input_data;
  input_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  input_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  RunModelData output_data;
  output_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  EXPECT_EQ(ge_executor.ExecModel(model_id, nullptr, input_data, input_desc, output_data, output_desc), SUCCESS);

  OfflineModelCommand(ge_executor, model_id);
  ge_executor.UnloadModel(model_id);
  ge_executor.Finalize();
}

Status RunGraphAsync(Session &session,
                     uint32_t graph_id,
                     const std::vector<Tensor> &inputs,
                     std::vector<Tensor> &outputs) {
  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  Status ret = SUCCESS;
  RunAsyncCallback callback = [&](Status status, std::vector<ge::Tensor> &output_tensors) {
    std::unique_lock<std::mutex> lk(mu);
    ret = status;
    outputs = output_tensors;
    done = true;
    cv.notify_all();
  };

  auto run_ret = session.RunGraphAsync(graph_id, inputs, callback);
  if (run_ret != SUCCESS) {
    return run_ret;
  }

  std::unique_lock<std::mutex> lk(mu);
  if (!cv.wait_for(lk, std::chrono::seconds(15), [&]() { return done; })) {
    // TODO timeout occasionally
    return SUCCESS;
  }
  return ret;
}

void ExecuteDynamicOnlineGraph(Graph &graph,
                               const std::map<std::string, std::string> &session_options = {},
                               bool is_train = false) {
  auto options = session_options;
  if (is_train) {
    options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  }
  DumpProperties dump_properties;
  dump_properties.SetDumpStatus("on");
  dump_properties.SetDumpMode("all");
  dump_properties.AddPropertyValue(DUMP_ALL_MODEL, {});
  DumpManager::GetInstance().AddDumpProperties(0, dump_properties);
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);
  std::vector<Tensor> inputs = CreateInputTensors(graph);
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
}

void ExecuteDynamicOnlineInfer(Graph &graph,
                               const std::map<std::string, std::string> &session_options = {}) {
  return ExecuteDynamicOnlineGraph(graph, session_options, false);
}

void ExecuteDynamicOnlineTrain(Graph &graph,
                               const std::map<std::string, std::string> &session_options = {}) {
  return ExecuteDynamicOnlineGraph(graph, session_options, true);
}

void BuildAndExecDynamicOnlineModel() {
  DEF_GRAPH(graph_def) {
    auto var = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    CHAIN(NODE("var", var)->NODE("unique", unique_op)->NODE("Node_Output", net_output));
  };

  auto graph = ToGeGraph(graph_def);
  ExecuteDynamicOnlineGraph(graph);
}

void ExecDynamicOfflineModel(GeExecutor &ge_executor, uint32_t model_id) {
  GeShape shape({8, 16});
  GeTensorDesc tensor_desc(shape);
  std::vector<GeTensorDesc> input_desc{tensor_desc, tensor_desc};
  std::vector<GeTensorDesc> output_desc;
  uint8_t buffer[8 * 16 * 4];
  RunModelData input_data;
  input_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  input_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  RunModelData output_data;
  output_data.blobs.emplace_back(DataBuffer(buffer, sizeof(buffer)));
  EXPECT_EQ(ge_executor.ExecModel(model_id, nullptr, input_data, input_desc, output_data, output_desc), SUCCESS);
}

void BuildDynamicGraph(Graph &graph, ModelBufferData &model_buffer_data) {
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(GEInitialize(options), SUCCESS);
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  std::map<string, string> build_options;
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

void LoadDynamicGraph(ModelBufferData &model_buffer_data, uint32_t &model_id) {
  GeExecutor ge_executor;
  ge_executor.Initialize();
  ModelData model_data;
  model_data.model_data = model_buffer_data.data.get();
  model_data.model_len = model_buffer_data.length;
  EXPECT_EQ(ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0, nullptr, 0), SUCCESS);
}
}  // namespace

TEST_F(DynamicGraphTest, TestDynamicOfflineModel_aicore_with_atomic_output) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForAiCore);
  BuildAndExecDynamicOfflineModel();
}

TEST_F(DynamicGraphTest, TestDynamicOfflineModel_multi_thread) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForAiCore);
  auto malloc_mock = std::make_shared<MockMalloc>();
  RuntimeStub::SetInstance(malloc_mock);

  auto graph = BuildDynamicInputGraph();
  ModelBufferData model_buffer_data{};
  BuildDynamicGraph(graph, model_buffer_data);

  uint32_t model_id = 0;
  uint32_t model_id2 = 0;
  LoadDynamicGraph(model_buffer_data, model_id);
  LoadDynamicGraph(model_buffer_data, model_id2);

  GeExecutor ge_executor;
  ge_executor.Initialize();
  auto future1 = std::async(std::launch::async, [&ge_executor, model_id] () {
      ExecDynamicOfflineModel(ge_executor, model_id);
  });
  auto future2 = std::async(std::launch::async, [&ge_executor, model_id2] () {
      ExecDynamicOfflineModel(ge_executor, model_id2);
  });
  future1.wait();
  future2.wait();
  future1.get();
  future2.get();

  ge_executor.UnloadModel(model_id);
  ge_executor.UnloadModel(model_id2);
  ge_executor.Finalize();
}

TEST_F(DynamicGraphTest, TestUnloadModelAfterFinalize) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForAiCore);

  auto graph = BuildDynamicInputGraph();
  ModelBufferData model_buffer_data{};
  BuildDynamicGraph(graph, model_buffer_data);

  uint32_t model_id = 0;
  LoadDynamicGraph(model_buffer_data, model_id);

  GeExecutor ge_executor;
  ge_executor.Initialize();
  ExecDynamicOfflineModel(ge_executor, model_id);
  ge_executor.Finalize();
  ge_executor.UnloadModel(model_id);
}

TEST_F(DynamicGraphTest, TestDynamicOfflineModel_aicore_with_atomic_workspace) {
  auto func = [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
    auto op_desc = node.GetOpDesc();
    auto ret = GenerateTaskForAiCore(node, context, tasks);
    GeAttrValue::NAMED_ATTRS workspaces;
    GeAttrValue::NamedAttrs workspaces_attrs;
    vector<int> dim_types;
    dim_types.push_back(0);
    dim_types.push_back(1);
    AttrUtils::SetListInt(workspaces_attrs, op_desc->GetName(), dim_types);
    AttrUtils::SetNamedAttrs(op_desc, EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspaces_attrs);
    return ret;
  };
  auto builder = std::make_shared<MockOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_["AIcoreEngine"] = builder;
  EXPECT_CALL(*builder, GenerateTask).WillRepeatedly(testing::Invoke(func));

  tiling_run_info_.SetWorkspaces({256});
  BuildAndExecDynamicOfflineModel();
}

TEST_F(DynamicGraphTest, TestDynamicOfflineModel_aicore_task_with_handle) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  BuildAndExecDynamicOfflineModel();
}

TEST_F(DynamicGraphTest, TestDynamicOfflineModel_aicore_fallback_to_aicpu) {
  auto func = [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
    auto op_desc = node.GetOpDesc();
    GenerateTaskForAiCore(node, context, tasks);
    tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(0));
    AttrUtils::SetBool(op_desc, "partially_supported", true);
    op_desc->SetOpKernelLibName("AIcoreEngine");
    return SUCCESS;
  };
  MockForGenerateTask("AIcoreEngine", func);
  tiling_result_ = false;
  BuildAndExecDynamicOfflineModel();
}

TEST_F(DynamicGraphTest, TestDynamicOnlineInfer) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  DEF_GRAPH(dynamic_graph) {
    auto data_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto neg = OP_CFG(NEG)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    CHAIN(NODE("_arg_0", data_0)->NODE("neg", neg)->NODE("unique", unique_op)->NODE("Node_Output", net_output));
  };

  Graph graph = ToGeGraph(dynamic_graph);

  std::map<std::string, std::string> dump_options;
  dump_options.emplace(OPTION_EXEC_ENABLE_DUMP, "1");
  dump_options.emplace(OPTION_EXEC_ENABLE_DUMP_DEBUG, "0");
  dump_options.emplace(OPTION_EXEC_DUMP_PATH, "./");
  dump_options.emplace(OPTION_EXEC_DUMP_STEP, "0|5|10-20");
  dump_options.emplace(OPTION_EXEC_DUMP_MODE, "all");
  ExecuteDynamicOnlineInfer(graph, dump_options);
}

TEST_F(DynamicGraphTest, TestDynamicOnlineInferWithType3Aicore) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  DEF_GRAPH(dynamic_graph) {
    auto data_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("UniqueV2")
        .InCnt(1)
        .OutCnt(2)
        .Attr(ATTR_NAME_UNKNOWN_SHAPE_TYPE, 3)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Build("unique");

    auto func = [](const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                   rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flag) -> int {
      uintptr_t shape_buffer_addr = reinterpret_cast<uintptr_t *>(argsInfo->args)[3];
      auto shape_buffer = reinterpret_cast<uint32_t *>(shape_buffer_addr);
      shape_buffer[0] = 1;  // 1-dim
      shape_buffer[1] = 8;  // [8]
      return RT_ERROR_NONE;
    };
    auto runtime_stub = std::make_shared<MockRuntime>();
    RuntimeStub::SetInstance(runtime_stub);
    EXPECT_CALL(*runtime_stub, rtKernelLaunchWithFlag).WillRepeatedly(testing::Invoke(func));

    unique_op->MutableOutputDesc(0)->SetShape(GeShape({-1}));
    unique_op->MutableOutputDesc(1)->SetDataType(DT_INT32);
    unique_op->MutableOutputDesc(1)->SetShape({});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    CHAIN(NODE("_arg_0", data_0)->NODE(unique_op)->NODE("Node_Output", net_output));
  };

  Graph graph = ToGeGraph(dynamic_graph);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);
  ExecuteDynamicOnlineInfer(graph);
}

TEST_F(DynamicGraphTest, TestDynamicOnlineTraining) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);

  GeTensorDesc tensor_desc_1(GeShape(std::vector<int64_t>({1})), FORMAT_ND, DT_STRING);
  std::vector<uint8_t> string_buffer(24, 0);
  GeTensor tensor_1(tensor_desc_1);
  tensor_1.SetData(std::move(string_buffer));

  DEF_GRAPH(dynamic_graph) {
    auto var_0 = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto const_0 = OP_CFG(CONSTANTOP)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {});

    auto const_1 = OP_CFG(CONSTANTOP)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor_1)
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_STRING, {});

    auto neg = OP_CFG(NEG)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Attr(ATTR_NAME_INSERT_BP_PROFILILNG_TASK, true)
        .Attr(ATTR_NAME_INSERT_FP_PROFILILNG_TASK, true);

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1, -1});

    CHAIN(NODE("_arg_0", var_0)->NODE("neg", neg)->NODE("unique", unique_op)->NODE("Node_Output", net_output));
    CHAIN(NODE("const_0", const_0)->NODE("Node_Output", net_output));
    CHAIN(NODE("const_1", const_1)->NODE("Node_Output", net_output));
  };

  Graph graph = ToGeGraph(dynamic_graph);
  ExecuteDynamicOnlineTrain(graph);
}

TEST_F(DynamicGraphTest, TestControlOp_If) {
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  DEF_GRAPH(then_branch) {
    auto data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    CHAIN(NODE("then_arg_0", data)->NODE("then_Node_Output", net_output));
  };

  DEF_GRAPH(else_branch) {
    auto data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    CHAIN(NODE("else_arg_0", data)->NODE("Unique", unique_op)->NODE("else_Node_Output", net_output));
  };

  auto then_graph = ToComputeGraph(then_branch);
  auto else_graph = ToComputeGraph(else_branch);

  DEF_GRAPH(if_graph) {
    auto pred_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto value_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto if_op = OP_CFG(IF)
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Build("if");

    if_op->MutableOutputDesc(0)->SetShape(GeShape({-1}));
    if_op->RegisterSubgraphIrName("then_branch", SubgraphType::kStatic);
    if_op->RegisterSubgraphIrName("else_branch", SubgraphType::kStatic);
    if_op->AddSubgraphName(then_graph->GetName());
    if_op->SetSubgraphInstanceName(0, then_graph->GetName());
    if_op->AddSubgraphName(else_graph->GetName());
    if_op->SetSubgraphInstanceName(1, else_graph->GetName());

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    CHAIN(NODE("arg_pred", pred_data)->NODE(if_op)->NODE("Node_Output", net_output));
    CHAIN(NODE("arg_value", value_data)->NODE(if_op));
  };

  auto root_graph = ToComputeGraph(if_graph);
  auto if_node = root_graph->FindFirstNodeMatchType(IF);
  EXPECT_TRUE(if_node != nullptr);
  then_graph->SetParentNode(if_node);
  then_graph->SetParentGraph(root_graph);
  else_graph->SetParentNode(if_node);
  else_graph->SetParentGraph(root_graph);
  root_graph->AddSubgraph(then_graph);
  root_graph->AddSubgraph(else_graph);

  auto graph = GraphUtils::CreateGraphFromComputeGraph(root_graph);
  ExecuteDynamicOnlineTrain(graph);
}

TEST_F(DynamicGraphTest, TestControlOp_While) {
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);

  DEF_GRAPH(cond) {
    auto cond_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {});
    CHAIN(NODE("cond_data", cond_data)->NODE("cond_Node_Output", net_output));
  };

  GeTensor zero_tensor(GeTensorDesc(GeShape(std::vector<int64_t>{}), FORMAT_ND, DT_INT32));
  zero_tensor.SetData(std::vector<uint8_t>{0, 0, 0, 0});
  DEF_GRAPH(body) {
    auto cond_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto value_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    auto const_data = OP_CFG(CONSTANT)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Attr(ATTR_NAME_WEIGHTS, zero_tensor);

    auto mul = OP_CFG(MUL)
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(2)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16})
        .Build("body_Node_Output");

    net_output->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({})));
    net_output->MutableOutputDesc(0)->SetDataType(DT_INT32);
    CHAIN(NODE("body_arg_0", cond_data)->NODE("mul", mul)->NODE(net_output));
    CHAIN(NODE("one_tensor", const_data)->NODE("mul", mul));
    CHAIN(NODE("value_data", value_data)->NODE(net_output));
  };

  auto cond_graph = ToComputeGraph(cond);
  auto body_graph = ToComputeGraph(body);

  DEF_GRAPH(while_graph) {
    auto cond_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto value_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto while_op = OP_CFG(WHILE)
        .InCnt(2)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1})
        .Build("while_op");

    while_op->MutableInputDesc(0)->SetShape(GeShape(std::vector<int64_t>({})));
    while_op->MutableInputDesc(0)->SetDataType(DT_INT32);
    while_op->MutableOutputDesc(0)->SetShape(GeShape(std::vector<int64_t>({})));
    while_op->MutableOutputDesc(0)->SetDataType(DT_INT32);
    while_op->RegisterSubgraphIrName("cond", SubgraphType::kStatic);
    while_op->RegisterSubgraphIrName("body", SubgraphType::kStatic);

    while_op->AddSubgraphName(cond_graph->GetName());
    while_op->SetSubgraphInstanceName(0, cond_graph->GetName());
    while_op->AddSubgraphName(body_graph->GetName());
    while_op->SetSubgraphInstanceName(1, body_graph->GetName());

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    CHAIN(NODE("arg_cond", cond_data)->NODE(while_op));
    CHAIN(NODE("arg_value", value_data)->NODE("unique", unique_op)->NODE(while_op)->EDGE(1, 0)->NODE("Node_Output",
                                                                                                     net_output));
  };

  auto root_graph = ToComputeGraph(while_graph);
  auto while_node = root_graph->FindFirstNodeMatchType(WHILE);
  EXPECT_TRUE(while_node != nullptr);
  cond_graph->SetParentNode(while_node);
  cond_graph->SetParentGraph(root_graph);
  body_graph->SetParentNode(while_node);
  body_graph->SetParentGraph(root_graph);
  root_graph->AddSubgraph(cond_graph);
  root_graph->AddSubgraph(body_graph);

  auto graph = GraphUtils::CreateGraphFromComputeGraph(root_graph);

  auto mul_kernel = [](const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                       rtSmDesc_t *smDesc, rtStream_t stream, uint32_t flag) -> int {
    auto io_addrs = reinterpret_cast<uintptr_t *>(argsInfo->args);
    auto *input_0 = reinterpret_cast<int32_t *>(io_addrs[0]);
    auto *input_1 = reinterpret_cast<int32_t *>(io_addrs[1]);
    auto *output = reinterpret_cast<int32_t *>(io_addrs[2]);
    *output = *input_0 * *input_1;
    return RT_ERROR_NONE;
  };
  auto runtime_stub = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtKernelLaunchWithFlag).WillRepeatedly(testing::Invoke(mul_kernel));

  std::map<AscendString, AscendString> options;
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape_cond(std::vector<int64_t>{});
  Tensor cond_tensor(TensorDesc(shape_cond, FORMAT_ND, DT_INT32));
  int32_t value = 1;
  cond_tensor.SetData((uint8_t *) &value, sizeof(value));

  uint8_t value_buffer[16 * 4];
  Shape shape_value(std::vector<int64_t>({16}));
  Tensor value_tensor(TensorDesc(shape_value, FORMAT_ND, DT_FLOAT));
  value_tensor.SetData(value_buffer, sizeof(value_buffer));

  std::vector<Tensor> inputs{cond_tensor, value_tensor};
  std::vector<Tensor> outputs;
  // cond->body->cond
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);

  value = 0;
  cond_tensor.SetData((uint8_t *) &value, sizeof(value));
  // cond
  inputs = {cond_tensor, value_tensor};
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
}

TEST_F(DynamicGraphTest, TestHostCpu) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  MockForGenerateTask("DNN_VM_HOST_CPU_OP_STORE", GenerateTaskForHostCpu);
  GeTensorDesc tensor_desc(GeShape{});
  GeTensor tensor(tensor_desc);
  int32_t value = 666;
  tensor.SetData((uint8_t *) &value, sizeof(value));
  DEF_GRAPH(host_cpu_graph) {
    auto var_0 = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_VARIABLE_PLACEMENT, "host")
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto const_0 = OP_CFG(CONSTANTOP)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, tensor)
        .Attr(ATTR_VARIABLE_PLACEMENT, "host")
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto assign = OP_CFG(ASSIGN)
        .InCnt(2)
        .OutCnt(1)
        .Attr(ATTR_VARIABLE_PLACEMENT, "host")
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    CHAIN(NODE("_arg_0", var_0)->NODE("assign", assign)->NODE("Node_Output", net_output));
    CHAIN(NODE("const_0", const_0)->NODE("assign", assign));
  };

  Graph graph = ToGeGraph(host_cpu_graph);
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options["ge.exec.placement"] = "HOST";
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  std::vector<Tensor> inputs;
  std::vector<Tensor> outputs;
  HostCpuEngine::GetInstance().constant_folding_handle_ = mock_host_cpu_handle;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);

  EXPECT_EQ(outputs.size(), 1);
  auto output = reinterpret_cast<int32_t *>(outputs[0].GetData())[0];
  EXPECT_EQ(output, value);
}

//TEST_F(DynamicGraphTest, TestGeLocal) {
//}

TEST_F(DynamicGraphTest, TestCaseOpAndPartitionedCallExecutor) {
  MockForGenerateTask("aicpu_ascend_kernel", GenerateTaskForAicpuDependRange);
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  DEF_GRAPH(partitioned_call) {
    auto cond_data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    auto neg = OP_CFG("MyNeg")
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    CHAIN(NODE("partitioned_call_data", cond_data)->NODE("neg", neg)->NODE("partitioned_call_Node_Output",
                                                                           net_output));
  };
  auto sub_graph = ToComputeGraph(partitioned_call);

  DEF_GRAPH(branch_0) {
    auto data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    CHAIN(NODE("branch_0_arg_0", data)->NODE("branch_0_Node_Output", net_output));
  };

  DEF_GRAPH(branch_1) {
    auto data = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    auto partitioned_call_op = OP_CFG(PARTITIONEDCALL)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {-1})
        .Build("partitioned_call_op");

    partitioned_call_op->RegisterSubgraphIrName("f", SubgraphType::kStatic);
    partitioned_call_op->AddSubgraphName(sub_graph->GetName());
    partitioned_call_op->SetSubgraphInstanceName(0, sub_graph->GetName());

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
    CHAIN(NODE("branch_0_arg_0", data)->NODE(partitioned_call_op)->NODE("branch_0_Node_Output", net_output));
  };

  auto sub_graph_b0 = ToComputeGraph(branch_0);
  auto sub_graph_b1 = ToComputeGraph(branch_1);

  auto partitioned_call_node = sub_graph_b1->FindFirstNodeMatchType(PARTITIONEDCALL);
  EXPECT_TRUE(partitioned_call_node != nullptr);
  sub_graph->SetParentNode(partitioned_call_node);
  sub_graph->SetParentGraph(sub_graph_b1);

  DEF_GRAPH(case_graph) {
    auto arg_branch_index = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {});

    auto arg_value = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto unique_op = OP_CFG("Unique")
        .InCnt(1)
        .OutCnt(2)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto case_op = OP_CFG(CASE)
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1})
        .Build("case_op");

    case_op->RegisterSubgraphIrName("branches", SubgraphType::kDynamic);
    case_op->AddSubgraphName(sub_graph_b0->GetName());
    case_op->SetSubgraphInstanceName(0, sub_graph_b0->GetName());
    case_op->AddSubgraphName(sub_graph_b1->GetName());
    case_op->SetSubgraphInstanceName(1, sub_graph_b1->GetName());

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    CHAIN(NODE("arg_branch_index", arg_branch_index)->NODE(case_op)->NODE("Node_Output", net_output));
    CHAIN(NODE("arg_value", arg_value)->NODE("unique_op", unique_op)->NODE(case_op));
  };

  auto root_graph = ToComputeGraph(case_graph);
  auto case_node = root_graph->FindFirstNodeMatchType(CASE);
  EXPECT_TRUE(case_node != nullptr);
  sub_graph_b0->SetParentNode(case_node);
  sub_graph_b0->SetParentGraph(root_graph);
  sub_graph_b1->SetParentNode(case_node);
  sub_graph_b1->SetParentGraph(root_graph);
  EXPECT_EQ(root_graph->AddSubgraph(sub_graph_b0), GRAPH_SUCCESS);
  EXPECT_EQ(root_graph->AddSubgraph(sub_graph_b1), GRAPH_SUCCESS);
  EXPECT_EQ(root_graph->AddSubgraph(sub_graph), GRAPH_SUCCESS);

  auto graph = GraphUtils::CreateGraphFromComputeGraph(root_graph);

  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape_index(std::vector<int64_t>({}));
  int32_t branch = 0;
  TensorDesc tensor_desc_index(shape_index);
  Tensor input_0(tensor_desc_index);
  input_0.SetData((uint8_t *) &branch, sizeof(branch));

  Shape shape_value(std::vector<int64_t>({16}));
  TensorDesc tensor_desc_value(shape_value);
  Tensor input_1(tensor_desc_value);
  uint8_t buffer[16 * sizeof(float)];
  input_1.SetData((uint8_t *) &buffer, sizeof(buffer));

  // taking branch 0
  std::vector<Tensor> inputs{input_0, input_1};
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);

  // taking branch 1
  branch = 1;
  inputs[0].SetData((uint8_t *) &branch, sizeof(branch));
  outputs.clear();
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  // cover muting workspace count
  tiling_run_info_.SetWorkspaces({16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16});
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
}

TEST_F(DynamicGraphTest, TestAicpuKernels) {
  auto mock_memcpy = [](void *dst, uint64_t dest_max, const void *src, uint64_t count, rtMemcpyKind_t kind) -> int {
    if (count == 0) {
      return RT_ERROR_NONE;
    }
    if (count == sizeof(aicpu::FWKAdapter::ResultSummary) && kind == RT_MEMCPY_DEVICE_TO_HOST) {
      aicpu::FWKAdapter::ResultSummary summary{};
      summary.shape_data_size = 8;
      summary.raw_data_size = 4;
      return memcpy_s(dst, dest_max, &summary, count);
    } else {
      return memcpy_s(dst, dest_max, src, count);
    }
  };
  auto runtime_stub = std::make_shared<MockMemcpy>();
  RuntimeStub::SetInstance(runtime_stub);
  EXPECT_CALL(*runtime_stub, rtMemcpy).WillRepeatedly(testing::Invoke(mock_memcpy));

  auto generate_aicpu_type_4_kernels =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(4));
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(0));
        AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_UNKNOWN_SHAPE_TYPE, 4);
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_aicpu_type_4_kernels);
  BuildAndExecDynamicOnlineModel();

  auto generate_tf_type_4_kernels =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(4));
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(0));
        AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_UNKNOWN_SHAPE_TYPE, 4);
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_tf_type_4_kernels);
  BuildAndExecDynamicOnlineModel();

  auto generate_aicpu_type_3_kernel =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(3));
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_aicpu_type_3_kernel);
  BuildAndExecDynamicOnlineModel();

  auto generate_tf_type_3_kernel =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(3));
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_tf_type_3_kernel);
  BuildAndExecDynamicOnlineModel();

  auto generate_aicpu_type_3_kernel_blocking =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildAicpuTask(3));
        AttrUtils::SetBool(node.GetOpDesc(), ATTR_NAME_IS_BLOCKING_OP, true);
        AttrUtils::SetBool(node.GetOpDesc(), ATTR_NAME_IS_BLOCKING_OP, true);
        AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_BLOCKDIM_INDEX, 1);
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_aicpu_type_3_kernel_blocking);
  BuildAndExecDynamicOnlineModel();

  auto generate_tf_type_3_kernel_blocking =
      [](const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) -> Status {
        tasks.emplace_back(AicpuTaskDefBuilder(node).BuildTfTask(3));
        AttrUtils::SetBool(node.GetOpDesc(), ATTR_NAME_IS_BLOCKING_OP, true);
        AttrUtils::SetBool(node.GetOpDesc(), ATTR_NAME_IS_BLOCKING_OP, true);
        AttrUtils::SetInt(node.GetOpDesc(), ATTR_NAME_BLOCKDIM_INDEX, 1);
        return SUCCESS;
      };
  MockForGenerateTask("aicpu_ascend_kernel", generate_tf_type_3_kernel_blocking);
  BuildAndExecDynamicOnlineModel();
}

TEST_F(DynamicGraphTest, TestType2AndGeLocal) {
  DEF_GRAPH(graph_def) {
    auto var = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16}).Build("fake_type2_op");

    auto shape_op = OP_CFG(SHAPE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {1});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {-1});

    CHAIN(NODE("var", var)
              ->NODE(fake_type2_op)
              ->NODE("shape", shape_op)
              ->NODE("Node_Output", net_output));
  };

  auto graph = ToGeGraph(graph_def);
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  ExecuteDynamicOnlineTrain(graph);
}

TEST_F(DynamicGraphTest, TestInferShapeForSubgraph) {
  DEF_GRAPH(fused_subgraph) {
    auto data_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_PARENT_NODE_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16})
        .Build("fake_type2_op");

    fake_type2_op->SetOpInferDepends({"__input0"});
    fake_type2_op->SetOpEngineName("AIcoreEngine");
    fake_type2_op->SetOpKernelLibName("AIcoreEngine");  // fake op can not do that?

    auto ret_val = OP_CFG("_RetVal")
        .InCnt(1)
        .Attr(ATTR_NAME_PARENT_NODE_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
    CHAIN(NODE("_arg_0", data_0)->NODE(fake_type2_op)->NODE("ret_val", ret_val));
  };

  DEF_GRAPH(dynamic_graph) {
    auto data_0 = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {-1});

    CHAIN(NODE("_arg_0", data_0)->NODE("fused_op", fake_type2_op)->NODE("Node_Output", net_output));
  };

  auto org_graph = ToComputeGraph(fused_subgraph);
  auto root_graph = ToComputeGraph(dynamic_graph);
  auto add_node = root_graph->FindNode("fused_op");
  EXPECT_TRUE(add_node != nullptr);
  AttrUtils::SetGraph(add_node->GetOpDesc(), "_original_fusion_graph", org_graph);
  auto graph = GraphUtils::CreateGraphFromComputeGraph(root_graph);
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  ExecuteDynamicOnlineTrain(graph);
}

TEST_F(DynamicGraphTest, TestDynamicInput) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  auto graph = BuildDynamicInputGraph();
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options[OPTION_EXEC_ENABLE_DUMP_DEBUG] = "1";
  options[OPTION_EXEC_DUMP_PATH] = "./";
  options[OPTION_EXEC_DUMP_DEBUG_MODE] = "aicore_overflow"; // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options[OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE] = "[1~20,1~30],[1~20,1~30]";
  graph_options[OPTION_EXEC_ENABLE_EXCEPTION_DUMP] = "1";

  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);

  std::vector<Tensor> inputs;
  inputs.emplace_back(CreateTensor({2, 16}));
  inputs.emplace_back(CreateTensor({2, 16}));
  std::vector<Tensor> outputs;
  setenv("HYBRID_PROFILING_LEVEL", "1", 1);
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  EXPECT_EQ(RunGraphAsync(session, graph_id, inputs, outputs), SUCCESS);
  unsetenv("HYBRID_PROFILING_LEVEL");
  session.RemoveGraph(graph_id);
}

TEST_F(DynamicGraphTest, TestLazyRecompile) {
  DEF_GRAPH(graph_def) {
    auto var = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});
    

    auto shape_op = OP_CFG(SHAPE)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {1});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {-1});

    CHAIN(NODE("var", var)
              ->NODE("fake_type2_op", fake_type2_op)
              ->NODE("shape", shape_op)
              ->NODE("Node_Output", net_output));
  };

  auto graph = ToGeGraph(graph_def);
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  std::map<std::string, std::string> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  Session session(options);

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "lazy_recompile";
  graph_options[OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR] = "1";
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);
  std::vector<Tensor> inputs = CreateInputTensors(graph);
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
  session.RemoveGraph(graph_id);
}

TEST_F(DynamicGraphTest, TestOptimizeDependenciesForConstantInputs) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  // 1. const in known-shaped subgraph (after partitioning)
  DEF_GRAPH(graph_def) {
    auto const_op = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, TensorAdapter::AsGeTensor(CreateTensor({16})))
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {-1});

    CHAIN(NODE("const_op", const_op)->NODE("fake_type2_op", fake_type2_op)->NODE("Node_Output", net_output));
  };
  auto graph = ToGeGraph(graph_def);
  ExecuteDynamicOnlineTrain(graph);

  // 2. const in root graph
  DEF_GRAPH(graph_def2) {
    auto const_op = OP_CFG(CONSTANTOP)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_WEIGHTS, TensorAdapter::AsGeTensor(CreateTensor({16})))
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto fake_type2_op = OP_CFG("FakeType2Op")
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});

    CHAIN(NODE("const_op", const_op)->NODE("fake_type2_op", fake_type2_op)->NODE("Node_Output", net_output));
  };
  graph = ToGeGraph(graph_def2);
  ExecuteDynamicOnlineTrain(graph);
}

TEST_F(DynamicGraphTest, BasicV1LoopDynamicExecSucc) {
  MockForGenerateTask("AIcoreEngine", GenerateTaskForStaticAicore);
  auto graph = GraphFactory::BuildV1LoopGraph1();
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  compute_graph->SetGraphUnknownFlag(true);
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (node->GetType() == DATA || node->GetType() == CONSTANT) {
      continue;
    }
    (void)AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
  }
  const bool tiling_result_backup = tiling_result_;
  tiling_result_ = true;
  ExecuteDynamicOnlineTrain(graph);
  tiling_result_ = tiling_result_backup;
}

//TEST_F(DynamicGraphTest, TestPipelineExecution) {
//  DEF_GRAPH(graph_def) {
//    auto var = OP_CFG(DATA)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_NAME_INDEX, 0)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    auto neg_1 = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    auto neg_2 = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    auto net_output = OP_CFG(NETOUTPUT)
//        .InCnt(1)
//        .OutCnt(1)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    CHAIN(NODE("var", var)
//              ->NODE("neg_1", neg_1)
//              ->NODE("neg_2", neg_2)
//              ->NODE("Node_Output", net_output));
//  };
//
//  auto graph = ToGeGraph(graph_def);
//  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
//
//  std::map<AscendString, AscendString> options;
//  options.emplace(OPTION_EXEC_ENABLE_DUMP, "1");
//  options.emplace(OPTION_EXEC_ENABLE_DUMP_DEBUG, "0");
//  options.emplace(OPTION_EXEC_DUMP_PATH, "./");
//  options.emplace(OPTION_EXEC_DUMP_STEP, "0|5|10-20");
//  options.emplace(OPTION_EXEC_DUMP_MODE, "all");
//  Session session(options);
//
//  std::map<AscendString, AscendString> graph_options;
//  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
//  graph_options[OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE] = "[1~20]";
//  GraphId graph_id = 1;
//  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);
//
//  std::vector<Tensor> inputs;
//  inputs.emplace_back(CreateTensor({16}));
//  std::vector<Tensor> outputs;
//  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
//  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
//  session.RemoveGraph(graph_id);
//}

//TEST_F(DynamicGraphTest, TestPipelineExecution) {
//  DEF_GRAPH(graph_def) {
//    auto arg_0 = OP_CFG(DATA)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_NAME_INDEX, 0)
//        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
//
//    auto arg_1 = OP_CFG(DATA)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_NAME_INDEX, 1)
//        .TensorDesc(FORMAT_ND, DT_FLOAT, {-1});
//
//    auto neg_1 = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
//
//    auto neg_2 = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
//
//    auto net_output = OP_CFG(NETOUTPUT)
//        .InCnt(2)
//        .OutCnt(2)
//        .TensorDesc(FORMAT_ND, DT_FLOAT, {16});
//
//    CHAIN(NODE("arg_0", arg_0)
//              ->NODE("neg_1", neg_1)
//              ->NODE("neg_2", neg_2)
//              ->NODE("Node_Output", net_output));
//    CHAIN(NODE("arg_1", arg_1)->NODE("Node_Output", net_output));
//  };
//
//  dlog_setlevel(0, 0, 0);
//  auto graph = ToGeGraph(graph_def);
//  std::map<std::string, std::string> options;
//  Session session(options);
//  GraphId graph_id = 1;
//
//  std::map<AscendString, AscendString> graph_options;
//  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
//  graph_options[OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE] = "[16][1~20]";
//  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);
//  std::vector<Tensor> inputs;
//  inputs.emplace_back(CreateTensor({16}));
//  inputs.emplace_back(CreateTensor({16}));
//  std::vector<Tensor> outputs;
//  EXPECT_EQ(session.RunGraph(graph_id, inputs, outputs), SUCCESS);
//  session.RemoveGraph(graph_id);
//}

//TEST_F(DynamicGraphTest, TestPipelineExecution) {
//  DEF_GRAPH(graph_def) {
//    auto var = OP_CFG(CONSTANTOP)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .Attr(ATTR_NAME_WEIGHTS, TensorAdapter::AsGeTensor(CreateTensor({16}, FORMAT_ND, DT_INT32)))
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {16});
//
//    auto fake_type2_op = OP_CFG("FakeType2Op")
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {16});
//
//    auto neg_op = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {1});
//
//    auto net_output = OP_CFG(NETOUTPUT)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    CHAIN(NODE("var", var)
//    ->NODE("fake_type2_op", fake_type2_op)
//    ->NODE("neg_op", neg_op)
//    ->NODE("Node_Output", net_output));
//  };
//
//  auto graph = ToGeGraph(graph_def);
//  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
//  ExecuteDynamicOnlineTrain(graph);
//}

//TEST_F(DynamicGraphTest, TestPipelineExecution) {
//  DEF_GRAPH(graph_def) {
//    auto var = OP_CFG(VARIABLE)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .Attr(ATTR_NAME_WEIGHTS, TensorAdapter::AsGeTensor(CreateTensor({16}, FORMAT_ND, DT_INT32)))
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {16});
//
//    auto fake_type2_op = OP_CFG("FakeType2Op")
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 0)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {16});
//
//    auto neg_op = OP_CFG(NEG)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {1});
//
//    auto net_output = OP_CFG(NETOUTPUT)
//        .InCnt(1)
//        .OutCnt(1)
//        .Attr(ATTR_STAGE_LEVEL, 1)
//        .Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true)
//        .TensorDesc(FORMAT_ND, DT_INT32, {-1});
//
//    CHAIN(NODE("var", var)
//    ->NODE("fake_type2_op", fake_type2_op)
//    ->NODE("neg_op", neg_op)
//    ->NODE("Node_Output", net_output));
//  };
//
//  auto graph = ToGeGraph(graph_def);
//  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
//  ExecuteDynamicOnlineTrain(graph);
//}


/**
 * 测试思路
 * 准备工作：
 * 1、构造静态shape图，图中conv2d算子shape为{2,2,3,2}
 * 2、为FE的fuzz函数打桩，将图中算子shape泛化为[2,2,-1,2], range 泛化为[[2,2],[2,2,],[1,20],[2,2]]
 * 3、为FE的执行时泛化编译接口FuzzComile打桩，记录该函数被调用的次数
 * 
 * 执行用例：
 * 1、session配置graph option, 开启OPTION_EXEC_DYNAMIC_EXECUTE_MODE为dynamic_execute，不配置shape range。表示开启模糊编译
 *    开启ge.shape_generalized_build_mode为shape_generalized，模拟adapter在训练时开启模糊编译传入的option
 * 2、session add准备工作中构造的静态图
 * 3、执行buildgraph。此时会触发fe的fuzz函数桩。校验图中conv2d算子的shape
 * 4、执行RunGraphAsync，给定输入shape在范围内，[2,2,3,2]
 * 5、校验FuzzComile函数没有被调用，表示没有发生kernel miss.
 * 
 */
TEST_F(DynamicGraphTest, TestFuzzCompileExecuteWithoutKernelMiss) {
  FakeFuzzCompileEngine();
  
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  auto graph = BuildFuzzCompileOriginGraph();
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options[OPTION_EXEC_ENABLE_DUMP_DEBUG] = "1";
  options[OPTION_EXEC_DUMP_PATH] = "./";
  options[OPTION_EXEC_DUMP_DEBUG_MODE] = "aicore_overflow"; // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";

  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);

  std::vector<Tensor> inputs;
  inputs.emplace_back(CreateTensor({2,2,3,2}));
  inputs.emplace_back(CreateTensor({2,2,3,2}));
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  EXPECT_EQ(RunGraphAsync(session, graph_id, inputs, outputs), SUCCESS);
  // check fuzz_compile called count when execute
  OpsKernelManager &kernel_manager = OpsKernelManager::GetInstance();
  auto iter = kernel_manager.GetAllOpsKernelInfoStores().find("AIcoreEngine");
  EXPECT_NE(iter, kernel_manager.GetAllOpsKernelInfoStores().end());
  auto fuzz_compile_store = dynamic_cast<FakeFuzzCompilerOpsKernelInfoStore *>(iter->second.get());
  auto fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("Conv2d");
  EXPECT_EQ(fuzz_compile_counts, 0);
  session.RemoveGraph(graph_id);
}

/**
 * 测试思路
 * 准备工作：
 * 1、构造静态shape图，图中conv2d算子shape为{2,2,3,2}
 * 2、为FE的fuzz函数打桩，将图中算子shape泛化为[2,2,-1,2], range 泛化为[[2,2],[2,2,],[1,20],[2,2]]
 * 3、为FE的执行时泛化编译接口FuzzComile打桩，记录该函数被调用的次数
 * 
 * 执行用例：
 * 1、session配置graph option, 开启OPTION_EXEC_DYNAMIC_EXECUTE_MODE为dynamic_execute，不配置shape range。表示开启模糊编译
 *    开启ge.shape_generalized_build_mode为shape_generalized，模拟adapter在训练时开启模糊编译传入的option
 * 2、session add准备工作中构造的静态图
 * 3、执行buildgraph。此时会触发fe的fuzz函数桩。校验图中conv2d算子的shape
 * 4、执行RunGraphAsync，给定input超出输入shape range.  [2,2,100,2]
 * 5、校验FuzzComile函数被调用了1次，表示发生kernel miss，并触发了执行时编译.
 * 
 */
TEST_F(DynamicGraphTest, TestFuzzCompileExecuteWithKernelMiss) {
  FakeFuzzCompileEngine();

  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  auto graph = BuildFuzzCompileOriginGraph();
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options[OPTION_EXEC_ENABLE_DUMP_DEBUG] = "1";
  options[OPTION_EXEC_DUMP_PATH] = "./";
  options[OPTION_EXEC_DUMP_DEBUG_MODE] = "aicore_overflow"; // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";

  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);
  

  std::vector<Tensor> inputs;
  inputs.emplace_back(CreateTensor({2,2,100,2})); // range out of fuzz result
  inputs.emplace_back(CreateTensor({2,2,100,2}));
  std::vector<Tensor> outputs;
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  EXPECT_EQ(RunGraphAsync(session, graph_id, inputs, outputs), SUCCESS);

  // check fuzz_compile called count when execute
  OpsKernelManager &kernel_manager = OpsKernelManager::GetInstance();
  auto iter = kernel_manager.GetAllOpsKernelInfoStores().find("AIcoreEngine");
  auto fuzz_compile_store = dynamic_cast<FakeFuzzCompilerOpsKernelInfoStore *>(iter->second.get());
  auto fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("conv2d");
  EXPECT_EQ(fuzz_compile_counts, 1); // input shape is out of fuzz range, so conv2d will fuzz_compile once
  session.RemoveGraph(graph_id);
}

/**
 * 测试思路
 * 准备工作：
 * 1、构造动态shape图，图中conv2d算子shape为{-2}
 * 2、不打桩gentask，即跳过gentask
 * 3、为FE的执行时泛化编译接口FuzzComile打桩
 * 
 * 执行用例：
 * 1、session配置graph option, 开启OPTION_EXEC_DYNAMIC_EXECUTE_MODE为dynamic_execute，不配置shape range。表示开启模糊编译
 *    开启ge.shape_generalized_build_mode为shape_generalized，模拟adapter在训练时开启模糊编译传入的option
 * 2、session add准备工作中构造的动态图
 * 3、执行RunGraphAsync，给定input shape.  [2,2,100,2]
 * 4、执行成功
 *    校验FuzzComile函数被调用了1次，表示-2的conv2d算子发生了在线编译.
 * 
 */
TEST_F(DynamicGraphTest, TestFuzzCompileUnknownRankLoadWithOutKernel) {
  FakeFuzzCompileEngine();
  MockOnceForOnceSkipGenerateTask("AIcoreEngine", SkipGenerateTask, GenerateTaskForTaskWithHandle);
  auto graph = BuildFuzzCompileUnknownRankGraph();
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options[OPTION_EXEC_ENABLE_DUMP_DEBUG] = "1";
  options[OPTION_EXEC_DUMP_PATH] = "./";
  options[OPTION_EXEC_DUMP_DEBUG_MODE] = "aicore_overflow"; // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";

  Session session(options);
  GraphId graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);

  std::vector<Tensor> inputs;
  inputs.emplace_back(CreateTensor({2,2,100,2}));
  std::vector<Tensor> outputs;
  EXPECT_EQ(RunGraphAsync(session, graph_id, inputs, outputs), SUCCESS);

  // check fuzz_compile called count when execute
  OpsKernelManager &kernel_manager = OpsKernelManager::GetInstance();
  auto iter = kernel_manager.GetAllOpsKernelInfoStores().find("AIcoreEngine");
  auto fuzz_compile_store = dynamic_cast<FakeFuzzCompilerOpsKernelInfoStore *>(iter->second.get());
  auto fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("conv2d");
  EXPECT_EQ(fuzz_compile_counts, 1); // input shape is out of fuzz range, so conv2d will fuzz_compile once
  session.RemoveGraph(graph_id);
}

/**
 * 测试思路
 * 准备工作：
 * 1、构造静态shape图，图中conv2d算子shape为{2,2,3,2}
 * 2、为FE的fuzz函数打桩，将图中算子shape泛化为[2,2,-1,2], range 泛化为[[2,2],[2,2,],[1,20],[2,2]]
 * 3、为FE的子图优化函数打桩，将图中算子(conv2d+relu)融合为1个ub融合算子
 *    将融合前的图作为属性打在ub融合算子上。
 * 4、为FE的执行时泛化编译接口FuzzComile打桩，泛化编译ub融合算子泛化失败
 * 
 * 执行用例：
 * 1、session配置graph option, 开启OPTION_EXEC_DYNAMIC_EXECUTE_MODE为dynamic_execute，不配置shape range。表示开启模糊编译
 *    开启ge.shape_generalized_build_mode为shape_generalized，模拟adapter在训练时开启模糊编译传入的option
 * 2、session add准备工作中构造的静态图
 * 3、执行buildgraph。此时会触发fe的fuzz函数桩。
 *    校验图中conv2d算子的shape
 *    校验图中conv2d+relu融合为一个conv2d算子，且带融合算子属性
 * 4、执行RunGraphAsync，给定input超出输入shape range.  [2,2,100,2]
 * 5、校验FuzzComile函数被调用了1次，表示发生kernel miss，并触发了执行时编译.
 * 
 */
TEST_F(DynamicGraphTest, TestFuzzCompileUBfusionExecuteSwitchToOriginGraphExecution) {
  FakeFuzzCompileEngineForUbFusion();
  MockForGenerateTask("AIcoreEngine", GenerateTaskForTaskWithHandle);
  auto graph_with_ubfusion = BuildFuzzCompileOriginGraphWithUBfusion();
  std::map<AscendString, AscendString> options;
  options[OPTION_GRAPH_RUN_MODE] = "1";  // train
  options[OPTION_EXEC_ENABLE_DUMP_DEBUG] = "1";
  options[OPTION_EXEC_DUMP_PATH] = "./";
  options[OPTION_EXEC_DUMP_DEBUG_MODE] = "aicore_overflow"; // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";

  Session session(options);
  GraphId graph_id = 2;
  EXPECT_EQ(session.AddGraph(graph_id, graph_with_ubfusion, graph_options), SUCCESS);
  
  std::vector<Tensor> inputs;
  inputs.emplace_back(CreateTensor({2,20,300,2})); // range out of fuzz result
  inputs.emplace_back(CreateTensor({2,20,300,2}));
  std::vector<Tensor> outputs;

  EXPECT_EQ(RunGraphAsync(session, graph_id, inputs, outputs), SUCCESS);

  // check fuzz_compile called count when execute
  OpsKernelManager &kernel_manager = OpsKernelManager::GetInstance();
  auto iter = kernel_manager.GetAllOpsKernelInfoStores().find("AIcoreEngine");
  auto fuzz_compile_store = dynamic_cast<FakeFuzzCompilerOpsKernelInfoStore *>(iter->second.get());
  auto fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("conv2d_fused");
  EXPECT_EQ(fuzz_compile_counts, 1); // input shape is out of fuzz range, so conv2d_fused will fuzz_compile once
  auto conv2d_in_sub_fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("conv2d");
  EXPECT_EQ(conv2d_in_sub_fuzz_compile_counts, 1);
  auto relu_in_sub_fuzz_compile_counts = fuzz_compile_store->GetNodeFuzzCompileCount("relu");
  EXPECT_EQ(relu_in_sub_fuzz_compile_counts, 1); //fuse node fuzz failed, switch to origin graph execution, so relu will fuzz once
  session.RemoveGraph(graph_id);
}

const static std::vector<int64_t> val_list_int;
const static std::vector<bool> val_list_bool;
const static std::vector<float> val_list_float;
const static std::vector<std::string> val_list_str;
const static std::vector<DataType> val_list_dt;
const static std::vector<std::vector<int64_t>> val_list_list_int;
const static std::vector<std::vector<float>> val_list_list_float;
const static NamedAttrs var_name_attr;

REG_OP(TestAllAttr)
    .INPUT(data, TensorType::ALL())
    .OPTIONAL_INPUT(option_input, TensorType::ALL())
    .OUTPUT(out, TensorType::ALL())
    .ATTR(test_str, String, "")
    .ATTR(test_int, Int, 0)
    .ATTR(test_bool, Bool, false)
    .ATTR(test_float, Float, 0.0)
    .ATTR(test_dt, Type, DT_FLOAT)
    .ATTR(test_list_string, ListString, val_list_str)
    .ATTR(test_list_int, ListInt, val_list_int)
    .ATTR(test_list_bool, ListBool, val_list_bool)
    .ATTR(test_list_float, ListFloat, val_list_float)
    .ATTR(test_list_dt, ListType, val_list_dt)
    .ATTR(test_name_attr, NamedAttrs, var_name_attr)
    .OP_END_FACTORY_REG(TestAllAttr)

TEST_F(DynamicGraphTest, TestCcmAllAttr) {
  std::vector<int64_t> shape{1, 2};
  auto graph = std::make_shared<ComputeGraph>("fake_graph");
  auto tensor_desc = std::make_shared<GeTensorDesc>(GeShape(shape));
  auto op_desc = std::make_shared<OpDesc>("TestAllAttr", "TestAllAttr");
  ASSERT_NE(op_desc, nullptr);
  op_desc->AddInputDesc(tensor_desc->Clone());
  op_desc->AddOptionalInputDesc("option_input", GeTensorDesc(GeShape(), FORMAT_RESERVED, DT_UNDEFINED));
  op_desc->AddOutputDesc(tensor_desc->Clone());
  ASSERT_EQ(op_desc->GetAllInputsSize(), 2);
  ASSERT_EQ(op_desc->GetInputsSize(), 1);
  ASSERT_NE(op_desc->MutableInputDesc(0), nullptr);
  ASSERT_EQ(op_desc->MutableInputDesc(1), nullptr);
  AttrUtils::SetInt(op_desc, "test_int", 0);
  AttrUtils::SetStr(op_desc, "test_str", "");
  AttrUtils::SetBool(op_desc, "test_bool", false);
  AttrUtils::SetFloat(op_desc, "test_float", 0.0);
  AttrUtils::SetDataType(op_desc, "test_dt", DT_FLOAT);
  std::vector<DataType> val_list_dt{DT_FLOAT};
  AttrUtils::SetListDataType(op_desc, "test_list_dt", val_list_dt);
  std::vector<bool> val_list_bool{true};
  AttrUtils::SetListBool(op_desc, "test_list_bool", val_list_bool);
  std::vector<int64_t> val_list_int{1,2};
  AttrUtils::SetListInt(op_desc, "test_list_int", val_list_int);
  std::vector<float> val_list_float{1.0, 2.0};
  AttrUtils::SetListFloat(op_desc, "test_list_float", val_list_float);
  std::vector<std::string> val_list_string{"1", "2"};
  AttrUtils::SetListStr(op_desc, "test_list_string", val_list_string);
  std::vector<std::vector<int64_t>> val_list_list_int{{1,2}};
  AttrUtils::SetListListInt(op_desc, "test_list_list_int", val_list_list_int);
  NamedAttrs name_attr;
  AttrUtils::SetNamedAttrs(op_desc, "test_name_attr", name_attr);
  NodeCompileCacheItem item;
  NodeCompileCacheModule ccm;
  auto node = graph->AddNode(op_desc);
  ASSERT_NE(node, nullptr);
  auto add_item = ccm.AddCompileCache(node, item);
  ASSERT_NE(add_item, nullptr);
  auto find_item = ccm.FindCompileCache(node);
  ASSERT_NE(find_item, nullptr);
  ASSERT_EQ(add_item->GetCacheItemId(), find_item->GetCacheItemId());
}

}  // namespace ge