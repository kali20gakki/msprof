/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_KERNEL_FACTORY_H_
#define MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_KERNEL_FACTORY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "framework/common/debug/ge_log.h"
#include "graph/graph.h"

using std::string;

namespace ge {
class DeploySchedulerKernel;

///
/// @brief DeploySchedulerKernel create factory
///
class DeployKernelFactory {
 public:
  // KernelCreator（function）, type definition
  using KERNEL_CREATOR_FUN = std::function<std::shared_ptr<DeploySchedulerKernel>(void)>;

  ///
  /// Get singleton instance
  ///
  static DeployKernelFactory &Instance() {
    static DeployKernelFactory instance;
    return instance;
  }

  ///
  /// create DeploySchedulerKernel
  /// @param [in] op_type operation type
  ///
  std::shared_ptr<DeploySchedulerKernel> Create(const std::string &op_type) {
    std::map<std::string, KERNEL_CREATOR_FUN>::iterator iter = creator_map_.find(op_type);
    if (iter != creator_map_.end()) {
      return iter->second();
    }
    return nullptr;
  }

  // DeploySchedulerKernel registration function to register different types of DeploySchedulerKernel to the factory
  class Register {
   public:
    ///
    /// @brief Constructor
    /// @param [in] type operation type
    /// @param [in| fun DeploySchedulerKernel function of the operation
    ///
    Register(const string &type, const KERNEL_CREATOR_FUN &fun) {
      DeployKernelFactory::Instance().RegisterCreator(type, fun);
    }
    ~Register() = default;
  };

 protected:
  DeployKernelFactory() = default;
  ~DeployKernelFactory() = default;

  // register creator, this function will call in the constructor
  void RegisterCreator(const string &type, const KERNEL_CREATOR_FUN &fun) {
    std::map<std::string, KERNEL_CREATOR_FUN>::iterator iter = creator_map_.find(type);
    if (iter != creator_map_.end()) {
      GELOGW("DeployKernelFactory::RegisterCreator: %s creator already exist", type.c_str());
      return;
    }

    creator_map_[type] = fun;
  }

 private:
  std::map<std::string, KERNEL_CREATOR_FUN> creator_map_{};
};

#define REGISTER_MDS_KERNEL(type, clazz)                  \
  std::shared_ptr<DeploySchedulerKernel> Creator_##type##_Kernel() { \
    std::shared_ptr<clazz> ptr = nullptr;             \
    ptr = MakeShared<clazz>();                        \
    return ptr;                                       \
  }                                                   \
  DeployKernelFactory::Register g_##type##_Kernel_Creator(type, Creator_##type##_Kernel)
}  // namespace ge
#endif //MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_KERNEL_FACTORY_H_
