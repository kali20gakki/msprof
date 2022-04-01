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

#include "op_format_dtype_strategy_manager.h"
#include "common/configuration.h"

namespace fe {
OpFormatDtypeStrategyManager::OpFormatDtypeStrategyManager(
    const std::string& engine_name, FormatDtypeQuerierPtr format_dtype_querier_ptr)
    : engine_name_(engine_name), precision_mode_(ENUM_ALLOW_FP32_TO_FP16),
      format_dtype_querier_ptr_(format_dtype_querier_ptr) {}

Status OpFormatDtypeStrategyManager::Initialize() {
  auto precision_mode_str = Configuration::Instance(engine_name_).GetPrecisionModeStr();
  auto iter = PRECISION_MODE_MAP.find(precision_mode_str);
  if (iter == PRECISION_MODE_MAP.end()) {
    FE_LOGW("The precision mode %s is not valid", precision_mode_str.c_str());
    precision_mode_ = ENUM_ALLOW_FP32_TO_FP16;
  } else {
    precision_mode_ = iter->second;
    FE_LOGD("Match Dtype in precision mode %s", precision_mode_str.c_str());
  }

  OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr = nullptr;
  FE_MAKE_SHARED(op_dtype_mixed_precision_matcher_ptr = std::make_shared<OpDtypeMixPrecisionMatcher>(), return FAILED);
  FE_CHECK_NOTNULL(op_dtype_mixed_precision_matcher_ptr);
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr = nullptr;
  FE_MAKE_SHARED(op_dtype_rise_matcher_ptr = std::make_shared<OpDtypeRiseMatcher>(), return FAILED);
  FE_CHECK_NOTNULL(op_dtype_rise_matcher_ptr);
  OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr = nullptr;
  FE_MAKE_SHARED(op_dtype_reduce_matcher_ptr = std::make_shared<OpDtypeReduceMatcher>(), return FAILED);
  FE_CHECK_NOTNULL(op_dtype_reduce_matcher_ptr);
  OpDtypePreciseMatcherPtr op_dtype_precise_matcher_ptr = nullptr;
  FE_MAKE_SHARED(op_dtype_precise_matcher_ptr = std::make_shared<OpDtypePreciseMatcher>(), return FAILED);
  FE_CHECK_NOTNULL(op_dtype_precise_matcher_ptr);

  OpDtypeSeletionStrategyBasePtr force_fp16_strategy = nullptr;
  FE_MAKE_SHARED(force_fp16_strategy =
    std::make_shared<OpDtypeSelectionStrategyForceFp16>(format_dtype_querier_ptr_,
                                        op_dtype_rise_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(force_fp16_strategy);

  OpDtypeSeletionStrategyBasePtr force_bf16_strategy = nullptr;
  FE_MAKE_SHARED(force_bf16_strategy =
    std::make_shared<OpDtypeSelectionStrategyForceBf16>(format_dtype_querier_ptr_,
                                        op_dtype_rise_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(force_bf16_strategy);

    OpDtypeSeletionStrategyBasePtr force_lowerprecision_strategy = nullptr;
  FE_MAKE_SHARED(force_lowerprecision_strategy =
    std::make_shared<OpDtypeSelectionStrategyForceLowerPrecision>(format_dtype_querier_ptr_,
                    op_dtype_rise_matcher_ptr, op_dtype_precise_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(force_lowerprecision_strategy);

  OpDtypeSeletionStrategyBasePtr allow_fp32_to_fp16 = nullptr;
  FE_MAKE_SHARED(allow_fp32_to_fp16 =
    std::make_shared<OpDtypeSelectionStrategyAllowFp32ToFp16>(
    format_dtype_querier_ptr_, op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(allow_fp32_to_fp16);

    OpDtypeSeletionStrategyBasePtr allow_fp32_to_bf16 = nullptr;
  FE_MAKE_SHARED(allow_fp32_to_bf16 =
    std::make_shared<OpDtypeSelectionStrategyAllowFp32ToBf16>(
    format_dtype_querier_ptr_, op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(allow_fp32_to_bf16);

    OpDtypeSeletionStrategyBasePtr allow_fp32_to_lowprecison = nullptr;
  FE_MAKE_SHARED(allow_fp32_to_lowprecison =
    std::make_shared<OpDtypeSelectionStrategyAllowFp32ToLowPrecision>(
    format_dtype_querier_ptr_, op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(allow_fp32_to_lowprecison);

  OpDtypeSeletionStrategyBasePtr default_mode_strategy = nullptr;
  FE_MAKE_SHARED(default_mode_strategy =
      std::make_shared<OpDtypeSelectionStrategyDefaultMode>(format_dtype_querier_ptr_, op_dtype_rise_matcher_ptr),
      return FAILED);
  FE_CHECK_NOTNULL(default_mode_strategy);

  OpDtypeSeletionStrategyBasePtr auto_mixed_precision_fp16_strategy = nullptr;
  FE_MAKE_SHARED(auto_mixed_precision_fp16_strategy =
    std::make_shared<OpDtypeSelectionStrategyAllowMixPrecisionFp16>(engine_name_, format_dtype_querier_ptr_,
    op_dtype_mixed_precision_matcher_ptr, op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(auto_mixed_precision_fp16_strategy);

  OpDtypeSeletionStrategyBasePtr auto_mixed_precision_bf16_strategy = nullptr;
  FE_MAKE_SHARED(auto_mixed_precision_bf16_strategy =
    std::make_shared<OpDtypeSelectionStrategyAllowMixPrecisionBf16>(engine_name_, format_dtype_querier_ptr_,
    op_dtype_mixed_precision_matcher_ptr, op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(auto_mixed_precision_bf16_strategy);

  OpDtypeSeletionStrategyBasePtr force_fp32_strategy = nullptr;
  FE_MAKE_SHARED(force_fp32_strategy = std::make_shared<OpDtypeSelectionStrategyForceFp32>(format_dtype_querier_ptr_,
    op_dtype_rise_matcher_ptr, op_dtype_reduce_matcher_ptr), return FAILED);
  FE_CHECK_NOTNULL(force_fp32_strategy);

  /* FORCE_FP16 */
  dtype_selection_strategies_.emplace_back(force_fp16_strategy);
  /* ALLOW_FP32_TO_FP16 */
  dtype_selection_strategies_.emplace_back(allow_fp32_to_fp16);
  /* AUTO_MIX_PRECISION */
  dtype_selection_strategies_.emplace_back(auto_mixed_precision_fp16_strategy);
  /* MUST_KEEP_ORIGIN_DTYPE */
  dtype_selection_strategies_.emplace_back(default_mode_strategy);
  /* FORCE_FP32 */
  dtype_selection_strategies_.emplace_back(force_fp32_strategy);
  /* FORCE_BF16 */
  dtype_selection_strategies_.emplace_back(force_bf16_strategy);
  /* FORCE_LOWERPRECISION */
  dtype_selection_strategies_.emplace_back(force_lowerprecision_strategy);
  /* ALLOW_FP32_TO_BF16 */
  dtype_selection_strategies_.emplace_back(allow_fp32_to_bf16);
  /* ALLOW_FP32_TO_LOWPRECISION */
  dtype_selection_strategies_.emplace_back(allow_fp32_to_lowprecison);
  /* AUTO_MIX_PRECISION_FP16 , is same with AUTO_MIX_PRECISION */
  /* AUTO_MIX_PRECISION_bf16 */
  dtype_selection_strategies_.emplace_back(auto_mixed_precision_bf16_strategy);

  FE_MAKE_SHARED(format_selection_default_strategy_ =
    std::make_shared<OpFormatSelectionStrategyDefaultMode>(format_dtype_querier_ptr_), return FAILED);
  FE_CHECK_NOTNULL(format_selection_default_strategy_);
  if (format_selection_default_strategy_->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtJdg][Init] Fail to initialize OpFormatSelectionStrategyDefaultMode pointer.");
    return FAILED;
  }

  FE_MAKE_SHARED(format_selection_prev_strategy_ =
    std::make_shared<OpFormatSelectionStrategyFollowPredecessor>(format_dtype_querier_ptr_), return FAILED);
  FE_CHECK_NOTNULL(format_selection_prev_strategy_);
  if (format_selection_prev_strategy_->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtJdg][Init] Fail to initialize OpFormatSelectionStrategyFollowPredecessor \
                    pointer.");
    return FAILED;
  }
  return SUCCESS;
}

OpFormatDtypeStrategyManager::~OpFormatDtypeStrategyManager() {}

Status OpFormatDtypeStrategyManager::GenerateInitialMatchedIndexVec(bool &is_matched_index_vec_inited,
                                                                    vector<uint32_t> &matched_index_vec,
                                                                    const std::vector<ge::Format> &input_format_vec) {
  if (!is_matched_index_vec_inited) {
    for (size_t index = 0; index < input_format_vec.size(); ++index) {
      matched_index_vec.push_back(index);
    }
    is_matched_index_vec_inited = true;
  }
  return SUCCESS;
}

Status OpFormatDtypeStrategyManager::MatchByDifferentMode(FormatDtypeSelectionBasicInfo &basic_info) {
  /* When the precison mode is force fp16, all op will be selected as fp16, for the following case auto
   * mixed precision is not correct:
   * Const or Variable or Data -> GeOp.
   * Const/Variable/Data will be fp32 commonly. If using auto mixed precision the GeOp will be selected as
   * fp32, which is not correct. */
  // get the attr of tensor to judge whether be mix precision
  ge::ConstGeTensorDescPtr current_tensor = basic_info.cur_tensor_desc;
  int64_t tensor_format_continuous = -1;
  (void)ge::AttrUtils::GetInt(current_tensor, FORMAT_CONTINUOUS, tensor_format_continuous);
  FE_LOGD("node %s get format_continous value is %ld.", basic_info.node->GetName().c_str(), tensor_format_continuous);

  int64_t keep_dtype = -1;
  (void)ge::AttrUtils::GetInt(basic_info.node->GetOpDesc(), KEEP_DTYPE, keep_dtype);

  bool force_fp32_to_fp16 = false;
  (void)ge::AttrUtils::GetBool(basic_info.node->GetOpDesc(), kForceFp32ToFp16, force_fp32_to_fp16);

  if (keep_dtype == 1 || IsDtypeSensitiveOp(basic_info.node->GetType())) {
    /* If user want keep data type of current node, we need to use allow fp32 to fp16 mode.
     * Output does not need to follow the predecessor's data type. */
    return dtype_selection_strategies_[ENUM_ALLOW_FP32_TO_FP16]->Run(basic_info, ForbiddenDtype::FORBIDDEN_NONE);
  } else if (((basic_info.op_kernel_info_ptr->GetOpInfo().opFileName == NULL_OP_FILE ||
               basic_info.op_kernel_info_ptr->GetOpInfo().opFileName == AICORE_NULL) &&
               precision_mode_ != ENUM_FORCE_FP16) ||
              (tensor_format_continuous == 1 && basic_info.is_input)) {
    return dtype_selection_strategies_[ENUM_AUTO_MIX_PRECISION_FP16]->Run(basic_info, ForbiddenDtype::FORBIDDEN_NONE);
  } else if (force_fp32_to_fp16 && precision_mode_ == ENUM_ALLOW_FP32_TO_FP16) {
    FE_LOGD("node %s has attr %s, and the follow tensors need select forcefp16", basic_info.node->GetName().c_str(),
            kForceFp32ToFp16.c_str());
    return dtype_selection_strategies_[ENUM_FORCE_FP16]->Run(basic_info,
                                                             ForbiddenDtype::FORBIDDEN_NONE);
  } else {
    switch (precision_mode_) {
        case ENUM_FORCE_FP16:
        case ENUM_ALLOW_FP32_TO_FP16:
        case ENUM_AUTO_MIX_PRECISION_FP16:
        case ENUM_FORCE_FP32:
            return dtype_selection_strategies_[precision_mode_]->Run(basic_info, ForbiddenDtype::FORBIDDEN_BF16);
        case ENUM_FORCE_BF16:
        case ENUM_ALLOW_FP32_TO_BF16:
        case ENUM_AUTO_MIX_PRECISION_BF16:
            return dtype_selection_strategies_[precision_mode_]->Run(basic_info, ForbiddenDtype::FORBIDDEN_FP16);
        default:
            return dtype_selection_strategies_[precision_mode_]->Run(basic_info, ForbiddenDtype::FORBIDDEN_NONE);
    }
  }
}

Status OpFormatDtypeStrategyManager::GetAllPossibleDtypeIndexByPrecisionMode(
    const std::map<uint32_t, int> &prio_index_map, const IndexNameMap &tensor_index_name_map,
    const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_input,
    vector<uint32_t> &matched_index_vec) {
  FE_CHECK_NOTNULL(node_ptr);
  auto op_desc_ptr = node_ptr->GetOpDesc();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  /* When matched index is empty, we need to set this flag = false. */
  bool is_matched_index_vec_inited = !matched_index_vec.empty();
  for (const auto &prio_index_iter : prio_index_map) {
    // 4.1 get the InputOrOutputInfo from the op kernel info store
    int32_t i = prio_index_iter.second;
    if (is_input && op_desc_ptr->MutableInputDesc(i) == nullptr) {
      continue;
    } else if (!is_input && op_desc_ptr->MutableOutputDesc(i) == nullptr) {
      continue;
    }
    auto tensor_iter = tensor_index_name_map.find(i);
    if (tensor_iter == tensor_index_name_map.end()) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][SetDtypeFmt][Op %s,type=%s]: the %s %u is not found in the ops store.",
          op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return OP_JUDGE_MAP_KEY_FIND_FAILED;
    }
    InputOrOutputInfoPtr tesor_kernel_info_ptr = nullptr;
    Status ret = op_kernel_info_ptr->GetTensorInfoByName(is_input, tensor_iter->second, tesor_kernel_info_ptr);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetTsInfo][Op %s,type=%s]: %s %u of %s is not found in the ops store.",
                      op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i, op_name.c_str());
      return ret;
    }

    FE_CHECK_NOTNULL(tesor_kernel_info_ptr);

    // 4.2 initialize all input format and dtype as supported
    std::vector<ge::Format> format_vec;
    if (format_dtype_querier_ptr_->GetSupportFormats(op_kernel_info_ptr, tesor_kernel_info_ptr, *(op_desc_ptr.get()),
                                                     format_vec) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetSptFmt][Op %s,type=%s]: Fail to get the support formats.",
                      op_name.c_str(), op_type.c_str());
      return FAILED;
    }

    GenerateInitialMatchedIndexVec(is_matched_index_vec_inited, matched_index_vec, format_vec);
    if (format_vec.empty()) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][GenerInitMtcIdxVec][Op %s,type=%s]: the input_format_vec of the %s %u is \
                      empty.", op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return OP_JUDGE_CHECK_FALSE_FAILED;
    }

    ge::ConstGeTensorDescPtr tensor_desc =
        is_input ? op_desc_ptr->GetInputDescPtr(i) : op_desc_ptr->GetOutputDescPtr(i);
    // 4.3 Generate basic information for dtype selection
    FormatDtypeSelectionBasicInfo basic_info = {
        op_kernel_info_ptr, tesor_kernel_info_ptr, node_ptr, static_cast<uint32_t>(i),
        tensor_desc, is_input, matched_index_vec};
    // 4.4 select the data type by different mode.
    ret = MatchByDifferentMode(basic_info);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][Match][Op %s,type=%s]: Failed to match dtype for tensor %u of node %s",
                      op_name.c_str(), op_type.c_str(), i, node_ptr->GetName().c_str());
      return ret;
    }
  }
  return SUCCESS;
}

Status OpFormatDtypeStrategyManager::GetAllPossibleFormatIndexByDefaultMode(
    const std::map<uint32_t, int> &prio_index_map, const IndexNameMap &tensor_index_name_map,
    const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_input,
    vector<uint32_t> &matched_index_vec) {
  auto op_desc_ptr = node_ptr->GetOpDesc();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  for (const auto &prio_index_iter : prio_index_map) {
    // 1. get the InputOrOutputInfo from the op kernel info store
    int32_t i = prio_index_iter.second;
    if (is_input && op_desc_ptr->MutableInputDesc(i) == nullptr) {
      continue;
    } else if (!is_input && op_desc_ptr->MutableOutputDesc(i) == nullptr) {
      continue;
    }
    auto tensor_iter = tensor_index_name_map.find(i);
    if (tensor_iter == tensor_index_name_map.end()) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][DefaultMode][Op %s,type=%s]: %s %u is not found in the ops store.",
                      op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return OP_JUDGE_MAP_KEY_FIND_FAILED;
    }
    InputOrOutputInfoPtr tesor_kernel_info_ptr = nullptr;
    Status ret = op_kernel_info_ptr->GetTensorInfoByName(is_input, tensor_iter->second, tesor_kernel_info_ptr);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][DefaultMode][Op %s,type=%s]: %s %u is not found in the ops store.",
                      op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return ret;
    }

    FE_CHECK_NOTNULL(tesor_kernel_info_ptr);
    ge::ConstGeTensorDescPtr tensor_desc =
        is_input ? op_desc_ptr->GetInputDescPtr(i) : op_desc_ptr->GetOutputDescPtr(i);
    uint32_t temp_i = static_cast<uint32_t>(i);
    FormatDtypeSelectionBasicInfo basic_info = {
        op_kernel_info_ptr, tesor_kernel_info_ptr, node_ptr, temp_i, tensor_desc, is_input, matched_index_vec};
    /* 2.1 Now for input, only when the predecessor is Data and the current
     * format is Heavy format we will match format with predecessors.
     * In other cases we just use original format.
     * format_selection_prev_strategy_ is used only for pytorch. */
    ret = FAILED;
    if (is_input) {
      ret = format_selection_prev_strategy_->Run(basic_info);
    }
    if (ret != SUCCESS) {
      format_selection_default_strategy_->Run(basic_info);
    }
  }
  return SUCCESS;
}

}  // namespace fe
