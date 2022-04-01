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

#ifndef FFTSENG_TASK_BUILDER_THREAD_CTX_AIC_AIV_DYNAMIC_TASK_BUILDER_H_
#define FFTSENG_TASK_BUILDER_THREAD_CTX_AIC_AIV_DYNAMIC_TASK_BUILDER_H_
#include "proto/task.pb.h"
#include "task_builder/fftsplus_task_builder.h"

namespace ffts {
class AICAIVDynamicTaskBuilder : public FFTSPlusTaskBuilder {
 public:
  AICAIVDynamicTaskBuilder();
  ~AICAIVDynamicTaskBuilder() override;
  Status GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) override;
 private:
  AICAIVDynamicTaskBuilder(const AICAIVDynamicTaskBuilder &builder) = delete;
  AICAIVDynamicTaskBuilder &operator=(const AICAIVDynamicTaskBuilder &builder) = delete;
};
}
#endif
