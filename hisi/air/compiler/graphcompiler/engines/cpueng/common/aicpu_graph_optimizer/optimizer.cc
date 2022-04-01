/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "optimizer.h"

#include "config/config_file.h"
#include "error_code/error_code.h"
#include "graph/debug/ge_attr_define.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"
#include "common/op_slice_util.h"
#include "common/aicore_util_types.h"

using namespace std;
using namespace ge;

namespace {
const string kPlaceHolderOpType = "PlaceHolder";
const string kFunctionOp = "FunctionOp";
const string kFrameworkOp = "FrameworkOp";
constexpr uint64_t kOpCheckModeOff = 0;
constexpr uint64_t kOpCheckModeOn = 1;
constexpr int64_t kFormatAgnostic = 1;
constexpr int64_t kShape4d = 4;

const set<Format> kGeFormatSet = {Format::FORMAT_NCHW, Format::FORMAT_NHWC, Format::FORMAT_ND};

const bool kAicpuSupportStrideWrite = false;
const int32_t kNCHWDimOfN = 0;
const int32_t kNCHWDimOfC = 1;
const int32_t kNCHWDimOfH = 2;
const int32_t kNCHWDimOfW = 3;

const int32_t kNHWCDimOfN = 0;
const int32_t kNHWCDimOfH = 1;
const int32_t kNHWCDimOfW = 2;
const int32_t kNHWCDimOfC = 3;
}  // namespace

namespace aicpu {
Status Optimizer::GetFrameworkOpType(const OpDescPtr &op_desc_ptr,
                                     string &op_type) const {
  // op_desc_ptr already check not null
  string original_type;
  CHECK_RES_BOOL(AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type),
      ErrorCode::GET_ATTR_FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
          kOriginalType.c_str(), op_desc_ptr->GetName().c_str()))
  if (original_type.empty()) {
    AICPU_REPORT_INNER_ERROR("Attr[%s] is empty, op[%s].", kOriginalType.c_str(),
        op_desc_ptr->GetName().c_str());
    return STR_IS_EMPTY;
  }
  op_desc_ptr->SetType(original_type);
  op_type = original_type;
  return SUCCESS;
}

void Optimizer::InitOpCheckMode() {
  // get TfDebugMode from config file
  string op_check_mode;
  if (ConfigFile::GetInstance().GetValue(kOpCheckMode, op_check_mode)) {
    uint64_t result = kOpCheckModeOff;
    if (StringToNum(op_check_mode, result).state != ge::SUCCESS) {
      AICPUE_LOGW("Tran op_check_mode [%s] to integer failed. default value is 0.",
                  op_check_mode.c_str());
      return;
    }
    if (result == kOpCheckModeOn) {
      AICPUE_LOGI("OpCheckMode is on.");
      op_check_mode_ = true;
      return;
    }
    AICPUE_LOGI("OpCheckMode is off.");
    return;
  }
  AICPUE_LOGW("Get [op_check_mode] from config file failed. op check mode is off.");
}

ge::Status Optimizer::InitSlicePattern(const std::string &slice,
                                       const ge::NodePtr &node) const {
  if (slice == "") {
    return SUCCESS;
  }
  const std::map<std::string, fe::SlicePattern> STR_SLICE_PATTERN_MAP {
        {"elemwise", fe::ELEMENT_WISE},
        {"elemwiseBroadcast", fe::ELEMENT_WISE_BROADCAST},
        {"broadcast", fe::BROADCAST},
        {"slidingWindow", fe::SLIDING_WINDOW},
        {"slidingWindowDeconv", fe::SLIDING_WINDOW_DECONV},
        {"cubeMatmul", fe::CUBE_MATMUL},
        {"reduce", fe::SLICE_PATTERN_REDUCE},
        {"resize", fe::SLICE_PATTERN_RESIZE},
        {"scatter", fe::SLICE_PATTERN_SCATTER},
        {"segment", fe::SLICE_PATTERN_SEGMENT},
  };
  AICPUE_LOGI("op[%s], slice[%s]", node->GetName().c_str(), slice.c_str());
  auto iter = STR_SLICE_PATTERN_MAP.find(slice);
  if (iter == STR_SLICE_PATTERN_MAP.end()) {
    AICPU_REPORT_CALL_ERROR("slice[%s] is invalid", slice.c_str());
    return FAILED;
  }
  Status state = fe::OpSliceUtil::SetOpSliceInfo(node, iter->second, kAicpuSupportStrideWrite);
  if (state != SUCCESS) {
    AICPU_REPORT_CALL_ERROR("op[%s] SetOpSliceInfo[%s] fail[%u]",
                            node->GetName().c_str(), slice.c_str(), state);
    return state;
  }
  return SUCCESS;
}

