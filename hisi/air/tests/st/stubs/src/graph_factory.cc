/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include "utils/graph_factory.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/debug/ge_attr_define.h"
#include "common/types.h"
#include "common/kernel_store.h"
#include "ge_running_env/tensor_utils.h"
#include "graph/utils/graph_utils.h"

FAKE_NS_BEGIN
namespace {
void SetMemTypeForHostMemInput(ComputeGraphPtr &graph) {
  for (const auto& node : graph->GetAllNodes()) {
    if (node == nullptr || node->GetOpDesc() == nullptr) {
      continue;
    }
    for (size_t i = 0U; i < node->GetOpDesc()->GetAllInputsSize(); ++i) {
      const GeTensorDescPtr &input_desc = node->GetOpDesc()->MutableInputDesc(static_cast<uint32_t>(i));
      if (input_desc == nullptr) {
        continue;
      }
      int64_t mem_type = 0;
      (void)ge::AttrUtils::SetInt(*input_desc, ge::ATTR_NAME_PLACEMENT, 2);
    }
  }
}
} // namespace

Graph GraphFactory::SingeOpGraph(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("data1");

    auto transdata = OP_CFG("MyTransdata")
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("support_dynamicshape", true)
        .Attr("tvm_magic", "RT_DEV_BINARY_MAGIC_ELF")
        .Build("transdata1");

    auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput");
    CHAIN(NODE(op_ptr)->NODE(transdata)->NODE(netoutput));
  };
  return ToGeGraph(dynamic_op);
}

Graph GraphFactory::SingeOpGraph2(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("data1");

    auto transdata = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "aicpu_tf_kernel")
        .Attr("op_para_size", 1)
        .Build("transdata1");

    auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput");
    CHAIN(NODE(op_ptr)->NODE(transdata)->NODE(netoutput));
  };
  return ToGeGraph(dynamic_op);
}

Graph GraphFactory::SingeOpGraph3(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("data2");

    auto reduce_sum = OP_CFG(REDUCESUM)
        .InCnt(2)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("_op_infer_depends", std::vector<std::string>({"__input1"}))
        .Attr("support_dynamicshape", true)
        .Attr("op_para_size", 1)
        .Build("reducesum");

    auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput");
    CHAIN(NODE(op_ptr)->NODE(reduce_sum));
    CHAIN(NODE(op_ptr2)->NODE(reduce_sum)->NODE(netoutput));
  };
  return ToGeGraph(dynamic_op);
}

Graph GraphFactory::HybridSingeOpGraphForHostMemInput() {
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .TensorDesc(FORMAT_ND, DT_UINT64, {4})
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .TensorDesc(FORMAT_ND, DT_UINT64, {4})
        .Attr("_force_unknown_shape", true)
        .Build("data2");

    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr("_force_infershape_when_running", true)
        .Attr("support_dynamicshape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("transdata1");

    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("transdata2");

    auto matmul = OP_CFG(MATMUL)
        .InCnt(2)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .TensorDesc(FORMAT_ND, DT_UINT64, {4})
        .Build("matmul");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .TensorDesc(FORMAT_ND, DT_UINT64, {4})
        .Build("net_output");

    CHAIN(NODE(op_ptr)->NODE(transdata1)->NODE(matmul)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->NODE(transdata2)->NODE(matmul));
  };
  ComputeGraphPtr graph_ptr = ToComputeGraph(dynamic_op);
  SetMemTypeForHostMemInput(graph_ptr);
  return ToGeGraph(dynamic_op);
}

