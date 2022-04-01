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

#ifndef FFTS_ENGINE_ENGINE_ENGINE_MANAGER_H_
#define FFTS_ENGINE_ENGINE_ENGINE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "inc/common/util/platform_info.h"
#include "optimizer/graph_optimizer/fftsplus_graph_optimizer.h"
#include "inc/ffts_error_codes.h"

namespace ffts {
using FFTSPlusGraphOptimizerPtr = std::shared_ptr<FFTSPlusGraphOptimizer>;
using GraphOptimizerPtr = std::shared_ptr<ge::GraphOptimizer>;

class EngineManager {
public:
  static EngineManager &Instance(const std::string &engine_name);

  Status Initialize(const std::map<string, string> &options, const std::string &engine_name, std::string &soc_version);

  Status Finalize();

  /*
   * to get the information of Graph Optimizer
   * param[out] the map of Graph Optimizer
   */
  void GetGraphOptimizerObjs(map<string, GraphOptimizerPtr> &graph_optimizers, const std::string &engine_name);

  std::string GetSocVersion();

private:
  EngineManager();
  ~EngineManager();
  FFTSPlusGraphOptimizerPtr ffts_plus_graph_opt_;
  std::string soc_version_;
  bool inited_;
};
}  // namespace ffts

#endif  // FFTS_ENGINE_ENGINE_ENGINE_MANAGER_H_
