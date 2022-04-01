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

#include "graph_optimizer/op_setter/op_setter.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "common/aicore_util_attr_define.h"
#include "common/string_utils.h"
#include "common/util/op_info_util.h"
#include "common/fe_utils.h"
#include "common/configuration.h"
#include "common/op_slice_util.h"
#include "common/unknown_shape_util.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
using OpSliceUtilPtr = std::shared_ptr<OpSliceUtil>;

const std::map<string, string> OpSetter::attr_map_ = {{fe::STR_INPUT_MEM_CONTINUES, fe::ATTR_NAME_CONTINUOUS_INPUT},
                                                      {fe::STR_OUTPUT_MEM_CONTINUES, fe::ATTR_NAME_CONTINUOUS_OUTPUT}};

OpSetter::OpSetter(const std::string engine_name, OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : engine_name_(engine_name), op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr) {}
OpSetter::~OpSetter() {}

Status OpSetter::SetOpInfo(const ge::ComputeGraph& graph) const {
  for (auto& node : graph.GetAllNodes()) {
    Status result = SetOpInfoByNode(node);
    if (result != SUCCESS) {
      return result;
    }
  }
  return SUCCESS;
}

Status OpSetter::SetOpInfoByNode(const ge::NodePtr &node_ptr) const {
  // 1. check the node_ptr and the op_desc_ptr
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);

  // 2. check the imply_type
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  int imply_type = -1;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, imply_type)) {
    FE_LOGD("Op[name=%s,type=%s]: Get the attribute FE_IMPLY_TYPE not success.", op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }

  // 3. get the op_kernel_info_ptr by op_impl_type and op_type
  OpImplType op_impl_type = static_cast<OpImplType>(imply_type);
  OpKernelInfoPtr op_kernel_info_ptr =
      OpsKernelManager::Instance(engine_name_).GetOpKernelInfoByOpType(op_impl_type, op_desc_ptr->GetType());
  if (op_kernel_info_ptr == nullptr) {
    FE_LOGW("Op[name=%s,type=%s]: get the op kernel info failed.", op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }
  FE_CHECK_NOTNULL(op_kernel_info_ptr);

  // 4. set the attributes of the op
  for (auto &it : attr_map_) {
    if (SetOpDescAttr(op_kernel_info_ptr, it.first, it.second, op_desc_ptr) != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][SetOpInfo][SetOpInfo] Op[name=%s,type=%s]: get the attribute [%s] from the op "
          "kernel info, and set the attribute [%s] failed!",
          op_name.c_str(), op_type.c_str(), it.first.c_str(), it.second.c_str());
      return FAILED;
    }
  }

  SlicePattern slice_pattern = op_kernel_info_ptr->GetOpSlicePattern();
  OpSliceUtilPtr slice_util_ptr = nullptr;
  FE_MAKE_SHARED(slice_util_ptr = std::make_shared<OpSliceUtil>(), return FAILED);
  if (slice_util_ptr->SetOpSliceInfo(node_ptr, slice_pattern, true) != SUCCESS) {
    FE_LOGW("Op[name=%s, type=%s]: set the slice info for node failed.", op_name.c_str(), op_type.c_str());
  }

  if (SetTbeOpSliceInfo(node_ptr, op_kernel_info_ptr) != SUCCESS) {
    FE_LOGW("Op[name=%s,type=%s]: set tbe slice info for node failed!", op_name.c_str(), op_type.c_str());
  }
  return SUCCESS;
}

Status OpSetter::SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) const {
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  OpStoreAdapterPtr op_store_adapter = nullptr;
  FE_CHECK_NOTNULL(op_store_adapter_manager_ptr_);
  Status status = op_store_adapter_manager_ptr_->GetOpStoreAdapter(EN_IMPL_HW_TBE, op_store_adapter);
  if (status != SUCCESS) {
    REPORT_FE_ERROR("[OrigGraphOpt][OpSetter] Failed to get op store adapter by impl_type[%d].", EN_IMPL_HW_TBE);
    return OP_COMPILER_CHECK_FALSE_FAILED;
  }
  if (op_kernel_info_ptr->GetOpInfo().opFileName == NULL_OP_FILE) {
    return SUCCESS;
  }

  // set tbe slice info
  Status ret = op_store_adapter->SetTbeOpSliceInfo(node_ptr, op_kernel_info_ptr);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[OrigGraphOpt][OpSetter] SetTbeOpSliceInfo failed, graph[%s].", op_desc_ptr->GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status OpSetter::SetOpDescAttr(const OpKernelInfoPtr &op_kernel_info_ptr, const std::string &attr_name,
                               const std::string &ge_attr_name, ge::OpDescPtr op_desc_ptr) const {
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  bool value = false;

  if (attr_name == STR_INPUT_MEM_CONTINUES) {
    value = op_kernel_info_ptr->GetInputMemContinuesFlag();
  } else if (attr_name == STR_OUTPUT_MEM_CONTINUES) {
    value = op_kernel_info_ptr->GetOutputMemContinuesFlag();
  } else {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetOpInfo][SetOpDescAttr] the attribute [%s] can not get from op kernel.",
                    attr_name.c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: get the attribute [%s] success, value is %d", op_name.c_str(), op_type.c_str(),
          attr_name.c_str(), value);

  if (!ge::AttrUtils::SetBool(op_desc_ptr, ge_attr_name, value)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetOpInfo][SetOpDescAttr] Op[name=%s,type=%s]: set the attribute [%s] failed.",
                    op_name.c_str(), op_type.c_str(), ge_attr_name.c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: set the attribute [%s] success.", op_name.c_str(), op_type.c_str(),
          ge_attr_name.c_str());
  return SUCCESS;
}
}  // namespace fe