///         net_output
///             |
///           matmul
///       /           \
///      /             \
///  transdata1    transdata2
///     |              |
///   op_ptr         op_ptr2
Graph GraphFactory::HybridSingeOpGraphAicpuForHostMemInput(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr(ATTR_NAME_INDEX, 0)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr(ATTR_NAME_INDEX, 1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("data2");

    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr("_ge_attr_op_kernel_lib_name", "aicpu_ascend_kernel")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr("_force_infershape_when_running", true)
        .Attr("support_dynamicshape", true)
        .Attr("_support_blockdim_flag", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("transdata1");

    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr("_ge_attr_op_kernel_lib_name", "aicpu_ascend_kernel")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Attr("_support_blockdim_flag", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("transdata2");

    auto matmul = OP_CFG(MATMUL)
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Attr(ATTR_NAME_PLACEMENT, 2)
        .Build("matmul");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1})
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("net_output");

    CHAIN(NODE(op_ptr)->NODE(transdata1)->NODE(matmul)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->NODE(transdata2)->NODE(matmul));
  };
  ComputeGraphPtr graph_ptr = ToComputeGraph(dynamic_op);
  SetMemTypeForHostMemInput(graph_ptr);
  return ToGeGraph(dynamic_op);
}

Graph GraphFactory::HybridSingeOpGraph(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data2");

    auto transdata1 = OP_CFG("MyTransdata")
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr("_force_infershape_when_running", true)
        .Attr("support_dynamicshape", true)
        .Build("transdata1");

    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Build("transdata2");

    auto matmul = OP_CFG(MATMUL)
        .InCnt(2)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .TensorDesc(FORMAT_ND, DT_UINT64, {4})
        .Build("matmul");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("net_output");

    CHAIN(NODE(op_ptr)->NODE(transdata1)->NODE(matmul)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->NODE(transdata2)->NODE(matmul));
  };

  return ToGeGraph(dynamic_op);
}

Graph GraphFactory::HybridSingeOpGraph2(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data2");

    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr("_force_infershape_when_running", true)
        .Attr("support_dynamicshape", true)
        .Build("transdata1");

    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Build("transdata2");

    auto matmul = OP_CFG(MATMUL)
        .InCnt(2)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Build("matmul");

    auto reshape = OP_CFG("Reshape")
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Build("reshape");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("net_output");

    CHAIN(NODE(op_ptr)->NODE(transdata1)->NODE(matmul)->NODE(reshape)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->NODE(transdata2)->NODE(matmul));
  };

  return ToGeGraph(dynamic_op);
}

///         net_output  
///             |    
///           matmul
///       /           \
///      /             \
///  transdata1    transdata2
///     |              |
///   op_ptr         op_ptr2
Graph GraphFactory::BuildAicpuSingeOpGraph(){
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, -1, -1})
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, -1, -1})
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Build("data2");

    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, -1, -1})
        .Attr("_ge_attr_op_kernel_lib_name", "aicpu_ascend_kernel")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_unknown_shape", true)
        .Attr("_force_infershape_when_running", true)
        .Attr("support_dynamicshape", true)
        .Attr("_support_blockdim_flag", true)
        .Build("transdata1");

    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, -1, -1})
        .Attr("_ge_attr_op_kernel_lib_name", "aicpu_ascend_kernel")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Attr("_support_blockdim_flag", true)
        .Build("transdata2");

    auto matmul = OP_CFG(MATMUL)
        .InCnt(2)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "AIcoreEngine")
        .Attr("op_para_size", 1)
        .Attr("compile_info_key", "ddd")
        .Attr("compile_info_json", "cccc")
        .Attr("_force_infershape_when_running", true)
        .Attr("_force_unknown_shape", true)
        .Attr("support_dynamicshape", true)
        .Build("matmul");

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
        .Build("net_output");

    CHAIN(NODE(op_ptr)->NODE(transdata1)->NODE(matmul)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->NODE(transdata2)->NODE(matmul));
  };

  return ToGeGraph(dynamic_op);
}

/*
 *    out0            out1
 *     |               |
 *   assign1        assign2
 *    /  \           /  \
 * var1  const1   var2  const2
 */
