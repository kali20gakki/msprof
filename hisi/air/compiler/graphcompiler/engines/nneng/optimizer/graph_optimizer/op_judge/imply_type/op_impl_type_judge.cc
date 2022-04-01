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

#include "graph_optimizer/op_judge/imply_type/op_impl_type_judge.h"
#include "graph/debug/ge_attr_define.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
const std::string LX_CORE_TYPE_WITH_DYNAMIC_UPPER = "DYNAMIC";
const std::string LX_CORE_TYPE_WITH_DYNAMIC_LOWER = "dynamic";
OpImplTypeJudge::OpImplTypeJudge(const std::string& engine_name, FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr)
    : OpJudgeBase(engine_name), ops_kernel_info_store_ptr_(fe_ops_kernel_info_store_ptr) {}
OpImplTypeJudge::~OpImplTypeJudge() {}

/*
 *  @ingroup fe
 *  @brief   set the highest prior imply type of op,
 *           update data type and format of op
 *  @param   [in|out] graph  compute graph
 *  @return  SUCCESS or FAILED
 */
Status OpImplTypeJudge::Judge(ge::ComputeGraph& graph) {
  // set the highest prior imply type of op
  FE_TIMECOST_START(OpImplTypeJudge);
  FE_CHECK_NOTNULL(ops_kernel_info_store_ptr_);
  for (auto& node : graph.GetAllNodes()) {
    Status result = JudgeByNode(node);
    if (result != SUCCESS) {
      return result;
    }
  }
  FE_TIMECOST_END(OpImplTypeJudge, "OpImplTypeJudge during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

Status OpImplTypeJudge::SetEngineType(ge::OpDescPtr op_desc_ptr) {
  std::string eng_type = ops_kernel_info_store_ptr_->GetFEOpsKernelInfoStoreName();
  if (ge::AttrUtils::SetStr(op_desc_ptr, kAttrEngineType, eng_type)) {
    return SUCCESS;
  } else {
    FE_LOGW("Set attr %s failed! Engine name is %s.", kAttrEngineType.c_str(), eng_type.c_str());
    return FAILED;
  }
}

Status OpImplTypeJudge::JudgeByNode(ge::NodePtr node_ptr) {
  // 1. check the op_type
  FE_CHECK_NOTNULL(node_ptr);
  string op_type = node_ptr->GetType();
  if (IsPlaceOrEnd(op_type)) {
    return SUCCESS;
  }

  // 2. check the attr of op_desc_ptr
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  if (SetEngineType(op_desc_ptr) != SUCCESS) {
    return OP_JUDGE_SET_CORE_TYPE_FAILED;
  }
  int64_t is_check_supported = 0;
  if (ge::AttrUtils::GetInt(op_desc_ptr, IS_CHECK_SUPPORTED, is_check_supported)) {
    std::string supported_flag = "not supported";
    uint64_t supported_value = is_check_supported;
    if ((supported_value & NOT_SUPPORTED_FLAG_BIT) == 0) {
      supported_flag = "supported";
    }
    FE_LOGD("Op[name=%s,type=%s]: the op has been check_supported, the result is %s.",
            op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), supported_flag.c_str());
    return SUCCESS;
  }
  if (ge::AttrUtils::HasAttr(op_desc_ptr, FE_IMPLY_TYPE)) {
    return SUCCESS;
  }

  // 3. set the imply type of op
  OpImplType impl_type = EN_RESERVED;
  return SetOpImplType(node_ptr, impl_type);
}

Status OpImplTypeJudge::SetOpImplType(const ge::NodePtr &node, OpImplType& imply_type) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  string op_name = op_desc_ptr->GetName().c_str();
  string op_type = op_desc_ptr->GetType().c_str();
  // 1. query the imply_type
  if (ops_kernel_info_store_ptr_->QueryHighPrioOpImplType(node, imply_type) != SUCCESS) {
    FE_LOGD("Op[name=%s,type=%s]: the op is not supported by the op info lib.", op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }

  // 2. check the imply_type
  auto iter = IMPL_TYPE_MAP.find(imply_type);
  if (iter == IMPL_TYPE_MAP.end()) {
    REPORT_FE_ERROR("[GraphOpt][OPImplJdg][CheckImplType][Op name=%s,type=%s]: the FE imply type [%d] is invalid.",
                    op_name.c_str(), op_type.c_str(), imply_type);
    return OP_JUDGE_MAP_KEY_FIND_FAILED;
  }

  // 3. set the fe and ge imply type of the op
  (void)ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, static_cast<int>(imply_type));
  bool is_ge_op = false;
  if (!ge::AttrUtils::GetBool(op_desc_ptr, IS_GE_OP, is_ge_op) || !is_ge_op) {
    (void)ge::AttrUtils::SetInt(op_desc_ptr, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(iter->second));
  }
  OpKernelInfoPtr op_kernel = nullptr;
  (void)ops_kernel_info_store_ptr_->GetHighPrioOpKernelInfoPtr(op_type, op_kernel);
  FE_CHECK_NOTNULL(op_kernel);
  std::string lx_core_type = op_kernel->GetCoreType();
  if (lx_core_type == LX_CORE_TYPE_WITH_DYNAMIC_UPPER || lx_core_type == LX_CORE_TYPE_WITH_DYNAMIC_LOWER) {
    std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> node_map;
    ops_kernel_info_store_ptr_->PrePareCompileParameter(node, op_type, imply_type, node_map);
    for (auto &comp_para : node_map) {
      OpStoreAdapterPtr op_store_adapter = comp_para.first;
      if (op_store_adapter->PreCompileOp(comp_para.second) != SUCCESS) {
        REPORT_FE_ERROR("[Graph][Compile][GetOpCoreType] PreCompileOp %s failed", op_type.c_str());
        return FAILED;
      }
    }
  } else {
    (void)ge::AttrUtils::SetStr(op_desc_ptr, kSgtCubeVectorCoreType, lx_core_type);
  }

  FE_LOGD("Op[name=%s,type=%s]: set the FE_IMPLY_TYPE [%s], set the IMPLY_TYPE [%s] and specific core type %s.",
      op_name.c_str(), op_type.c_str(), GetImplTypeString(imply_type).c_str(),
      GetGeImplTypeString(iter->second).c_str(), lx_core_type.c_str());
  return SUCCESS;
}

