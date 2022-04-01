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

#include "engine_manager.h"

#include <string>
#include "inc/ffts_log.h"
#include "inc/ffts_utils.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_type.h"
#include "ge/ge_api_types.h"

using std::map;
using std::string;

namespace ffts {
EngineManager::EngineManager() : ffts_plus_graph_opt_(nullptr), inited_(false) {}

EngineManager::~EngineManager() {}

EngineManager &EngineManager::Instance(const std::string &engine_name) {
  FFTS_LOGD("Instance get, engine_name:[%s]", engine_name.c_str());
  static EngineManager ffts_plus_em;
  return ffts_plus_em;
}

Status EngineManager::Initialize(const map<string, string> &options, const std::string &engine_name,
                                 std::string &soc_version) {
  FFTS_LOGD("Initialize start, engine_name:[%s]", engine_name.c_str());
  FFTS_TIMECOST_START(EngineMgrInit);
  if (inited_) {
    FFTS_LOGW("EngineManager has been inited, directly return!");
    return SUCCESS;
  }
  soc_version_ = soc_version;
  FFTS_MAKE_SHARED(
          ffts_plus_graph_opt_ = std::make_shared<FFTSPlusGraphOptimizer>(),
          return FAILED);
  Status ret = ffts_plus_graph_opt_->Initialize(options, nullptr);
  if (ret != SUCCESS) {
    REPORT_FFTS_ERROR("[EngineMngr][Init] FFTSPlusGraphOptimizer init failed ");
    return GRAPH_OPTIMIZER_INIT_FAILED;
  }

  inited_ = true;
  FFTS_LOGI("Initialize successfully.");
  FFTS_TIMECOST_END(EngineMgrInit, "EngineManager::Initialize");
  return SUCCESS;
}

Status EngineManager::Finalize() {
  if (inited_) {
    FFTS_TIMECOST_START(EngineMgrFinal);
    FFTS_LOGD("Finalize begin.");

    Status ret = SUCCESS;
    if (ffts_plus_graph_opt_ != nullptr) {
      ret = ffts_plus_graph_opt_->Finalize();
      FFTS_LOGE_IF(ret != SUCCESS, "Finalize GraphOptimizer failed!");
    }

    Status ret_status = (ret != SUCCESS);
    if (ret_status) {
      FFTS_LOGW("EngineManager finalize not success!");
      return FAILED;
    }

    inited_ = false;
    FFTS_LOGD("Finalize successfully.");
    FFTS_TIMECOST_END(EngineMgrFinal, "EngineManager::Finalize");
    return SUCCESS;
  }

  FFTS_LOGW("Already Finalized, directly return.");
  return SUCCESS;
}


/*
 * to get the information of Graph Optimizer
 * param[out] the map of Graph Optimizer
 */
void EngineManager::GetGraphOptimizerObjs(map<string, GraphOptimizerPtr> &graph_optimizers,
                                          const std::string &engine_name) {
  FFTS_LOGD("Get GraphOptimizer start.");
  if (ffts_plus_graph_opt_ == nullptr) {
    FFTS_LOGD("Engine named %s is not initialized.", engine_name.c_str());
    return;
  }
  graph_optimizers[kFFTSPlusCoreName] = ffts_plus_graph_opt_;
  FFTS_LOGD("Get GraphOptimizer success.");
}

std::string EngineManager::GetSocVersion() {
  return soc_version_;
}
}  // namespace ffts