Graph GraphFactory::BuildVarInitGraph1() {
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor = GenerateTensor(shape);
  DEF_GRAPH(var_init) {
    auto var1 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var1");
    auto var2 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var2");
    auto assign1 = OP_CFG(ASSIGN)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InNames({"ref", "value"})
        .OutNames({"ref"})
        .Build("assign1");
    auto assign2 = OP_CFG(ASSIGN)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InNames({"ref", "value"})
        .OutNames({"ref"})
        .Build("assign2");
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .Weight(data_tensor)
        .InCnt(1)
        .OutCnt(1)
        .Build("const1");
    auto const2 = OP_CFG(CONSTANT)
        .Weight(data_tensor).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("const2");

    CHAIN(NODE(var1)->NODE(assign1));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(assign1));
    CHAIN(NODE(var2)->NODE(assign2));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE(assign2));

    ADD_OUTPUT(assign1, 0);
    ADD_OUTPUT(assign2, 0);
  };

  return ToGeGraph(var_init);
}

/*
 * out0   out1
 *  |      |
 * var1   var2
 */
Graph GraphFactory::BuildCheckpointGraph1() {
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  DEF_GRAPH(graph) {
    auto var1 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var1");
    auto var2 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var2");

    ADD_OUTPUT(var1, 0);
    ADD_OUTPUT(var2, 0);
  };
  return ToGeGraph(graph);
}

/*
 *     out0       out1
 *      |          |
 *   conv2d1      add1
 *    /  \        /  \
 * data1 var1  data2 var2
 */
Graph GraphFactory::BuildVarTrainGraph1() {
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  DEF_GRAPH(graph) {
    auto data1 = OP_DATA(0)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {8,3,16,16})
        .Build("data1");
    auto data2 = OP_DATA(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {})
        .Build("data2");

    auto var1 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var1");
    auto var2 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var2");

    auto conv2d1 = OP_CFG(CONV2D)
        .InCnt(2)
        .OutCnt(1)
        .Build("conv2d1");
    conv2d1->MutableInputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    conv2d1->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
    conv2d1->MutableInputDesc(1)->SetOriginFormat(FORMAT_HWCN);
    conv2d1->MutableInputDesc(1)->SetFormat(FORMAT_HWCN);
    conv2d1->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    conv2d1->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);

    auto add1 = OP_CFG(ADD)
        .InCnt(2)
        .OutCnt(1)
        .Build("add1");

    CHAIN(NODE(data1)->NODE(conv2d1));
    CHAIN(NODE(var1)->EDGE(0, 1)->NODE(conv2d1));
    CHAIN(NODE(data2)->NODE(add1));
    CHAIN(NODE(var2)->EDGE(0, 1)->NODE(add1));

    ADD_OUTPUT(conv2d1, 0);
    ADD_OUTPUT(add1, 0);
  };

  return ToGeGraph(graph);
}

/*
 *          netoutput
 *         /c      c\
 *   assign1        assign2
 *    /  \           /  \
 * var1  const1   var2  const2
 */
Graph GraphFactory::BuildVarWriteNoOutputRefGraph1() {
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor = GenerateTensor(shape);
  DEF_GRAPH(var_init) {
    auto var1 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var1");
    auto var2 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var2");
    auto assign1 = OP_CFG(ASSIGNVARIABLEOP)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(2)
        .Build("assign1");
    auto assign2 = OP_CFG(ASSIGNVARIABLEOP)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(2)
        .Build("assign2");
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .Weight(data_tensor)
        .InCnt(1)
        .OutCnt(1)
        .Build("const1");
    auto const2 = OP_CFG(CONSTANT)
        .Weight(data_tensor).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("const2");


    assign1->MutableInputDesc(0)->SetRefPortByIndex(std::vector<uint32_t>({0}));
    assign2->MutableInputDesc(0)->SetRefPortByIndex(std::vector<uint32_t>({0}));

    CHAIN(NODE(var1)->NODE(assign1));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(assign1));
    CHAIN(NODE(var2)->NODE(assign2));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE(assign2));

    ADD_TARGET(assign1);
    ADD_TARGET(assign2);
  };

  return ToGeGraph(var_init);
}

