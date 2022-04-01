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

#include "graph_optimizer/spacesize_calculator/spacesize_calculator.h"

#include "common/fe_inner_attr_define.h"
#include "common/fe_error_code.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/unknown_shape_util.h"
#include "graph/utils/tensor_utils.h"
#include "param_calculate/tensorsize_calculator.h"

namespace fe {
SpaceSizeCalculator::SpaceSizeCalculator() {}

SpaceSizeCalculator::~SpaceSizeCalculator() {}

Status SpaceSizeCalculator::CalculateRunningParams(const ge::ComputeGraph &graph) const {
  FE_LOGD("Begin to calculate running parameters of each op in graph[%s].", graph.GetName().c_str());

  for (auto &node_ptr : graph.GetDirectNode()) {
    FE_CHECK_NOTNULL(node_ptr);
    string op_type = node_ptr->GetType();
    if (op_type == OP_TYPE_PLACE_HOLDER || op_type == OP_TYPE_END) {
      continue;
    }
    ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc_ptr);

    (void)TensorSizeCalculator::CalculateOpTensorSize(*(op_desc_ptr.get()));
  }
  FE_LOGD("Finish calculating running parameters of each op in graph[%s].", graph.GetName().c_str());
  return SUCCESS;
}

Status SpaceSizeCalculator::CalculateAICoreRunningParams(const ge::ComputeGraph &graph) const {
  FE_LOGD("Begin to calculate running parameters of each op in graph[%s].", graph.GetName().c_str());

  for (auto node_ptr : graph.GetDirectNode()) {
    FE_CHECK_NOTNULL(node_ptr);
    string op_type = node_ptr->GetType();
    if (op_type == OP_TYPE_PLACE_HOLDER || op_type == OP_TYPE_END) {
      continue;
    }
    ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc_ptr);

    // only deal aicore node
    if (!ge::AttrUtils::HasAttr(op_desc_ptr, FE_IMPLY_TYPE)) {
      continue;
    }

    Status status = TensorSizeCalculator::CalculateOpTensorSize(*(op_desc_ptr.get()));
    if (status != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][CalcTensorSize][CalcAicoreRunPara] Fail to calculate running parameters of \
                      op [%s, %s].", op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
      return status;
    }
  }
  FE_LOGD("Finish calculating running parameters of each op in graph[%s].", graph.GetName().c_str());
  return SUCCESS;
}

}  // namespace fe
