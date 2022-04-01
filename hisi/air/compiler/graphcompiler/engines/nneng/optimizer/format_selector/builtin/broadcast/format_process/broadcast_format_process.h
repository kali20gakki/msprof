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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_FORMAT_PROCESS_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_FORMAT_PROCESS_H_

#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/format/axis_util.h"
#include "common/op_info_common.h"
#include "external/graph/types.h"
#include "format_selector/builtin/process/format_process_registry.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/op_desc.h"

namespace fe {
class BroadcastFormatProcess : public FormatProcessBase {
 public:
  BroadcastFormatProcess(){};
  ~BroadcastFormatProcess() override {};
  Status Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args, FormatProccessResult &result) override;

  virtual bool CheckOriginFormat(const std::vector<ge::Format> &formats, const vector<ge::GeShape> &input_shapes);
  virtual bool CheckOriginShape(const std::vector<ge::GeShape> &shapes);
  Status Check6HDShape(const std::vector<ge::GeShape> &shapes, const ge::Format &supprt_format,
                       size_t &dim_value) const;

  virtual bool CheckPartNonScalarInputs(const FormatProccessInputArg &input_arg) = 0;
  virtual bool CheckAllNonScalarInputs(const FormatProccessArgs &args) = 0;

 protected:
  Status GetDimValue(const std::string &dim, const ge::Format &format, const ge::GeShape &shape,
                     int64_t &dim_value) const;
  bool CheckAxisIsAligned(const ge::DataType &dtype, const int64_t &dim_value, const int64_t &shape_number) const;
  bool CheckAxisNeedBroadcast(const std::string &dim, const std::vector<ge::Format> &formats,
                              const std::vector<ge::GeShape> &shapes) const;

 private:
  bool CheckScalarExists(const std::vector<ge::GeShape> &shapes) const;
  void GenerateOutputFormats(const vector<ge::GeShape> &output_shapes, const ge::Format &format,
                             vector<ge::Format> &output_formats) const;
  void InsertFormatVec(const size_t &size, const ge::Format &format, vector<vector<ge::Format>> &formats) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_BROADCAST_FORMAT_PROCESS_BROADCAST_FORMAT_PROCESS_H_