/*
 *                     (out1)   (out2)       +---------------------+
 *                      |         |          |                     |
 * +------- add1       exit1    exit2      mul1                    |
 * |       /    \T    /F          \F     /T    \                   |
 * | const2       switch1         switch2     data2                |
 * |                \    \       /      \                          |
 * |                 \    loopcond     merge2 <-- nextiteration2 --+
 * |                  \        |           \
 * |                   \     less1        enter2
 * |                    \   /    \          |
 * +-> nextiteration1 -> merge1  data1    const3
 *                        |
 *                     enter1
 *                       |
 *                     const1
 */
Graph GraphFactory::BuildV1LoopGraph1() {
  DEF_GRAPH(g) {
    auto const_data1 = GenerateTensor(DT_INT32, {});
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data1)
        .OutCnt(1)
        .Build("const1");
    auto enter1 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter1");
    auto merge1 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("merge1");
    auto nextiteration1 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("nextiteration1");
    auto data1 = OP_DATA(0)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Build("data1");
    auto const_data3 = GenerateTensor({8,3,224,224});
    auto const3 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data3)
        .OutCnt(1)
        .Build("const3");
    auto less1 = OP_CFG(LESS)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("less1");
    auto enter2 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("enter2");
    auto loopcond1 = OP_CFG(LOOPCOND)
        .TensorDesc(FORMAT_ND, DT_BOOL, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("loopcond1");
    auto merge2 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2)
        .Build("merge2");
    auto nextiteration2 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("nextiteration2");
    auto switch1 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("switch1");
    auto const_data2 = GenerateTensor(DT_INT32, {});
    auto const2 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data2)
        .OutCnt(1)
        .Build("const2");
    auto switch2 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2).Build("switch2");
    auto data2 = OP_DATA(1)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Build("data2");
    auto mul1 = OP_CFG(MUL)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(1).Build("mul1");
    auto add1 = OP_CFG(ADD)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("add1");
    auto exit1 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit1");
    auto exit2 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit2");

    less1->MutableOutputDesc(0)->SetDataType(DT_BOOL);
    less1->MutableOutputDesc(0)->SetOriginDataType(DT_BOOL);

    CHAIN(NODE(const1)->NODE(enter1)->NODE(merge1)->NODE(switch1)->EDGE(1, 1)->NODE(add1)->NODE(nextiteration1)->EDGE(0, 1)->NODE(merge1));
    CHAIN(NODE(const2)->EDGE(0, 0)->NODE(add1));
    CHAIN(NODE(merge1)->NODE(less1)->NODE(loopcond1)->EDGE(0, 1)->NODE(switch1)->NODE(exit1));
    CHAIN(NODE(data1)->EDGE(0, 1)->NODE(less1));
    CHAIN(NODE(loopcond1)->EDGE(0, 1)->NODE(switch2)->NODE(exit2));
    CHAIN(NODE(const3)->NODE(enter2)->NODE(merge2)->EDGE(0, 0)->NODE(switch2)->EDGE(1, 0)->NODE(mul1)->NODE(nextiteration2)->EDGE(0, 1)->NODE(merge2));
    CHAIN(NODE(data2)->EDGE(0, 1)->NODE(mul1));

    ADD_OUTPUT(exit1, 0);
    ADD_OUTPUT(exit2, 0);
  };
  return ToGeGraph(g);
}

/*
 *                     (out1)   (out2)       +------------------------------+
 *                      |         |          |     c                        |
 * +------- add1       exit1    exit2      mul1 <------ enter3 <---- const4 |
 * |       /    \T    /F          \F     /T    \                            |
 * | const2       switch1         switch2     data2                         |
 * |                \    \       /      \                                   |
 * |                 \    loopcond     merge2 <-- nextiteration2 -----------+
 * |                  \        |           \
 * |                   \     less1        enter2
 * |                    \   /    \          |
 * +-> nextiteration1 -> merge1  data1    const3
 *                        |
 *                     enter1
 *                       |
 *                     const1
 */
