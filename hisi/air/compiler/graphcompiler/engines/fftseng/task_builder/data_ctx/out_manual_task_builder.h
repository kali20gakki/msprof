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
#ifndef FFTS_ENGINE_TASK_BUILDER_DATA_CTX_OUT_MANUAL_TASK_BUILDER_H_
#define FFTS_ENGINE_TASK_BUILDER_DATA_CTX_OUT_MANUAL_TASK_BUILDER_H_
#include "task_builder/mode/data_task_builder.h"


namespace ffts {
class OutTaskBuilder : public DataTaskBuilder {
 public:
  OutTaskBuilder();
  explicit OutTaskBuilder(CACHE_OPERATION operation);
  ~OutTaskBuilder() override;

  Status FillManualDataCtx(size_t out_anchor_index, const ge::NodePtr &node, const DataContextParam &param,
                           domi::FftsPlusTaskDef *ffts_plus_task_def, domi::FftsPlusDataCtxDef *data_ctx_def) override;

  Status GetSuccessorContextId(uint32_t out_anchor_index, const ge::NodePtr &node, std::vector<uint32_t> &succ_list,
                               uint32_t &cons_cnt) override;

  Status UptSuccListOfRelatedNode(const ge::NodePtr &node, const std::vector<uint32_t> &succ_list,
                                  domi::FftsPlusTaskDef *ffts_plus_task_def) const;


  OutTaskBuilder(const OutTaskBuilder &builder) = delete;
  OutTaskBuilder &operator=(const OutTaskBuilder &builder) = delete;

};

}  // namespace ffts
#endif // FFTS_ENGINE_TASK_BUILDER_DATA_CTX_OUT_MANUAL_TASK_BUILDER_H_
