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

#include "heavy_format_selector.h"
#include "common/math_util.h"

namespace fe {
HeavyFormatSelector::HeavyFormatSelector(FormatDtypeQuerierPtr format_dtype_querier_ptr)
    : format_dtype_querier_ptr_(format_dtype_querier_ptr),
      precise_dtype_matcher_ptr_(nullptr),
      input_and_output_kernel_(),
      matched_index_() {}

HeavyFormatSelector::~HeavyFormatSelector() {}

Status HeavyFormatSelector::Initalize() {
  FE_MAKE_SHARED(precise_dtype_matcher_ptr_ = std::make_shared<OpDtypePreciseMatcher>(), return FAILED);
  input_and_output_kernel_.emplace_back();
  input_and_output_kernel_.emplace_back();
  return SUCCESS;
}

FormatScore GetFormatScore(const ge::Format &distributed_heavy_format, const ge::Format &current_original_format,
                           const ge::Format &kernel_format) {
  if (kernel_format == distributed_heavy_format) {
    return FormatScore::DISTRIBUTED_HEAVY_FORMAT_SCORE;
  } else if (kernel_format == ge::FORMAT_NC1HWC0 || kernel_format == ge::FORMAT_FRACTAL_Z ||
             kernel_format == ge::FORMAT_C1HWNCoC0 || kernel_format == ge::FORMAT_FRACTAL_NZ) {
    return FormatScore::OTHER_HEAVY_FORMAT_SCORE;
  } else if (kernel_format == current_original_format || kernel_format == ge::FORMAT_ND) {
    return FormatScore::ORIGINAL_FORMAT_SCORE;
  } else {
    return FormatScore::OTHER_FORMAT_SCORE;
  }
}

/* Get the largest element's index in vec */
template <typename T>
int32_t GetLargestElement(std::vector<T> vec) {
  T max = 0;
  int32_t matched_index = INVALID_KERNEL_INDEX;
  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec.at(i) >= max) {
      max = vec.at(i);
      matched_index = static_cast<int32_t>(i);
    }
  }
  return matched_index;
}

