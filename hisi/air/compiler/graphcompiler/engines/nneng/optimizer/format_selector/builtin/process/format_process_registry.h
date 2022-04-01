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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_PROCESS_FORMAT_PROCESS_REGISTRY_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_PROCESS_FORMAT_PROCESS_REGISTRY_H_

#include <functional>
#include <memory>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/util/op_info_util.h"
#include "graph/ge_tensor.h"
#include "graph/node.h"
#include "graph/op_desc.h"

namespace fe {
struct OriginInfo {
  vector<ge::Format> input_formats;
  vector<ge::DataType> input_dtypes;
  vector<ge::GeShape> input_shapes;
  vector<ge::GeShape> output_shapes;
};

using OriginInfoPtr = std::shared_ptr<OriginInfo>;

struct FormatProccessArgs {
  OpPattern op_pattern;
  ge::Format support_format;
  OriginInfoPtr origin_info_ptr;
  ge::Format propagat_primary_format = ge::FORMAT_RESERVED;
  int32_t propagat_sub_format = 0;
};

struct FormatProccessResult {
  vector<vector<ge::Format>> input_format_res;
  vector<vector<ge::Format>> output_format_res;
};

struct FormatProccessInputArg {
  ge::Format input_format_;
  ge::DataType input_dtype_;
  ge::GeShape input_shape_;
  ge::Format propagat_primary_format_ = ge::FORMAT_RESERVED;
  int32_t propagat_sub_format_ = 0;
  FormatProccessInputArg(ge::Format input_format, ge::DataType input_dtype, ge::GeShape input_shape,
                         ge::Format propagat_primary_format, int32_t propagat_sub_format)
      : input_format_(input_format),
        input_dtype_(input_dtype),
        input_shape_(input_shape),
        propagat_primary_format_(propagat_primary_format),
        propagat_sub_format_(propagat_sub_format) {}
};

class FormatProcessBase {
 public:
  FormatProcessBase(){};
  virtual ~FormatProcessBase(){};
  virtual Status Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args, FormatProccessResult &result) = 0;
};

using FormatProcessBasePtr = std::shared_ptr<FormatProcessBase>;
using FormatProcessBuilder = std::function<std::shared_ptr<FormatProcessBase>()>;

class FormatProcessRegister {
 public:
  FormatProcessRegister(FormatProcessBuilder builder, OpPattern op_pattern, ge::Format format);
  ~FormatProcessRegister() = default;
};

FormatProcessBasePtr BuildFormatProcess(const FormatProccessArgs &args);
bool FormatProcessExists(const FormatProccessArgs &args);

#define REGISTER_FORMAT_PROCESS(clazz, op_pattern, format_str, format)                                                 \
  FormatProcessBasePtr create_##op_pattern##format_str##_process() { return std::make_shared<clazz>(); } \
  FormatProcessRegister f_##op_pattern##format_str##_register(create_##op_pattern##format_str##_process, op_pattern,   \
                                                              format)
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_PROCESS_FORMAT_PROCESS_REGISTRY_H_
