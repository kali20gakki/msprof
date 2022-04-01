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
#ifndef EXECUTOR_RUNTIME_LOCAL_EXECUTION_RUNTIME_H_
#define EXECUTOR_RUNTIME_LOCAL_EXECUTION_RUNTIME_H_

#include "exec_runtime/execution_runtime.h"
#include "runtime/deploy/local_model_deployer.h"
#include "runtime/deploy/local_exchange_service.h"

namespace ge {
class LocalExecutionRuntime : public ExecutionRuntime {
 public:
  Status Initialize(const std::map<std::string, std::string> &options) override;

  Status Finalize() override;

  ModelDeployer &GetModelDeployer() override;

  ExchangeService &GetExchangeService() override;

 private:
  LocalModelDeployer model_deployer_;
  LocalExchangeService exchange_service_;
};
}  // namespace ge
#endif  // EXECUTOR_RUNTIME_LOCAL_EXECUTION_RUNTIME_H_
