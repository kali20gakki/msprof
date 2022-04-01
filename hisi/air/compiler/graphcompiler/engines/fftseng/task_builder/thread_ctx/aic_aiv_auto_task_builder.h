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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_AIC_AIV_AUTO_TASK_BUILDER_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_AIC_AIV_AUTO_TASK_BUILDER_H_
#include "task_builder/fftsplus_task_builder.h"

namespace ffts {
class AICAIVAutoTaskBuilder : public FFTSPlusTaskBuilder {
 public:
  AICAIVAutoTaskBuilder();
  ~AICAIVAutoTaskBuilder() override;

  /*
   * @ingroup ffts
   * @brief   Generate tasks
   * @param   [in] node Node of compute graph
   * @param   [in] context Context for generate tasks
   * @param   [out] task_defs Save the generated tasks.
   * @return  SUCCESS or FAILED
   */
  Status GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) override;

 private:
  AICAIVAutoTaskBuilder(const AICAIVAutoTaskBuilder &builder) = delete;
  AICAIVAutoTaskBuilder &operator=(const AICAIVAutoTaskBuilder &builder) = delete;
};

}  // namespace ffts
#endif  // AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_AIC_AIV_AUTO_TASK_BUILDER_H_
