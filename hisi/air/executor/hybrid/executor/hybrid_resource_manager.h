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

#ifndef GE_HYBRID_EXECUTOR_HYBRID_RESOURCE_MANAGER_H_
#define GE_HYBRID_EXECUTOR_HYBRID_RESOURCE_MANAGER_H_

#include "hybrid/executor/hybrid_data_flow.h"
#include "hybrid/model/graph_item.h"

namespace ge {
namespace hybrid {
class ResourceManager {
 public:
  ResourceManager() = default;
  explicit ResourceManager(const GraphItem *const graph_item) : graph_item_(graph_item) {}
  ~ResourceManager() = default;
  Status Init(const GraphItem *const graph_item);
  DataFlowResourcePtr GetDataFlowResource(const int64_t handle) const;
  DataFlowKernelBasePtr GetDataFlowKernel(const std::string &type) const;
  void ClearDataFlowResources();

 private:
  Status InitDataFlowResource();
  mutable std::mutex mutex_;
  const GraphItem *graph_item_ = nullptr;
  // for data flow ops
  std::unordered_map<int64_t, DataFlowResourcePtr> data_flow_resources_;
  std::unordered_map<std::string, DataFlowKernelBasePtr> data_flow_kernels_;
};
}  // namespace hybrid
}  // namespace ge
#endif  // GE_HYBRID_EXECUTOR_HYBRID_RESOURCE_MANAGER_H_
