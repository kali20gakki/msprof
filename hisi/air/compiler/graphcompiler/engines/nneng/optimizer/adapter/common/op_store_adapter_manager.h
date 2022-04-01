/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_ADAPTER_COMMON_OP_STORE_ADAPTER_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_ADAPTER_COMMON_OP_STORE_ADAPTER_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "adapter/adapter_itf/op_store_adapter.h"

namespace fe {
class OpStoreAdapterManager;
using OpStoreAdapterManagerPtr = std::shared_ptr<OpStoreAdapterManager>;

class OpStoreAdapterManager {
 public:
  OpStoreAdapterManager();

  ~OpStoreAdapterManager();

  OpStoreAdapterManager(const OpStoreAdapterManager &) = delete;

  OpStoreAdapterManager &operator=(const OpStoreAdapterManager &) = delete;

  Status Initialize(const std::map<std::string, std::string> &options, const std::string &engine_name);

  Status Finalize();

  Status GetOpStoreAdapter(const OpImplType &op_impl_type, OpStoreAdapterPtr &adapter_ptr) const;

 private:
  bool init_flag_;
  std::map<std::string, OpStoreAdapterPtr> map_all_op_store_adapter_;
  Status InitializeAdapter(const std::string adapter_type, const std::map<std::string, std::string> &options,
                           const std::string &engine_name);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_ADAPTER_COMMON_OP_STORE_ADAPTER_MANAGER_H_
