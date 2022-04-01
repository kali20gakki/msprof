/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
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

#include "dsa_ops_kernel_builder.h"

#include <map>
#include <string>
#include <vector>

#include "common/comm_log.h"
#include "common/constants_define.h"
#include "common/op_tensor_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "param_calculate/tensorsize_calculator.h"
#include "dsa/dsa_task_builder.h"
#include "register/ops_kernel_builder_registry.h"

namespace fe {
REGISTER_OPS_KERNEL_BUILDER(kDsaCoreName, DsaOpsKernelBuilder);

Status DsaOpsKernelBuilder::Initialize(const std::map<std::string, std::string> &options) {
  return SUCCESS;
}

Status DsaOpsKernelBuilder::Finalize() {
  return SUCCESS;
}

Status DsaOpsKernelBuilder::CalcOpRunningParam(ge::Node &node) {
  return SUCCESS;
}

Status DsaOpsKernelBuilder::GenerateTask(const ge::Node &node, ge::RunContext &context,
                                         std::vector<domi::TaskDef> &tasks) {
  DsaTaskBuilder task_builder;
  Status status = task_builder.GenerateTask(node, context, tasks);
  return status;
}
}  // namespace fe
