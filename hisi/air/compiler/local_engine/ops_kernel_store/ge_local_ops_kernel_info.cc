/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "local_engine/ops_kernel_store/ge_local_ops_kernel_info.h"
#include <memory>
#include "local_engine/common/constant/constant.h"
#include "common/plugin/ge_util.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "local_engine/ops_kernel_store/op/op_factory.h"

namespace ge {
namespace ge_local {
Status GeLocalOpsKernelInfoStore::Initialize(const std::map<std::string, std::string> &options) {
  (void)options;
  GELOGI("GeLocalOpsKernelInfoStore init start.");

  OpInfo default_op_info = {.engine = kGeLocalEngineName,
                            .opKernelLib = kGeLocalOpKernelLibName,
                            .computeCost = 0,
                            .flagPartial = false,
                            .flagAsync = false,
                            .isAtomic = false,
                            .opFileName = "",
                            .opFuncName = ""};
  // Init op_info_map_
  auto all_ops = OpFactory::Instance().GetAllOps();
  for (auto &op : all_ops) {
    op_info_map_[op] = default_op_info;
  }

  GELOGI("GeLocalOpsKernelInfoStore inited success. op num=%zu", op_info_map_.size());

  return SUCCESS;
}

Status GeLocalOpsKernelInfoStore::Finalize() {
  op_info_map_.clear();
  return SUCCESS;
}

void GeLocalOpsKernelInfoStore::GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const {
  infos = op_info_map_;
}

bool GeLocalOpsKernelInfoStore::CheckSupported(const OpDescPtr &op_desc, std::string &reason) const {
  (void)reason;
  if (op_desc == nullptr) {
    return false;
  }
  return op_info_map_.count(op_desc->GetType()) > 0;
}

Status GeLocalOpsKernelInfoStore::CreateSession(const std::map<std::string, std::string> &session_options) {
  // Do nothing
  (void)session_options;
  return SUCCESS;
}

Status GeLocalOpsKernelInfoStore::DestroySession(const std::map<std::string, std::string> &session_options) {
  // Do nothing
  (void)session_options;
  return SUCCESS;
}
Status GeLocalOpsKernelInfoStore::SetCutSupportedInfo(const NodePtr &node) {
  // 1. Whether the variable type is identified as a trainable variable
  // 2, whether to turn on smdp1 and 3
  // To meet the above two points, set the current variable
  // node to be tangent in the variable segmentation information
  (void)node;
  return SUCCESS;
}
}  // namespace ge_local
}  // namespace ge
