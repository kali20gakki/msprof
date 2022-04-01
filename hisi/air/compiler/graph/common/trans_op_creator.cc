/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "trans_op_creator.h"

#include "framework/common/ge_format_util.h"
#include "framework/common/types.h"
#include "framework/common/debug/ge_log.h"
#include "common/formats/formats.h"
#include "common/formats/format_transfers/format_transfer_transpose.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "common/plugin/ge_util.h"

namespace ge {
// For some historical reasons, only these two formats need groups
const static std::set<Format> kFormatWithGroups = {FORMAT_FRACTAL_Z, FORMAT_FRACTAL_Z_3D};

OpDescPtr TransOpCreator::CreateTransDataOp(const std::string &op_name, Format src_format, Format dst_format) {
  auto op_desc = MakeShared<OpDesc>(op_name, TRANSDATA);
  if (op_desc == nullptr) {
    GELOGE(FAILED, "Failed to new transdata opdesc.");
    return nullptr;
  }

  if (HasSubFormat(src_format)) {
    const int32_t src_subformat = GetSubFormat(src_format);
    if (!AttrUtils::SetInt(op_desc, FORMAT_TRANSFER_SRC_SUBFORMAT, src_subformat)) {
      GELOGW("Set attr [src_subformat] for node [%s] failed.", op_name.c_str());
    }
    src_format = static_cast<ge::Format>(GetPrimaryFormat(src_format));
    if (kFormatWithGroups.count(src_format) > 0UL && !AttrUtils::SetInt(op_desc, "groups", src_subformat)) {
      GELOGW("Set attr [groups] for node [%s] failed.", op_name.c_str());
    }
  }

  if (!AttrUtils::SetStr(op_desc, FORMAT_TRANSFER_SRC_FORMAT, TypeUtils::FormatToSerialString(src_format))) {
    GELOGW("Set attr [src_format] for node [%s] failed.", op_name.c_str());
  }

  if (HasSubFormat(dst_format)) {
    const int32_t dst_subformat = GetSubFormat(dst_format);
    if (!AttrUtils::SetInt(op_desc, FORMAT_TRANSFER_DST_SUBFORMAT, dst_subformat)) {
      GELOGW("Set attr [dst_subformat] for node [%s] failed.", op_name.c_str());
    }
    dst_format = static_cast<ge::Format>(GetPrimaryFormat(dst_format));
    if (kFormatWithGroups.count(dst_format) > 0UL && !AttrUtils::SetInt(op_desc, "groups", dst_subformat)) {
      GELOGW("Set attr [groups] for node [%s] failed.", op_name.c_str());
    }
  }

  if (!AttrUtils::SetStr(op_desc, FORMAT_TRANSFER_DST_FORMAT, TypeUtils::FormatToSerialString(dst_format))) {
    GELOGW("Set attr [dst_format] for node [%s] failed.", op_name.c_str());
  }

  return op_desc;
}

OpDescPtr TransOpCreator::CreateTransPoseDOp(const std::string &op_name, const Format src_format,
                                             const Format dst_format) {
  auto op_desc = MakeShared<OpDesc>(op_name, TRANSPOSED);
  if (op_desc == nullptr) {
    GELOGE(FAILED, "Failed to new transopsed opdesc.");
    return nullptr;
  }

  std::vector<int64_t> perm_arg;
  if (formats::GetPermByForamt(src_format, dst_format, perm_arg) != SUCCESS) {
    GELOGW("Get perm by foramt failed.");
    return op_desc;
  }

  if (!AttrUtils::SetListInt(op_desc, PERMUTE_ATTR_PERM, perm_arg)) {
    GELOGW("SetStr PERMUTE_ATTR_PERM failed.");
  }
  return op_desc;
}

OpDescPtr TransOpCreator::CreateCastOp(const std::string &op_name, const DataType input_dtype,
                                       const DataType output_dtype) {
  auto op_desc = MakeShared<OpDesc>(op_name, CAST);
  if (op_desc == nullptr) {
    GELOGE(FAILED, "Failed to new cast opdesc.");
    return nullptr;
  }
  if (!AttrUtils::SetInt(op_desc, CAST_ATTR_SRCT, static_cast<int64_t>(input_dtype))) {
    GELOGW("SetInt CAST_ATTR_SRCT failed");
  }
  if (!AttrUtils::SetInt(op_desc, CAST_ATTR_DSTT, static_cast<int64_t>(output_dtype))) {
    GELOGW("SetInt CAST_ATTR_DSTT failed");
  }
  if (!AttrUtils::SetInt(op_desc, CAST_ATTR_DST_TYPE, static_cast<int64_t>(output_dtype))) {
    GELOGW("SetInt CAST_ATTR_DST_TYPE failed");
  }
  if (!AttrUtils::SetBool(op_desc, CAST_ATTR_TRUNCATE, false)) {
    GELOGW("SetBool CAST_ATTR_TRUNCATE failed");
  }

  return op_desc;
}
}  // namespace ge
