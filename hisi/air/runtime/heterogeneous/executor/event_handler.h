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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_EVENT_HANDLER_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_EVENT_HANDLER_H_

#include <cstdint>
#include <string>
#include "ge/ge_api_error_codes.h"
#include "executor/executor_context.h"
#include "proto/deployer.pb.h"
#include "hccl/hccl_types.h"

namespace ge {
class EventHandler {
 public:
  Status Initialize();
  void HandleEvent(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response);

 private:
  void HandlePreDownloadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response);
  void HandleDownloadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) const;
  void HandleLoadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) const;
  void HandleUnloadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) const;
  void HandleInitVarManagerRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response);
  void HandleFillSharedContent(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response);
  void HandleDeployRankTable(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response);

  std::unique_ptr<ExecutorContext> context_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_EVENT_HANDLER_H_
