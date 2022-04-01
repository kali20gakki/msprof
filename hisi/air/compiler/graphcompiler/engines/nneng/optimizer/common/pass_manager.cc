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

#include "common/pass_manager.h"
#include "common/configuration.h"
#include "common/fe_error_code.h"
#include "common/fe_log.h"
#include "graph_optimizer/fusion_common/graph_node_map_util.h"

namespace fe {
const vector<GraphPass *> &PassManager::GraphPasses() { return graph_passes_; }

Status PassManager::AddPass(const std::string &pass_name, const std::string &engine_name, GraphPass *pass,
                            const std::string fusion_type) {
  FE_CHECK(pass == nullptr, REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][AddPass] pass is null, AddPass failed"),
           return PARAM_INVALID);
  if (fusion_config_parser_ptr_ == nullptr ||
      fusion_config_parser_ptr_->GetFusionSwitchByName(pass_name, fusion_type)) {
    pass->SetName(pass_name);
    graph_passes_.push_back(pass);
  } else {
    FE_LOGD("Fusion pass :%s switch is off.", pass_name.c_str());
  }

  return SUCCESS;
}

Status PassManager::Run(ge::ComputeGraph &graph) { return Run(graph, graph_passes_); }

Status PassManager::Run(ge::ComputeGraph &graph, vector<GraphPass *> &passes) {
  bool not_changed = true;

  for (auto pass : passes) {
    FE_CHECK(pass == nullptr, REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][RUN] pass is null, Run pass failed"),
             return PARAM_INVALID);

    Status status = pass->Run(graph);
    // if pass takes effect, set not_changed to be false
    if (status == SUCCESS) {
      not_changed = false;
    } else if (status != NOT_CHANGED) {
      // error situation
      REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][RUN] Pass Run failed, status:%d", status);
      return status;
    }
  }

  return not_changed ? NOT_CHANGED : SUCCESS;
}

Status PassManager::Run(ge::ComputeGraph &graph, OpsKernelInfoStorePtr ops_kernel_info_store_ptr) {
  return Run(graph, graph_passes_, ops_kernel_info_store_ptr);
}

Status PassManager::Run(ge::ComputeGraph &graph, vector<GraphPass *> &passes,
                        OpsKernelInfoStorePtr ops_kernel_info_store_ptr) {
  bool not_changed = true;

  for (auto &pass : passes) {
    FE_CHECK(pass == nullptr, REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][RUN] pass is null, Run pass failed"),
             return PARAM_INVALID);

    PatternFusionBasePass *convert_pass = dynamic_cast<PatternFusionBasePass *>(pass);
    FE_CHECK(convert_pass == nullptr,
             REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][RUN] Pass[%s]: fail to dynamic_cast GraphPass to \
             PatternFusionBasePass.", pass->GetName().c_str()),
             return FAILED);

    Status status = convert_pass->Run(graph, ops_kernel_info_store_ptr);
    // if pass takes effect, set not_changed to be false
    if (status == SUCCESS) {
      not_changed = false;
    } else if (status != NOT_CHANGED) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_PASS_NAME] = pass->GetName();
      error_key_map[EM_PASS_TYPE] = "built-in-ai-core-graph-pass";
      LogErrorMessage(EM_RUN_PASS_FAILED, error_key_map);
      // error situation
      REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][RUN] Pass Run failed, status:%d", status);
      return status;
    }
  }

  return not_changed ? NOT_CHANGED : SUCCESS;
}

PassManager::PassManager(FusionConfigParserPtr fusion_config_parser_ptr)
    : fusion_config_parser_ptr_(std::move(fusion_config_parser_ptr)) {}

PassManager::~PassManager() {
  for (auto pass : graph_passes_) {
    if (pass != nullptr) {
      delete pass;
      pass = nullptr;
    }
  }
}
}  // namespace fe
