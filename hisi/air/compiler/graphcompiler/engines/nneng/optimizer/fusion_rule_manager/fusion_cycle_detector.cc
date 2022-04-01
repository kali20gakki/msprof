/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "fusion_rule_manager/fusion_cycle_detector.h"
#include "common/fe_log.h"

namespace fe {
FusionCycleDetector::FusionCycleDetector() {}

FusionCycleDetector::~FusionCycleDetector() {}

std::vector<FusionPattern *> FusionCycleDetector::DefinePatterns() {
  std::vector<FusionPattern *> ret;
  return ret;
}

Status FusionCycleDetector::Fusion(ge::ComputeGraph &graph, Mapping &mapping, std::vector<ge::NodePtr> &new_nodes) {
  return SUCCESS;
}

Status FusionCycleDetector::Initialize(const ge::ComputeGraph &graph) {
  std::unique_ptr<ConnectionMatrix> connection_matrix;

  GetConnectionMatrix(connection_matrix);
  if (connection_matrix == nullptr) {
    FE_MAKE_SHARED(connection_matrix =
        std::unique_ptr<fe::ConnectionMatrix>(new(std::nothrow) fe::ConnectionMatrix(graph)),
        return FAILED);
  }
  connection_matrix->Generate(graph);
  SetConnectionMatrix(connection_matrix);
  return SUCCESS;
}

Status FusionCycleDetector::UpdateConnectionMatrix(
    const ge::ComputeGraph &graph, vector<ge::NodePtr> &fusion_nodes) {
  std::unique_ptr<fe::ConnectionMatrix> connection_matrix;
  GetConnectionMatrix(connection_matrix);
  if (connection_matrix == nullptr) {
    return FAILED;
  }

  connection_matrix->Update(graph, fusion_nodes);
  SetConnectionMatrix(connection_matrix);
  return SUCCESS;
}

bool FusionCycleDetector::IsConnected(const ge::NodePtr &a, const ge::NodePtr &b) {
  std::unique_ptr<ConnectionMatrix> connection_matrix;

  GetConnectionMatrix(connection_matrix);
  if (connection_matrix == nullptr) {
    return false;
  }
  bool result = connection_matrix->IsConnected(a, b);
  SetConnectionMatrix(connection_matrix);
  return result;
}
}