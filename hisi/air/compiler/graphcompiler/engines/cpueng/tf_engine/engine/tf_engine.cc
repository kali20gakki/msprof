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
#include "tf_engine.h"
#include "common/util/log.h"
#include "common/util/util.h"
#include "common/aicpu_ops_kernel_info_store/aicpu_ops_kernel_info_store.h"
#include "common/aicpu_graph_optimizer/aicpu_graph_optimizer.h"
#include "common/aicpu_ops_kernel_builder/aicpu_ops_kernel_builder.h"
#include "common/util/platform_info.h"
#include "tf_kernel_builder/tf_kernel_builder.h"

using fe::PlatformInfoManager;
namespace aicpu {
BaseEnginePtr TfEngine::instance_ = nullptr;

BaseEnginePtr TfEngine::Instance() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    instance_.reset(new (std::nothrow) TfEngine);
  });
  return instance_;
}

ge::Status TfEngine::Initialize(const std::map<string, string> &options) {
  if (ops_kernel_info_store_ == nullptr) {
    ops_kernel_info_store_ = std::make_shared<AicpuOpsKernelInfoStore>(kTfEngine);
    if (ops_kernel_info_store_ == nullptr) {
      AICPU_REPORT_INNER_ERROR(
          "Create AicpuOpsKernelInfoStore object failed for TF engine.");
      return ge::FAILED;
    }
  }
  if (graph_optimizer_ == nullptr) {
    graph_optimizer_ = std::make_shared<AicpuGraphOptimizer>(kTfEngine);
    if (graph_optimizer_ == nullptr) {
      AICPU_REPORT_INNER_ERROR(
          "Create AicpuGraphOptimizer object failed for TF engine.");
      return ge::FAILED;
    }
  }
  if (ops_kernel_builder_ == nullptr) {
    ops_kernel_builder_ = std::make_shared<AicpuOpsKernelBuilder>(kTfEngine);
    if (ops_kernel_builder_ == nullptr) {
      AICPU_REPORT_INNER_ERROR(
          "Create AicpuOpsKernelBuilder object failed for TF engine.");
      return ge::FAILED;
    }
  }
  BaseEnginePtr (* instance_ptr)() = &TfEngine::Instance;
  return LoadConfigFile(reinterpret_cast<void*>(instance_ptr));
}

void TfEngine::GetOpsKernelInfoStores(std::map<string, OpsKernelInfoStorePtr> &ops_kernel_info_stores) const {
  if (ops_kernel_info_store_ != nullptr) {
    ops_kernel_info_stores[kTfOpsKernelInfo] = ops_kernel_info_store_;
  }
}

void TfEngine::GetGraphOptimizerObjs(std::map<string, GraphOptimizerPtr> &graph_optimizers) const {
  if (graph_optimizer_ != nullptr) {
    graph_optimizers[kTfGraphOptimizer] = graph_optimizer_;
  }
}

void TfEngine::GetOpsKernelBuilderObjs(std::map<string, OpsKernelBuilderPtr> &ops_kernel_builders) const {
  if (ops_kernel_builder_ != nullptr) {
    ops_kernel_builders[kTfOpsKernelBuilder] =ops_kernel_builder_;
  }
}

ge::Status TfEngine::Finalize() {
  ops_kernel_info_store_ = nullptr;
  graph_optimizer_ = nullptr;
  ops_kernel_builder_ = nullptr;
  return ge::SUCCESS;
}

FACTORY_ENGINE_CLASS_KEY(TfEngine, kTfEngine)
} // namespace aicpu

ge::Status Initialize(const std::map<string, string> &options) {
  AICPU_IF_BOOL_EXEC(
      PlatformInfoManager::Instance().InitializePlatformInfo() != ge::GRAPH_SUCCESS,
      AICPU_REPORT_CALL_ERROR(
          "Call fe::PlatformInfoManager::InitializePlatformInfo function failed");
      return ge::FAILED)
  return aicpu::TfEngine::Instance()->Initialize(options);
}

void GetOpsKernelInfoStores(std::map<string, OpsKernelInfoStorePtr> &ops_kernel_info_stores) {
  aicpu::TfEngine::Instance()->GetOpsKernelInfoStores(ops_kernel_info_stores);
}

void GetGraphOptimizerObjs(std::map<string, GraphOptimizerPtr> &graph_optimizers) {
  aicpu::TfEngine::Instance()->GetGraphOptimizerObjs(graph_optimizers);
}

void GetOpsKernelBuilderObjs(std::map<string, OpsKernelBuilderPtr> &ops_kernel_builders) {
  aicpu::TfEngine::Instance()->GetOpsKernelBuilderObjs(ops_kernel_builders);
}

ge::Status Finalize() {
  (void)fe::PlatformInfoManager::Instance().Finalize();
  return aicpu::TfEngine::Instance()->Finalize();
}
