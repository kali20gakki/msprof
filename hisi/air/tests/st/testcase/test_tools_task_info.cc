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

#include "test_tools_task_info.h"

#include "gtest/gtest.h"
#include "framework/common/types.h"
#include "framework/common/taskdown_common.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/build/memory/var_mem_assign_util.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"
#include "toolchain/prof_common.h"

namespace ge {
std::set<std::string> actual_info_type = {};

namespace {
class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
 public:
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

  Status LoadTask(GETaskInfo &task) override {
    return SUCCESS;
  }

  Status UnloadTask(GETaskInfo &task) override {
    return SUCCESS;
  }
};
FakeOpsKernelInfoStore g_fake_ops_kernel_info_store;
} // namespace

static void AddSubGraph(const NodePtr &func_node, const ComputeGraphPtr &subgraph) {
  size_t index = func_node->GetOpDesc()->GetSubgraphInstanceNames().size();
  func_node->GetOpDesc()->AddSubgraphName(subgraph->GetName());
  func_node->GetOpDesc()->SetSubgraphInstanceName(index, subgraph->GetName());

  NodeUtils::SetSubgraph(*func_node, index, subgraph);
}

void AddPartitionedCall(const ComputeGraphPtr &graph, const std::string &func_name, const ComputeGraphPtr &subgraph) {
  const auto &func_node = graph->FindNode(func_name);
  EXPECT_NE(func_node, nullptr);

  func_node->GetOpDesc()->RegisterSubgraphIrName("f", SubgraphType::kStatic);
  AddSubGraph(func_node, subgraph);
}

void AddIfBranchs(const ComputeGraphPtr &graph, const std::string &func_name,
                  const ComputeGraphPtr &then_graph, const ComputeGraphPtr &else_graph) {
  const auto &func_node = graph->FindNode(func_name);
  EXPECT_NE(func_node, nullptr);

  func_node->GetOpDesc()->RegisterSubgraphIrName("then_branch", SubgraphType::kStatic);
  AddSubGraph(func_node, then_graph);

  func_node->GetOpDesc()->RegisterSubgraphIrName("else_branch", SubgraphType::kStatic);
  AddSubGraph(func_node, else_graph);
}

void AddCaseBranch(const ComputeGraphPtr &graph, const std::string &func_name, const ComputeGraphPtr &subgraph) {
  const auto &func_node = graph->FindNode(func_name);
  EXPECT_NE(func_node, nullptr);

  func_node->GetOpDesc()->RegisterSubgraphIrName("branches", SubgraphType::kDynamic);
  AddSubGraph(func_node, subgraph);
}

void SetUnknownOpKernel(const ComputeGraphPtr &graph, uint32_t &mem_offset, bool reset_index) {
  const static std::set<std::string> kGeLocalTypes{
      DATA, CONSTANT, CONSTANTOP, VARIABLE, NETOUTPUT, AIPP_DATA_TYPE, FILECONSTANT
  };
  const static std::set<std::string> kRtsLibTypes{
      IDENTITY, IDENTITYN, READVARIABLEOP, PROFILINGTRAININGTRACE, MEMCPYASYNC,
      STREAMACTIVE, STREAMSWITCH, STREAMMERGE, ENTER, REFENTER, LOOPCOND, NEXTITERATION, REFNEXTITERATION,
      EXIT, REFEXIT, LABELSET, LABELGOTO, LABELGOTOEX, LABELSWITCH, LABELSWITCHBYINDEX
  };
  static uint32_t node_index = 0U;
  if (reset_index) {
    node_index = 0U;
  }

  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_INT64);
  TensorUtils::SetSize(tensor_desc, 64);

  const bool owner_is_unknown = graph->GetGraphUnknownFlag();
  if (owner_is_unknown) {
    EXPECT_TRUE(AttrUtils::SetBool(graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, owner_is_unknown));
  }

  uint32_t var_offset = 0U;
  for (const auto &node : graph->GetDirectNode()) {
    const auto op_desc = node->GetOpDesc();
    op_desc->SetId(node_index++);
    AttrUtils::SetBool(op_desc, "OwnerGraphIsUnknown", owner_is_unknown);
    if (kGeLocalTypes.count(op_desc->GetType()) > 0U) {
      op_desc->SetOpKernelLibName("DNN_VM_GE_LOCAL_OP_STORE");
    } else if (kRtsLibTypes.count(op_desc->GetType()) > 0U) {
      op_desc->SetOpKernelLibName("DNN_VM_RTS_OP_STORE");
    } else {
      op_desc->SetOpKernelLibName("AIcoreEngine");
    }

    std::vector<int64_t> output_offset;
    for (size_t i = 0U; i < op_desc->GetOutputsSize(); ++i) {
      op_desc->UpdateOutputDesc(i, tensor_desc);
      if (op_desc->GetType() == CONSTANTOP || op_desc->GetType() == VARIABLE) {
        output_offset.emplace_back(var_offset);
        var_offset += 64U;
      } else {
        output_offset.emplace_back(mem_offset);
        mem_offset += 64U;
      }
    }
    op_desc->SetOutputOffset(output_offset);
    op_desc->SetWorkspace({});
    op_desc->SetWorkspaceBytes({});
  }

  ASSERT_EQ(VarManager::Instance(graph->GetSessionID())->Init(0, graph->GetSessionID(), 0, 0), SUCCESS);
  ASSERT_EQ(VarMemAssignUtil::AssignStaticMemory2Node(graph), SUCCESS);
  for (const auto &node : graph->GetDirectNode()) {
    const auto op_desc = node->GetOpDesc();
    std::vector<int64_t> input_offset;
    std::vector<std::string> out_src_name;
    std::vector<int64_t> out_src_index;
    for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
      op_desc->UpdateInputDesc(i, tensor_desc);
      if ((node->GetType() == NETOUTPUT) && (node->GetName() != NODE_NAME_NET_OUTPUT)) {
        EXPECT_TRUE(AttrUtils::SetInt(op_desc->MutableInputDesc(i), ATTR_NAME_PARENT_NODE_INDEX, i));
      }

      const auto in_anchor = node->GetInDataAnchor(i);
      if ((in_anchor == nullptr) || (in_anchor->GetPeerOutAnchor() == nullptr)) {
        input_offset.emplace_back(-1);
        continue;
      }

      const auto out_anchor = in_anchor->GetPeerOutAnchor();
      const auto peer_node = out_anchor->GetOwnerNode();
      out_src_name.emplace_back(peer_node->GetName());
      out_src_index.emplace_back(i);
      const std::vector<int64_t> output_offset = peer_node->GetOpDesc()->GetOutputOffset();
      if (static_cast<size_t>(out_anchor->GetIdx()) >= output_offset.size()) {
        input_offset.emplace_back(-1);
        continue;
      }

      input_offset.emplace_back(output_offset.at(out_anchor->GetIdx()));
    }

    op_desc->SetInputOffset(input_offset);
    if (node->GetType() == NETOUTPUT) {
      op_desc->SetSrcName(out_src_name);
      op_desc->SetSrcIndex(out_src_index);
    }
  }
}