Graph GraphFactory::BuildV1LoopGraph2_CtrlEnterIn() {
  DEF_GRAPH(g) {
    auto const_data1 = GenerateTensor(DT_INT32, {});
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data1)
        .OutCnt(1)
        .Build("const1");
    auto enter1 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter1");
    auto merge1 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("merge1");
    auto nextiteration1 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("nextiteration1");
    auto data1 = OP_DATA(0)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Build("data1");
    auto const_data3 = GenerateTensor({8,3,224,224});
    auto const3 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data3)
        .OutCnt(1)
        .Build("const3");
    auto less1 = OP_CFG(LESS)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("less1");
    auto enter2 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("enter2");
    auto loopcond1 = OP_CFG(LOOPCOND)
        .TensorDesc(FORMAT_ND, DT_BOOL, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("loopcond1");
    auto merge2 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2)
        .Build("merge2");
    auto nextiteration2 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("nextiteration2");
    auto switch1 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("switch1");
    auto const_data2 = GenerateTensor(DT_INT32, {});
    auto const2 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data2)
        .OutCnt(1)
        .Build("const2");
    auto switch2 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2).Build("switch2");
    auto data2 = OP_CFG(DATA)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1).Build("data2");
    auto mul1 = OP_CFG(MUL)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(1).Build("mul1");
    auto add1 = OP_CFG(ADD)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("add1");
    auto exit1 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit1");
    auto exit2 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit2");
    auto const_data4 = GenerateTensor({8,3,224,224});
    auto const4 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data4)
        .OutCnt(1)
        .Build("const4");
    auto enter3 = OP_CFG(ENTER)
        .Attr("is_constant", true)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter3");

    less1->MutableOutputDesc(0)->SetDataType(DT_BOOL);
    less1->MutableOutputDesc(0)->SetOriginDataType(DT_BOOL);

    CHAIN(NODE(const1)->NODE(enter1)->NODE(merge1)->NODE(switch1)->EDGE(1, 1)->NODE(add1)->NODE(nextiteration1)->EDGE(0, 1)->NODE(merge1));
    CHAIN(NODE(const2)->EDGE(0, 0)->NODE(add1));
    CHAIN(NODE(merge1)->NODE(less1)->NODE(loopcond1)->EDGE(0, 1)->NODE(switch1)->NODE(exit1));
    CHAIN(NODE(data1)->EDGE(0, 1)->NODE(less1));
    CHAIN(NODE(loopcond1)->EDGE(0, 1)->NODE(switch2)->NODE(exit2));
    CHAIN(NODE(const3)->NODE(enter2)->NODE(merge2)->EDGE(0, 0)->NODE(switch2)->EDGE(1, 0)->NODE(mul1)->NODE(nextiteration2)->EDGE(0, 1)->NODE(merge2));
    CHAIN(NODE(data2)->EDGE(0, 1)->NODE(mul1));
    CHAIN(NODE(const4)->NODE(enter3));
    CTRL_CHAIN(NODE(enter3)->NODE(mul1));

    ADD_OUTPUT(exit1, 0);
    ADD_OUTPUT(exit2, 0);
  };
  return ToGeGraph(g);
}

/*
 *                     (out1)   (out2)       +------------------------------+
 *                      |         |          |                              |
 * +------- add1       exit1    exit2      mul1 <------ enter3 <---- const4 |
 * |       /    \T    /F          \F     /T                                 |
 * | const2       switch1         switch2                                   |
 * |                \    \       /      \                                   |
 * |                 \    loopcond     merge2 <-- nextiteration2 -----------+
 * |                  \        |           \
 * |                   \     less1        enter2
 * |                    \   /    \          |
 * +-> nextiteration1 -> merge1  data1    const3
 *                        |
 *                     enter1
 *                       |
 *                     const1
 */
