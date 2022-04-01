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

#ifndef FUSION_ENGINE_UTILS_FFTS_PLUS_TASK_BUILDER_FFTS_PLUS_MIX_L2_MODE_H_
#define FUSION_ENGINE_UTILS_FFTS_PLUS_TASK_BUILDER_FFTS_PLUS_MIX_L2_MODE_H_
#include "task_builder/mode/thread_task_builder.h"

namespace ffts {
class Mixl2ModeTaskBuilder : public TheadTaskBuilder {
 public:
  Mixl2ModeTaskBuilder();
  ~Mixl2ModeTaskBuilder() override;

  Status Initialize() override;

  Status GenFftsPlusContextId(ge::ComputeGraph &sgt_graph, std::vector<ge::NodePtr> &sub_graph_nodes,
                              uint64_t &ready_context_num, uint64_t &total_context_number) override;
  Status GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes, domi::TaskDef &task_def) override;
};
}
#endif