void DelStaticForOffline(const ComputeGraphPtr &graph, uint32_t &mem_offset) {
  // Offline model will set new session_id, static var invalid.
  for (const auto &node : graph->GetDirectNode()) {
    const auto &op_desc = node->GetOpDesc();
    if (op_desc->GetType() != CONSTANTOP && op_desc->GetType() != VARIABLE) {
      continue;
    }

    op_desc->SetOutputOffset({ mem_offset });
    for (const auto &in_anchor : node->GetOutDataAnchor(0)->GetPeerInDataAnchors()) {
      auto input_offset = in_anchor->GetOwnerNode()->GetOpDesc()->GetInputOffset();
      input_offset[in_anchor->GetIdx()] = mem_offset;
      in_anchor->GetOwnerNode()->GetOpDesc()->SetInputOffset(input_offset);
    }

    mem_offset += 64U;
  }
}

void InitAiCpuAllShape(const OpDescPtr &op_desc, std::vector<uint8_t> &aicpu_ext_info) {
  size_t offset = 0U;
  const size_t info_len = (op_desc->GetInputsSize() + op_desc->GetOutputsSize()) * sizeof(hybrid::AicpuShapeAndType);
  aicpu_ext_info.resize((sizeof(hybrid::AicpuExtInfo) * 2U) + info_len);
  {
    auto &ext_info = *(reinterpret_cast<hybrid::AicpuExtInfo *>(aicpu_ext_info.data() + offset));
    offset += sizeof(hybrid::AicpuExtInfo);
    ext_info.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_INPUT_SHAPE;
    ext_info.infoLen = op_desc->GetInputsSize() * sizeof(hybrid::AicpuShapeAndType);

    for (size_t i = 0; i < op_desc->GetInputsSize(); ++i) {
      auto &shape_and_type = *(reinterpret_cast<hybrid::AicpuShapeAndType *>(aicpu_ext_info.data() + offset));
      offset += sizeof(hybrid::AicpuShapeAndType);
      shape_and_type.type = 0;
      shape_and_type.dims[0] = 0;
    }
  }
  {
    auto &ext_info = *(reinterpret_cast<hybrid::AicpuExtInfo *>(aicpu_ext_info.data() + offset));
    offset += sizeof(hybrid::AicpuExtInfo);
    ext_info.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_OUTPUT_SHAPE;
    ext_info.infoLen = op_desc->GetOutputsSize() * sizeof(hybrid::AicpuShapeAndType);

    for (size_t i = 0; i < op_desc->GetOutputsSize(); ++i) {
      auto &shape_and_type = *(reinterpret_cast<hybrid::AicpuShapeAndType *>(aicpu_ext_info.data() + offset));
      offset += sizeof(hybrid::AicpuShapeAndType);
      shape_and_type.type = 0;
      shape_and_type.dims[0] = 0;
    }
  }
}

void InitAiCpuAsyncWait(const OpDescPtr &op_desc, std::vector<uint8_t> &aicpu_ext_info) {
  size_t offset = 0U;
  aicpu_ext_info.resize(sizeof(hybrid::AicpuExtInfo) + sizeof(hybrid::AsyncWaitInfo));
  auto &ext_info = *(reinterpret_cast<hybrid::AicpuExtInfo *>(aicpu_ext_info.data() + offset));
  ext_info.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_ASYNCWAIT;
  ext_info.infoLen = sizeof(hybrid::AsyncWaitInfo);

  offset += sizeof(hybrid::AicpuExtInfo);
  auto &async_wait_info = *(reinterpret_cast<hybrid::AsyncWaitInfo *>(aicpu_ext_info.data() + offset));
  async_wait_info.waitType = 0;
  async_wait_info.waitId = 0;
  async_wait_info.timeOut = 0;
  async_wait_info.reserved = 0;
}

void InitAippNodeStatic(const ComputeGraphPtr &graph, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "static_aipp"));
}

void InitAippNodeRelated(const ComputeGraphPtr &graph, const std::string &op_name, const std::string &related_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp"));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_DATA_AIPP_DATA_NAME_MAP, related_name));

  const std::vector<string> aipp_io_attr{ "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, aipp_io_attr));
  EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, aipp_io_attr));
}

void InitAippNodeDynamic(const ComputeGraphPtr &graph, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  NamedAttrs aipp_attr;
  aipp_attr.SetAttr("aipp_mode", GeAttrValue::CreateFrom<int64_t>(2)); // domi::AippOpParams_AippMode_dynamic
  aipp_attr.SetAttr("related_input_rank", GeAttrValue::CreateFrom<int64_t>(0));
  aipp_attr.SetAttr("max_src_image_size", GeAttrValue::CreateFrom<int64_t>(2048));
  aipp_attr.SetAttr("support_rotation", GeAttrValue::CreateFrom<int64_t>(1));
  EXPECT_TRUE(AttrUtils::SetNamedAttrs(op_desc, ATTR_NAME_AIPP, aipp_attr));
}

void CleanAippNodeInfo(const ComputeGraphPtr &graph, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  op_desc->DelAttr(ATTR_NAME_AIPP);
  op_desc->DelAttr(ATTR_DATA_RELATED_AIPP_MODE);
  op_desc->DelAttr(ATTR_DATA_AIPP_DATA_NAME_MAP);

  op_desc->DelAttr(ATTR_NAME_AIPP_INPUTS);
  op_desc->DelAttr(ATTR_NAME_AIPP_OUTPUTS);
  op_desc->DelAttr(ATTR_DYNAMIC_AIPP_INPUT_DIMS);
}

void InitConstantNode(const ComputeGraphPtr &graph, const std::string &op_name, int32_t const_value) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  GeTensorDesc data_desc = op_desc->GetOutputDesc(0);
  GeTensorPtr weight_value = MakeShared<GeTensor>(data_desc, reinterpret_cast<uint8_t *>(&const_value), sizeof(int32_t));
  EXPECT_TRUE(AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight_value));
}

void InitConstantNode(const ComputeGraphPtr &graph, const std::string &op_name, const GeTensorDesc &tensor_desc,
                      const std::string &const_value) {
  const auto &node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto &op_desc = node->GetOpDesc();

  const auto weight_value = MakeShared<GeTensor>(tensor_desc, (const uint8_t *)const_value.data(), const_value.length());
  EXPECT_TRUE(AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight_value));

  EXPECT_EQ(op_desc->UpdateOutputDesc(0U, tensor_desc), SUCCESS);
  TensorUtils::SetSize(*op_desc->MutableOutputDesc(0U), weight_value->GetData().size());
}

