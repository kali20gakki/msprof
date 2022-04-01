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

#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_node_executor.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.h"
#include "register/ffts_plus_update_manager.h"

namespace ge {
namespace hybrid {
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::FFTS, FftsPlusSubNodeExecutor);

Status FftsPlusSubNodeExecutor::LoadTask(const HybridModel &model, const NodePtr &node, NodeTaskPtr &task) const {
  GELOGD("Load FFTS Plus Sub task: [%s] ", node->GetName().c_str());
  std::string core_type;
  if ((!AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type)) || core_type.empty()) {
    GELOGD("[%s] Skip for CUBE_VECTOR_CORE_TYPE not set.", node->GetName().c_str());
    return SUCCESS;
  }

  if (IsFftsKernelNode(*node->GetOpDesc())) {
    core_type = "MIX_L2";
  }

  const FftsPlusSubTaskPtr ffts_plus_sub_task = FftsPlusSubTaskFactory::Instance().Create(core_type);
  if (ffts_plus_sub_task == nullptr) {
    REPORT_CALL_ERROR("E19999", "[%s] Unsupported FFTS Plus core type:%s", node->GetName().c_str(), core_type.c_str());
    GELOGE(UNSUPPORTED, "[%s] Unsupported FFTS Plus core type:%s", node->GetName().c_str(), core_type.c_str());
    return UNSUPPORTED;
  }

  task = ffts_plus_sub_task;
  GE_CHK_STATUS_RET(ffts_plus_sub_task->Load(model, node), "[%s] Sub task init failed.", node->GetName().c_str());
  GELOGD("Done loading FFTS Plus Sub task: [%s]", node->GetName().c_str());
  return SUCCESS;
}

Status FftsPlusSubNodeExecutor::Initialize() {
  GELOGD("Start Initialize.");
  GE_CHK_STATUS_RET(FftsPlusUpdateManager::Instance().Initialize(), "[Load][Libs]Failed");
  return SUCCESS;
}

Status FftsPlusSubNodeExecutor::Finalize() {
  GELOGD("Done Finalize.");
  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge