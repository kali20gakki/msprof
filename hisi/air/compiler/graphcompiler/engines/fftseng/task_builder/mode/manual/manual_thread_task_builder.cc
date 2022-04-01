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

#include "manual_thread_task_builder.h"
#include "inc/ffts_utils.h"

namespace ffts {
ManualTheadTaskBuilder::ManualTheadTaskBuilder() {}
ManualTheadTaskBuilder::~ManualTheadTaskBuilder() {}

Status ManualTheadTaskBuilder::Initialize() {
  mode_type_ = ModeType::MANUAL_MODE_TYPE;
  FFTS_MAKE_SHARED(aic_aiv_task_builder_ptr_ =
          std::make_shared<AICAIVTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(mix_aic_aiv_task_builder_ptr_ =
          std::make_shared<MixAICAIVTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(collection_ops_task_builder_ptr_ =
          std::make_shared<CollectionOpsTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(aicpu_task_builder_ptr_ =
          std::make_shared<AicpuTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(runtime_ops_task_builder_ptr_ =
          std::make_shared<RuntimeOpsTaskBuilder>(), return FAILED);
  return SUCCESS;
}

void ManualTheadTaskBuilder::GenFftsPlusHcclId(const ge::NodePtr &node, uint32_t &contextId) const {
  if (kHCCLOpType.count(node->GetType()) == 0) {
    return;
  }
  ge::OpDescPtr op_desc = node->GetOpDesc();
  std::vector<domi::FftsPlusCtxDef> hccl_sub_tasks;
  std::vector<uint32_t> ctx_id_list;
  hccl_sub_tasks = op_desc->TryGetExtAttr(kHcclSubTasks, hccl_sub_tasks);
  for (size_t i = 0; i < hccl_sub_tasks.size(); i++) {
    ctx_id_list.push_back(contextId++);
  }
  if (!ctx_id_list.empty()) {
    (void)ge::AttrUtils::SetListInt(op_desc, kCtxIdList, ctx_id_list);
  }
  FFTS_LOGD("GenFftsPlusHcclId nodetype:%s, name:%s, ctx_id_list:%s", op_desc->GetType().c_str(),
            op_desc->GetName().c_str(), fe::StringUtils::IntegerVecToString(ctx_id_list).c_str());
  return;
}

Status ManualTheadTaskBuilder::GenFftsPlusContextId(ge::ComputeGraph &sgt_graph,
                                                    std::vector<ge::NodePtr> &sub_graph_nodes,
                                                    uint64_t &ready_context_num,
                                                    uint64_t &total_context_number) {
  // firstly, find node which precnt is 0
  uint32_t contextId = total_context_number;
  for (auto node : sgt_graph.GetDirectNode()) {
    if (!node) {
      continue;
    }
    ge::OpDescPtr op_desc = node->GetOpDesc();
    FFTS_LOGD("GenFftsPlusContextId nodetype:%s, name:%s", op_desc->GetType().c_str(),
              op_desc->GetName().c_str());
    if (IsNoCtx(node)) {
      continue;
    }

    // judge node's pre_cnt is 0
    bool pre_node = true;
    for (const auto &up_node : node->GetInAllNodes()) {
      ge::OpDescPtr up_op_desc = up_node->GetOpDesc();
      if (!IsSubGraphData(up_op_desc)) {
        pre_node = false;
        break;
      }
    }
    if (!pre_node) {
      continue;
    }

    GenFftsPlusHcclId(node, contextId);
    if (kHCCLOpType.count(node->GetType()) == 0) {
      (void)ge::AttrUtils::SetInt(op_desc, kContextId, contextId++);
    }
    sub_graph_nodes.push_back(node);
  }
  ready_context_num = contextId;
  // secondly, find node which precnt is non 0
  for (auto &node : sgt_graph.GetDirectNode()) {
    if (!node) {
      continue;
    }
    ge::OpDescPtr op_desc = node->GetOpDesc();
    if (IsNoCtx(node)) {
      continue;
    }
    uint32_t has_set_contextId;
    if (ge::AttrUtils::GetInt(op_desc, kContextId, has_set_contextId)) {
      continue;
    }
    GenFftsPlusHcclId(node, contextId);
    if (kHCCLOpType.count(node->GetType()) == 0) {
      (void)ge::AttrUtils::SetInt(op_desc, kContextId, contextId++);
    }

    sub_graph_nodes.push_back(node);
  }
  total_context_number = contextId;
  return SUCCESS;
}

Status ManualTheadTaskBuilder::GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes,
                                                  domi::TaskDef &task_def) {
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FFTS_CHECK_NOTNULL(ffts_plus_task_def);

  for (auto &sub_node : sub_graph_nodes) {
    TaskBuilderType task_builder_type;
    Status status = GetNodeContextTypeByNode(sub_node, task_builder_type);
    if (status != SUCCESS) {
      FFTS_LOGD("Task builder type get failed, node type:%s, name:%s.",
                sub_node->GetType().c_str(), sub_node->GetName().c_str());
      return FAILED;
    }
    FFTS_LOGD("Node type:%s, name:%s. task builder type is %u.",
              sub_node->GetType().c_str(), sub_node->GetName().c_str(), static_cast<uint32_t>(task_builder_type));

    FFTSPlusTaskBuilderPtr task_builder = GetTaskBuilder(task_builder_type);
    FFTS_CHECK_NOTNULL(task_builder);
    status = task_builder->GenerateTaskDef(sub_node, ffts_plus_task_def);
    if (status != SUCCESS) {
      return status;
    }
  }

  for (auto &sub_node : sub_graph_nodes) {
    auto sub_op_desc = sub_node->GetOpDesc();
    if (kHCCLOpType.count(sub_op_desc->GetType()) > 0) {
      vector<vector<int64_t>> succ_list_list;
      (void)ge::AttrUtils::GetListListInt(sub_op_desc, kSuccListList, succ_list_list);
      std::vector<uint32_t> ctx_id_list;
      (void)ge::AttrUtils::GetListInt(sub_op_desc, kCtxIdList, ctx_id_list);
      for (size_t i = 0; i < succ_list_list.size(); i++) {
        for (const auto &succ_id : succ_list_list[i]) {
          aic_aiv_task_builder_ptr_->UpdateSuccList(succ_id, ctx_id_list[i], ffts_plus_task_def);
        }
      }
    } else {
      vector <uint32_t> succ_lists;
      uint32_t ctx_id = 0;
      (void)ge::AttrUtils::GetInt(sub_op_desc, kContextId, ctx_id);
      (void)ge::AttrUtils::GetListInt(sub_op_desc, kSuccList, succ_lists);
      FFTS_LOGD("GenContextDef nodetype:%s, name:%s, succ_lists:%s", sub_node->GetType().c_str(),
                sub_node->GetName().c_str(), fe::StringUtils::IntegerVecToString(succ_lists).c_str());
      for (auto succ_id : succ_lists) {
        aic_aiv_task_builder_ptr_->UpdateSuccList(succ_id, ctx_id, ffts_plus_task_def);
      }
    }
  }
  for (auto &sub_node : sub_graph_nodes) {
    Status status = GenerateDataTaskDef(sub_node, ffts_plus_task_def, mode_type_);
    if (status != SUCCESS) {
      return status;
    }
  }
  return SUCCESS;
}
}  // namespace ffts
