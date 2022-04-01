/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "aicpu_engine.h"
#include "common/util/log.h"
#include "common/util/platform_info.h"
#include "common/util/util.h"

using fe::PlatformInfoManager;
using namespace ge;
namespace aicpu {

BaseEnginePtr AicpuEngine::instance_ = nullptr;

BaseEnginePtr AicpuEngine::Instance() {
  static std::once_flag flag;
  std::call_once(flag,
                 []() { instance_.reset(new (std::nothrow) AicpuEngine); });
  return instance_;
}

ge::Status AicpuEngine::Initialize(
    const std::map<std::string, std::string> &options) {
  if (ops_kernel_info_store_ == nullptr) {
    ops_kernel_info_store_ =
        std::make_shared<AicpuOpsKernelInfoStore>(kAicpuEngine);
    if (ops_kernel_info_store_ == nullptr) {
      AICPU_REPORT_INNER_ERROR("Create AicpuOpsKernelInfoStore object falied");
      return ge::FAILED;
    }
  }
  if (graph_optimizer_ == nullptr) {
    graph_optimizer_ = std::make_shared<AicpuGraphOptimizer>(kAicpuEngine);
    if (graph_optimizer_ == nullptr) {
      AICPU_REPORT_INNER_ERROR("Create AicpuGraphOptimizer object falied");
      return ge::FAILED;
    }
  }
  if (ops_kernel_builder_ == nullptr) {
    ops_kernel_builder_ = std::make_shared<AicpuOpsKernelBuilder>(kAicpuEngine);
    if (ops_kernel_builder_ == nullptr) {
      AICPU_REPORT_INNER_ERROR("Create AicpuOpsKernelBuilder object falied");
      return ge::FAILED;
    }
  }
  BaseEnginePtr (*instance_ptr)() = &AicpuEngine::Instance;
  return LoadConfigFile(reinterpret_cast<void *>(instance_ptr));
}

void AicpuEngine::GetOpsKernelInfoStores(
    std::map<std::string, OpsKernelInfoStorePtr> &ops_kernel_info_stores)
    const {
  if (ops_kernel_info_store_ != nullptr) {
    ops_kernel_info_stores[kAicpuOpsKernelInfo] = ops_kernel_info_store_;
  }
}

void AicpuEngine::GetGraphOptimizerObjs(
    std::map<std::string, GraphOptimizerPtr> &graph_optimizers) const {
  if (graph_optimizer_ != nullptr) {
    graph_optimizers[kAicpuGraphOptimizer] = graph_optimizer_;
  }
}

void AicpuEngine::GetOpsKernelBuilderObjs(
    std::map<std::string, OpsKernelBuilderPtr> &ops_kernel_builders) const {
  if (ops_kernel_builder_ != nullptr) {
    ops_kernel_builders[kAicpuOpsKernelBuilder] = ops_kernel_builder_;
  }
}

ge::Status AicpuEngine::Finalize() {
  ops_kernel_info_store_ = nullptr;
  graph_optimizer_ = nullptr;
  ops_kernel_builder_ = nullptr;
  return ge::SUCCESS;
}

FACTORY_ENGINE_CLASS_KEY(AicpuEngine, kAicpuEngine)

}  // namespace aicpu

ge::Status Initialize(const std::map<std::string, std::string> &options) {
  AICPU_IF_BOOL_EXEC(
      PlatformInfoManager::Instance().InitializePlatformInfo() != ge::GRAPH_SUCCESS,
      AICPU_REPORT_CALL_ERROR(
          "Call fe::PlatformInfoManager::InitializePlatformInfo function failed");
      return ge::FAILED)
  return aicpu::AicpuEngine::Instance()->Initialize(options);
}

void GetOpsKernelInfoStores(
    std::map<std::string, OpsKernelInfoStorePtr> &ops_kernel_info_stores) {
  aicpu::AicpuEngine::Instance()->GetOpsKernelInfoStores(
      ops_kernel_info_stores);
}

void GetGraphOptimizerObjs(
    std::map<std::string, GraphOptimizerPtr> &graph_optimizers) {
  aicpu::AicpuEngine::Instance()->GetGraphOptimizerObjs(graph_optimizers);
}

void GetOpsKernelBuilderObjs(
    std::map<std::string, OpsKernelBuilderPtr> &ops_kernel_builders) {
  aicpu::AicpuEngine::Instance()->GetOpsKernelBuilderObjs(ops_kernel_builders);
}

ge::Status Finalize() {
  (void)PlatformInfoManager::Instance().Finalize();
  return aicpu::AicpuEngine::Instance()->Finalize();
}