Graph GraphFactory::BuildV1LoopGraph4_DataEnterInByPassMerge() {
  DEF_GRAPH(g) {
    auto const_data1 = GenerateTensor(DT_INT32, {});
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data1)
        .OutCnt(1)
        .Build("const1");
    auto enter1 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter1");
    auto merge1 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("merge1");
    auto nextiteration1 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("nextiteration1");
    auto data1 = OP_DATA(0)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Build("data1");
    auto const_data3 = GenerateTensor({8,3,224,224});
    auto const3 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data3)
        .OutCnt(1)
        .Build("const3");
    auto less1 = OP_CFG(LESS)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("less1");
    auto enter2 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("enter2");
    auto loopcond1 = OP_CFG(LOOPCOND)
        .TensorDesc(FORMAT_ND, DT_BOOL, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("loopcond1");
    auto merge2 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2)
        .Build("merge2");
    auto nextiteration2 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("nextiteration2");
    auto switch1 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("switch1");
    auto const_data2 = GenerateTensor(DT_INT32, {});
    auto const2 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data2)
        .OutCnt(1)
        .Build("const2");
    auto switch2 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2).Build("switch2");
    auto mul1 = OP_CFG(MUL)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(1).Build("mul1");
    auto add1 = OP_CFG(ADD)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("add1");
    auto exit1 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit1");
    auto exit2 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit2");
    auto const_data4 = GenerateTensor({8,3,224,224});
    auto const4 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data4)
        .OutCnt(1)
        .Build("const4");
    auto enter3 = OP_CFG(ENTER)
        .Attr("is_constant", true)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter3");

    less1->MutableOutputDesc(0)->SetDataType(DT_BOOL);
    less1->MutableOutputDesc(0)->SetOriginDataType(DT_BOOL);

    CHAIN(NODE(const1)->NODE(enter1)->NODE(merge1)->NODE(switch1)->EDGE(1, 1)->NODE(add1)->NODE(nextiteration1)->EDGE(0, 1)->NODE(merge1));
    CHAIN(NODE(const2)->EDGE(0, 0)->NODE(add1));
    CHAIN(NODE(merge1)->NODE(less1)->NODE(loopcond1)->EDGE(0, 1)->NODE(switch1)->NODE(exit1));
    CHAIN(NODE(data1)->EDGE(0, 1)->NODE(less1));
    CHAIN(NODE(loopcond1)->EDGE(0, 1)->NODE(switch2)->NODE(exit2));
    CHAIN(NODE(const3)->NODE(enter2)->NODE(merge2)->EDGE(0, 0)->NODE(switch2)->EDGE(1, 0)->NODE(mul1)->NODE(nextiteration2)->EDGE(0, 1)->NODE(merge2));
    CHAIN(NODE(const4)->NODE(enter3)->EDGE(0, 1)->NODE(mul1));

    ADD_OUTPUT(exit1, 0);
    ADD_OUTPUT(exit2, 0);
  };
  return ToGeGraph(g);
}

