/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_RUNTIME_OPS_MANUAL_TASK_BUILDER_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_RUNTIME_OPS_MANUAL_TASK_BUILDER_H_
#include <map>
#include <memory>
#include <vector>
#include "task_builder/fftsplus_task_builder.h"
#include "proto/task.pb.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "runtime/rt.h"

namespace ffts {
using FftsPlusCtxDefPtr = std::shared_ptr<domi::FftsPlusCtxDef>;
// used for label switch
class RuntimeOpsTaskBuilder : public FFTSPlusTaskBuilder {
 public:
  RuntimeOpsTaskBuilder();
  ~RuntimeOpsTaskBuilder() override;

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
  RuntimeOpsTaskBuilder(const RuntimeOpsTaskBuilder &builder) = delete;
  RuntimeOpsTaskBuilder &operator=(const RuntimeOpsTaskBuilder &builder) = delete;

 private:
  Status JudgeContextType(const ge::NodePtr &node, rtFftsPlusContextType_t &context_type);
  Status FillCaseSwitchContextData(const domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def_ptr,
                                   domi::FftsPlusCaseSwitchCtxDef *case_switch_ctx_def) const;
  Status GenCondSwitchContextTaskDef(FftsPlusComCtx_t &sub_ffts_plus_context,
                                     domi::FftsPlusCondSwitchCtxDef *task_def_ptr);
  Status FillSdmaContextData(const domi::FftsPlusSdmaCtxDef *sdma_ctx_def_ptr,
                             domi::FftsPlusSdmaCtxDef *sdma_ctx_def) const;
  Status FillLabelContext(const ge::OpDescPtr &op_desc, domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                          vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;
  Status FillSdmaContext(const ge::OpDescPtr &op_desc, domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                         FftsPlusCtxDefPtr &ctx_def_ptr, vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;
  Status FillCaseSwitchContext(const ge::OpDescPtr &op_desc, domi::FftsPlusCtxDef *ffts_plus_ctx_def,
                               FftsPlusCtxDefPtr &ctx_def_ptr, vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;
 protected:
  const std::unordered_set<std::string> COND_SWITCH_NODE_TYPE = {"StreamSwitch", "LabelSwitchByIndex"};
  const std::unordered_set<std::string> CASE_SWITCH_NODE_TYPE = {"StreamSwitch", "LabelSwitchByIndex"};
  const std::unordered_set<std::string> SDMA_NODE_TYPE = {"MemcpyAsync", "Enter", "RefEnter",
                                                          "LoopCond", "NextIteration", "RefNextIteration",
                                                          "Exit", "RefExit", "Identity"};
  const std::unordered_set<std::string> LABEL_NODE_TYPE = {"LabelSet", "LabelGotoEx", "LabelGoto"};
  const std::map<rtFftsPlusContextType_t, std::unordered_set<std::string>> RTS_CONTEXT_TYPE_MAP = {
      {RT_CTX_TYPE_CASE_SWITCH, CASE_SWITCH_NODE_TYPE}, {RT_CTX_TYPE_LABEL, LABEL_NODE_TYPE},
      {RT_CTX_TYPE_SDMA, SDMA_NODE_TYPE}};

};

}  // namespace ffts
#endif  // AIR_COMPILER_GRAPHCOMPILER_ENGINE_FFTSENG_TASK_BUILDER_THREAD_CTX_RUNTIME_OPS_MANUAL_TASK_BUILDER_H_
