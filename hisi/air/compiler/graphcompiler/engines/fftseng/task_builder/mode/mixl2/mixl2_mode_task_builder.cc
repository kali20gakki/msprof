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
 * See the License for the specific language gov erning permissions and
 * limitations under the License.
 */

#include "mixl2_mode_task_builder.h"

namespace ffts {
Mixl2ModeTaskBuilder::Mixl2ModeTaskBuilder() {}
Mixl2ModeTaskBuilder::~Mixl2ModeTaskBuilder() {}

Status Mixl2ModeTaskBuilder::Initialize() {
  mode_type_ = ModeType::MIX_L2_MODE_TYPE;
  FFTS_MAKE_SHARED(mix_L2_task_builder_ptr_ = std::make_shared<MixL2TaskBuilder>(), return FAILED);
  return SUCCESS;
}

Status Mixl2ModeTaskBuilder::GenFftsPlusContextId(ge::ComputeGraph &sgt_graph,
                                                  std::vector<ge::NodePtr> &sub_graph_nodes,
                                                  uint64_t &ready_context_num, uint64_t &total_context_number) {
  (void)sgt_graph;
  if (sub_graph_nodes.empty()) {
    REPORT_FFTS_ERROR("[FFTSPlusMixL2Mode] [GenFftsPlusContextId] No node to generate task.");
    return FAILED;
  }
  ge::NodePtr node = sub_graph_nodes[0];
  uint32_t contextId = total_context_number;
  ge::OpDescPtr op_desc = node->GetOpDesc();
  FFTS_LOGD("GenFftsPlusContextId nodetype:%s, name:%s", op_desc->GetType().c_str(),
            op_desc->GetName().c_str());
  (void)ge::AttrUtils::SetInt(op_desc, kContextId, contextId++);
  ready_context_num = contextId;
  total_context_number = contextId;
  return SUCCESS;
}

Status Mixl2ModeTaskBuilder::GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes,
                                                domi::TaskDef &task_def) {
  if (sub_graph_nodes.empty()) {
    REPORT_FFTS_ERROR("[FFTSPlusMixL2Mode] [GenSubGraphTaskDef] No node to generate taskdef.");
    return FAILED;
  }
  ge::NodePtr node = sub_graph_nodes[0];
  FFTS_LOGD("GenSubGraphTaskDef name:%s", node->GetOpDesc()->GetName().c_str());
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FFTS_CHECK_NOTNULL(ffts_plus_task_def);

  TaskBuilderType task_builder_type = TaskBuilderType::EN_TASK_TYPE_MIX_L2_AIC_AIV;
  FFTSPlusTaskBuilderPtr task_builder = GetTaskBuilder(task_builder_type);
  FFTS_CHECK_NOTNULL(task_builder);
  Status status = task_builder->GenerateTaskDef(node, ffts_plus_task_def);
  return status;
}
}  // namespace ffts
