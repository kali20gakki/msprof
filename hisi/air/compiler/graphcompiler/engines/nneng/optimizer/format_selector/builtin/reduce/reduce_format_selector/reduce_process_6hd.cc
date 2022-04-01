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

#include "format_selector/builtin/reduce/reduce_format_selector/reduce_process_6hd.h"

namespace fe {
Status ReduceProcess6HD::Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args,
                                 FormatProccessResult &result) {
  OriginInfoPtr origin_info_ptr = args.origin_info_ptr;
  vector<ge::Format> input_formats = origin_info_ptr->input_formats;
  vector<ge::GeShape> input_shapes = origin_info_ptr->input_shapes;
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  // 1. check origin formats and shapes
  if (!CheckOriginFormatAndShape(input_formats, input_shapes, DIMENSION_NUM_FIVE)) {
    FE_LOGD("Op[name=%s,type=%s]: check origin format and shape not success.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  // 2. check the axis attribute of the op_desc
  bool is_reduce_c = CheckContainReduceAxis(op_desc, input_formats, input_shapes, C_AXIS_NAME);
  if (is_reduce_c) {
    FE_LOGD("reduce axis contains C is not allowed for NDC1HWC0.");
    return FAILED;
  }

  // 3. genareate result
  GenerateFormats(input_shapes.size(), origin_info_ptr->output_shapes.size(),
                  {args.support_format}, {args.support_format}, result);
  return SUCCESS;
}

REGISTER_FORMAT_PROCESS(ReduceProcess6HD, OP_PATTERN_REDUCE, FORMAT_NDC1HWC0, ge::FORMAT_NDC1HWC0);
}  // namespace fe
