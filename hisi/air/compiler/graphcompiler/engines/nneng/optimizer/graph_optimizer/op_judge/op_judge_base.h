/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_OP_JUDGE_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_OP_JUDGE_BASE_H_

#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"

namespace fe {
class OpJudgeBase {
 public:
  explicit OpJudgeBase(const std::string& engine_name);
  virtual ~OpJudgeBase();

  /**
   * judge the set the highest prior imply type of op
   * @param [in|out] graph compute graph
   * @return SUCCESS or FAILED
   */
  virtual Status Judge(ge::ComputeGraph &graph) = 0;

 protected:
  std::string engine_name_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_OP_JUDGE_BASE_H_