Status OpImplTypeJudge::GetLXCoreType(ge::NodePtr &node_ptr) {
  string op_name = node_ptr->GetOpDesc()->GetName();
  string op_type = node_ptr->GetOpDesc()->GetType();
  bool res = IsTbeOp(node_ptr);
  if (!res) {
    FE_LOGD("Op[name=%s,type=%s]: node_ptr is not tbe op.", op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }
  OpKernelInfoPtr op_kernel_info_ptr;
  Status ret = ops_kernel_info_store_ptr_->GetHighPrioOpKernelInfoPtr(op_type, op_kernel_info_ptr);
  if (ret != SUCCESS) {
    FE_LOGE("Op[name=%s,type=%s]: get GetHighPrioOpKernelInfoPtr failed.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }
  if (op_kernel_info_ptr == nullptr) {
    FE_LOGE("Op[name=%s,type=%s]: get the op kernel info failed.", op_name.c_str(), op_type.c_str());
    return FAILED;
  }
  std::string lx_core_type_str = op_kernel_info_ptr->GetCoreType();
  if (lx_core_type_str == LX_CORE_TYPE_WITH_DYNAMIC_LOWER || lx_core_type_str == LX_CORE_TYPE_WITH_DYNAMIC_UPPER) {
    OpImplType impl_type = op_kernel_info_ptr->GetOpStoreImplType();
    OpStoreAdapterPtr op_store_adapter = nullptr;
    if (ops_kernel_info_store_ptr_->GetOpStoreAdapter(impl_type, op_store_adapter) != SUCCESS) {
      REPORT_FE_ERROR("Op %s,type %s:fail to get op_store_adapter by op iml type [%d].",
                      op_name.c_str(), op_type.c_str(), impl_type);
      return FAILED;
    }
    bool is_dynamic_impl = IsOpDynamicImpl(node_ptr->GetOpDesc());
    if (op_store_adapter->GetLXOpCoreType(node_ptr, op_kernel_info_ptr, is_dynamic_impl, lx_core_type_str) !=SUCCESS) {
      FE_LOGW("Op %s,type %s:fail to GetLXOpCoreType.", op_name.c_str(), op_type.c_str());
    }
  }
  (void)ge::AttrUtils::SetStr(node_ptr->GetOpDesc(), CORE_TYPE_VALUE, lx_core_type_str);
  FE_LOGD("Op %s,type %s:set lx_core_type[%s] success.", op_name.c_str(), op_type.c_str(), lx_core_type_str.c_str());
  return SUCCESS;
}

Status OpImplTypeJudge::JudgeInSubGraph(ge::ComputeGraph& graph) {
  FE_CHECK_NOTNULL(ops_kernel_info_store_ptr_);
  for (auto &node : graph.GetDirectNode()) {
    if (GetLXCoreType(node) != SUCCESS) {
      FE_LOGE("Op %s,type %s:fail to GetLXOpCoreType.",
              node->GetOpDesc()->GetName().c_str(), node->GetOpDesc()->GetType().c_str());
      return FAILED;
    }
    Status result = JudgeInSubGraphByNode(node);
    if (result != SUCCESS) {
      return result;
    }
  }
  return SUCCESS;
}

Status OpImplTypeJudge::JudgeInSubGraphByNode(ge::NodePtr node_ptr) {
  // 1. check the op_type
  FE_CHECK_NOTNULL(node_ptr);
  string op_type = node_ptr->GetType();
  if (IsPlaceOrEnd(op_type)) {
    return SUCCESS;
  }

  // 2. check the imply_type
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  if (ge::AttrUtils::HasAttr(op_desc_ptr, FE_IMPLY_TYPE)) {
    return SUCCESS;
  }

  // 3. set the imply type of op
  OpImplType impl_type = EN_RESERVED;
  return SetOpImplType(node_ptr, impl_type);
}
}  // namespace fe
