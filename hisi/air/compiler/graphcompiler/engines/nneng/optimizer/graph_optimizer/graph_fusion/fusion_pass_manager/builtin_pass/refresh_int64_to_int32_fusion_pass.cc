
/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include "refresh_int64_to_int32_fusion_pass.h"
#include <iostream>
#include <vector>
#include <string>
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/configuration.h"
#include "common/aicore_util_constants.h"
#include "external/graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

namespace fe {
static const string kRefreshInt64ToInt32PassName = "RefreshInt64ToInt32FusionPass";
static const string kFusedNode = "NetOutput";
static const string kPatternFusedNode = "NetOutput";

vector<FusionPattern *> RefreshInt64ToInt32FusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern(kRefreshInt64ToInt32PassName);
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("New a pattern object failed."), return patterns);
  pattern->AddOpDesc(kPatternFusedNode, {kFusedNode}).SetOutput(kPatternFusedNode);
  patterns.push_back(pattern);
  return patterns;
}

bool RefreshInt64ToInt32FusionPass::HasBeenMatched(ge::ComputeGraph &graph) const {
  bool need_refresh_int64 = true;
  string refresh_flag = "refresh_int64_flag";
  if (ge::AttrUtils::GetBool(graph, refresh_flag, need_refresh_int64) && !need_refresh_int64) {
    return true;
  }
  ge::AttrUtils::SetBool(graph, refresh_flag, false);
  return false;
}

static Status TransformConstDataType(const ge::NodePtr &node) {
  FE_CHECK_NOTNULL(node);
  auto op_desc = node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc);
  auto out_desc = op_desc->MutableOutputDesc(0);
  FE_CHECK_NOTNULL(out_desc);
  if (out_desc->GetDataType() != ge::DT_INT64) {
    return SUCCESS;
  }
  // read data from const tensor and cast data type to int32 manually
  auto tensor_ptrs = ge::OpDescUtils::MutableWeights(node);
  if (tensor_ptrs.empty()) {
    return FAILED;
  }
  ge::GeTensorPtr const_tensor_ptr = tensor_ptrs.at(0);
  FE_CHECK_NOTNULL(const_tensor_ptr);
  auto tensor_data = const_tensor_ptr->GetData();
  int64_t *const_data_ptr = reinterpret_cast<int64_t *>(tensor_data.GetData());
  FE_CHECK_NOTNULL(const_data_ptr);
  size_t size = tensor_data.GetSize() / sizeof(int64_t);
  std::vector<int32_t> cast_data_vec;
  for (size_t i = 0; i < size; ++i) {
    int32_t const_data = static_cast<int32_t>(*(const_data_ptr + i));
    cast_data_vec.push_back(const_data);
  }
  const_tensor_ptr->SetData(reinterpret_cast<uint8_t *>(cast_data_vec.data()), size * sizeof(int32_t));
  const_tensor_ptr->MutableTensorDesc().SetDataType(ge::DT_INT32);
  out_desc->SetDataType(ge::DT_INT32);
  return SUCCESS;
}

static Status TransformNonConstDataType(const ge::NodePtr &node) {
  FE_CHECK_NOTNULL(node);
  auto op_desc = node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc);
  // update input desc
  for (const auto &input_desc : op_desc->GetAllInputsDescPtr()) {
    if (input_desc != nullptr && input_desc->GetDataType() == ge::DT_INT64) {
      input_desc->SetDataType(ge::DT_INT32);
    }
  }
  // update output desc
  for (const auto &output_desc : op_desc->GetAllOutputsDescPtr()) {
    if (output_desc != nullptr && output_desc->GetDataType() == ge::DT_INT64) {
      output_desc->SetDataType(ge::DT_INT32);
    }
  }
  return SUCCESS;
}

static bool IsConstNode(const ge::NodePtr &node) {
  return (node->GetType() == CONSTANT || node->GetType() == CONSTANTOP);
}

Status RefreshInt64ToInt32FusionPass::RefreshInt64ToInt32(const ge::ComputeGraph &graph) const {
  FE_LOGD("Begin to refresh int64 data type to int32.");
  string soc_version = fe::Configuration::Instance(fe::AI_CORE_NAME).GetSocVersion();
  bool isLHISI = ((soc_version == SOC_VERSION_HI3796CV300ES || soc_version == SOC_VERSION_HI3796CV300CS ||
                   soc_version == SOC_VERSION_SD3403));
  if (!isLHISI) {
    FE_LOGD("SocVersion is not lhisi, graph not change");
    return SUCCESS;
  }

  for (auto &node : graph.GetDirectNode()) {
    FE_CHECK_NOTNULL(node);
    auto op_desc = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc);
    Status ret;
    if (IsConstNode(node)) {
      ret = TransformConstDataType(node);
    } else {
      ret = TransformNonConstDataType(node);
    }
    if (ret != SUCCESS) {
      return FAILED;
    }
  }
  FE_LOGD("Refresh int64 data type for lhisi success");
  return SUCCESS;
}

Status RefreshInt64ToInt32FusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                                             vector<ge::NodePtr> &fusionNodes) {
  FE_LOGD("Start to do RefreshInt64ToInt32FusionPass");
  if (HasBeenMatched(graph)) {
    return SUCCESS;
  }
  return RefreshInt64ToInt32(graph);
}

REGISTER_PASS(kRefreshInt64ToInt32PassName, BUILT_IN_GRAPH_PASS, RefreshInt64ToInt32FusionPass);
}  // namespace fe