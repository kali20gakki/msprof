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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_SETTER_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_SETTER_H_

#include "common/graph/fe_graph_utils.h"
#include "common/op_info_common.h"
#include "format_selector/manager/format_dtype_manager_base.h"

namespace fe {
class FormatDtypeSetter : public FormatDtypeManagerBase {
 public:
  FormatDtypeSetter(const std::string& engine_name, OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  ~FormatDtypeSetter() override;
  Status SetSupportFormatDtype(const ge::ComputeGraph& graph) const;
  Status SetSupportFormatDtypeByNode(ge::NodePtr node_ptr, const HeavyFormatInfo& heavy_format_info) const;

 private:
  Status SetSupportFormatDtypeByNode(ge::NodePtr node_ptr) const;
  void JudgeFirstLayerConv(const ge::NodePtr& node, const ge::OpDescPtr& op_desc) const;
  std::string engine_name_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_SETTER_H_
