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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_SETTER_OP_SETTER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_SETTER_OP_SETTER_H_

#include "ops_store/op_kernel_info.h"
#include "adapter/common/op_store_adapter_manager.h"

namespace fe {

class OpSetter {
 public:
  explicit OpSetter(const std::string engine_name, OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  virtual ~OpSetter();
  Status SetOpInfo(const ge::ComputeGraph& graph) const;
  Status SetOpInfoByNode(const ge::NodePtr &node_ptr) const;

  Status SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) const;

 private:
  Status SetOpDescAttr(const OpKernelInfoPtr& op_kernel_info_ptr, const std::string& attr_name,
                       const std::string& ge_attr_name, ge::OpDescPtr op_desc_ptr) const;

  std::string engine_name_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;

  static const std::map<string, string> attr_map_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_SETTER_OP_SETTER_H_