Graph GraphFactory::BuildGraphForMergeShapeNPass() {
  DEF_GRAPH(g) {
    auto data1 = OP_CFG(DATA).InCnt(1).OutCnt(1).Build("data1");
    auto data2 = OP_CFG(DATA).InCnt(1).OutCnt(1).Build("data2");
    auto relu1 = OP_CFG(RELU).InCnt(1).InCnt(1).TensorDesc(FORMAT_ND, DT_INT32, {}).Build("relu1");
    auto relu2 = OP_CFG(RELU).InCnt(1).InCnt(1).TensorDesc(FORMAT_ND, DT_INT32, {}).Build("relu2");
    auto shapeN1 = OP_CFG(SHAPEN).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_INT32, {}).Build("shapeN1");
    auto shapeN2 = OP_CFG(SHAPEN).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_INT32, {}).Build("shapeN2");
    auto relu3 = OP_CFG(RELU).InCnt(2).InCnt(1).Build("relu3");
    CHAIN(NODE(data1)->NODE(relu1)->NODE(shapeN1)->EDGE(0,0)->NODE(relu3)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE(data2)->NODE(relu2)->NODE(shapeN2)->EDGE(0,1)->NODE(relu3));
    CTRL_CHAIN(NODE(relu1)->NODE(shapeN1));
    CTRL_CHAIN(NODE(relu2)->NODE(shapeN2));
    CTRL_CHAIN(NODE(shapeN1)->NODE(relu3));
    CTRL_CHAIN(NODE(shapeN2)->NODE(relu3));
  };
  auto graph = ToGeGraph(g);
  auto compute_graph = ge::GraphUtils::GetComputeGraph(graph);
  auto node1 = compute_graph->FindNode("data1");
  auto input_desc_data1 = node1->GetOpDesc()->MutableOutputDesc(0);
  input_desc_data1->SetShape(GeShape({-1, -1, 2, 3}));

  auto node2 = compute_graph->FindNode("relu1");
  auto input_desc_relu1 = node2->GetOpDesc()->MutableInputDesc(0);
  input_desc_relu1->SetShape(GeShape({-1, -1, 2, 3}));
  auto output_desc_relu1 = node2->GetOpDesc()->MutableOutputDesc(0);
  output_desc_relu1->SetShape(GeShape({-1, -1, 2, 3}));

  auto node3 = compute_graph->FindNode("shapeN1");
  OpDescPtr op_desc_ptr = node3->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, "_split_shapen_origin_name", "ShapeN_unknown");
  auto input_desc_shapeN1 = node3->GetOpDesc()->MutableInputDesc(0);
  input_desc_shapeN1->SetShape(GeShape({-1, -1, 16, 16}));
  auto output_desc_shapeN1 = node3->GetOpDesc()->MutableOutputDesc(0);
  output_desc_shapeN1->SetShape(GeShape({-1, -1, 16, 16}));

  auto node4 = compute_graph->FindNode("relu3");
  auto input_desc_relu3 = node4->GetOpDesc()->MutableInputDesc(0);
  auto input_desc_relu3_2 = node4->GetOpDesc()->MutableInputDesc(1);
  input_desc_relu3->SetShape(GeShape({-1, -1, 2, 3}));
  input_desc_relu3_2->SetShape(GeShape({-1, -1, 2, 3}));
  auto output_desc_relu3 = node4->GetOpDesc()->MutableOutputDesc(0);
  output_desc_relu3->SetShape(GeShape({-1, -1, 2, 3}));

  auto node5 = compute_graph->FindNode("netoutput1");
  auto input_desc_netoutput1 = node5->GetOpDesc()->MutableInputDesc(0);
  input_desc_netoutput1->SetShape(GeShape({-1, 1, 2, 3}));

  auto node6 = compute_graph->FindNode("data2");
  auto input_desc_data2 = node6->GetOpDesc()->MutableOutputDesc(0);
  input_desc_data2->SetShape(GeShape({-1, -1, 2, 3}));

  auto node7 = compute_graph->FindNode("relu2");
  auto input_desc_relu2 = node7->GetOpDesc()->MutableInputDesc(0);
  input_desc_relu2->SetShape(GeShape({-1, -1, 2, 3}));
  auto output_desc_relu2 = node7->GetOpDesc()->MutableOutputDesc(0);
  output_desc_relu2->SetShape(GeShape({-1, -1, 2, 3}));

  auto node8 = compute_graph->FindNode("shapeN2");
  OpDescPtr op_desc_ptr1 = node8->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr1, "_split_shapen_origin_name", "ShapeN_unknown");
  auto input_desc_shapeN2 = node8->GetOpDesc()->MutableInputDesc(0);
  input_desc_shapeN2->SetShape(GeShape({-1, -1, 16, 16}));
  auto output_desc_shapeN2 = node8->GetOpDesc()->MutableOutputDesc(0);
  output_desc_shapeN2->SetShape(GeShape({-1, -1, 16, 16}));

  return graph;
}

/*
 *                     (out1)   (out2)              sub1 -------------------------+
 *                      |         |             /         \                       |
 * +------- add1       exit1    exit2      mul1            \                      |
 * |       /    \T    /F          \F     /T    \            \                     |
 * | const2       switch1         switch2     const4 <--c--- enter3 <---- const5  |
 * |                \    \       /      \                                         |
 * |                 \    loopcond     merge2 <-- nextiteration2 -----------------+
 * |                  \        |           \
 * |                   \     less1        enter2
 * |                    \   /    \          |
 * +-> nextiteration1 -> merge1  data1    const3
 *                        |
 *                     enter1
 *                       |
 *                     const1
 */
