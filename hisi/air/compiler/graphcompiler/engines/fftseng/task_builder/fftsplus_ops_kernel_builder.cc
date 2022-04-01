/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "fftsplus_ops_kernel_builder.h"
#include "inc/ffts_utils.h"
#include "graph/utils/attr_utils.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "register/ops_kernel_builder_registry.h"
#include "fftsplus_task_builder.h"

namespace ffts {
const std::string kFFTSPlusCoreName = "ffts_plus";

REGISTER_OPS_KERNEL_BUILDER(kFFTSPlusCoreName, FFTSPlusOpsKernelBuilder);

FFTSPlusOpsKernelBuilder::FFTSPlusOpsKernelBuilder() {}

FFTSPlusOpsKernelBuilder::~FFTSPlusOpsKernelBuilder() {}

Status FFTSPlusOpsKernelBuilder::Initialize(const std::map<std::string, std::string> &options) {
  FFTS_MAKE_SHARED(manual_thread_task_builder_ptr_ =
          std::make_shared<ManualTheadTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(auto_thread_task_builder_ptr_ =
          std::make_shared<AutoTheadTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(mixl2_mode_task_builder_ptr_ =
          std::make_shared<Mixl2ModeTaskBuilder>(), return FAILED);
  return SUCCESS;
}

Status FFTSPlusOpsKernelBuilder::Finalize() {return SUCCESS;}

Status FFTSPlusOpsKernelBuilder::CalcOpRunningParam(ge::Node &node) {
  return SUCCESS;
}

TheadTaskBuilderPtr FFTSPlusOpsKernelBuilder::GetNormBuilder(const ge::Node &node, ge::ComputeGraphPtr &sgt_graph,
                                                             domi::TaskDef &task_def, uint64_t &ready_num,
                                                             uint64_t &total_num) {
  ge::OpDescPtr op_desc = node.GetOpDesc();
  TheadTaskBuilderPtr base_mode_ptr = nullptr;
  std::string sub_graph_name = op_desc->GetSubgraphInstanceName(0);
  if (sub_graph_name.empty()) {
    return base_mode_ptr;
  }
  auto ai_graph = node.GetOwnerComputeGraph();
  if (ai_graph == nullptr) {
    return base_mode_ptr;
  }
  auto root_graph = ge::GraphUtils::FindRootGraph(ai_graph);
  if (root_graph == nullptr) {
    return base_mode_ptr;
  }
  sgt_graph = root_graph->GetSubgraph(sub_graph_name);
  if (sgt_graph == nullptr) {
    return base_mode_ptr;
  }
  Status status = GenPersistentContext(node, ready_num, total_num, task_def);
  if (status != SUCCESS) {
    return base_mode_ptr;
  }
  base_mode_ptr = GetFftsPlusMode(*sgt_graph);
  return base_mode_ptr;
}

Status FFTSPlusOpsKernelBuilder::GenerateTask(const ge::Node &node, ge::RunContext &context,
                                              std::vector<domi::TaskDef> &task_defs) {
  FFTS_LOGD("FFTSPlusOpsKernelBuilder GenerateTask node name:%s, node type:%s",
            node.GetName().c_str(), node.GetType().c_str());
  ge::OpDescPtr op_desc = node.GetOpDesc();
  Status status;
  uint64_t ready_context_num = 0;
  uint64_t total_context_number = 0;
  domi::TaskDef task_def;
  std::vector<ge::NodePtr> sub_graph_nodes;
  ge::ComputeGraphPtr sgt_graph = nullptr;
  TheadTaskBuilderPtr  base_mode_ptr = nullptr;
  if (op_desc->HasAttr(ATTR_NAME_ALIAS_ENGINE_NAME)) {
    FFTS_LOGD("GenerateTask MIXL2");
    ge::Node &temp_node = const_cast<ge::Node&>(node);
    ge::NodePtr node_ptr = temp_node.shared_from_this();
    sub_graph_nodes.emplace_back(node_ptr);
    FFTS_MAKE_SHARED(sgt_graph = std::make_shared<ge::ComputeGraph>("MIX_L2"), return FAILED);
    base_mode_ptr = mixl2_mode_task_builder_ptr_;
  } else {
    base_mode_ptr = GetNormBuilder(node, sgt_graph, task_def, ready_context_num, total_context_number);
  }
  FFTS_CHECK_NOTNULL(base_mode_ptr);
  (void)base_mode_ptr->Initialize();
  status = base_mode_ptr->GenFftsPlusContextId(*sgt_graph, sub_graph_nodes, ready_context_num, total_context_number);
  if (status != SUCCESS) {
    return status;
  }

  FFTS_LOGD("FFTSPlusOpsKernelBuilder GenerateTask node name:%s, node type:%s, readynum:%lu, totalnumber:%lu",
            node.GetName().c_str(), node.GetType().c_str(), ready_context_num, total_context_number);

  status = base_mode_ptr->GenSubGraphTaskDef(sub_graph_nodes, task_def);
  if (status != SUCCESS) {
    FFTS_LOGD("GenSubGraphTaskDef failed, node name:%s, node type:%s, errno:%u.",
              node.GetName().c_str(), node.GetType().c_str(), status);
    return status;
  }

  if (GenSubGraphSqeDef(task_def, ready_context_num) != SUCCESS) {
    FFTS_LOGD("GenSubGraphSqeDef failed, node name:%s, node type:%s, errno:%u.",
              node.GetName().c_str(), node.GetType().c_str(), status);
    return FAILED;
  }
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  task_def.set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  ffts_plus_task_def->set_op_index(op_desc->GetId());
  task_defs.push_back(task_def);
  FFTS_LOGD("FFTSPlusOpsKernelBuilder GenerateTask node name:%s, node type:%s , id:%ld success",
            node.GetName().c_str(), node.GetType().c_str(), op_desc->GetId());
  return SUCCESS;
}

Status FFTSPlusOpsKernelBuilder::GenPersistentContext(const ge::Node &node, uint64_t &ready_context_num,
                                                      uint64_t &total_context_number, domi::TaskDef &task_def) const {
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  CachePersistTaskBuilder cp;
  if (cp.GenContextDef(node, ffts_plus_task_def) == SUCCESS) {
    total_context_number++;
    ready_context_num++;
  }
  return SUCCESS;
}

TheadTaskBuilderPtr FFTSPlusOpsKernelBuilder::GetFftsPlusMode(const ge::ComputeGraph &sgt_graph) {
  ModeType mode_type = ModeType::MANUAL_MODE_TYPE;

  for (const auto &node : sgt_graph.GetDirectNode()) {
    if (IsUnKnownShapeOp(*(node->GetOpDesc().get()))) {
      // Dynamic shape, auto thread
      mode_type = ModeType::DYNAMIC_MODE_TYPE;
      break;
    } else {
      ThreadSliceMapPtr slice_info_ptr = nullptr;
      slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
      if (slice_info_ptr != nullptr && slice_info_ptr->thread_mode == 1) {
        mode_type = ModeType::AUTO_MODE_TYPE;
        break;
      }
    }
  }

  switch (mode_type) {
    case ModeType::MANUAL_MODE_TYPE:
      return manual_thread_task_builder_ptr_;
    case ModeType::AUTO_MODE_TYPE:
      return auto_thread_task_builder_ptr_;
    case ModeType::DYNAMIC_MODE_TYPE:
      auto_thread_task_builder_ptr_->SetModeType(ModeType::DYNAMIC_MODE_TYPE);
      return auto_thread_task_builder_ptr_;
    default:
      return nullptr;
  }
}

Status FFTSPlusOpsKernelBuilder::GenSubGraphSqeDef(domi::TaskDef &task_def,
                                                   const uint64_t &ready_context_num) const {
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  uint64_t gen_ctx_num = ffts_plus_task_def->ffts_plus_ctx_size();

  domi::FftsPlusSqeDef* ffts_plus_sqe = ffts_plus_task_def->mutable_ffts_plus_sqe();
  for (size_t i = 0; i < gen_ctx_num; i++) {
    FFTS_LOGD("Gen subGraph sqe def :%s.",
              ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(i))->DebugString().c_str());
  }

  ffts_plus_sqe->set_ready_context_num(ready_context_num);
  ffts_plus_sqe->set_total_context_num(gen_ctx_num);

  return SUCCESS;
}
}  // namespace ffts
