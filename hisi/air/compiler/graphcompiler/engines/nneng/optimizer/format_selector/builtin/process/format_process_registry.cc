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

#include "format_selector/builtin/process/format_process_registry.h"
#include "common/fe_type_utils.h"
#include "common/fe_utils.h"
#include "graph/utils/type_utils.h"

namespace fe {
struct FormatProcessRegistry {
  Status RegisterBuilder(OpPattern op_pattern, ge::Format support_format, FormatProcessBuilder builder) {
    FE_LOGD("RegisterBuilder to register the Builder");
    src_dst_builder[op_pattern][support_format] = std::move(builder);
    return SUCCESS;
  }

  std::map<OpPattern, std::map<ge::Format, FormatProcessBuilder>> src_dst_builder;
};

FormatProcessRegistry &GetFormatProcessRegistry() {
  static FormatProcessRegistry registry;
  return registry;
}

FormatProcessBasePtr BuildFormatProcess(const FormatProccessArgs &args) {
  auto registry = GetFormatProcessRegistry();
  const auto dst_builder = registry.src_dst_builder.find(args.op_pattern);
  if (dst_builder == registry.src_dst_builder.end()) {
    return nullptr;
  }
  const auto builder_iter = dst_builder->second.find(args.support_format);
  if (builder_iter == dst_builder->second.end()) {
    return nullptr;
  }
  return builder_iter->second();
}

bool FormatProcessExists(const FormatProccessArgs &args) {
  auto registry = GetFormatProcessRegistry();
  auto dst_builder = registry.src_dst_builder.find(args.op_pattern);
  if (dst_builder == registry.src_dst_builder.end()) {
    return false;
  }
  return dst_builder->second.count(args.support_format) > 0;
}

FormatProcessRegister::FormatProcessRegister(FormatProcessBuilder builder, OpPattern op_pattern, ge::Format format) {
  FE_LOGD("The parameter[pattern[%s]], parameter[format[%s]] of FormatProcessRegister",
          GetOpPatternString(op_pattern).c_str(), ge::TypeUtils::FormatToSerialString(format).c_str());
  auto ret = GetFormatProcessRegistry().RegisterBuilder(op_pattern, format, std::move(builder));
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][GenBuiltInFmt][FmtProcsReg] Failed to register the builder for the format of op_pattern[%d].",
        op_pattern);
  } else {
    FE_LOGD("Processor of pattern[%s] and format[%s] is registered", GetOpPatternString(op_pattern).c_str(),
            FormatToStr(format).c_str());
  }
}
}  // namespace fe