void InitKernelTaskDef_TE(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name,
                          TBEKernelStore &kernel_store) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::TVM)));

  std::vector<char> kernel_bin(64, '\0');
  TBEKernelPtr kernel_handle = MakeShared<ge::OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
  EXPECT_TRUE(op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, kernel_handle));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName()));
  kernel_store.AddTBEKernel(kernel_handle);

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
  uint16_t args_offset[9] = {0};
  context.set_args_offset(args_offset, 9 * sizeof(uint16_t));
}

void InitKernelTaskDef_TE_SM(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::TVM)));

  std::vector<char> kernel_bin(64, '\0');
  const auto kernel_handle = MakeShared<ge::OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
  EXPECT_TRUE(op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, kernel_handle));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName()));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  rtSmDesc_t l2CtrlInfo;
  l2CtrlInfo.data[0].L2_mirror_addr = 1024;
  kernel_def.set_sm_desc(&l2CtrlInfo, sizeof(rtSmDesc_t));

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
  uint16_t args_offset[9] = {0};
  context.set_args_offset(args_offset, 9 * sizeof(uint16_t));
}

void InitKernelTaskDef_CUSTOM(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  // attrHandle
  std::vector<uint8_t> op_attr(128, 64);
  EXPECT_TRUE(AttrUtils::SetBytes(op_desc, ATTR_NAME_OPATTR, Buffer::CopyFrom(op_attr.data(), op_attr.size())));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  std::vector<uint64_t> args_info(5, 0);
  kernel_def.set_args(args_info.data(), args_info.size() * sizeof(uint64_t));
  kernel_def.set_args_size(args_info.size() * sizeof(uint64_t));

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::CUSTOMIZED));
  std::vector<uint16_t> args_offset{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8};
  context.set_args_offset(args_offset.data(), args_offset.size() * sizeof(uint16_t));
  context.set_args_count(5);
}

void InitKernelTaskDef_AI_CPU(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::AI_CPU)));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
  uint16_t args_offset[9] = {0};
  context.set_args_offset(args_offset, 9 * sizeof(uint16_t));
}

void InitKernelTaskDef_CPU_AllShape(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::AI_CPU)));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
  uint16_t args_offset[9] = {0};
  context.set_args_offset(args_offset, 9 * sizeof(uint16_t));

  EXPECT_TRUE(AttrUtils::SetBool(op_desc, "_AllShape", true));
  std::vector<uint8_t> aicpu_ext_info;
  InitAiCpuAllShape(op_desc, aicpu_ext_info);
  kernel_def.set_kernel_ext_info(aicpu_ext_info.data(), aicpu_ext_info.size());
  kernel_def.set_kernel_ext_info_size(aicpu_ext_info.size());
}

void InitKernelTaskDef_CPU_Blocking(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::AI_CPU)));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
  uint16_t args_offset[9] = {0};
  context.set_args_offset(args_offset, 9 * sizeof(uint16_t));

  EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_NAME_IS_BLOCKING_OP, true));
  std::vector<uint8_t> aicpu_ext_info;
  InitAiCpuAsyncWait(op_desc, aicpu_ext_info);
  kernel_def.set_kernel_ext_info(aicpu_ext_info.data(), aicpu_ext_info.size());
  kernel_def.set_kernel_ext_info_size(aicpu_ext_info.size());
}

void InitKernelTaskDef_CUST_CPU(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name,
                                CustAICPUKernelStore &kernel_store) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  std::vector<char> kernel_bin(64, '\0');
  const auto kernel_handle = MakeShared<ge::OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
  EXPECT_TRUE(op_desc->SetExtAttr(OP_EXTATTR_CUSTAICPU_KERNEL, kernel_handle));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName()));
  kernel_store.AddCustAICPUKernel(kernel_handle);

  domi::TaskDef &task_def = *model_def.add_task();
  domi::KernelDef &kernel_def = *task_def.mutable_kernel();
  domi::KernelContext &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);
  kernel_def.set_so_name("cust_ai_cpu_" + std::to_string(kernel_store.DataSize()));
  kernel_def.set_kernel_name("cust_ai_cpu_kernel");

  rtSmDesc_t l2CtrlInfo;
  l2CtrlInfo.data[0].L2_mirror_addr = 1024;
  kernel_def.set_sm_desc(&l2CtrlInfo, sizeof(rtSmDesc_t));
  kernel_def.set_flowtable("InitCceTask");

  std::vector<uint16_t> args_info(128, 0);
  kernel_def.set_args(args_info.data(), args_info.size());
  kernel_def.set_args_size(args_info.size());

  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::CUST_AI_CPU));
  context.set_op_index(op_desc->GetId());
  context.set_is_flowtable(true);

  context.set_args_count(1);
  std::vector<uint16_t> args_offset{0, 1};
  context.set_args_offset(args_offset.data(), sizeof(uint16_t));
}

void InitKernelTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_NO_TASK_AND_DUMP_NEEDED, true));    // for IsNoTaskAndDumpNeeded
  EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"1", "2"}));

  auto &task_def = *model_def.add_task();
  auto &kernel_def = *task_def.mutable_kernel();
  auto &context = *kernel_def.mutable_context();
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  task_def.set_stream_id(0);

  kernel_def.set_stub_func("stub_func");
  kernel_def.set_args_size(64);
  string args(64, '1');
  kernel_def.set_args(args.data(), 64);

  context.set_op_index(op_desc->GetId());
  context.set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
  context.set_kernel_func_id(1);
  context.set_is_flowtable(false);
  context.set_args_count(1);
  context.set_args_offset("args111111", 10);
}

void InitKernelExTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  std::vector<uint8_t> task_info(120, 0);
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  op_desc->SetWorkspace({ 512 });
  op_desc->SetWorkspaceBytes({ static_cast<int64_t>(task_info.size()) });
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::AI_CPU)));

  auto &task_def = *model_def.add_task();
  auto &kernel_ex_def = *task_def.mutable_kernel_ex();
  task_def.set_type(RT_MODEL_TASK_KERNEL_EX);
  task_def.set_stream_id(0);
  kernel_ex_def.set_op_index(op_desc->GetId());

  std::vector<uint8_t> args_info(sizeof(STR_FWK_OP_KERNEL), 0);
  kernel_ex_def.set_args(args_info.data(), sizeof(STR_FWK_OP_KERNEL));
  kernel_ex_def.set_args_size(sizeof(STR_FWK_OP_KERNEL));

  kernel_ex_def.set_task_info_size(task_info.size());
  kernel_ex_def.set_task_info(task_info.data(), task_info.size());

  std::string kernel_ext_info = std::string{7,0,0,0,4,0,0,0,1,0,0,0};
  kernel_ex_def.set_kernel_ext_info(kernel_ext_info.data(), kernel_ext_info.size());
}

