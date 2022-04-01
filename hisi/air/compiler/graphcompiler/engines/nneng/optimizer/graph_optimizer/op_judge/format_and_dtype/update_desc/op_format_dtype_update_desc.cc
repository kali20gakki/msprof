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

#include "graph_optimizer/op_judge/format_and_dtype/update_desc/op_format_dtype_update_desc.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
OpFormatDtypeUpdateDesc::OpFormatDtypeUpdateDesc(FormatDtypeQuerierPtr format_dtype_querier_ptr)
    : op_format_dtype_update_tensor_desc_ptr_(nullptr),
      format_dtype_querier_ptr_(format_dtype_querier_ptr) {}
OpFormatDtypeUpdateDesc::~OpFormatDtypeUpdateDesc() {}

Status OpFormatDtypeUpdateDesc::Initialize() {
  FE_MAKE_SHARED(op_format_dtype_update_tensor_desc_ptr_ =
      std::make_shared<OpFormatDtypeUpdateDescBase>(format_dtype_querier_ptr_), return FAILED);
  FE_CHECK_NOTNULL(op_format_dtype_update_tensor_desc_ptr_);
  return SUCCESS;
}
Status OpFormatDtypeUpdateDesc::UpdateTensorDescInfo(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                     const uint32_t &matched_index,
                                                     const IndexNameMap &tensor_index_name_map, const bool &is_input,
                                                     ge::NodePtr &node_ptr) {
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  FE_CHECK_NOTNULL(op_kernel_info_ptr);

  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();

  uint32_t tensor_size = is_input ? op_desc_ptr->GetAllInputsSize() : op_desc_ptr->GetAllOutputsDescSize();
  for (uint32_t i = 0; i < tensor_size; ++i) {
    if (is_input && op_desc_ptr->MutableInputDesc(i) == nullptr) {
      continue;
    } else if (!is_input && op_desc_ptr->MutableOutputDesc(i) == nullptr) {
      continue;
    }
    // query input format from op kernel info store
    auto tensor_iter = tensor_index_name_map.find(i);
    if (tensor_iter == tensor_index_name_map.end()) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdTensorDesc] Op[name=%s,type=%s]: the %s index %u is not \
                      found from the op store.", op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return OP_JUDGE_MAP_KEY_FIND_FAILED;
    }

    InputOrOutputInfoPtr tensor_info_ptr = nullptr;
    Status ret = op_kernel_info_ptr->GetTensorInfoByName(is_input, tensor_iter->second, tensor_info_ptr);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdTensorDesc] Op[name=%s,type=%s]: the %s index %u is not \
                      found from the op store.", op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(is_input), i);
      return ret;
    }

    FE_CHECK_NOTNULL(tensor_info_ptr);
    ge::GeTensorDescPtr tensor_desc = is_input ? op_desc_ptr->MutableInputDesc(i) : op_desc_ptr->MutableOutputDesc(i);
    UpdateInfo update_info = {op_kernel_info_ptr, tensor_info_ptr, matched_index, node_ptr, i, *tensor_desc, is_input};
    ret = op_format_dtype_update_tensor_desc_ptr_->UpdateDtype(update_info);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdTensorDesc] Fail to update date type of node[%s, %s].",
                      node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
      return ret;
    }
    ret = op_format_dtype_update_tensor_desc_ptr_->UpdateFormat(update_info);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdTensorDesc] Fail to update format of node[%s, %s].",
                      node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
      return ret;
    }
  }
  return SUCCESS;
}

}  // namespace fe
