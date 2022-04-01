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

#ifndef FUSION_ENGINE_UTILS_FFTS_PLUS_TASK_BUILDER_MIX_l2_TASK_BUILDER_H_
#define FUSION_ENGINE_UTILS_FFTS_PLUS_TASK_BUILDER_MIX_l2_TASK_BUILDER_H_
#include <map>
#include <memory>
#include <vector>
#include "proto/task.pb.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "task_builder/fftsplus_task_builder.h"
namespace ffts {
using FftsPlusCtxDefPtr = std::shared_ptr<domi::FftsPlusCtxDef>;
class MixL2TaskBuilder : public FFTSPlusTaskBuilder {
 public:
  MixL2TaskBuilder();
  ~MixL2TaskBuilder() override;

  /*
   * @ingroup fe
   * @brief   Generate tasks
   * @param   [in] node Node of compute graph
   * @param   [in] context Context for generate tasks
   * @param   [out] task_defs Save the generated tasks.
   * @return  SUCCESS or FAILED
   */
  Status GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) override;
  Status FillContextData(const ge::NodePtr &node, const domi::FftsPlusMixAicAivCtxDef *src_ctx_def,
                         domi::FftsPlusMixAicAivCtxDef *dst_ctx_def) const;
 private:
  MixL2TaskBuilder(const MixL2TaskBuilder &builder) = delete;
  MixL2TaskBuilder &operator=(const MixL2TaskBuilder &builder) = delete;

  Status AddAdditionalArgs(ge::OpDescPtr &op_desc, domi::FftsPlusTaskDef *ffts_plus_task_def,
                           const size_t &ctx_num) const;
};
}
#endif
