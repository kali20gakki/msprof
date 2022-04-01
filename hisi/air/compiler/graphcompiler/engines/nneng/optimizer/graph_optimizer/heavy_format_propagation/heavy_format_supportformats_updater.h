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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SUPPORT_FORMATS_UPDATER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SUPPORT_FORMATS_UPDATER_H_
#include <memory>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "format_selector/manager/format_dtype_setter.h"
#include "graph/compute_graph.h"

namespace fe {
using FormatDtypeSetterPtr = std::shared_ptr<FormatDtypeSetter>;
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;

class HeavyFormatSupportFormatsUpdater {
 public:
  explicit HeavyFormatSupportFormatsUpdater(FormatDtypeQuerierPtr format_dtype_querier_ptr,
                                            FormatDtypeSetterPtr format_dtype_setter_ptr);
  ~HeavyFormatSupportFormatsUpdater();

  Status UpdateSupportFormats(const ge::NodePtr& node_ptr, const OpKernelInfoPtr& op_kernel_info_ptr,
                              const std::vector<IndexNameMap>& tensor_map, const HeavyFormatInfo& heavy_format_info);

 private:
  bool NeedUpdateSupportFormats(const ge::OpDescPtr& op_desc_ptr, const HeavyFormatInfo& heavy_format_info,
                                const vector<ge::Format>& kernel_formats, ge::Format propaga_heavy_format);
  bool IsFzRelaFormat(const HeavyFormatInfo& heavy_format_info) const;
  bool IsSelectFormatOrBroadcast(const ge::OpDescPtr& op_desc_ptr, const OpKernelInfoPtr& op_kernel_info_ptr);

  void UpdateSubFormatForTensors(const ge::OpDescPtr &op_desc_ptr, const HeavyFormatInfo& heavy_format_info) const;
  FormatDtypeQuerierPtr format_dtype_querier_ptr_;
  FormatDtypeSetterPtr format_dtype_setter_ptr_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SUPPORT_FORMATS_UPDATER_H_
