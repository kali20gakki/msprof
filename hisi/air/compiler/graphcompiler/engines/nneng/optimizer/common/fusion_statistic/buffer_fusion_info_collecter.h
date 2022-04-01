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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_BUFFER_FUSION_INFO_COLLECTER_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_BUFFER_FUSION_INFO_COLLECTER_H_

#include <set>
#include "common/aicore_util_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/graph_comm.h"
#include "register/graph_optimizer/fusion_common/fusion_statistic_recorder.h"

namespace fe {

using PassNameIdMap = std::map<std::string, std::set<int64_t>>;
using PassNameIdPair = std::pair<std::string, std::set<int64_t>>;

class BufferFusionInfoCollecter {
 public:
  explicit BufferFusionInfoCollecter();

  virtual ~BufferFusionInfoCollecter();

  BufferFusionInfoCollecter(const BufferFusionInfoCollecter &) = delete;

  BufferFusionInfoCollecter &operator=(const BufferFusionInfoCollecter &) = delete;

  Status CountBufferFusionTimes(ge::ComputeGraph &graph) const;

 private:
  Status GetPassNameOfScopeId(ge::ComputeGraph &graph, PassNameIdMap &pass_name_scope_id_map) const;

  Status GetPassNameOfFailedId(ge::ComputeGraph &graph, PassNameIdMap &pass_name_fusion_failed_id_map) const;

  void SetPassName(const ge::ComputeGraph &graph, std::set<std::string> &pass_name_set) const;
};
}

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_BUFFER_FUSION_INFO_COLLECTER_H_
