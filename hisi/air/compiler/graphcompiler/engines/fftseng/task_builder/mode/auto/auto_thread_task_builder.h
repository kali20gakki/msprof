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

#ifndef FFTS_ENGINE_TASK_BUILDER_MODE_AUTO_AUTO_THREAD_TASK_BUILDER_H_
#define FFTS_ENGINE_TASK_BUILDER_MODE_AUTO_AUTO_THREAD_TASK_BUILDER_H_
#include "task_builder/mode/thread_task_builder.h"
#include "common/opskernel/ops_kernel_builder.h"

namespace ffts {
class AutoTheadTaskBuilder : public TheadTaskBuilder {
 public:
  AutoTheadTaskBuilder();
  ~AutoTheadTaskBuilder() override;

  Status Initialize() override;

  Status GenFftsPlusContextId(ge::ComputeGraph &sgt_graph, std::vector<ge::NodePtr> &sub_graph_nodes,
                              uint64_t &ready_context_num, uint64_t &total_context_number) override;

  Status GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes,
                            domi::TaskDef &task_def) override;

 private:
  void SetCtxIdList(ge::NodePtr &node, uint32_t &context_id, const uint32_t &window_size) const;

  void GetStartOrEndFlag(const ge::NodePtr &node, bool &conn_start, bool &conn_end) const;

  void SetAllAttrInFirstNode(ge::ComputeGraph &sgt_graph, const vector<uint32_t> &at_start_ctx_id_list,
                             const uint32_t &out_label_ctx_id) const;

  void SetAttrExceptCtxIdList(ge::ComputeGraph &sgt_graph, const vector<uint32_t> &at_start_ctx_id_list,
                              const vector<uint32_t> &at_end_ctx_id_list, int &count_node_conn_end,
                              const uint32_t &out_label_ctx_id, std::vector<ge::NodePtr> &sub_graph_nodes,
                              uint64_t &total_context_number);

  Status GenInLabelAtStartCtxDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) const;

  Status GenOutLabelAtEndCtxDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) const;

  Status AddSuccListInCtx(domi::FftsPlusTaskDef *ffts_plus_task_def, const FFTSPlusTaskBuilderPtr &task_builder,
                          const vector<uint32_t> &context_id_list, const vector<uint32_t> &output_context_id_list,
                          const bool &flag_add_write_back) const;

  Status FillContextSuccList(const ge::NodePtr &sub_node, domi::FftsPlusTaskDef *ffts_plus_task_def,
                             const FFTSPlusTaskBuilderPtr &task_builder, const vector<uint32_t> &context_id_list,
                             const vector<uint32_t> &at_end_ctx_id_list) const;
};
}  // namespace ffts
#endif  // FFTS_ENGINE_TASK_BUILDER_MODE_AUTO_AUTO_THREAD_TASK_BUILDER_H_
