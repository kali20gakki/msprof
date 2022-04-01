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

#ifndef FFTS_ENGINE_INC_FFTS_UTIL_H_
#define FFTS_ENGINE_INC_FFTS_UTIL_H_

#include <memory>
#include <vector>
#include "inc/ffts_log.h"
#include "inc/ffts_type.h"
#include "graph/op_desc.h"
#include "graph/node.h"
#include "graph/utils/type_utils.h"

namespace ffts {
// send error message to error manager
void LogErrorMessage(std::string error_code, const std::map<std::string, std::string> &args_map);

int64_t GetMicroSecondTime();

FFTSMode GetPlatformFFTSMode();

void DumpSubGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix);

void DumpGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix);

void DumpGraph(const ge::ComputeGraph &graph, const std::string &suffix);

bool IsUnKnownShapeOp(const ge::OpDesc &op_desc);

std::string RealPath(const std::string &path);

void IsNodeSpecificType(const std::unordered_set<string> &types,
                        ge::NodePtr &node, bool &matched);

bool IsPeerOutWeight(ge::NodePtr &node, const int &anchor_index,
                     ge::NodePtr &peer_out_node);

bool IsSubGraphData(const ge::OpDescPtr &op_desc_ptr);

bool IsSubGraphNetOutput(const ge::OpDescPtr &op_desc_ptr);

// Record the start time of stage.
#define FFTS_TIMECOST_START(stage) int64_t start_usec_##stage = GetMicroSecondTime();

// Print the log of time cost of stage.
#define FFTS_TIMECOST_END(stage, stage_name)                                              \
  {                                                                                     \
    int64_t end_usec_##stage = GetMicroSecondTime();                                    \
    FFTS_LOGV("[FFTS_PERFORMANCE]The time cost of %s is [%ld] micro second.", (stage_name), \
            (end_usec_##stage - start_usec_##stage));                                   \
  }

template <typename T, typename... Args>
inline std::shared_ptr<T> FFTSComGraphMakeShared(Args &&... args) {
  using T_nc = typename std::remove_const<T>::type;
  std::shared_ptr<T> ret(new (std::nothrow) T_nc(std::forward<Args>(args)...));
  return ret;
}

inline Status CheckInt64AddOverflow(int64_t a, int64_t b) {
  if (((b > 0) && (a > (static_cast<int64_t>(INT64_MAX) - b))) ||
      ((b < 0) && (a < (static_cast<int64_t>(INT64_MIN) - b)))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckInt64MulOverflow(int64_t m, int64_t n) {
  if (m > 0) {
    if (n > 0) {
      if (m > (static_cast<int64_t>(INT64_MAX) / n)) {
        return FAILED;
      }
    } else {
      if (n < (static_cast<int64_t>(INT64_MIN) / m)) {
        return FAILED;
      }
    }
  } else {
    if (n > 0) {
      if (m < (static_cast<int64_t>(INT64_MIN) / n)) {
        return FAILED;
      }
    } else {
      if ((m != 0) && (n < (static_cast<int64_t>(INT64_MAX) / m))) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

#define FFTS_INT64_ADDCHECK(a, b)                                              \
  if (CheckInt64AddOverflow((a), (b)) != SUCCESS) {                          \
    FFTS_LOGE("Int64 %ld and %ld addition can result in overflow!", (a), (b)); \
    return FAILED;                                                   \
  }

#define FFTS_INT64_MULCHECK(a, b)                                                                      \
  if (CheckInt64MulOverflow((a), (b)) != SUCCESS) {                                                  \
    FFTS_LOGE("INT64 %ld and %ld multiplication can result in overflow!", (int64_t)(a), (int64_t)(b)); \
    return FAILED;                                                                           \
  }
}  // namespace ffts
#endif  // FFTS_ENGINE_INC_FFTS_UTIL_H_