Status HeavyFormatSelector::CalcFormatScore(const ge::OpDesc::Vistor<ge::GeTensorDescPtr> &all_tensors,
                                            const fe::OpKernelInfoPtr &op_kernel_info_ptr,
                                            const ge::OpDescPtr &op_desc_ptr, uint32_t kernel_format_index,
                                            const HeavyFormatInfo &heavy_format_info, InputOrOutputIndex in_or_out,
                                            uint64_t &score) {
  auto size = all_tensors.size();
  for (size_t index = 0; index < size; ++index) {
    auto tensor = all_tensors.at(index);
    /* Here we just skip the array size and empty check because we have
     * done that in last function SelectQualifiedFormat. */
    auto tensor_info = input_and_output_kernel_[in_or_out].at(index);
    vector<ge::Format> kernel_formats;
    if (format_dtype_querier_ptr_->GetSupportFormats(op_kernel_info_ptr, tensor_info, *(op_desc_ptr.get()),
                                                     kernel_formats) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][CalFmtScore] Failed to get the support formats for node %s.",
                      op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    if (kernel_format_index >= kernel_formats.size()) {
      FE_LOGW("Format index %u is larger than kernel size %zu. %s", kernel_format_index, kernel_formats.size(),
              op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    uint64_t format_score = static_cast<uint64_t>(GetFormatScore(
        heavy_format_info.expected_heavy_format, tensor->GetOriginFormat(), kernel_formats.at(kernel_format_index)));
    FE_UINT64_ADDCHECK(score, format_score);
    score += format_score;
  }
  return SUCCESS;
}

Status HeavyFormatSelector::GetMostSuitableFormatIndex(const fe::OpKernelInfoPtr &op_kernel_info_ptr,
                                                       const ge::NodePtr &current_node,
                                                       const HeavyFormatInfo &heavy_format_info,
                                                       const std::vector<IndexNameMap> &tensor_map,
                                                       int32_t &most_suitable_index) {
  /* First Clear input_and_output_kernel_ */
  for (auto &ele : input_and_output_kernel_) {
    auto new_vec = std::vector<InputOrOutputInfoPtr>();
    ele.swap(new_vec);
  }

  std::vector<uint32_t> new_matched_index = std::vector<uint32_t>();
  matched_index_.swap(new_matched_index);

  Status ret = SelectQualifiedFormat(op_kernel_info_ptr, current_node, heavy_format_info, tensor_map);
  if (ret != SUCCESS) {
    return FAILED;
  }

  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto all_input_tensor = op_desc_ptr->GetAllInputsDescPtr();
  auto all_output_tensor = op_desc_ptr->GetAllOutputsDescPtr();
  /* Here we define a vector which store the corresponding score of current
   * combination of format.
   * The format score is define as below:
   * Same as the heavy format which is distributed right now : 2000;
   * Other heavy format : 200
   * Same as the tensor's original format(ND included) : 100
   * Other format: 1 */
  /* For each index value in matched_index, we calculate its score and
   * store the score in score_record_vec */
  std::vector<uint64_t> score_record_vec;
  for (const auto &kernel_format_index : matched_index_) {
    uint64_t score = 0;
    Status in_ret = CalcFormatScore(all_input_tensor, op_kernel_info_ptr, op_desc_ptr, kernel_format_index,
                                    heavy_format_info, INPUT_INDEX, score);
    if (in_ret != SUCCESS) {
      return FAILED;
    }

    in_ret = CalcFormatScore(all_output_tensor, op_kernel_info_ptr, op_desc_ptr, kernel_format_index, heavy_format_info,
                             OUTPUT_INDEX, score);
    if (in_ret != SUCCESS) {
      return FAILED;
    }
    score_record_vec.emplace_back(score);
  }
  string score = StringUtils::IntegerVecToString<uint64_t>(score_record_vec);
  FE_LOGD("The score is %s", score.c_str());
  auto largest_index = GetLargestElement(score_record_vec);
  if (largest_index != INVALID_KERNEL_INDEX) {
    most_suitable_index = matched_index_[largest_index];
    return SUCCESS;
  } else {
    return FAILED;
  }
}

Status HeavyFormatSelector::Match(const OpKernelInfoPtr &op_kernel_info_ptr,
                                  const ge::OpDescPtr &op_desc_ptr,
                                  const ge::OpDesc::Vistor<ge::GeTensorDescPtr> &all_tensors,
                                  InputOrOutputIndex in_or_out) {
  Status ret;
  for (size_t index = 0; index < all_tensors.size(); ++index) {
    auto &tensor = all_tensors.at(index);
    if (index >= input_and_output_kernel_[in_or_out].size()) {
      FE_LOGW("Output tensor index %zu is larger than kernel size %zu. %s", index,
              input_and_output_kernel_[in_or_out].size(), op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    auto output_info = input_and_output_kernel_[in_or_out].at(index);
    vector<ge::DataType> kernel_data_types;
    if (format_dtype_querier_ptr_->GetSupportDataTypes(op_kernel_info_ptr, output_info, *(op_desc_ptr.get()),
                                                       kernel_data_types) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][Match] Failed to get the support data_types for node %s.",
                      op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    ge::DataType current_data_type = tensor->GetDataType();
    ret =
        precise_dtype_matcher_ptr_->FindSuitableDtype(kernel_data_types, current_data_type, matched_index_, ge::DT_MAX);
    if (ret == FAILED) {
      FE_LOGD("Failed to FindSuitableDtype for node %s.", op_desc_ptr->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status HeavyFormatSelector::MatchDtypeForAllInputAndOutput(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                           const ge::NodePtr &current_node) {
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  if (input_and_output_kernel_.empty() || input_and_output_kernel_.size() < INPUT_OUTPUT_INDEX_BOTTOM) {
    FE_LOGW("Input tensor vector of node %s is empty, its size is %zu.", current_node->GetName().c_str(),
            input_and_output_kernel_.size());
    return FAILED;
  }
  Status ret;
  auto all_input_tensor = op_desc_ptr->GetAllInputsDescPtr();
  if (all_input_tensor.empty()) {
    FE_LOGW("Input tensor vector of node %s is empty", current_node->GetName().c_str());
    return FAILED;
  }
  ret = Match(op_kernel_info_ptr, op_desc_ptr, all_input_tensor, INPUT_INDEX);
  if (ret != SUCCESS) {
    return ret;
  }

  auto all_output_tensor = op_desc_ptr->GetAllOutputsDescPtr();
  if (all_output_tensor.empty()) {
    FE_LOGW("Output tensor vector of node %s is empty", current_node->GetName().c_str());
    return FAILED;
  }
  ret = Match(op_kernel_info_ptr, op_desc_ptr, all_output_tensor, OUTPUT_INDEX);
  if (ret != SUCCESS) {
    return ret;
  }

  string matched_index_string = StringUtils::IntegerVecToString<uint32_t>(matched_index_);
  FE_LOGD("After matching dtype, matched index is %s for node %s", matched_index_string.c_str(),
          current_node->GetName().c_str());
  return SUCCESS;
}

Status HeavyFormatSelector::SelectQualifiedFormat(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                  const ge::NodePtr &current_node,
                                                  const HeavyFormatInfo &heavy_format_info,
                                                  const std::vector<IndexNameMap> &tensor_map) {
  InputOrOutputInfoPtr input_or_output_info;

  Status ret = GetAllInputAndOutputKernelInfo(op_kernel_info_ptr, current_node, tensor_map, input_and_output_kernel_);
  if (ret != SUCCESS) {
    return FAILED;
  }

  if (input_and_output_kernel_.empty() || input_and_output_kernel_.size() < INPUT_OUTPUT_INDEX_BOTTOM) {
    FE_LOGW("Input tensor vector of node %s is empty, its size is %zu.", current_node->GetName().c_str(),
            input_and_output_kernel_.size());
    return FAILED;
  }

  ret = SearchHeavyFormatInKernel(op_kernel_info_ptr, current_node->GetOpDesc(), heavy_format_info);
  if (ret != SUCCESS) {
    return FAILED;
  }

  ret = MatchDtypeForAllInputAndOutput(op_kernel_info_ptr, current_node);
  if (ret != SUCCESS) {
    FE_LOGD("Failed to MatchDtypeForAllInputAndOutput");
    return FAILED;
  }
  return SUCCESS;
}

Status HeavyFormatSelector::SearchHeavyFormatInKernel(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                      const ge::OpDescPtr &op_desc_ptr,
                                                      const HeavyFormatInfo &heavy_format_info) {
  InputOrOutputInfoPtr input_or_output_info;
  if (heavy_format_info.is_input) {
    if (static_cast<size_t>(heavy_format_info.anchor_index) >= input_and_output_kernel_[INPUT_INDEX].size()) {
      FE_LOGW("Input tensor idx %u is larger than kernel size %zu. %s", heavy_format_info.anchor_index,
              input_and_output_kernel_[INPUT_INDEX].size(), op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    input_or_output_info = input_and_output_kernel_[INPUT_INDEX].at(heavy_format_info.anchor_index);
  } else {
    if (static_cast<size_t>(heavy_format_info.anchor_index) >= input_and_output_kernel_[OUTPUT_INDEX].size()) {
      FE_LOGW("Output tensor idx %u is larger than kernel size %zu. %s", heavy_format_info.anchor_index,
              input_and_output_kernel_[OUTPUT_INDEX].size(), op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    input_or_output_info = input_and_output_kernel_[OUTPUT_INDEX].at(heavy_format_info.anchor_index);
  }
  vector<ge::Format> kernel_formats;
  if (format_dtype_querier_ptr_->GetSupportFormats(op_kernel_info_ptr, input_or_output_info, *(op_desc_ptr.get()),
                                                   kernel_formats) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][FmtPropagate][SearchHvyFmtInKernel] Failed to get the support formats for \
                    node %s.",
        op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  vector<ge::DataType> kernel_data_types;
  if (format_dtype_querier_ptr_->GetSupportDataTypes(op_kernel_info_ptr, input_or_output_info, *(op_desc_ptr.get()),
                                                     kernel_data_types) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][FmtPropagate][SearchHvyFmtInKernel] Failed to get the support data_types for \
                    node %s.",
        op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  for (size_t i = 0; i < kernel_formats.size(); ++i) {
    /* Heavy format or ALL */
    if (kernel_formats[i] == heavy_format_info.expected_heavy_format) {
      matched_index_.emplace_back(i);
    }
  }
  string matched_string = StringUtils::IntegerVecToString(matched_index_);
  FE_LOGD("Format index %s is qualified for node %s", matched_string.c_str(), op_desc_ptr->GetName().c_str());
  return SUCCESS;
}
}  // namespace fe