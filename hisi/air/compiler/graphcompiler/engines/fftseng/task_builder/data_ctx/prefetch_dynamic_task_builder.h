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
#ifndef FFTS_ENGINE_TASK_BUILDER_DATA_CTX_PREFETCH_DYNAMIC_TASK_BUILDER_H_
#define FFTS_ENGINE_TASK_BUILDER_DATA_CTX_PREFETCH_DYNAMIC_TASK_BUILDER_H_
#include "task_builder/mode/data_task_builder.h"

namespace ffts {
class PrefetchDynamicTaskBuilder : public DataTaskBuilder {
 public:
  PrefetchDynamicTaskBuilder();
  ~PrefetchDynamicTaskBuilder() override;

  Status FillDynamicDataCtx(const size_t &in_anchor_index, const ge::NodePtr &node,
                            domi::FftsPlusTaskDef *ffts_plus_task_def, const rtFftsPlusContextType_t &context_type,
                            const vector<uint32_t> &context_id_list) override;

  PrefetchDynamicTaskBuilder(const PrefetchDynamicTaskBuilder &builder) = delete;
  PrefetchDynamicTaskBuilder &operator=(const PrefetchDynamicTaskBuilder &builder) = delete;
};

}  // namespace ffts
#endif  // FFTS_ENGINE_TASK_BUILDER_DATA_CTX_PREFETCH_DYNAMIC_TASK_BUILDER_H_