void InitKernelExTaskDef_AllShape(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  std::vector<uint8_t> task_info(120, 0);
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  op_desc->SetWorkspace({ 512 });
  op_desc->SetWorkspaceBytes({ static_cast<int64_t>(task_info.size()) });

  auto &task_def = *model_def.add_task();
  auto &kernel_ex_def = *task_def.mutable_kernel_ex();
  task_def.set_type(RT_MODEL_TASK_KERNEL_EX);
  task_def.set_stream_id(0);
  kernel_ex_def.set_op_index(op_desc->GetId());

  std::vector<uint8_t> args_info(sizeof(STR_FWK_OP_KERNEL), 0);
  kernel_ex_def.set_args(args_info.data(), sizeof(STR_FWK_OP_KERNEL));
  kernel_ex_def.set_args_size(sizeof(STR_FWK_OP_KERNEL));

  kernel_ex_def.set_task_info_size(task_info.size());
  kernel_ex_def.set_task_info(task_info.data(), task_info.size());

  EXPECT_TRUE(AttrUtils::SetBool(op_desc, "_AllShape", true));
  std::vector<uint8_t> aicpu_ext_info;
  InitAiCpuAllShape(op_desc, aicpu_ext_info);
  kernel_ex_def.set_kernel_ext_info(aicpu_ext_info.data(), aicpu_ext_info.size());
  kernel_ex_def.set_kernel_ext_info_size(aicpu_ext_info.size());
}

void InitKernelExTaskDef_Blocking(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  std::vector<uint8_t> task_info(120, 0);
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  op_desc->SetWorkspace({ 512 });
  op_desc->SetWorkspaceBytes({ static_cast<int64_t>(task_info.size()) });

  auto &task_def = *model_def.add_task();
  auto &kernel_ex_def = *task_def.mutable_kernel_ex();
  task_def.set_type(RT_MODEL_TASK_KERNEL_EX);
  task_def.set_stream_id(0);
  kernel_ex_def.set_op_index(op_desc->GetId());

  std::vector<uint8_t> args_info(sizeof(STR_FWK_OP_KERNEL), 0);
  kernel_ex_def.set_args(args_info.data(), sizeof(STR_FWK_OP_KERNEL));
  kernel_ex_def.set_args_size(sizeof(STR_FWK_OP_KERNEL));

  kernel_ex_def.set_task_info_size(task_info.size());
  kernel_ex_def.set_task_info(task_info.data(), task_info.size());

  EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_NAME_IS_BLOCKING_OP, true));
  std::vector<uint8_t> aicpu_ext_info;
  InitAiCpuAsyncWait(op_desc, aicpu_ext_info);
  kernel_ex_def.set_kernel_ext_info(aicpu_ext_info.data(), aicpu_ext_info.size());
  kernel_ex_def.set_kernel_ext_info_size(aicpu_ext_info.size());
}

void InitStreamActiveDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_STREAM_ACTIVE);
  task_def.set_stream_id(0);

  auto &stream_active_def = *task_def.mutable_stream_active();
  stream_active_def.set_op_index(op_desc->GetId());
}

void InitStreamSwitchDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_STREAM_SWITCH);
  task_def.set_stream_id(0);

  auto &stream_switch_def = *task_def.mutable_stream_switch();
  stream_switch_def.set_op_index(op_desc->GetId());
}

void InitStreamSwitchNDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_STREAM_SWITCH_N);
  task_def.set_stream_id(0);

  auto &stream_switch_def = *task_def.mutable_stream_switch();
  stream_switch_def.set_op_index(op_desc->GetId());
}

void InitStreamMergeDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  NodeUtils::AppendOutputAnchor(node, 2);

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
  task_def.set_stream_id(0);

  auto &memcpy_async = *task_def.mutable_memcpy_async();
  memcpy_async.set_op_index(op_desc->GetId());
  memcpy_async.set_count(1);
  memcpy_async.set_kind(RT_MEMCPY_HOST_TO_HOST);
  memcpy_async.set_dst_max(64);
  memcpy_async.set_src(180);
  memcpy_async.set_dst(180);
}

void InitLabelSwitchDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  std::vector<uint32_t> label_idx_list;
  EXPECT_TRUE(AttrUtils::GetListInt(op_desc, ATTR_NAME_LABEL_SWITCH_LIST, label_idx_list));

  domi::TaskDef &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_STREAM_LABEL_SWITCH_BY_INDEX);
  task_def.set_stream_id(op_desc->GetStreamId());

  domi::LabelSwitchByIndexDef &label_task_def = *task_def.mutable_label_switch_by_index();
  label_task_def.set_op_index(op_desc->GetId());
  label_task_def.set_label_max(label_idx_list.size());
}

void InitLabelSetDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  domi::TaskDef &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_LABEL_SET);
  task_def.set_stream_id(op_desc->GetStreamId());

  domi::LabelSetDef &label_task_def = *task_def.mutable_label_set();
  label_task_def.set_op_index(op_desc->GetId());
}

void InitLabelGotoDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  domi::TaskDef &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_STREAM_LABEL_GOTO);
  task_def.set_stream_id(op_desc->GetStreamId());

  domi::LabelGotoExDef &label_task_def = *task_def.mutable_label_goto_ex();
  label_task_def.set_op_index(op_desc->GetId());
}

void InitMemcpyAsyncDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
  task_def.set_stream_id(0);

  auto &memcpy_async = *task_def.mutable_memcpy_async();
  memcpy_async.set_op_index(op_desc->GetId());
  memcpy_async.set_count(1);
  memcpy_async.set_kind(RT_MEMCPY_HOST_TO_HOST);
  memcpy_async.set_dst_max(64);
  memcpy_async.set_src(180);
  memcpy_async.set_dst(180);
}

void InitMemcpyAddrAsyncDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_MEMCPY_ADDR_ASYNC);
  task_def.set_stream_id(0);

  auto &memcpy_async = *task_def.mutable_memcpy_async();
  memcpy_async.set_op_index(op_desc->GetId());
  memcpy_async.set_count(1);
  memcpy_async.set_kind(RT_MEMCPY_HOST_TO_HOST);
  memcpy_async.set_dst_max(64);
  memcpy_async.set_src(180);
  memcpy_async.set_dst(180);
}

void InitEndGraphDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_MODEL_END_GRAPH);
  task_def.set_stream_id(0);
  task_def.set_id(op_desc->GetId());
}

void InitHcclTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_HCCL);
  task_def.set_stream_id(0);
  OpsKernelInfoStore *ptr = &g_fake_ops_kernel_info_store;
  op_desc->SetExtAttr("OpsKernelInfoStorePtr", ptr);

  auto &kernel_hccl_def = *task_def.mutable_kernel_hccl();
  kernel_hccl_def.set_op_index(op_desc->GetId());
  kernel_hccl_def.set_hccl_type("HcomBroadcast");
}

void InitModelExitTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_MODEL_EXIT);
  task_def.set_stream_id(0);
}

void InitProfilerTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_PROFILER_TRACE);
  task_def.set_stream_id(0);

  auto &log_timestamp_def = *task_def.mutable_log_timestamp();
  log_timestamp_def.set_notify(false);
  log_timestamp_def.set_logid(10);
  log_timestamp_def.set_flat(0);
}

void InitEventTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  {
    auto &task_def = *model_def.add_task();
    task_def.set_type(RT_MODEL_TASK_EVENT_RECORD);
    task_def.set_stream_id(0);
    task_def.set_event_id(0);
    task_def.mutable_event_ex()->set_event_type(0);
  }

  {
    auto &task_def = *model_def.add_task();
    task_def.set_type(RT_MODEL_TASK_EVENT_WAIT);
    task_def.set_stream_id(0);
    task_def.set_event_id(0);
    task_def.mutable_event_ex()->set_event_type(0);
  }
}

void InitFusionTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  {
    auto &task_def = *model_def.add_task();
    task_def.set_type(RT_MODEL_TASK_FUSION_START);
    task_def.set_stream_id(0);
  }

  {
    auto &task_def = *model_def.add_task();
    task_def.set_type(RT_MODEL_TASK_FUSION_END);
    task_def.set_stream_id(0);
  }
}

void InitFftsplusTaskDef(const ComputeGraphPtr &graph, domi::TaskDef &task_def) {
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  auto &ffts_plus_task_def = *task_def.mutable_ffts_plus_task();
  ffts_plus_task_def.set_op_index(graph->GetParentNode()->GetOpDesc()->GetId());
  std::cout << "parent node: " << graph->GetParentNode()->GetName().c_str() << std::endl;
  std::cout << "parent node id: " << graph->GetParentNode()->GetOpDesc()->GetId() << std::endl;
  ffts_plus_task_def.set_addr_size(4);
  domi::FftsPlusSqeDef *sqedef = ffts_plus_task_def.mutable_ffts_plus_sqe();
  //header
  domi::StarsSqeHeaderDef *headerdef = sqedef->mutable_sqe_header();
  headerdef->set_l1_lock(1);
  headerdef->set_l1_unlock(1);
  headerdef->set_block_dim(1);
  //sqe
  sqedef->set_wrr_ratio(1);
  sqedef->set_sqe_index(1);

  sqedef->set_total_context_num(2);
  sqedef->set_ready_context_num(1);
  sqedef->set_preload_context_num(1);

  sqedef->set_prefetch_ost_num(1);
  sqedef->set_cmaint_ost_num(1);

  sqedef->set_aic_prefetch_lower(1);
  sqedef->set_aic_prefetch_upper(1);
  sqedef->set_aiv_prefetch_lower(1);
  sqedef->set_aiv_prefetch_upper(1);
}

void InitFftsPlusCaseDefaultDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_CASE_SWITCH));
  domi::FftsPlusCaseDefaultCtxDef *case_default_def = ctx_def.mutable_case_default_ctx();
  case_default_def->set_successor_num(26);
  case_default_def->set_aten(32);
  case_default_def->set_start_label_id(1);
  case_default_def->set_label_list_len(32);
  case_default_def->set_pred_cnt_init(1);
  case_default_def->set_pred_cnt(32);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    case_default_def->add_successor_list(2); // len = 26
  }
}

void InitFftsPlusNotifyDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_NOTIFY_WAIT));
  domi::FftsPlusNotifyCtxDef *notify_def = ctx_def.mutable_notify_ctx();
  notify_def->set_successor_num(26);
  notify_def->set_aten(1);
  notify_def->set_pred_cnt_init(1);
  notify_def->set_pred_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    notify_def->add_successor_list(1); // len = 26
  }
  notify_def->set_atm(1);
  notify_def->set_satm(1);

  notify_def->set_thread_id(1);
  notify_def->set_thread_dim(1);

  notify_def->set_notify_id_base(1);
  notify_def->set_auto_window(1);
  for (int i = 0; i < 16; ++i) {
    notify_def->add_notify_id(1);
  }
}

void InitWriteValueDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_WRITE_VALUE));
  domi::FftsPlusWriteValueCtxDef *write_def = ctx_def.mutable_write_value_ctx();
  write_def->set_successor_num(26);
  write_def->set_aten(1);
  write_def->set_pred_cnt_init(1);
  write_def->set_pred_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    write_def->add_successor_list(1); // len = 26
  }
  write_def->set_atm(1);
  write_def->set_thread_id(1);
  write_def->set_thread_dim(1);

  write_def->set_aw_size(1);
  write_def->set_aw_snoop(1);
  write_def->set_aw_cache(1);
  write_def->set_aw_prot(1);
  write_def->set_aw_va(1);

  write_def->set_ar_size(1);
  write_def->set_ar_snoop(1);
  write_def->set_ar_cache(1);
  write_def->set_ar_prot(1);
  write_def->set_ar_va(1);

  write_def->set_write_addr_base(0x147);
  write_def->set_write_addr_offset(32);
  for (int j = 0; j < 4; ++j) {
    write_def->add_write_value(1);
  }
}

void InitMixAicAivDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_id(0);
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIC));
  domi::FftsPlusMixAicAivCtxDef *mixctx_def = ctx_def.mutable_mix_aic_aiv_ctx();
  mixctx_def->set_successor_num(26);
  mixctx_def->set_aten(1);
  mixctx_def->set_prefetch_config(1);
  mixctx_def->set_pred_cnt_init(1);
  mixctx_def->set_pred_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    mixctx_def->add_successor_list(1); // len = 26
  }
  mixctx_def->set_schem(1);
  mixctx_def->set_atm(1);
  mixctx_def->set_prefetch_enable_bitmap(1);
  mixctx_def->set_prefetch_once_bitmap(1);

  mixctx_def->set_pmg(1);
  mixctx_def->set_ns(1);
  mixctx_def->set_part_id(1);
  mixctx_def->set_qos(1);

  mixctx_def->set_non_tail_block_ratio_n(1);
  mixctx_def->set_tail_block_ratio_n(1);

  mixctx_def->set_thread_id(1);
  mixctx_def->set_thread_dim(1);

  mixctx_def->set_non_tail_block_dim(1);
  mixctx_def->set_tail_block_dim(1);

  mixctx_def->set_aiv_task_param_ptr_offset(1);
  mixctx_def->set_aic_task_param_ptr_offset(1);

  mixctx_def->add_kernel_name("mixaic_a");
  //ctx_def->add_kernel_name("mixaic_b");
  //ctx_def->add_kernel_name("mixaiv_a");
  mixctx_def->add_kernel_name("mixaiv_b");
  // task_addr = {0,200,700,1000,2000, 3500}
  // task_addr_offset = {20,40,2,100,200}
  mixctx_def->add_task_addr(0x12);
  mixctx_def->add_task_addr(0x16);
  mixctx_def->add_task_addr(0x1a);
  mixctx_def->add_task_addr(0x1e);
  mixctx_def->add_task_addr(0x22);
  mixctx_def->add_task_addr(0x26);

  mixctx_def->add_task_addr_offset(32);
  mixctx_def->add_task_addr_offset(32);
  mixctx_def->add_task_addr_offset(32);
  mixctx_def->add_task_addr_offset(32);
  mixctx_def->add_task_addr_offset(32);

  mixctx_def->set_input_output_count(1);
  mixctx_def->set_save_task_addr(1);
  for (int j = 0; j < 4; ++j) {
    mixctx_def->add_src_slot(1);  // len = 4, context ID for source data which is out of subgraph
  }
}

void InitSdmaDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_SDMA));
  domi::FftsPlusSdmaCtxDef *smda_def = ctx_def.mutable_sdma_ctx();
  smda_def->set_successor_num(26);
  smda_def->set_aten(1);
  smda_def->set_pred_cnt_init(1);
  smda_def->set_pred_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    smda_def->add_successor_list(1); // len = 26
  }

  smda_def->set_atm(1);
  smda_def->set_pmg(1);
  smda_def->set_ns(1);
  smda_def->set_part_id(1);
  smda_def->set_qos(1);

  smda_def->set_thread_id(1);
  smda_def->set_thread_dim(1);

  smda_def->set_sdma_sqe_header(1);

  smda_def->set_src_stream_id(1);
  smda_def->set_src_sub_stream_id(1);
  smda_def->set_dst_stream_id(1);
  smda_def->set_dst_sub_stream_id(1);

  smda_def->set_src_addr_base(0x457);
  smda_def->set_src_addr_offset(32);
  smda_def->set_dst_addr_base(0x126);
  smda_def->set_dst_addr_offset(32);

  smda_def->set_non_tail_data_len(1);
  smda_def->set_tail_data_len(1);
}

void InitDataCtx(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_FLUSH_DATA));
  domi::FftsPlusDataCtxDef *data_def = ctx_def.mutable_data_ctx();
  data_def->set_successor_num(26);
  data_def->set_aten(1);
  data_def->set_cnt_init(1);
  data_def->set_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    data_def->add_successor_list(1); // len = 26
  }
  data_def->set_atm(1);
  data_def->set_pmg(1);
  data_def->set_ns(1);
  data_def->set_part_id(1);
  data_def->set_qos(1);

  data_def->set_orig_consumer_counter(1);
  data_def->set_run_consumer_counter(1);

  data_def->set_thread_id(1);
  data_def->set_thread_dim(1);

  data_def->set_addr_base(0x125);
  data_def->set_addr_offset(32);

  data_def->set_non_tail_num_outter(1);
  data_def->set_non_tail_num_inner(1);
  data_def->set_non_tail_len_inner(1);
  data_def->set_non_tail_stride_outter(1);
  data_def->set_non_tail_stride_inner(1);

  data_def->set_tail_num_outter(1);
  data_def->set_tail_num_inner(1);
  data_def->set_tail_len_inner(1);
  data_def->set_tail_stride_outter(1);
  data_def->set_tail_stride_inner(1);
}

void InitCondSwitchCtx(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_COND_SWITCH));
  domi::FftsPlusCondSwitchCtxDef *switch_def = ctx_def.mutable_cond_switch_ctx();
  switch_def->set_true_successor_num(12);
  switch_def->set_false_successor_num(14);
  switch_def->set_aten(32);

  switch_def->set_condition(4);
  switch_def->set_pred_cnt_init(32);
  switch_def->set_pred_cnt(32);

  for (int i = 0; i < RT_CTX_FALSE_SUCCESSOR_NUM; ++i) {
    if (i < RT_CTX_TRUE_SUCCESSOR_NUM) {
      switch_def->add_true_successor_list(1);    // len = 12
    }
    switch_def->add_false_successor_list(1);   // len = 14
  }
  switch_def->set_atm(32);

  switch_def->set_thread_id(1);
  switch_def->set_thread_dim(32);

  switch_def->set_ar_size(32);
  switch_def->set_snoop(32);
  switch_def->set_ar_cache(32);
  switch_def->set_ar_prot(32);
  switch_def->set_va(32);

  switch_def->set_load_addr0_base(0x142);
  switch_def->set_ld0_en(32);
  switch_def->set_load_addr0_offset(32);

  switch_def->set_load_addr1_base(0x365);
  switch_def->set_ld1_en(64);
  switch_def->set_load_addr1_offset(32);

  switch_def->set_cmp_value_1(1);
  switch_def->set_cmp_value_2(1);
}

void InitFftsPlusCachePersistDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def,
                                 const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();

  const uint32_t persistent_id = 0x00000001U;
  AttrUtils::SetInt(op_desc->MutableInputDesc(0U), "_cache_persist", persistent_id);

  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_PERSISTENT_CACHE));
  domi::FftsPlusCachePersistCtxDef *cache_persist_def = ctx_def.mutable_cache_persist_ctx();
  cache_persist_def->set_successor_num(26);
  cache_persist_def->set_aten(1);
  cache_persist_def->set_pred_cnt_init(1);
  cache_persist_def->set_pred_cnt(1);
  for (int i = 1; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    cache_persist_def->add_successor_list(1); // 16 bits, len = 26
  }
  cache_persist_def->set_persistent_en(1);
  cache_persist_def->set_persistent_id(1);
  cache_persist_def->set_persistent_size(1);
}

void InitFftsPlusAicCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
  domi::FftsPlusAicAivCtxDef *aic_aiv_def = ctx_def.mutable_aic_aiv_ctx();
  aic_aiv_def->set_successor_num(26);
  aic_aiv_def->set_aten(1);
  aic_aiv_def->set_prefetch_config(1);
  aic_aiv_def->set_pred_cnt_init(1);
  aic_aiv_def->set_pred_cnt(1);
  for (int i = 1; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    aic_aiv_def->add_successor_list(1); // 16 bits, len = 26
  }
  aic_aiv_def->set_schem(1);
  aic_aiv_def->set_atm(1);
  aic_aiv_def->set_prefetch_enable_bitmap(1);
  aic_aiv_def->set_prefetch_once_bitmap(1);

  aic_aiv_def->set_pmg(1);
  aic_aiv_def->set_ns(1);
  aic_aiv_def->set_part_id(1);
  aic_aiv_def->set_qos(1);

  aic_aiv_def->set_thread_id(2);
  aic_aiv_def->set_thread_dim(1);

  aic_aiv_def->set_non_tail_block_dim(6);
  aic_aiv_def->set_tail_block_dim(5);

  //aic_aiv_def->set_task_param_ptr_base(0x235689);
  aic_aiv_def->set_task_param_ptr_offset(32);
  // task_addr = {0,200,700,1000,2000, 3500}
  // task_addr_offset = {20,40,2,100,200}
  aic_aiv_def->add_task_addr(0);
  aic_aiv_def->add_task_addr(200);
  aic_aiv_def->add_task_addr(700);
  aic_aiv_def->add_task_addr(1000);
  aic_aiv_def->add_task_addr(2000);
  aic_aiv_def->add_task_addr(3500);

  aic_aiv_def->add_task_addr_offset(20);
  aic_aiv_def->add_task_addr_offset(40);
  aic_aiv_def->add_task_addr_offset(2);
  aic_aiv_def->add_task_addr_offset(100);
  aic_aiv_def->add_task_addr_offset(200);

  aic_aiv_def->set_input_output_count(3);
  aic_aiv_def->set_save_task_addr(1);
  aic_aiv_def->add_kernel_name("aictest");
  aic_aiv_def->add_kernel_name("aivtest");
  for (int j = 1; j < 4; ++j) {
    aic_aiv_def->add_src_slot(1);  // len = 4, context ID for source data which is out of subgraph
  }
}

void InitFftsPlusAicpuCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpu_def = ctx_def.mutable_aicpu_ctx();
  aicpu_def->set_successor_num(26);
  aicpu_def->set_aten(1);
  aicpu_def->set_pred_cnt_init(1);
  aicpu_def->set_pred_cnt(1);
  for (int j = 0; j < RT_CTX_SUCCESSOR_NUM; ++j) {
    aicpu_def->add_successor_list(1);   // len = 26
  }
  aicpu_def->set_atm(1);
  aicpu_def->set_sqe_index(1);
  aicpu_def->set_kernel_type(2);
  aicpu_def->set_bm(1);
  aicpu_def->set_topic_type(1);
  aicpu_def->set_qos(1);

  aicpu_def->set_thread_id(1);
  aicpu_def->set_thread_dim(1);

  aicpu_def->set_non_tail_block_dim(1);
  aicpu_def->set_tail_block_dim(1);

  aicpu_def->set_sub_topic_id(1);
  aicpu_def->set_topic_id(1);
  aicpu_def->set_group_id(1);

  aicpu_def->set_task_param_offset(32);

  size_t len = sizeof(hybrid::AicpuExtInfo) + sizeof(hybrid::AsyncWaitInfo);
  std::vector<int8_t> aicpu_ext_info(len, 0);
  int offset = 0;
  hybrid::AicpuExtInfo *ext_info = reinterpret_cast<hybrid::AicpuExtInfo *>(aicpu_ext_info.data() + offset);
  ext_info->infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_ASYNCWAIT;
  ext_info->infoLen = sizeof(hybrid::AsyncWaitInfo);
  offset += sizeof(hybrid::AicpuExtInfo);
  hybrid::AsyncWaitInfo *async_wait_info = reinterpret_cast<hybrid::AsyncWaitInfo *>(aicpu_ext_info.data() + offset);
  async_wait_info->waitType = 0;
  async_wait_info->waitId = 0;
  async_wait_info->timeOut = 0;
  async_wait_info->reserved = 0;

  std::vector<int8_t> aicpu_arg_info(150, 0);
  domi::aicpuKernelDef *kerneldef = aicpu_def->mutable_kernel();
  kerneldef->set_args_size(aicpu_arg_info.size());
  kerneldef->set_args(aicpu_arg_info.data(), aicpu_arg_info.size());
  kerneldef->set_so_name("libaicpu");
  kerneldef->set_kernel_name("aicpu");
  kerneldef->set_kernel_ext_info(aicpu_ext_info.data(), len);
  kerneldef->set_kernel_ext_info_size(len);
}

void InitCustomFftsPlusAicpuCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def,
                                   const std::string &op_name) {
  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpu_def = ctx_def.mutable_aicpu_ctx();
  aicpu_def->set_successor_num(26);
  aicpu_def->set_aten(1);
  aicpu_def->set_pred_cnt_init(1);
  aicpu_def->set_pred_cnt(1);
  for (int j = 0; j < RT_CTX_SUCCESSOR_NUM; ++j) {
    aicpu_def->add_successor_list(1);   // len = 26
  }
  aicpu_def->set_atm(1);
  aicpu_def->set_sqe_index(1);
  aicpu_def->set_kernel_type(4);
  aicpu_def->set_bm(1);
  aicpu_def->set_topic_type(1);
  aicpu_def->set_qos(1);

  aicpu_def->set_thread_id(1);
  aicpu_def->set_thread_dim(1);

  aicpu_def->set_non_tail_block_dim(1);
  aicpu_def->set_tail_block_dim(1);

  aicpu_def->set_sub_topic_id(1);
  aicpu_def->set_topic_id(1);
  aicpu_def->set_group_id(1);

  aicpu_def->set_task_param_offset(32);

  size_t len = sizeof(hybrid::AicpuExtInfo) + sizeof(hybrid::AsyncWaitInfo);
  std::vector<int8_t> aicpu_ext_info(len, 0);
  int offset = 0;
  hybrid::AicpuExtInfo *ext_info = reinterpret_cast<hybrid::AicpuExtInfo *>(aicpu_ext_info.data() + offset);
  ext_info->infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_ASYNCWAIT;
  ext_info->infoLen = sizeof(hybrid::AsyncWaitInfo);
  offset += sizeof(hybrid::AicpuExtInfo);
  hybrid::AsyncWaitInfo *async_wait_info = reinterpret_cast<hybrid::AsyncWaitInfo *>(aicpu_ext_info.data() + offset);
  async_wait_info->waitType = 0;
  async_wait_info->waitId = 0;
  async_wait_info->timeOut = 0;
  async_wait_info->reserved = 0;

  std::vector<int8_t> aicpu_arg_info(150, 0);
  domi::aicpuKernelDef *kerneldef = aicpu_def->mutable_kernel();
  kerneldef->set_args_size(aicpu_arg_info.size());
  kerneldef->set_args(aicpu_arg_info.data(), aicpu_arg_info.size());
  kerneldef->set_so_name("libaicpu");
  kerneldef->set_kernel_name("aicpu");
  kerneldef->set_kernel_ext_info(aicpu_ext_info.data(), len);
  kerneldef->set_kernel_ext_info_size(len);
}

void InitFftsPlusAicpuFwkCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name) {
  const auto &node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto &op_desc = node->GetOpDesc();
  op_desc->SetWorkspace({64});   // offset
  op_desc->SetWorkspaceBytes({120});    // length

  ctx_def.set_op_index(op_desc->GetId());
  ctx_def.set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));

  domi::FftsPlusAicpuCtxDef &aicpu_def = *ctx_def.mutable_aicpu_ctx();
  aicpu_def.set_successor_num(26);
  aicpu_def.set_aten(1);
  aicpu_def.set_pred_cnt_init(1);
  aicpu_def.set_pred_cnt(1);
  for (int i = 0; i < RT_CTX_SUCCESSOR_NUM; ++i) {
    aicpu_def.add_successor_list(1);   // len = 26
  }
  aicpu_def.set_atm(1);
  aicpu_def.set_sqe_index(1);
  aicpu_def.set_kernel_type(1);
  aicpu_def.set_bm(1);
  aicpu_def.set_topic_type(1);
  aicpu_def.set_qos(1);

  aicpu_def.set_thread_id(1);
  aicpu_def.set_thread_dim(1);

  aicpu_def.set_non_tail_block_dim(1);
  aicpu_def.set_tail_block_dim(1);

  aicpu_def.set_sub_topic_id(1);
  aicpu_def.set_topic_id(1);
  aicpu_def.set_group_id(1);

  aicpu_def.set_task_param_offset(32);

  domi::aicpuKernelDef &kerneldef = *aicpu_def.mutable_kernel();
  std::vector<char> args_val(sizeof(STR_FWK_OP_KERNEL), '0');
  kerneldef.set_args_size(args_val.size());
  kerneldef.set_args(args_val.data(), args_val.size());
  kerneldef.set_so_name("libaicpu");
  kerneldef.set_kernel_name("aicpu");

  std::vector<uint8_t> aicpu_ext_info(sizeof(hybrid::AicpuExtInfo) + sizeof(hybrid::AsyncWaitInfo), 0);
  size_t offset = 0U;
  hybrid::AicpuExtInfo *ext_info = reinterpret_cast<hybrid::AicpuExtInfo *>(&aicpu_ext_info[offset]);
  offset += sizeof(hybrid::AicpuExtInfo);
  ext_info->infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_ASYNCWAIT;
  ext_info->infoLen = sizeof(hybrid::AsyncWaitInfo);
  hybrid::AsyncWaitInfo *async_wait_info = reinterpret_cast<hybrid::AsyncWaitInfo *>(&aicpu_ext_info[offset]);
  async_wait_info->waitType = 0U;
  async_wait_info->waitId = 0U;
  async_wait_info->timeOut = 0U;
  async_wait_info->reserved = 0U;

  kerneldef.set_kernel_ext_info(aicpu_ext_info.data(), aicpu_ext_info.size());
  kerneldef.set_kernel_ext_info_size(aicpu_ext_info.size());
}

void InitCmoTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_CMO);
  auto &cmo_task_def = *task_def.mutable_cmo_task();
  cmo_task_def.set_cmo_type(1);
  cmo_task_def.set_logic_id(0);
  cmo_task_def.set_qos(0);
  cmo_task_def.set_part_id(0);
  cmo_task_def.set_pmg(0);
  cmo_task_def.set_op_code(0);
  cmo_task_def.set_num_inner(0);
  cmo_task_def.set_num_outer(0);
  cmo_task_def.set_length_inner(0);
  cmo_task_def.set_source_addr(0);
  cmo_task_def.set_strider_outer(0);
  cmo_task_def.set_strider_inner(0);
}

void InitCmoBarrierTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def) {
  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_BARRIER);
  auto &cmo_barrier_task_def = *task_def.mutable_cmo_barrier_task();
  cmo_barrier_task_def.set_logic_id_num(0);
  domi::CmoBarrierInfoDef &cmo_barrier_info_def = *cmo_barrier_task_def.add_barrier_info();
  cmo_barrier_info_def.set_cmo_type(0);
  cmo_barrier_info_def.set_logic_id(0);
}

void InitDSATaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name,
                    const bool set_ptr_or_value) {
  auto &task_def = *model_def.add_task();
  task_def.set_type(RT_MODEL_TASK_DSA_TASK);
  task_def.set_stream_id(0);

  domi::DSATaskDef *dsa_task = task_def.mutable_dsa_task();

  const auto node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto op_desc = node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  const vector<int64_t> v_memory_type{RT_MEMORY_L1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, v_memory_type);
  op_desc->SetOutputOffset({1024});
  {  // RT_MEMORY_L1
    auto tensor_desc = op_desc->MutableOutputDesc(0);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 1024);
  }

  const vector<int64_t> v_memory_type1{RT_MEMORY_L1, RT_MEMORY_L1, RT_MEMORY_L1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, v_memory_type1);
  op_desc->SetInputOffset({1024, 1024, 1024});
  {  // RT_MEMORY_L1
    for (int i = 0; i < 3; ++i) {
      auto tensor_desc = op_desc->MutableInputDesc(i);
      EXPECT_NE(tensor_desc, nullptr);
      TensorUtils::SetSize(*tensor_desc, 64);
      TensorUtils::SetDataOffset(*tensor_desc, 1024);
    }
  }

  op_desc->SetWorkspace({1308, 1458});
  op_desc->SetWorkspaceBytes({150, 150});
  const vector<int64_t> v_memory_type2{RT_MEMORY_HBM, RT_MEMORY_HBM};
  AttrUtils::SetListInt(op_desc, TVM_ATTR_NAME_WORKSPACE_TYPE, v_memory_type2);
  AttrUtils::SetListInt(op_desc, ATTR_NAME_WORKSPACE_TYPE_LIST, v_memory_type2);

  dsa_task->set_op_index(op_desc->GetId());
  dsa_task->set_start(1);
  dsa_task->set_sqe_type(1);
  dsa_task->set_distribution_type(1);
  dsa_task->set_data_type(1);
  dsa_task->set_alg_type(1);
  dsa_task->set_input_vld(1);
  dsa_task->set_input_value_addr_flag(1);
  if (set_ptr_or_value) {
    dsa_task->set_input1_value_or_ptr(0);
  } else {
    dsa_task->set_input1_value_or_ptr(1);
  }
  dsa_task->set_input2_value_or_ptr(1);
  dsa_task->set_seed_value_or_ptr(0);
  dsa_task->set_random_count_value_or_ptr(0);
  domi::DSATaskArgsDef *dsa_task_args = dsa_task->mutable_args();
  dsa_task_args->set_output_addr(0x12);
  dsa_task_args->set_workspace_philox_count_addr(0x24);
  dsa_task_args->set_workspace_input_addr(0x457);
  dsa_task_args->set_seed_value_or_addr("5");
  dsa_task_args->set_input1_value_or_addr("1");
  dsa_task_args->set_input2_value_or_addr("2");
}

void InitFusionOpInfo(const ComputeGraphPtr &graph, const std::string &op_name) {
  const auto &node = graph->FindNode(op_name);
  EXPECT_NE(node, nullptr);
  const auto &op_desc = node->GetOpDesc();
  std::vector<std::string> origin_names = {"test1", "test2", "test3"};
  (void)AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, origin_names);
}

int32_t ReporterCallback(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
  if (type == static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_HASH)) {
    MsprofHashData *hash_data = reinterpret_cast<MsprofHashData *>(data);
    hash_data->hashId = 22U;
    return MsprofErrorCode::MSPROF_ERROR_NONE;
  }

  if (type == static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_REPORT)) {
    ReporterData *report_data = reinterpret_cast<ReporterData *>(data);
    std::string tag(report_data->tag);
    actual_info_type.insert(tag);
    return MsprofErrorCode::MSPROF_ERROR_NONE;
  }

  if (type == static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_DATA_MAX_LEN)) {
    return MsprofErrorCode::MSPROF_ERROR_NONE;
  }

  if (type == static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_INIT)) {
    actual_info_type.clear();
    return MsprofErrorCode::MSPROF_ERROR_NONE;
  }
  if (type == static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_UNINIT)) {
    actual_info_type.clear();
    return MsprofErrorCode::MSPROF_ERROR_NONE;
  }

  return -1;
}

} // namespace ge
