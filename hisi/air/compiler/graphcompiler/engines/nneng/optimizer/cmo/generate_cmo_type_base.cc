/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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


#include "generate_cmo_type_base.h"
#include <string>
#include "common/fe_log.h"
#include "common/configuration.h"
#include "common/op_info_common.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_attr_define.h"
#include "common/util/platform_info.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/op_desc_utils.h"

namespace fe {
const std::string kAssociatedAttr = "AssociatedTask_";
const uint64_t kDefaultL2CacheSize = 8388608; // 8*1024*1024

GenerateCMOTypeBase::GenerateCMOTypeBase() {
}

void GenerateCMOTypeBase::AddToNodeCmoAttr(const ge::OpDescPtr &op_desc,
                                           const std::string &cmo_type,
                                           const std::vector<CmoAttr> &attr_vec) const {
  map<std::string, std::vector<CmoAttr>> cmo{};
  cmo = op_desc->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  const auto iter = cmo.find(cmo_type);
  if (iter == cmo.end()) {
    cmo.emplace(std::make_pair(cmo_type, attr_vec));
  } else {
    iter->second.insert(iter->second.cend(), attr_vec.cbegin(), attr_vec.cend());
  }
  (void)op_desc->SetExtAttr(kOpExtattrNameCmo, cmo);

  // for GE cut stream
  int32_t associate_task_count = 0;
  associate_task_count = op_desc->TryGetExtAttr(kAssociatedAttr, associate_task_count);
  associate_task_count += static_cast<int32_t>(attr_vec.size());
  if (associate_task_count < 0) {
    associate_task_count = INT32_MAX;
  }
  (void)op_desc->SetExtAttr(kAssociatedAttr, associate_task_count);
}

bool GenerateCMOTypeBase::CheckParentOpIsAiCore(const ge::InDataAnchorPtr &in_anchor) const {
  auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
  if (peer_out_anchor == nullptr) {
    return false;
  }

  auto peer_out_node = peer_out_anchor->GetOwnerNode();
  if (peer_out_node == nullptr || peer_out_node->GetOpDesc() == nullptr) {
    return false;
  }

  return IsAiCoreOp(peer_out_node);
}

bool GenerateCMOTypeBase::ReadIsLifeCycleEnd(const ge::NodePtr &node, const ge::InDataAnchorPtr &in_anchor) const {
  auto idx        = in_anchor->GetIdx();
  auto op_desc    = node->GetOpDesc();
  auto input_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(idx));
  if (input_desc == nullptr) {
    return false;
  }
  return IsLifeCycleEnd(*node, input_desc, idx);
}

void GenerateCMOTypeBase::CalcTotalTensorSize(const ge::GeTensorDescPtr &tensor_desc,
                                              int64_t &total_tensor_size) const {
  int64_t tensor_size = 0;
  if (ge::TensorUtils::GetSize(*tensor_desc, tensor_size) != ge::GRAPH_SUCCESS) {
    return;
  }
  total_tensor_size += tensor_size;
  if (total_tensor_size < 0) {
    total_tensor_size = INT64_MAX;
  }
  return;
}

int64_t GenerateCMOTypeBase::GetInputTensorSize(const ge::OpDescPtr &op_desc) const {
  const size_t inputs_size = op_desc->GetAllInputsSize();
  int64_t total_tensor_size = 0;
  for (size_t i = 0; i < inputs_size; i++) {
    ge::GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      FE_LOGW("paramater tensor_desc should not be nullptr at index(%zu)", i);
      continue;
    }
    CalcTotalTensorSize(tensor_desc, total_tensor_size);
  }
  return total_tensor_size;
}

int64_t GenerateCMOTypeBase::GetOutputTensorSize(const ge::OpDescPtr &op_desc) const {
  const size_t outputs_size = op_desc->GetOutputsSize();
  int64_t total_tensor_size = 0;
  for (size_t i = 0; i < outputs_size; i++) {
    ge::GeTensorDescPtr tensor_desc = op_desc->MutableOutputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      FE_LOGW("paramater tensor_desc should not be nullptr at index(%zu)", i);
      continue;
    }
    CalcTotalTensorSize(tensor_desc, total_tensor_size);
  }
  return total_tensor_size;
}

int64_t GenerateCMOTypeBase::GetWorkSpaceSize(const ge::OpDescPtr &op_desc) const {
  int64_t total_workspace_size = 0;
  const std::vector<int64_t> v_workspace_size = op_desc->GetWorkspaceBytes();
  for (const auto workspace_bytes : v_workspace_size) {
    total_workspace_size += workspace_bytes;
  }
  return total_workspace_size;
}

int64_t GenerateCMOTypeBase::GetWeightSize(const ge::NodePtr &node) const {
  int64_t total_weight_size = 0;
  vector<ge::ConstGeTensorPtr> weights = ge::OpDescUtils::GetWeights(*node);
  for (const ge::ConstGeTensorPtr& weight : weights) {
    total_weight_size += static_cast<int64_t>(weight->GetData().size());
  }
  return total_weight_size;
}

uint64_t GenerateCMOTypeBase::GetCacheSize() const {
  static uint64_t cache_size = 0;
  if (cache_size == 0) {
    string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
    PlatformInfo platform_info;
    OptionalInfo opti_compilation_info;
    if (PlatformInfoManager::Instance().GetPlatformInfo(soc_version, platform_info, opti_compilation_info) != SUCCESS) {
      FE_LOGW("soc version:%s cannot get l2 cache size, use default:%lu", soc_version.c_str(), kDefaultL2CacheSize);
      cache_size = kDefaultL2CacheSize;
    } else {
      cache_size = platform_info.soc_info.l2_size;
    }
  }
  return cache_size;
}
} // namespace fe
