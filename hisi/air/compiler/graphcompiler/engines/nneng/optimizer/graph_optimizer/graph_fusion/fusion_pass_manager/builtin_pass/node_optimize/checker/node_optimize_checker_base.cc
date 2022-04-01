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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"

namespace fe {
bool NodeOptimizeCheckerBase::IsDimC(const ge::NodePtr &node_ptr, const string &dim_attr, bool is_input) const {
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  string node_name = node_ptr->GetName();
  // 1. get the dim attribute
  int dim_attr_value = 0;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, dim_attr, dim_attr_value)) {
    FE_LOGD("Node[%s]: can't get the attribute concat_dim, check failed.", node_name.c_str());
    return false;
  }
  ge::OpDesc::Vistor<ge::GeTensorDesc> all_tensor_desc =
      is_input ? op_desc_ptr->GetAllInputsDesc() : op_desc_ptr->GetAllOutputsDesc();

  int i = 0;
  for (auto &tensor_desc : all_tensor_desc) {
    // 2. get the postion of the c axis
    int pos = 0;
    Status status = GetPosOfDimC(tensor_desc, pos);
    if (status != SUCCESS) {
      FE_LOGD("Node[%s]: get the dim_c position of the input [%d] not success.", node_name.c_str(), i);
      return false;
    }

    // 3. if the dim_attr_value < 0, add the dim_num
    int dim_num = tensor_desc.GetOriginShape().GetDimNum();
    if (dim_attr_value < 0) {
      dim_attr_value += dim_num;
    }
    if (pos != dim_attr_value) {
      FE_LOGD("Node[%s]: the dim_c position of the input [%d] is not equal to concat_dim, check failed.",
          node_name.c_str(), i);
      return false;
    }
    i++;
  }
  return true;
}
Status NodeOptimizeCheckerBase::GetDimC(const ge::GeTensorDesc &tensor_desc, int &dim_c) const {
  int pos = 0;
  Status status = GetPosOfDimC(tensor_desc, pos);
  if (status != SUCCESS) {
    FE_LOGD("Get the position of dim C not success.");
    return FAILED;
  }
  ge::GeShape shape = tensor_desc.GetOriginShape();
  int dim_num = shape.GetDimNum();
  if (pos + 1 > dim_num) {
    FE_LOGD("The position + 1 > dim_num, the position [%d], the dim_num [%d].", pos, dim_num);
    return FAILED;
  }
  dim_c = shape.GetDim(pos);
  return SUCCESS;
}

Status NodeOptimizeCheckerBase::GetPosOfDimC(const ge::GeTensorDesc &tensor_desc, int &pos) const {
  ge::Format origin_format = tensor_desc.GetOriginFormat();
  if (origin_format == ge::FORMAT_NCHW) {
    pos = NCHW_DIM_C;
  } else if (origin_format == ge::FORMAT_NHWC) {
    pos = NHWC_DIM_C;
  } else if (origin_format == ge::FORMAT_HWCN) {
    pos = HWCN_DIM_C;
  } else if (origin_format == ge::FORMAT_CHWN) {
    pos = CHWN_DIM_C;
  } else {
    FE_LOGD("Unsupported origin_format: [%d].", origin_format);
    return FAILED;
  }
  return SUCCESS;
}

bool NodeOptimizeCheckerBase::IsDimCOfInputAligned(const ge::GeTensorDesc &tensor_desc, const int &dim_c,
                                                   const ge::DataType &quant_data_type) const {
  ge::DataType origin_data_type = tensor_desc.GetOriginDataType();
  if (quant_data_type == ge::DT_INT8 || quant_data_type == ge::DT_INT4) {
    origin_data_type = quant_data_type;
  }
  if (origin_data_type == ge::DT_FLOAT16 || origin_data_type == ge::DT_FLOAT) {
    return dim_c % 16 == 0;
  } else if (origin_data_type == ge::DT_INT8) {
    return dim_c % 32 == 0;
  } else if (origin_data_type == ge::DT_INT4) {
    return dim_c % 64 == 0;
  }
  FE_LOGD("Unsupported origin_data_type is [%d].", origin_data_type);
  return false;
}

bool NodeOptimizeCheckerBase::IsInputNotData(const ge::NodePtr &node_ptr) const {
  auto input_nodes = node_ptr->GetInDataNodes();
  if (input_nodes.empty()) {
    return false;
  }
  ge::NodePtr input_node = input_nodes.at(0);
  if (input_node == nullptr) {
    return false;
  }
  if (input_node->GetType() == DATA) {
    return false;
  }
  return true;
}
}  // namespace fe