Status Optimizer::OptimizeOriginalGraphJudgeInsert(
    const ComputeGraph &graph,
    const map<string, OpFullInfo> &all_op_info) const {
  for (const NodePtr &curr_node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(curr_node)
    OpDescPtr curr_op_desc_ptr = curr_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(curr_op_desc_ptr)
    string op_type = curr_op_desc_ptr->GetType();
    // if op type is placeholder or function_op or framework_op, skip it
    if (op_type == kPlaceHolderOpType || op_type == kFunctionOp ||
        op_type == kFrameworkOp) {
      AICPUE_LOGD("Current op type is [%s]. Don't need to set format.",
                  op_type.c_str());
      continue;
    }

    auto iter = all_op_info.find(op_type);
    if (iter == all_op_info.end()) {
      AICPUE_LOGW("Op type[%s] is not exist in tf kernel librarys.", op_type.c_str());
    } else {
      auto op_info = iter->second;
      bool format_agnostic = op_info.formatAgnostic;
      if (format_agnostic) {
        const string name = "_format_agnostic";
        bool b_ret = AttrUtils::SetInt(curr_op_desc_ptr, name, kFormatAgnostic);
        AICPU_IF_BOOL_EXEC(
            !(b_ret),
            AICPU_REPORT_CALL_ERROR(
                "Call ge::AttrUtils::SetInt failed to set attr[%s], op[%s]",
                name.c_str(), curr_op_desc_ptr->GetName().c_str());
            return FAILED)
      } else {
        AICPU_CHECK_RES_WITH_LOG(
            UpdateInputFormatAndShape(op_info, curr_op_desc_ptr),
            "Call UpdateInputFormatAndShape function failed, op[%s].",
            curr_op_desc_ptr->GetType().c_str())
        AICPU_CHECK_RES_WITH_LOG(
            UpdateOutputFormatAndShape(op_info, curr_op_desc_ptr),
            "Call UpdateOutputFormatAndShape function failed, op[%s].",
            curr_op_desc_ptr->GetType().c_str())
      }
      ge::Status ret = InitSlicePattern(op_info.slicePattern, curr_node);
      if (ret != SUCCESS) {
        return ret;
      }
    }
  }
  return SUCCESS;
}

Status Optimizer::UpdateInputFormatAndShape(const OpFullInfo &op_info,
                                            const OpDescPtr &op_desc_ptr) const {
  uint32_t index = 0;
  for (GeTensorDesc input_desc : op_desc_ptr->GetAllInputsDesc()) {
    ge::Format src_format = input_desc.GetFormat();
    if (!kGeFormatSet.count(src_format)) {
      AICPUE_LOGD("input %u is not need update format and shape.", index);
      return SUCCESS;
    }

    map<string, string> formats = op_info.inOutFormat;
    string format_name = Stringcat("input", index);
    ge::Format dst_format;
    GetFormat(formats, format_name, dst_format);
    if ((dst_format == ge::FORMAT_NHWC) || (dst_format == ge::FORMAT_NCHW)) {
      (void)UpdateTensorDesc(input_desc, src_format, dst_format);
      AICPU_CHECK_RES_WITH_LOG(op_desc_ptr->UpdateInputDesc(index, input_desc),
          "Call UpdateInputDesc failed to update input[%u] desc, op[%s].",
          index, op_desc_ptr->GetName().c_str())
    }
    index++;
  }
  return SUCCESS;
}

void Optimizer::UpdateTensorDesc(GeTensorDesc &tensor_desc,
                                 const ge::Format &src_format,
                                 const ge::Format &dst_format) const {
  if (src_format != dst_format) {
    tensor_desc.SetFormat(dst_format);
    vector<int64_t> dims = tensor_desc.GetShape().GetDims();
    if (dims.size() != kShape4d) {
      AICPUE_LOGW("Input tensor is not 4D, but it's format is 4D.");
      return;
    }
    vector<int64_t> newDims(dims);
    if (src_format == ge::FORMAT_NCHW) {
      newDims[0] = dims[kNCHWDimOfN];
      newDims[1] = dims[kNCHWDimOfH];
      newDims[2] = dims[kNCHWDimOfW];
      newDims[3] = dims[kNCHWDimOfC];
    } else if (src_format == ge::FORMAT_NHWC) {
      newDims[0] = dims[kNHWCDimOfN];
      newDims[1] = dims[kNHWCDimOfC];
      newDims[2] = dims[kNHWCDimOfH];
      newDims[3] = dims[kNHWCDimOfW];
    }
    tensor_desc.SetShape(GeShape(newDims));
  }
}

Status Optimizer::UpdateOutputFormatAndShape(const OpFullInfo &op_info,
                                             const OpDescPtr &op_desc_ptr) const {
  uint32_t index = 0;
  for (GeTensorDesc output_desc : op_desc_ptr->GetAllOutputsDesc()) {
    ge::Format src_format = output_desc.GetFormat();
    if (!kGeFormatSet.count(src_format)) {
      AICPUE_LOGD("output[%u] is not need update format and shape.", index);
      return SUCCESS;
    }

    map<string, string> formats = op_info.inOutFormat;
    string format_name = Stringcat("output", index);
    ge::Format dst_format;
    GetFormat(formats, format_name, dst_format);
    if ((dst_format == ge::FORMAT_NHWC) || (dst_format == ge::FORMAT_NCHW)) {
      (void)UpdateTensorDesc(output_desc, src_format, dst_format);
      AICPU_CHECK_RES_WITH_LOG(op_desc_ptr->UpdateOutputDesc(index, output_desc),
          "Call UpdateOutputDesc failed to update output[%u] desc, op[%s].",
          index, op_desc_ptr->GetName().c_str())
    }
    index++;
  }
  return SUCCESS;
}

void Optimizer::GetFormat(const map<string, string> &formats,
                          const string &format_name, ge::Format &format) const {
  auto iter = formats.find(format_name);
  if (iter != formats.end()) {
    string formatStr = iter->second;
    if (formatStr == "NHWC") {
      format = ge::FORMAT_NHWC;
    } else if (formatStr == "NCHW") {
      format = ge::FORMAT_NCHW;
    } else {
      format = ge::FORMAT_ND;
    }
  } else {
    format = ge::FORMAT_ND;
  }
}
}  // namespace aicpu
