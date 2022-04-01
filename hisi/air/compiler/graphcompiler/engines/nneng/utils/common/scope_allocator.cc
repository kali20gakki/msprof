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

#include "common/scope_allocator.h"
#include "common/aicore_util_attr_define.h"
#include "common/comm_log.h"
#include "graph/utils/attr_utils.h"

namespace fe {
ScopeAllocator::ScopeAllocator() : scope_id_(0), neg_scope_id_(0) {}

ScopeAllocator::~ScopeAllocator() {}

ScopeAllocator& ScopeAllocator::Instance() {
  static ScopeAllocator scope_allocator;
  return scope_allocator;
}

int64_t ScopeAllocator::AllocateScopeId() {
  scope_id_++;
  return scope_id_;
}

int64_t ScopeAllocator::GetCurrentScopeId() const {
  return scope_id_;
}

int64_t ScopeAllocator::AllocateNegScopeId() {
  neg_scope_id_--;
  return neg_scope_id_;
}

int64_t ScopeAllocator::GetCurrentNegScopeId() const {
  return neg_scope_id_;
}

bool ScopeAllocator::HasScopeAttr(ge::ConstOpDescPtr op_desc) {
  if (op_desc == nullptr) {
    return false;
  }

  return op_desc->HasAttr(SCOPE_ID_ATTR);
}

bool ScopeAllocator::GetScopeAttr(ge::ConstOpDescPtr op_desc, int64_t &scope_id) {
  if (op_desc == nullptr) {
    return false;
  }

  return ge::AttrUtils::GetInt(op_desc, SCOPE_ID_ATTR, scope_id);
}

bool ScopeAllocator::SetScopeAttr(const ge::OpDescPtr op_desc, const int64_t &scope_id) {
  if (op_desc == nullptr) {
    REPORT_CM_ERROR("[SubGraphOpt][PostProcess][SetScopeAttr] opdef is nullptr.");
    return false;
  }

  return ge::AttrUtils::SetInt(op_desc, SCOPE_ID_ATTR, scope_id);
}

bool ScopeAllocator::HasL1ScopeAttr(ge::ConstOpDescPtr op_desc) {
  if (op_desc == nullptr) {
    return false;
  }

  return op_desc->HasAttr(L1_SCOPE_ID_ATTR);
}

bool ScopeAllocator::GetL1ScopeAttr(ge::ConstOpDescPtr op_desc, int64_t &scope_id) {
  if (op_desc == nullptr) {
    return false;
  }

  return ge::AttrUtils::GetInt(op_desc, L1_SCOPE_ID_ATTR, scope_id);
}

bool ScopeAllocator::SetL1ScopeAttr(const ge::OpDescPtr op_desc, const int64_t &scope_id) {
  if (op_desc == nullptr) {
    REPORT_CM_ERROR("[SubGraphOpt][PostProcess][SetL1ScopeAttr] opdef is nullptr.");
    return false;
  }

  return ge::AttrUtils::SetInt(op_desc, L1_SCOPE_ID_ATTR, scope_id);
}
}  // namespace fe