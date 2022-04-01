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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_FORMAT_PROCESS_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_FORMAT_PROCESS_H_

#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/format/axis_util.h"
#include "common/op_info_common.h"
#include "external/graph/types.h"
#include "format_selector/builtin/process/format_process_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/type_utils.h"

namespace fe {
class ReduceFormatProcess : public FormatProcessBase {
 public:
  ReduceFormatProcess(){};
  ~ReduceFormatProcess() override {};

  /**
   * Check whether the dimNum of the origin shape is equal to dim_min,
   * and check whether the origin format is identifiable.
   * @param formats origin formats
   * @param shapes origin shapes
   * @param dim refernce value
   * @return
   */
  virtual bool CheckOriginFormatAndShape(const vector<ge::Format> &formats, const vector<ge::GeShape> &shapes,
                                         const size_t &dim);

 protected:
  bool CheckContainReduceAxis(const ge::OpDesc &op_desc, const vector<ge::Format> &formats,
                              const vector<ge::GeShape> &shapes, const string &axis_name);

  void GenerateFormats(const size_t &in_shape_size, const size_t &out_shape_size,
                       const vector<ge::Format> &support_in_formats, const vector<ge::Format> &support_out_formats,
                       FormatProccessResult &result) const;

  const string LAST_AXIS_NAME = "last";
  const string LASTBUTONE_AXIS_NAME = "lastButOne";

 private:
  Status GetOriginAxisIndexByName(const ge::Format &format, const ge::GeShape &shape, const std::string &axis_name,
                                  int64_t &axis_index);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_FORMAT_PROCESS_H_
