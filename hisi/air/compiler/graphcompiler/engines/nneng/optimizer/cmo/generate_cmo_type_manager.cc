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

#include "generate_cmo_type_manager.h"
#include "generate_cmo_type_invalid.h"
#include "generate_cmo_type_prefetch.h"
#include "generate_cmo_type_writeback.h"
#include "common/fe_log.h"

namespace fe {
Status GenerateCMOTypeManager::Register(CmoType type) {
  GenerateCMOTypeBasePtr cmo_type_ptr = nullptr;
  switch (type) {
    case CmoType::CMO_TYPE_PREFETCH:
      FE_MAKE_SHARED(cmo_type_ptr = std::make_shared<GenerateCMOTypePrefetch>(), return FAILED);
      break;
    case CmoType::CMO_TYPE_INVALID:
      FE_MAKE_SHARED(cmo_type_ptr = std::make_shared<GenerateCMOTypeInvalid>(), return FAILED);
      break;
    case CmoType::CMO_TYPE_WRITEBACK:
      FE_MAKE_SHARED(cmo_type_ptr = std::make_shared<GenerateCMOTypeWriteback>(), return FAILED);
      break;
    default:
      break;
  }
  if (cmo_type_ptr != nullptr) {
    cmo_generate_map_.insert(std::make_pair(type, cmo_type_ptr));
  }
  return SUCCESS;
}

Status GenerateCMOTypeManager::Initialize() {
  if (!cmo_generate_map_.empty()) {
    return SUCCESS;
  }
  for (int i = 0; i < static_cast<int>(CmoType::CMO_TYPE_BUTT); i++) {
    if (Register(static_cast<CmoType>(i)) != SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status GenerateCMOTypeManager::Finalize() {
  cmo_generate_map_.clear();
  return SUCCESS;
}

void GenerateCMOTypeManager::GenerateType(const ge::NodePtr &node) const {
  FE_LOGD("begin to generate cmo type for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
  for (int i = 0; i < static_cast<int>(CmoType::CMO_TYPE_BUTT); i++) {
    auto cmo_type = cmo_generate_map_.find(static_cast<CmoType>(i));
    if (cmo_type != cmo_generate_map_.end()) {
      cmo_type->second->GenerateType(node);
    }
  }
  FE_LOGD("end to generate cmo type for node:[name=%s, type=%s]",
          node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
}
}