Graph GraphFactory::BuildV1LoopGraph3_CtrlEnterIn2() {
  DEF_GRAPH(g) {
    auto const_data1 = GenerateTensor(DT_INT32, {});
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data1)
        .OutCnt(1)
        .Build("const1");
    auto enter1 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter1");
    auto merge1 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("merge1");
    auto nextiteration1 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("nextiteration1");
    auto data1 = OP_DATA(0)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Build("data1");
    auto const_data3 = GenerateTensor({8,3,224,224});
    auto const3 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data3)
        .OutCnt(1)
        .Build("const3");
    auto less1 = OP_CFG(LESS)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("less1");
    auto enter2 = OP_CFG(ENTER)
        .Attr("is_constant", false)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("enter2");
    auto loopcond1 = OP_CFG(LOOPCOND)
        .TensorDesc(FORMAT_ND, DT_BOOL, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("loopcond1");
    auto merge2 = OP_CFG(MERGE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2)
        .Build("merge2");
    auto nextiteration2 = OP_CFG(NEXTITERATION)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1).OutCnt(1)
        .Build("nextiteration2");
    auto switch1 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(2)
        .Build("switch1");
    auto const_data2 = GenerateTensor(DT_INT32, {});
    auto const2 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .Weight(const_data2)
        .OutCnt(1)
        .Build("const2");
    auto switch2 = OP_CFG(SWITCH)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(2).Build("switch2");
    auto mul1 = OP_CFG(MUL)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2).OutCnt(1).Build("mul1");
    auto add1 = OP_CFG(ADD)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(2)
        .OutCnt(1)
        .Build("add1");
    auto exit1 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_INT32, {})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit1");
    auto exit2 = OP_CFG(EXIT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1)
        .OutCnt(1)
        .Build("exit2");
    auto const_data4 = GenerateTensor({8,3,224,224});
    auto const4 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data4)
        .OutCnt(1)
        .Build("const4");
    auto const_data5 = GenerateTensor({8,3,224,224});
    auto const5 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .Weight(const_data5)
        .OutCnt(1)
        .Build("const5");
    auto enter3 = OP_CFG(ENTER)
        .Attr("is_constant", true)
        .Attr("frame_name", "While/While_context")
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(1)
        .OutCnt(1)
        .Build("enter3");
    auto sub1 = OP_CFG(SUB)
        .TensorDesc(FORMAT_ND, DT_FLOAT, {8,3,224,224})
        .InCnt(2)
        .OutCnt(1)
        .Build("sub1");

    less1->MutableOutputDesc(0)->SetDataType(DT_BOOL);
    less1->MutableOutputDesc(0)->SetOriginDataType(DT_BOOL);

    CHAIN(NODE(const1)->NODE(enter1)->NODE(merge1)->NODE(switch1)->EDGE(1, 1)->NODE(add1)->NODE(nextiteration1)->EDGE(0, 1)->NODE(merge1));
    CHAIN(NODE(const2)->EDGE(0, 0)->NODE(add1));
    CHAIN(NODE(merge1)->NODE(less1)->NODE(loopcond1)->EDGE(0, 1)->NODE(switch1)->NODE(exit1));
    CHAIN(NODE(data1)->EDGE(0, 1)->NODE(less1));
    CHAIN(NODE(loopcond1)->EDGE(0, 1)->NODE(switch2)->NODE(exit2));
    CHAIN(NODE(const3)->NODE(enter2)->NODE(merge2)->EDGE(0, 0)->NODE(switch2)->EDGE(1, 0)->NODE(mul1)->NODE(sub1)->NODE(nextiteration2)->EDGE(0, 1)->NODE(merge2));
    CHAIN(NODE(const4)->EDGE(0, 1)->NODE(mul1));
    CHAIN(NODE(const5)->NODE(enter3)->EDGE(0, 1)->NODE(sub1));

    CTRL_CHAIN(NODE(enter3)->NODE(const4));

    ADD_OUTPUT(exit1, 0);
    ADD_OUTPUT(exit2, 0);
  };
  return ToGeGraph(g);
}

FAKE_NS_END