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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_FUSION_STATISTIC_WRITER_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_FUSION_STATISTIC_WRITER_H_

#include <set>
#include <fstream>
#include <nlohmann/json.hpp>
#include "common/aicore_util_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/graph_comm.h"
#include "common/util/json_util.h"
#include "register/graph_optimizer/fusion_common/fusion_statistic_recorder.h"

namespace fe {

using json = nlohmann::json;
using PassNameIdMap = std::map<std::string, std::set<int64_t>>;
using PassNameIdPair = std::pair<std::string, std::set<int64_t>>;

const std::string PASS_NAME_ATTR = "pass_name";

class FusionStatisticWriter {
 public:
  static FusionStatisticWriter &Instance();

  FusionStatisticWriter(const FusionStatisticWriter &) = delete;

  FusionStatisticWriter &operator=(const FusionStatisticWriter &) = delete;

  void ClearHistoryFile();

  void Finalize();

 private:
  explicit FusionStatisticWriter();

  virtual ~FusionStatisticWriter();

  void WriteAllFusionInfoToJsonFile();

  void WriteFusionInfoToJsonFile(const std::string &file_name, const std::string &session_and_graph_id);

  void WriteJsonFile(const std::string &file_name, const std::map<std::string, FusionInfo> &graph_fusion_info_map,
                     const std::map<std::string, FusionInfo> &buffer_fusion_info_map,
                     const string &session_and_graph_id);

  void SaveFusionTimesToJson(const std::map<std::string, FusionInfo> &fusion_info_map, const string &graph_type,
                             json &js) const;

  string RealPath(const string &file_name) const;

  std::mutex file_mutex_;

  std::ofstream file_;
};
}

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FUSION_STATISTIC_FUSION_STATISTIC_WRITER_H_
