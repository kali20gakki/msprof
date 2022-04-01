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

#ifndef GE_HYBRID_NODE_EXECUTOR_DATA_FLOW_KERNELS_H_
#define GE_HYBRID_NODE_EXECUTOR_DATA_FLOW_KERNELS_H_

#include "hybrid/executor/hybrid_data_flow.h"
#include "hybrid/node_executor/task_context.h"

namespace ge {
namespace hybrid {
class DataFlowKernelFactory {
 public:
  using DataFlowKernelCreator = std::function<DataFlowKernelBasePtr()>;
  virtual ~DataFlowKernelFactory() = default;
  static DataFlowKernelFactory &GetInstance();
  void RegisterKernelCreator(const std::string &type, const DataFlowKernelCreator &creator);
  DataFlowKernelBasePtr CreateKernel(const std::string &type);
  class DataFlowKernelRegistrar {
   public:
    DataFlowKernelRegistrar(const std::string &type, const DataFlowKernelFactory::DataFlowKernelCreator &creator) {
      DataFlowKernelFactory::GetInstance().RegisterKernelCreator(type, creator);
    }
    ~DataFlowKernelRegistrar() = default;
  };
 private:
  DataFlowKernelFactory() = default;
  std::mutex data_flow_kernel_mutex_;
  std::unordered_map<std::string, DataFlowKernelCreator> data_flow_kernel_creators_;
};

class DataFlowStack : public DataFlowKernelBase {
 public:
  DataFlowStack() = default;
  ~DataFlowStack() override = default;
  Status Compute(TaskContext &context, const int64_t handle) override;
};

class DataFlowStackPush : public DataFlowKernelBase {
 public:
  DataFlowStackPush() = default;
  ~DataFlowStackPush() override = default;
  Status Compute(TaskContext &context, const int64_t handle) override;
};

class DataFlowStackPop : public DataFlowKernelBase {
 public:
  DataFlowStackPop() = default;
  ~DataFlowStackPop() override = default;
  Status Compute(TaskContext &context, const int64_t handle) override;
};

class DataFlowStackClose : public DataFlowKernelBase {
 public:
  DataFlowStackClose() = default;
  ~DataFlowStackClose() override = default;
  Status Compute(TaskContext &context, const int64_t handle) override;
};
}  // namespace hybrid
}  // namespace ge

#define REGISTER_DATA_FLOW_KERNEL(type, clazz)                    \
  std::shared_ptr<DataFlowKernelBase> Creator_##type##_Kernel() { \
    std::shared_ptr<clazz> ptr = nullptr;                         \
    ptr = MakeShared<clazz>();                                    \
    return ptr;                                                   \
  }                                                               \
  DataFlowKernelFactory::DataFlowKernelRegistrar g_##type##_Kernel_Creator(type, Creator_##type##_Kernel)

#endif  // GE_HYBRID_NODE_EXECUTOR_DATA_FLOW_KERNELS_H_
