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

#include "graph_optimizer/op_compiler/op_compiler.h"
#include <utility>
#include <vector>
#include "adapter/common/op_store_adapter_manager.h"
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_type_utils.h"
#include "common/ffts_plus_type.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "common/util/op_info_util.h"
#include "common/scope_allocator.h"
#include "framework/common/types.h"
#include "ge/ge_api_types.h"
#include "graph/tuning_utils.h"
#include "graph/utils/attr_utils.h"
#include "ops_store/ops_kernel_manager.h"
#include "platform_info.h"
#include "graph_optimizer/spacesize_calculator/tensor_compute_util.h"

namespace fe {
static const size_t COMPRESS_PARAMETER_SIZE = 8;
static const size_t MULTI_CORE_COMPRESS_PARAMETER_SIZE = 9;
static const size_t COMPRESS_PARAMETER_INFOSIZE_INDEX = 1;
static const int64_t WEIGHT_SIZE_THRESHOLD = 262144;  // 256K
static const double WEIGHT_CHECK_THRESHOLD = 0.8;
static const uint64_t CUBE_K_OF_INT8 = 32;

OpCompiler::OpCompiler(const std::string &compiler_name, const std::string &engine_name,
                       OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : init_flag_(false),
      engine_name_(engine_name),
      op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr),
      compiler_name_(compiler_name) {}

string OpCompiler::GetCompilerName() {
  return compiler_name_;
}

OpCompiler::~OpCompiler() {}

/*
 *  @ingroup fe
 *  @brief   initialize op compiler
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::Initialize() {
  // if graph optimizer has been initialized, return SUCCESS
  if (init_flag_) {
    FE_LOGW("OpCompiler has been initialized.");
    return SUCCESS;
  }

  init_flag_ = true;
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   finalize op compiler
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::Finalize() {
  if (!init_flag_) {
    FE_LOGW("OpCompiler finalize is not allowed before initialize first.");
    return SUCCESS;
  }

  init_flag_ = false;
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   add node to fusion_node_map according to scope id
 *  @param   [in]  node           node pointer
 *  @param   [in] scope_id         scope id
 *  @param   [out] fusion_node_map  scope id and node map
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::AddNodeToFusionMap(ge::Node &node, const int64_t scope_id, ScopeNodeIdMap &fusion_node_map) {
  ScopeNodeIdMap::iterator nodelist_it = fusion_node_map.find(scope_id);
  if (nodelist_it == fusion_node_map.end()) {
    std::vector<ge::Node *> node_list_new;
    node_list_new.push_back(&node);
    fusion_node_map.insert(std::pair<int64_t, std::vector<ge::Node *>>(scope_id, node_list_new));
  } else {
    nodelist_it->second.push_back(&node);
  }

  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   get scope node map
 *  @param   [in]  graph        node pointer
 *  @param   [out] fusion_map    scope id and node map
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::GetScopeNodeMap(const ge::ComputeGraph& graph, ScopeNodeIdMap& fusion_node_map) {
  std::vector<ge::NodePtr> all_nodes;
  for (auto const &node : graph.GetDirectNode()) {
    all_nodes.push_back(node);
  }
  if (GetScopeNodeMap(graph, all_nodes, fusion_node_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][GetScopeNdMap] Get scope_node_map failed.");
    return FAILED;
  }

  return SUCCESS;
}

Status OpCompiler::VerifyScopeIdAttr(const int64_t &scope_id, const bool &is_l1_fusion,
                                     std::map<int64_t, bool> &fusion_scope_type_map) {
  auto scope_iter = fusion_scope_type_map.find(scope_id);
  if (scope_iter != fusion_scope_type_map.end()) {
    if (scope_iter->second != is_l1_fusion) {
      if (is_l1_fusion) {
        FE_LOGE(
            "The scopeId[%ld] of this node is used for L1 fusion"
            "while the same scopeId of other node is used for UB fusion."
            "The scope id of L1 and UB fusion must not be the same.",
            scope_id);
      } else {
        FE_LOGE(
            "The scopeId[%ld] of this node is used for UB fusion"
            "while the same scopeId of other node is used for L1 fusion."
            "The scope id of L1 and UB fusion must not be the same.",
            scope_id);
      }
      return FAILED;
    }
  } else {
    fusion_scope_type_map.emplace(scope_id, is_l1_fusion);
  }
  return SUCCESS;
}

Status OpCompiler::AddNormalTbeNodeIntoMap(ge::NodePtr node, ge::OpDescPtr op_desc_ptr,
                                           ScopeNodeIdMap &fusion_node_map,
                                           std::map<int64_t, bool> &fusion_scope_type_map) {
  int64_t scope_id = 0;
  bool is_l1_fusion = false;
  bool has_scope_id = GetFusionScopeAttr(op_desc_ptr, scope_id, is_l1_fusion);
  string op_type = op_desc_ptr->GetType();
  int tmp_imply_type = 0;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, tmp_imply_type)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][AddNdIntoMap] get imply type failed, op[%s, optype[%s]].",
                    op_desc_ptr->GetName().c_str(), op_type.c_str());
    return OP_COMPILER_CHECK_FALSE_FAILED;
  }

  OpImplType impl_type = static_cast<OpImplType>(tmp_imply_type);
  if (IsInvalidImplType(engine_name_, impl_type)) {
    return SUCCESS;
  }
  bool is_tbe = IsTbe(impl_type);
  /* In two scenario the scope id will be negative and duplicated.
   * 1. automatic ub fusion: some nodes are all -1 and they are expected
   *    to compile alone.
   * 2. ms-tuning process, some nodes will be duplicated and their negative scope id is also duplicated. */
  if (has_scope_id && is_tbe && scope_id >= 0) {
    if (VerifyScopeIdAttr(scope_id, is_l1_fusion, fusion_scope_type_map) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][CheckScopeId] The scope_id of node[%s], type [%s] is not valid.",
                      node->GetName().c_str(), op_type.c_str());
      return FAILED;
    }
    if (AddNodeToFusionMap(*node, scope_id, fusion_node_map) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][AddNdIntoMap] AddNodeToFusionMap failed, node [%s, type [%s]].",
                      node->GetName().c_str(), op_type.c_str());
      return FAILED;
    }
  } else {
    FEOpsStoreInfo op_store_info;
    if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(impl_type, op_store_info) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][AddNdIntoMap] node [%s] Failed to get op store info by impl_type[%d].",
                      node->GetName().c_str(), impl_type);
      return OP_COMPILER_CHECK_FALSE_FAILED;
    }
    if (op_store_info.need_compile) {
      std::vector<ge::Node *> node_vec;
      node_vec.push_back(node.get());
      int64_t single_op_scope_id = ScopeAllocator::Instance().AllocateNegScopeId();
      fusion_node_map[single_op_scope_id] = node_vec;
      (void)ScopeAllocator::SetScopeAttr(op_desc_ptr, single_op_scope_id);
    }
  }
  return SUCCESS;
}

bool CheckScopeNodeMap(const ge::NodePtr &node) {
  auto op_desc_ptr = node->GetOpDesc();
  bool is_virtual_op = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_NOTASK, is_virtual_op);
  if (is_virtual_op) {
    FE_LOGD("Op %s is virtual, type = %s, no need compile.", node->GetName().c_str(), node->GetType().c_str());
    return false;
  }

  bool need_single_op_compile = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NEED_COMPILE, need_single_op_compile);
  if (need_single_op_compile) {
    FE_LOGD("Op %s needs single-op-compile, type = %s, no need compile.", node->GetName().c_str(),
            node->GetType().c_str());
    return false;
  }

  if (IsUnKnownShapeOp(*(op_desc_ptr.get()))) {
    bool is_support_unknown_shape = true;
    if (ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, is_support_unknown_shape) &&
        !is_support_unknown_shape) {
      FE_LOGD("op[name:%s,type:%s] is not support dynamic shape, no need to compile.", node->GetName().c_str(),
              node->GetType().c_str());
      return false;
    }
  }

  if (ge::AttrUtils::HasAttr(op_desc_ptr, kOpShapeOrRangeUnsupport)) {
    FE_LOGD("Op[name:%s] is unkwown rank shape, no need compile.", node->GetName().c_str());
    return false;
  }
  return true;
}

Status OpCompiler::GetScopeNodeMap(const ge::ComputeGraph& graph,
                                   const std::vector<ge::NodePtr>& scope_nodes,
                                   ScopeNodeIdMap& fusion_node_map) {
  vector<string> vec = {
      OP_TYPE_PLACE_HOLDER, OP_TYPE_END, CONSTANTOP, fe::CONSTANT, CAST, SWAPCO, BNHOST, RESHAPE, REFORMAT,
      COMPRESSOP,           COMPRESSFCOP};
  std::map<int64_t, bool> fusion_scope_type_map;
  for (auto &node : scope_nodes) {
    FE_CHECK(node == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][GetScopeNdMap] Node is nullptr."), return FAILED);

    string session_graph_id;
    if (ge::AttrUtils::GetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) && !session_graph_id.empty()) {
      FE_LOGD("SessionGraphId=%s, node is %s", session_graph_id.c_str(), node->GetName().c_str());
      (void)ge::AttrUtils::SetStr(node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    }
    string op_type = node->GetType();
    vector<string>::iterator ret = std::find(vec.begin(), vec.end(), op_type);
    if (ret != vec.end()) {
      continue;
    }

    auto op_desc_ptr = node->GetOpDesc();
    FE_CHECK(op_desc_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][GetScopeNdMap] opDescPtr is nullptr."),
             return FAILED);

    if (!CheckScopeNodeMap(node)) {
      continue;
    }

    // get op kernel info store name
    Status status = AddNormalTbeNodeIntoMap(node, op_desc_ptr, fusion_node_map, fusion_scope_type_map);
    if (status != SUCCESS) {
      return status;
    }
  }
  return SUCCESS;
}

bool OpCompiler::IsTbe(const OpImplType& impl_type) const {
  return impl_type == EN_IMPL_CUSTOM_TBE || impl_type == EN_IMPL_PLUGIN_TBE || impl_type == EN_IMPL_HW_TBE ||
         impl_type == EN_IMPL_NON_PERSISTENT_CUSTOM_TBE || impl_type == EN_IMPL_VECTOR_CORE_HW_TBE ||
         impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE;
}

bool IsNeedPreCompile(ge::NodePtr &node, ge::OpDescPtr &op_desc_ptr, bool &need_precompile_graph) {
  string op_type = node->GetType();
  string const_op_type;
  bool const_flag = ge::NodeUtils::GetConstOpType(node, const_op_type);
  bool type_check = (op_type == OP_TYPE_PLACE_HOLDER || op_type == OP_TYPE_END || op_type == RESHAPE ||
                     op_type == OP_TYPE_PHONY_CONCAT || op_type == REFORMAT || op_type == COMPRESSOP ||
                     op_type == COMPRESSFCOP || const_flag);

  if (type_check) {
    return false;
  }

  if (need_precompile_graph) {
    bool need_precompile_node = false;
    (void)ge::AttrUtils::GetBool(op_desc_ptr, NEED_RE_PRECOMPILE, need_precompile_node);
    if (!need_precompile_node) {
      return false;
    }
  }

  /* In FE, we don't need to precompile and compile op Cast and TransData.
   * They will be compiled by GE after trans-nodes fusion and merging. */
  bool not_need_compile = (op_type == CAST || op_type == SWAPCO || op_type == BNHOST);
  if (not_need_compile) {
    (void)ge::AttrUtils::SetBool(op_desc_ptr, ge::ATTR_NEED_COMPILE, true);
    FE_LOGD("We skip the pre-compile of op %s.", node->GetName().c_str());
    return false;
  }

  if (!CheckScopeNodeMap(node)) {
    FE_LOGD("Op[name:%s] is get CheckScopeNodeMap failed.", node->GetName().c_str());
    return false;
  }
  return true;
}

void GetRePreCompileSwitch(ge::ComputeGraph &graph, string &session_graph_id, bool &need_precompile_graph) {
  if (!ge::AttrUtils::GetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
    FE_LOGW("Get session_graph_id failed");
  }

  if (!ge::AttrUtils::GetBool(graph, NEED_RE_PRECOMPILE, need_precompile_graph)) {
    FE_LOGD("Failed to get need_re_precompile attribute from graph.");
  }
  FE_LOGD("The need_re_precompile flag %d.", need_precompile_graph);
}

Status OpCompiler::SetPreCompParameter(
    const ge::NodePtr &node, const FEOpsStoreInfo &op_store_info, const string &session_graph_id, OpImplType imply_type,
    std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> &pre_comp_map) {
  const std::string &imply_type_str = op_store_info.fe_ops_store_name;
  OpStoreAdapterPtr op_store_adapter = nullptr;
  FE_CHECK_NOTNULL(op_store_adapter_manager_ptr_);
  Status status = op_store_adapter_manager_ptr_->GetOpStoreAdapter(imply_type, op_store_adapter);
  if (status != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetPreCompPara] Failed to get op store adapter, imply_type [%d].",
                    imply_type);
    return FAILED;
  }

  OpKernelInfoPtr op_kernel_info_ptr =
      OpsKernelManager::Instance(engine_name_).GetOpKernelInfoByOpType(imply_type_str, node->GetType());
  if (op_kernel_info_ptr == nullptr) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][SetPreCompPara] GetOpKernelInfo failed in PreCompileOp. OpType [%s] \
                    StoreType [%s]",
        node->GetType().c_str(), imply_type_str.c_str());
    return FAILED;
  }

  if (SetMemoryTypeForOutput(node, op_kernel_info_ptr) != SUCCESS) {
    return FAILED;
  }

  bool is_custom_op = false;
  (void)ge::AttrUtils::GetBool(node->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, is_custom_op);
  std::string op_dsl_file_path;
  bool ret_status = (is_custom_op && op_kernel_info_ptr != nullptr && !op_kernel_info_ptr->GetOpImpPath().empty());
  if (ret_status) {
    op_dsl_file_path = op_kernel_info_ptr->GetOpImpPath();
  } else {
    op_dsl_file_path = op_store_info.op_impl_file_path;
  }

  PreCompileNodePara pre_comp_node_para = {node.get(), op_kernel_info_ptr, imply_type_str, op_dsl_file_path,
                                           session_graph_id};
  if (pre_comp_map.find(op_store_adapter) == pre_comp_map.end()) {
    vector<PreCompileNodePara> pre_comp_node_para_vec;
    pre_comp_node_para_vec.push_back(pre_comp_node_para);
    pre_comp_map.emplace(make_pair(op_store_adapter, pre_comp_node_para_vec));
  } else {
    pre_comp_map[op_store_adapter].push_back(pre_comp_node_para);
  }
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   precompile tbe op
 *  @param   [in|out] graph  compute graph
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::PreCompileOp(ge::ComputeGraph &graph) {
  string session_graph_id;

  bool need_precompile_graph = false;
  GetRePreCompileSwitch(graph, session_graph_id, need_precompile_graph);
  std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> pre_comp_map;
  for (auto &node : graph.GetDirectNode()) {
    FE_CHECK(node == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] node is nullptr."), return FAILED);
    string op_type = node->GetType();
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    FE_CHECK(op_desc_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] opDescPtr is nullptr."),
             return FAILED);

    // LXfusion after slice, not precompile COMPIED_FUSION_OP
    bool no_need_compile = false;
    (void)ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_IS_COMPIED_FUSION_OP, no_need_compile);
    if (no_need_compile) {
      FE_LOGD("Op[name:%s, type:%s] not need optimize fused graph.", node->GetName().c_str(), node->GetType().c_str());
      continue;
    }

    if (!IsNeedPreCompile(node, op_desc_ptr, need_precompile_graph)) {
      FE_LOGD("Op[name:%s, type:%s] not need precompile.", op_desc_ptr->GetName().c_str(), op_type.c_str());
      continue;
    }

    // get op imply type
    int tmp_imply_type = 0;
    if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, tmp_imply_type)) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] get imply type failed, op[%s, optype [%s], imply_type [%d]].",
                      op_desc_ptr->GetName().c_str(), op_type.c_str(), tmp_imply_type);
      return OP_COMPILER_CHECK_FALSE_FAILED;
    }

    // get op kernel info store name
    OpImplType imply_type = static_cast<OpImplType>(tmp_imply_type);
    FEOpsStoreInfo op_store_info;
    if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(imply_type, op_store_info) != SUCCESS) {
      FE_LOGW("Engine[%s] failed to get op store info by impl_type[%d].", engine_name_.c_str(), imply_type);
      return SUCCESS;
    }

    if (op_store_info.need_pre_compile) {
      // Prepare precompile parameter
      if (SetPreCompParameter(node, op_store_info, session_graph_id, imply_type, pre_comp_map) != SUCCESS) {
        return FAILED;
      }
    }
  }

  // Do Precompile
  for (auto &comp_para : pre_comp_map) {
    OpStoreAdapterPtr op_store_adapter = comp_para.first;
    if (op_store_adapter->PreCompileOp(comp_para.second) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to pre-compile graph [%s]", graph.GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OpCompiler::PreCompileThreadOpHelper(const ge::NodePtr &node, const ge::OpDescPtr &op_desc_ptr,
                                            const ge::OpDescPtr &old_op_desc, const string &session_graph_id,
                                            const ge::ComputeGraph& graph) {
   // get op imply type
  int tmp_imply_type = 0;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, tmp_imply_type)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] get imply type failed, op[%s, optype [%s], imply_type [%d]].",
                    op_desc_ptr->GetName().c_str(), node->GetType().c_str(), tmp_imply_type);
    return OP_COMPILER_CHECK_FALSE_FAILED;
  }

  // get op kernel info store name
  OpImplType imply_type = static_cast<OpImplType>(tmp_imply_type);
  FEOpsStoreInfo op_store_info;
  if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(imply_type, op_store_info) != SUCCESS) {
    FE_LOGW("Engine[%s] failed to get op store info by impl_type[%d].", engine_name_.c_str(), imply_type);
    return SUCCESS;
  }

  std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> pre_comp_map;
  if (op_store_info.need_pre_compile) {
    // Prepare precompile parameter
    FE_LOGD("Op[name:%s, type:%s] start to set pre compile parameter.", op_desc_ptr->GetName().c_str(),
            node->GetType().c_str());
    if (SetPreCompParameter(node, op_store_info, session_graph_id, imply_type, pre_comp_map) != SUCCESS) {
      return FAILED;
    }
  }

  // Do Precompile
  for (auto &comp_para : pre_comp_map) {
    OpStoreAdapterPtr op_store_adapter = comp_para.first;
    if (op_store_adapter->PreCompileOp(comp_para.second) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to pre-compile graph [%s]", graph.GetName().c_str());
      return FAILED;
    }

    // set op_pattern
    for (auto &node_para : comp_para.second) {
      ge::Node *node_para_node = node_para.node;
      string op_pattern;
      ge::AttrUtils::GetStr(node_para_node->GetOpDesc(), node_para_node->GetName() + "_pattern", op_pattern);
      ge::AttrUtils::SetStr(old_op_desc, node_para_node->GetName() + "_pattern", op_pattern);
    }
  }
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   precompile tbe thread op
 *  @param   [in|out] graph  compute graph
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::PreCompileThreadOp(ge::ComputeGraph &graph, bool &sgt_flag) {
  string session_graph_id;
  bool need_precompile_graph = false;
  GetRePreCompileSwitch(graph, session_graph_id, need_precompile_graph);
  for (auto &node : graph.GetDirectNode()) {
    FE_CHECK(node == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] node is nullptr."), return FAILED);
    string op_type = node->GetType();
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    FE_CHECK(op_desc_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompOp] opDescPtr is nullptr."),
             return FAILED);

    if (!IsNeedPreCompile(node, op_desc_ptr, need_precompile_graph)) {
      FE_LOGD("Op[name:%s, type:%s] not need precompile.", op_desc_ptr->GetName().c_str(), op_type.c_str());
      continue;
    }
    uint32_t thread_scope_id = 0;
    uint32_t thread_mode = 1;
    ge::AttrUtils::GetInt(op_desc_ptr, kThreadScopeId, thread_scope_id);
    ge::AttrUtils::GetInt(op_desc_ptr, kThreadMode, thread_mode);
    int flag = (thread_scope_id == 0 || !thread_mode);
    if (flag) {
      continue;
    }
    sgt_flag = true;
    FE_LOGD("start to do pre compile thread op, sgt_flag: %d", sgt_flag);
    ge::OpDescPtr old_op_desc = ge::AttrUtils::CopyOpDesc(op_desc_ptr);
    string op_name = op_desc_ptr->GetName();

    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = op_desc_ptr->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
    FE_CHECK(slice_info_ptr == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompThreadOp] slice_info_ptr is nullptr."), return FAILED);

    auto slice_size = slice_info_ptr->input_tensor_slice.size();
    FE_LOGD("slice_size: %zu", slice_size);
    for (size_t i = 0; i < slice_size; i++) {
      if (i != 0 && i != (slice_size - 1)) {
        continue;
      }
      op_desc_ptr->SetName(op_name + "_thread_" + to_string(i));
      auto input_tensors = op_desc_ptr->GetAllInputsDescPtr();
      for (size_t j = 0; j < input_tensors.size(); j++) {
        auto tensor = input_tensors.at(j);
        ge::GeShape slice_shape;
        vector<int64_t> dims_shape;
        for (auto &range : slice_info_ptr->input_tensor_slice[i][j]) {
          dims_shape.emplace_back(range.higher - range.lower);
        }
        slice_shape = ge::GeShape(dims_shape);
        tensor->SetShape(slice_shape);

        ge::GeShape slice_origin_shape = ge::GeShape(slice_info_ptr->ori_input_tensor_shape[i][j]);
        tensor->SetOriginShape(slice_origin_shape);
        FE_LOGD(
            "PreCompileThreadOp, optype:%s, opname:%s, set thread %zu's for input tensor %s, tensor index %zu. slice \
                shape: %s, slice_origin_shape: %s.",
            node->GetType().c_str(), node->GetName().c_str(), i, tensor->GetName().c_str(), j,
            StringUtils::IntegerVecToString(slice_shape.GetDims()).c_str(),
            StringUtils::IntegerVecToString(slice_origin_shape.GetDims()).c_str());
      }
      auto output_tensors = op_desc_ptr->GetAllOutputsDescPtr();
      for (size_t j = 0; j < output_tensors.size(); j++) {
        auto tensor = output_tensors.at(j);
        ge::GeShape slice_shape;
        vector<int64_t> dims_shape;
        vector<vector<int64_t>> slice_range_vec;
        for (auto &range : slice_info_ptr->output_tensor_slice[i][j]) {
          dims_shape.emplace_back(range.higher - range.lower);
          vector<int64_t> slice_range;
          slice_range.emplace_back(range.lower);
          slice_range.emplace_back(range.higher - 1);
          slice_range_vec.emplace_back(slice_range);
        }
        slice_shape = ge::GeShape(dims_shape);
        tensor->SetShape(slice_shape);
        ge::AttrUtils::SetListListInt(tensor, ge::ATTR_NAME_DATA_SLICE, slice_range_vec);

        ge::GeShape slice_origin_shape;
        vector<int64_t> dims_origin_shape;
        for (auto &shape : slice_info_ptr->ori_output_tensor_shape[i][j]) {
          dims_origin_shape.emplace_back(shape);
        }
        slice_origin_shape = ge::GeShape(dims_origin_shape);
        tensor->SetOriginShape(slice_origin_shape);
        FE_LOGD(
            "PreCompileThreadOp, optype:%s, opname:%s, set thread %zu's for output tensor %s, tensor index %zu. \
                slice shape: %s, slice_origin_shape: %s.",
            node->GetType().c_str(), node->GetName().c_str(), i, tensor->GetName().c_str(), j,
            StringUtils::IntegerVecToString(slice_shape.GetDims()).c_str(),
            StringUtils::IntegerVecToString(slice_origin_shape.GetDims()).c_str());
      }

      ge::graphStatus ret = op_desc_ptr->InferDataSlice();
      vector<int64_t> input_size_;
      ge::AttrUtils::GetListInt(op_desc_ptr, "input_size", input_size_);
      FE_LOGD("After inferDataSlice, ret: %d, op[name:%s, type:%s] input_size_: %s.", ret,
              op_desc_ptr->GetName().c_str(), op_type.c_str(), StringUtils::IntegerVecToString(input_size_).c_str());

      Status status = PreCompileThreadOpHelper(node, op_desc_ptr, old_op_desc, session_graph_id, graph);
      if (status != SUCCESS) {
        return status;
      }
    }
    node->UpdateOpDesc(old_op_desc);
  }

  return SUCCESS;
}

Status OpCompiler::SetMemoryTypeForOutput(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr) const {
  auto op_desc = node->GetOpDesc();
  auto op_type = op_desc->GetType();
  auto op_name = op_desc->GetName();

  IndexNameMap output_index_map;
  Status res = GetOutputNameMap(*(op_desc.get()), op_kernel_info_ptr, output_index_map);
  if (res != SUCCESS) {
    return res;
  }

  size_t out_data_anchors_size = node->GetAllOutDataAnchors().size();
  for (size_t i = 0; i != out_data_anchors_size; ++i) {
    auto out_anchor = node->GetOutDataAnchor(i);
    size_t peer_in_data_nodes = out_anchor->GetPeerInDataNodesSize();
    if (peer_in_data_nodes != 0) {
      continue;
    }

    std::map<uint32_t, std::string>::const_iterator tensor_iter = output_index_map.find(i);
    if (tensor_iter == output_index_map.end()) {
      FE_LOGW("Node[type=%s,name=%s]: the output %zu is not found in the ops store.", op_type.c_str(), op_name.c_str(),
              i);
      continue;
    }

    InputOrOutputInfoPtr out_info = nullptr;
    res = op_kernel_info_ptr->GetTensorInfoByName(false, tensor_iter->second, out_info);
    if (res != SUCCESS) {
      FE_LOGW("Node[type=%s,name=%s]: the output name %s is not found in the ops store.", op_type.c_str(),
              op_name.c_str(), tensor_iter->second.c_str());
      continue;
    }
    OpParamType param_type = out_info->GetParamType();
    if (param_type == OpParamType::OPTIONAL) {
      FE_LOGI("Node[type=%s,name=%s]: success to set the attribute %s to %d for the output %s.", op_type.c_str(),
              op_name.c_str(), ge::ATTR_NAME_MEMORY_SIZE_CALC_TYPE.c_str(),
              static_cast<int32_t>(ge::MemorySizeCalcType::ALWAYS_EMPTY),
              out_info->GetName().c_str());
      auto output_desc_ptr = op_desc->MutableOutputDesc(i);
      if (output_desc_ptr == nullptr) {
        continue;
      }
      (void)ge::AttrUtils::SetInt(output_desc_ptr, ge::ATTR_NAME_MEMORY_SIZE_CALC_TYPE,
                                  static_cast<int64_t>(ge::MemorySizeCalcType::ALWAYS_EMPTY));
    }
  }
  return SUCCESS;
}

Status OpCompiler::ParseTvmJsonToSetAttr(const ge::NodePtr& node, const ge::OpDescPtr op_desc_ptr,
                                         const std::string &json_file_path) const {
  // package tvm json info
  TbeJsonFileParsePtr parse_ptr = nullptr;
  FE_MAKE_SHARED(parse_ptr = std::make_shared<TbeJsonFileParse>(*node), return fe::OP_COMPILER_MAKE_SHARED_FAILED);
  FE_CHECK(parse_ptr == nullptr, FE_LOGE("parsePtr is nullptr."), return FAILED);
  if (parse_ptr->PackageTvmJsonInfo(json_file_path, "") != SUCCESS) {
    FE_LOGE("PackageTvmJsonInfo failed, op[%s, optype [%s]].", op_desc_ptr->GetName().c_str(),
            op_desc_ptr->GetType().c_str());

    return FAILED;
  }
  return SUCCESS;
}

Status OpCompiler::ParseSgtTvmJsonToSetAttr(const ge::NodePtr &node, const ge::OpDescPtr op_desc_ptr,
                                            const std::string &json_file_path) const {
  // package tvm json info
  FE_LOGI("Parse sgt json file %s for node %s", json_file_path.c_str(), op_desc_ptr->GetName().c_str());
  TbeSgtJsonFileParsePtr parse_ptr = nullptr;
  FE_MAKE_SHARED(parse_ptr = std::make_shared<TbeSgtJsonFileParse>(*node), return fe::OP_COMPILER_MAKE_SHARED_FAILED);
  FE_CHECK(parse_ptr == nullptr, FE_LOGE("parsePtr is nullptr."), return FAILED);
  vector<std::string> json_path_vec = fe::StringUtils::Split(json_file_path, ';');
  if (parse_ptr->PackageTvmJsonInfo(json_path_vec, json_path_vec) != SUCCESS) {
    FE_LOGE("PackageTvmJsonInfo failed, op[%s, optype [%s]].", op_desc_ptr->GetName().c_str(),
            op_desc_ptr->GetType().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status OpCompiler::ParseJsonAndUpdateOp(const ge::NodePtr &node, const ge::OpDescPtr op_desc_ptr,
                                        const ScopeJsonMap::const_iterator &json_iter) {
  std::string json_file_path = json_iter->second;
  domi::TEBinInfo info;
  info.json_file_path = json_file_path;
  // package tvm json info
  TbeJsonFileParsePtr parse_ptr = nullptr;
  FE_MAKE_SHARED(parse_ptr = std::make_shared<TbeJsonFileParse>(*node), return fe::OP_COMPILER_MAKE_SHARED_FAILED);
  FE_CHECK(parse_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][ParseJsUpdOp] parsePtr is nullptr."),
           return FAILED);

  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  if (!slice_info_ptr || (!slice_info_ptr->thread_mode && json_iter->second.find(";") == string::npos)
      || !OpIsAutoThread(slice_info_ptr)) {
    if (ParseTvmJsonToSetAttr(node, op_desc_ptr, json_iter->second) == FAILED) {
      return FAILED;
    }

    if (UpdateCompressOp(node) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ParseJsUpdOp] Fail to compress conv node[%s].", node->GetName().c_str());
      return FAILED;
    }

    if (SetCompressWeightAttr(node) != SUCCESS) {
      FE_LOGW("Fail to set compress weight attribute on node[%s].", node->GetName().c_str());
    }
  } else {
    if (ParseSgtTvmJsonToSetAttr(node, op_desc_ptr, json_iter->second) == FAILED) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OpCompiler::ParseJsonAndCompressOp(const ge::ComputeGraph& graph, map<int64_t, std::string>& scope_json_map,
                                          const std::vector<ge::NodePtr>& nodes_be_compiled) {
  map<int64_t, bool> is_json_parsed;
  bool is_after_merge_step = Configuration::Instance(engine_name_).GetBuildStep() == ge::BUILD_STEP_AFTER_MERGE;
  // get info from json file and set op descriptor
  auto node_size = nodes_be_compiled.size();
  for (size_t index = 0; index < node_size; ++index) {
    ge::NodePtr node = nodes_be_compiled.at(index);
    FE_CHECK(node == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][Compile][ParseJsCompsOp] node(index %zu) is nullptr.", index),
             return FAILED);

    auto op_desc_ptr = node->GetOpDesc();
    FE_CHECK(op_desc_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][ParseJsCompsOp] opDescPtr is nullptr."),
             return FAILED);

    int64_t scope_id = 0;
    if (!GetFusionScopeAttr(op_desc_ptr, scope_id)) {
      continue;
    }

    // find the json file path according to scope id
    ScopeJsonMap::const_iterator json_iter = scope_json_map.find(scope_id);
    if (json_iter == scope_json_map.end()) {
      bool need_precompile_node = false;
      bool buff_fusion_status =
          ((ge::AttrUtils::GetBool(op_desc_ptr, NEED_RE_PRECOMPILE, need_precompile_node) && need_precompile_node));
      if (buff_fusion_status) {
        FE_LOGD("scopeId [%ld], op[%s, optype [%s]].", scope_id, op_desc_ptr->GetName().c_str(),
                op_desc_ptr->GetType().c_str());
        continue;
      } else if (is_after_merge_step) {
        FE_LOGW("Node[%s, %s] has scope id[%ld], but json file path is not found during after merge step.",
                op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), scope_id);
        continue;
      } else {
        REPORT_FE_ERROR("[SubGraphOpt][Compile][ParseJson] scopeId [%ld], op [%s], type[%s], json file is not found.",
                        scope_id, op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
        return OP_COMPILER_CHECK_FALSE_FAILED;
      }
    }

    if (is_json_parsed.count(scope_id) && scope_id >= 0) {
      continue;
    }

    Status ret = ParseJsonAndUpdateOp(node, op_desc_ptr, json_iter);
    if (ret != SUCCESS) {
      return ret;
    }

    is_json_parsed[scope_id] = true;
  }
  return SUCCESS;
}

bool OpCompiler::StopCompileOpInTuningAndAfterBuilderMode() {
  std::string build_mode_value = Configuration::Instance(engine_name_).GetBuildMode();
  std::string step_mode_value = Configuration::Instance(engine_name_).GetBuildStep();
  bool no_need_to_wait_task_finish =
      (build_mode_value == ge::BUILD_MODE_TUNING &&
       (step_mode_value == ge::BUILD_STEP_AFTER_BUILDER || step_mode_value == ge::BUILD_STEP_AFTER_BUILDER_SUB ||
        step_mode_value == ge::BUILD_STEP_BEFORE_UB_MATCH));
  if (no_need_to_wait_task_finish) {
    FE_LOGD("No need to wait task finish if build_mode is [%s] and step is [%s].", build_mode_value.c_str(),
            step_mode_value.c_str());
    return true;
  }
  return false;
}

Status OpCompiler::CompileOpOnly(const ge::ComputeGraph &graph, CompileInfoParam &compile_info) const {
  if (compile_info.fusion_nodes_map.empty()) {
    FE_LOGI("No node in graph need to compile.");
    return SUCCESS;
  }

  OpStoreAdapterPtr op_store_adapter = nullptr;
  FE_CHECK_NOTNULL(op_store_adapter_manager_ptr_);
  Status status = op_store_adapter_manager_ptr_->GetOpStoreAdapter(EN_IMPL_HW_TBE, op_store_adapter);

  if (status != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompOpOnly] Failed to get op store adapter by impl_type[%d].",
                    EN_IMPL_HW_TBE);
    return OP_COMPILER_CHECK_FALSE_FAILED;
  }

  // get scope id and json file path map
  Status ret = op_store_adapter->CompileOp(compile_info);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompOpOnly] CompileOp failed, graph[%s].", graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status OpCompiler::GetFusionScope(const ge::ComputeGraph& graph,
                                  const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                                  ScopeNodeIdMap &fusion_nodes_map, std::vector<ge::NodePtr> &nodes_be_compiled) {
  std::string graph_name = graph.GetName();
  auto all_nodes = graph.GetDirectNode();
  for (auto &node : all_nodes) {
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), "_graph_name", graph_name);
  }

  if (buff_fus_rollback_nodes.empty()) {
    if (GetScopeNodeMap(graph, fusion_nodes_map) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][GetFusScope] GetScopeNodeMap failed, graph [%s].",
                      graph.GetName().c_str());
      return FAILED;
    }
    for (auto &node : all_nodes) {
      nodes_be_compiled.emplace_back(node);
    }
  } else {
    if (GetScopeNodeMap(graph, buff_fus_rollback_nodes, fusion_nodes_map) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][GetFusScope] GetRollbackScopeNodeMap failed, graph [%s].",
                      graph.GetName().c_str());
      return FAILED;
    }
    nodes_be_compiled = buff_fus_rollback_nodes;
  }
  return SUCCESS;
}

Status OpCompiler::CheckCompiledFusionGraph(const ge::ComputeGraph& graph) const {
  auto all_nodes = graph.GetDirectNode();
  for (const auto &node : all_nodes) {
    bool no_need_compile = false;
    (void)ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_IS_COMPIED_FUSION_OP, no_need_compile);
    if (no_need_compile) {
      FE_LOGD("Find op[name:%s, type:%s] is compiled fusion op.", node->GetName().c_str(), node->GetType().c_str());
      return true;
    }
  }
  return false;
}

Status OpCompiler::GetMixComFusionScope(const ge::ComputeGraph& graph,
                                        const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                                        ScopeNodeIdMap &fusion_nodes_map, std::vector<ge::NodePtr> &nodes_be_compiled) {
  FE_LOGD("Graph has compiled fusion node, filter compiled nodes and calculate scope id when get need compile nodes.");
  std::string graph_name = graph.GetName();
  auto all_nodes = graph.GetDirectNode();
  for (const auto &node : all_nodes) {
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), "_graph_name", graph_name);
    bool no_need_compile = false;
    (void)ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_IS_COMPIED_FUSION_OP, no_need_compile);
    if (no_need_compile) {
      FE_LOGD("Op[name:%s, type:%s] not need optimize fused graph.", node->GetName().c_str(), node->GetType().c_str());
      continue;
    }
    nodes_be_compiled.emplace_back(node);
  }

  FE_LOGD("Size of nodes_be_compiled is %zu.", nodes_be_compiled.size());

  if (GetScopeNodeMap(graph, nodes_be_compiled, fusion_nodes_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][GetMixComFusionScope] GetScopeNodeMap failed, graph [%s].",
                    graph.GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   compile tbe op
 *  @param   [in|out] graph  compute graph
 *  @return  SUCCESS or FAILED
 */
Status OpCompiler::CompileOp(const ge::ComputeGraph& graph, std::vector<ge::NodePtr>& buff_fus_compile_failed_nodes,
                             const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                             const std::vector<ge::NodePtr>& buff_fus_to_del_nodes) {
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;

  // LXfusion after slice, not compile COMPIED_FUSION_OP
  if (CheckCompiledFusionGraph(graph)) {
    GetMixComFusionScope(graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  } else {
    GetFusionScope(graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  }

  if (fusion_nodes_map.empty()) {
    FE_LOGI("No node in graph need to compile.");
    return SUCCESS;
  }

  OpStoreAdapterPtr op_store_adapter = nullptr;
  FE_CHECK_NOTNULL(op_store_adapter_manager_ptr_);
  Status status = op_store_adapter_manager_ptr_->GetOpStoreAdapter(EN_IMPL_HW_TBE, op_store_adapter);

  if (status != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompOp] Failed to get op store adapter by impl_type[%d].", EN_IMPL_HW_TBE);
    return OP_COMPILER_CHECK_FALSE_FAILED;
  }

  // get scope id and json file path map
  ScopeJsonMap scope_json_map;
  Status ret = op_store_adapter->CompileOp(fusion_nodes_map, scope_json_map, buff_fus_compile_failed_nodes,
                                           buff_fus_to_del_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompOp] Failed to compile for graph[%s].", graph.GetName().c_str());
    return ret;
  }
  if (StopCompileOpInTuningAndAfterBuilderMode()) {
    FE_LOGD("No need to wait task finish.");
    return SUCCESS;
  }
  ret = ParseJsonAndCompressOp(graph, scope_json_map, nodes_be_compiled);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompOp] AfterCompileOp failed, graph[%s].", graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status GetParam(const ge::OpDescPtr& op_desc, int64_t& fm_h, int64_t& fm_w, int64_t& weight_h, int64_t& weight_w,
                int64_t& cin, int64_t& cout, uint64_t& cube_k) {
  auto fm_in_tensor_desc = op_desc->GetInputDescPtr(0);
  FE_CHECK_NOTNULL(fm_in_tensor_desc);
  auto weight_tensor_desc = op_desc->GetInputDescPtr(1);
  FE_CHECK_NOTNULL(weight_tensor_desc);
  auto fm_out_tensor_desc = op_desc->GetOutputDescPtr(0);
  FE_CHECK_NOTNULL(fm_out_tensor_desc);
  if (!GetDimValueByFormatAndShape(fm_out_tensor_desc->GetOriginFormat(), fm_out_tensor_desc->GetOriginShape(), "H",
                                   fm_h)) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][WeightCmprs][node %s]Failed to get Dim H of output feature map with format %u",
        op_desc->GetName().c_str(), fm_out_tensor_desc->GetOriginFormat());
    return FAILED;
  }
  if (!GetDimValueByFormatAndShape(fm_out_tensor_desc->GetOriginFormat(), fm_out_tensor_desc->GetOriginShape(), "W",
                                   fm_w)) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][WeightCmprs][node %s]Failed to get Dim W of output feature map with format %u",
        op_desc->GetName().c_str(), fm_out_tensor_desc->GetOriginFormat());
    return FAILED;
  }
  if (!GetDimValueByFormatAndShape(weight_tensor_desc->GetOriginFormat(), weight_tensor_desc->GetOriginShape(), "H",
                                   weight_h)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][WeightCmprs][node %s]Failed to get Dim H of weight with format %u",
                    op_desc->GetName().c_str(), weight_tensor_desc->GetOriginFormat());
    return FAILED;
  }
  if (!GetDimValueByFormatAndShape(weight_tensor_desc->GetOriginFormat(), weight_tensor_desc->GetOriginShape(), "W",
                                   weight_w)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][WeightCmprs][node %s]Failed to get Dim W of weight with format %u",
                    op_desc->GetName().c_str(), weight_tensor_desc->GetOriginFormat());
    return FAILED;
  }
  if (!GetDimValueByFormatAndShape(fm_in_tensor_desc->GetOriginFormat(), fm_in_tensor_desc->GetOriginShape(), "C",
                                   cin)) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][WeightCmprs][node %s] Failed to get Dim C of input feature map with format %u",
        op_desc->GetName().c_str(), fm_in_tensor_desc->GetOriginFormat());
    return FAILED;
  }
  if (!GetDimValueByFormatAndShape(fm_out_tensor_desc->GetOriginFormat(), fm_out_tensor_desc->GetOriginShape(), "C",
                                   cout)) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][WeightCmprs][node %s] Failed to get Dim C of output feature map with format %u",
        op_desc->GetName().c_str(), fm_out_tensor_desc->GetOriginFormat());
    return FAILED;
  }

  // if data type is int8, cube_k should be 32
  if (fm_in_tensor_desc->GetDataType() == ge::DT_INT8) {
    cube_k = CUBE_K_OF_INT8;
  }
  return SUCCESS;
}

Status OpCompiler::SetCompressWeightAttr(ge::NodePtr node) {
  // if the node is FC, just set the attr.
  // Because the weight of FC is always very large.
  if (node->GetType() == FCOP) {
    (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_FE_WEIGHT_COMPRESS, true);
    FE_LOGI("Node[%s, %s] has been set _fe_weight_compress attribute.", node->GetName().c_str(),
            node->GetType().c_str());
    return SUCCESS;
  }
  if (node->GetType() != CONV2D && node->GetType() != MATMULV2OP) {
    return SUCCESS;
  }

  // check whether is dynamic shape
  if (IsFeSupportedDynamicOp(*(node->GetOpDesc()))) {
    FE_LOGD("The shape of node[%s, %s] is dynamic, no need to calculate weight compress parameter.",
            node->GetName().c_str(), node->GetType().c_str());
    return SUCCESS;
  }

  FE_LOGD("SetCompressWeightAttr on node[%s, %s] begin", node->GetName().c_str(), node->GetType().c_str());
  if (node->GetType() == MATMULV2OP) {
    ge::ConstGeTensorDescPtr weight_tensor_desc_ptr = node->GetOpDesc()->GetInputDescPtr(1);
    if (weight_tensor_desc_ptr == nullptr) {
      FE_LOGW("The weight desc of node[%s] is empty.", node->GetName().c_str());
      return SUCCESS;
    }
    int64_t element_cnt = 1;
    ge::GeShape weight_origin_shape = weight_tensor_desc_ptr->GetOriginShape();
    ge::DataType weight_origin_dtype = weight_tensor_desc_ptr->GetOriginDataType();
    if (weight_origin_dtype == ge::DT_UNDEFINED) {
      FE_LOGW("Origin data type of weight desc of node[%s] is invalid.", node->GetName().c_str());
      return FAILED;
    }
    TensorComputeUtil::GetElementCountByMultiply(weight_origin_shape, element_cnt);
    int64_t matmul_weight_size = -1;
    int32_t output_real_calc_flag = 1;

    if (TensorComputeUtil::GetTensorSizeByDataType(element_cnt, weight_origin_dtype, matmul_weight_size,
                                                   output_real_calc_flag) != SUCCESS) {
      FE_LOGW("Get tensor size failed! element_cnt[%ld], ge::DataType[%d].", element_cnt, weight_origin_dtype);
      return FAILED;
    }
    FE_LOGI("The weight size of node[%s] is %ld.", node->GetName().c_str(), matmul_weight_size);
    if (matmul_weight_size > WEIGHT_SIZE_THRESHOLD) {
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_FE_WEIGHT_COMPRESS, true);
      FE_LOGI("Node[%s, %s] has been set _fe_weight_compress attr.", node->GetName().c_str(), node->GetType().c_str());
    }
    return SUCCESS;
  }

  // then begin to deal Conv2D
  string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatformInfo platform_info;
  OptionalInfo opti_compilation_info;
  if (PlatformInfoManager::Instance().GetPlatformInfo(soc_version, platform_info, opti_compilation_info) != SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return FAILED;
  }
  uint64_t cube_k = platform_info.ai_core_spec.cube_k_size;
  uint64_t cube_m = platform_info.ai_core_spec.cube_m_size;
  uint64_t cube_n = platform_info.ai_core_spec.cube_n_size;
  double cube_freq = platform_info.ai_core_spec.cube_freq;
  double ddr_band_width = cube_freq * platform_info.ai_core_memory_rates.ddr_rate;

  ge::OpDescPtr op_desc = node->GetOpDesc();

  int64_t fm_h = 0;
  int64_t fm_w = 0;
  int64_t weight_h = 0;
  int64_t weight_w = 0;
  int64_t cin = 0;
  int64_t cout = 0;
  if (GetParam(op_desc, fm_h, fm_w, weight_h, weight_w, cin, cout, cube_k) != SUCCESS) {
    return FAILED;
  }

  FE_INT64_MULCHECK(fm_h, fm_w);
  int64_t fm_hw = fm_h * fm_w;
  FE_INT64_MULCHECK(weight_h, weight_w);
  int64_t weight_hw_cin = weight_h * weight_w;
  FE_INT64_MULCHECK(weight_hw_cin, cin);
  weight_hw_cin = weight_hw_cin * cin;

  FE_DOUBLE_ZEROCHECK(cube_m);
  FE_DOUBLE_ZEROCHECK(cube_k);
  double tmp1 = std::ceil(static_cast<double>(fm_hw) / cube_m);
  double tmp2 = std::ceil(static_cast<double>(weight_hw_cin) / cube_k);
  FE_DOUBLE_MULCHECK(tmp1, tmp2);

  double cube_cycle = tmp1 * tmp2;
  FE_DOUBLE_ZEROCHECK(cube_n);
  double tmp3 = std::ceil(static_cast<double>(cout) / cube_n);
  FE_DOUBLE_MULCHECK(cube_cycle, tmp3);
  cube_cycle *= tmp3;

  FE_INT64_MULCHECK(weight_hw_cin, cout);
  int64_t weight_size = weight_hw_cin * cout;

  FE_DOUBLE_ZEROCHECK(ddr_band_width);
  FE_DOUBLE_MULCHECK(cube_freq, weight_size);
  double weight_cycle = weight_size * cube_freq / ddr_band_width;

  if (weight_size > WEIGHT_SIZE_THRESHOLD) {
    FE_DOUBLE_ZEROCHECK(cube_cycle);
    double ret = weight_cycle / cube_cycle;
    FE_LOGD("The result for weight compress of [%s] is %f.", node->GetName().c_str(), ret);
    if (ret >= WEIGHT_CHECK_THRESHOLD) {
      (void)ge::AttrUtils::SetBool(op_desc, ATTR_NAME_FE_WEIGHT_COMPRESS, true);
      FE_LOGI("Node[%s, %s] has been set _fe_weight_compress attr. Ret = %f.", node->GetName().c_str(),
              node->GetType().c_str(), ret);
    }
  }
  return SUCCESS;
}

Status OpCompiler::UpdateCompressOp(ge::NodePtr node) const {
  // 1 Only deal conv and FC node
  if (node->GetType() != CONV2D_COMPRESS && node->GetType() != FC_COMPRESS && node->GetType() != MATMULV2_COMPRESS) {
    return SUCCESS;
  }
  ge::OpDescPtr conv_desc = node->GetOpDesc();

  // 2 Get the compress_parameters from conv compress node, the size should be 8
  std::vector<int64_t> compress_param_vec;
  if (!ge::AttrUtils::GetListInt(conv_desc, ATTR_NAME_COMPRESS_PARAMETERS, compress_param_vec)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] Fail to get attr compress_weight for node[%s].",
                    conv_desc->GetName().c_str());
    return FAILED;
  }
  // the size of compress_parameters in json should be 8 or 9
  size_t compress_param_size = compress_param_vec.size();
  if (compress_param_size != COMPRESS_PARAMETER_SIZE && compress_param_size != MULTI_CORE_COMPRESS_PARAMETER_SIZE) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] The size of compress_weight of [%s] should be 8 or 9.",
                    conv_desc->GetName().c_str());
    return FAILED;
  }

  // 3. check whether the shape of compress index tensor has already been updated.
  std::vector<int64_t> compress_index_desc_dims = {compress_param_vec[COMPRESS_PARAMETER_INFOSIZE_INDEX]};
  ge::GeTensorDescPtr index_tensor_desc_ptr = conv_desc->MutableInputDesc(TENSOR_INDEX_COMPRESS_INDEX);
  FE_CHECK(index_tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] The compress index tensor of conv node[%s] is null.",
                           conv_desc->GetName().c_str()),
           return FAILED);
  if (index_tensor_desc_ptr->MutableShape().GetDims() == compress_index_desc_dims) {
    FE_LOGD("The shape of compress index of conv node[%s, %s] has already been updated.", conv_desc->GetName().c_str(),
            conv_desc->GetType().c_str());
    return SUCCESS;
  }

  // 4. find compress op
  ge::NodePtr compress_node = nullptr;
  uint32_t compress_index = COMPRESSOP_INDEX_COMPRESS_INDEX;
  ge::InDataAnchorPtr compress_index_in_anchor = node->GetInDataAnchor(TENSOR_INDEX_COMPRESS_INDEX);
  if (compress_index_in_anchor != nullptr) {
    ge::OutDataAnchorPtr compress_index_peer_out_anchor = compress_index_in_anchor->GetPeerOutAnchor();
    if (compress_index_peer_out_anchor != nullptr) {
      compress_index = static_cast<uint32_t>(compress_index_peer_out_anchor->GetIdx());
      compress_node = compress_index_peer_out_anchor->GetOwnerNode();
    }
  }
  FE_CHECK(compress_node == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] Can not find compress op from node[%s]'s input nodes",
                           conv_desc->GetName().c_str()),
           return FAILED);

  ge::OpDescPtr compress_opdesc = compress_node->GetOpDesc();
  FE_CHECK(compress_opdesc == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] The compress op desc from node[%s] is null.",
                           conv_desc->GetName().c_str()),
           return FAILED);
  // if the type of compress node is not Compress or CompressFcOp, just print warning log
  if (compress_opdesc->GetType() != COMPRESSOP && compress_opdesc->GetType() != COMPRESSFCOP) {
    FE_LOGW("The compress node[%s, %s] is neither Compress nor CompressFcOp.", compress_opdesc->GetName().c_str(),
            compress_opdesc->GetType().c_str());
  }

  // 6. update the shape of compress index of conv and compress node
  ge::GeTensorDescPtr compress_index_tensor_desc_ptr = compress_opdesc->MutableOutputDesc(compress_index);
  FE_CHECK(compress_index_tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp]The compress index tensor of compress node[%s] is null.",
                           compress_opdesc->GetName().c_str()),
           return FAILED);

  ge::GeShape index_shape(compress_index_desc_dims);
  index_tensor_desc_ptr->SetShape(index_shape);
  index_tensor_desc_ptr->SetOriginShape(index_shape);
  compress_index_tensor_desc_ptr->SetShape(index_shape);
  compress_index_tensor_desc_ptr->SetOriginShape(index_shape);

  // 7 Set compress_parameters attr on compress node
  if (!ge::AttrUtils::SetListInt(compress_opdesc, ATTR_NAME_COMPRESS_PARAMETERS, compress_param_vec)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][UpdCmprsOp] Fail to set attr compress_weight for node[%s].",
                    compress_opdesc->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

void OpCompiler::GetNodesNeedRePrcmpl(const Vistor<ge::NodePtr> &all_nodes,
                                      std::unordered_set<int64_t> &need_re_compile_scope_id,
                                      std::vector<ge::NodePtr> &nodes_be_compiled,
                                      std::vector<ge::NodePtr> &all_nodes_after_lx_fusion) {
  for (auto &node : all_nodes) {
    all_nodes_after_lx_fusion.emplace_back(node);
    bool need_re_compile = false;
    string cmp_strategy;
    (void)ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, cmp_strategy);
    if ((ge::AttrUtils::GetBool(node->GetOpDesc(), NEED_RE_PRECOMPILE, need_re_compile) && need_re_compile) ||
        !cmp_strategy.empty()) {
      int64_t scope_id = 0;
      bool has_scope_id = GetFusionScopeAttr(node->GetOpDesc(), scope_id);
      if (has_scope_id && scope_id >= 0) {
        need_re_compile_scope_id.emplace(scope_id);
        /* fusion nodes will be emplaced into nodes_be_compiled in the following step.
         * Here only count all possible scope-ids. */
        FE_LOGD("Node %s need re-pe-compile and compile. We need to find all nodes with its scope id %ld.",
                node->GetName().c_str(), scope_id);
      } else {
        nodes_be_compiled.emplace_back(node);
        FE_LOGD("Single Node %s need re-pe-compile and compile.", node->GetName().c_str());
      }
    }
  }
}

Status OpCompiler::GetFusionScope(ge::ComputeGraph &graph, ScopeNodeIdMap &fusion_nodes_map,
                                  std::vector<ge::NodePtr> &nodes_be_compiled,
                                  std::vector<ge::NodePtr> &all_nodes_after_lx_fusion) {
  std::string graph_name = graph.GetName();
  auto all_nodes = graph.GetDirectNode();
  /* Find the minimum scope id in this graph. */
  for (auto &node : all_nodes) {
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), "_graph_name", graph_name);
  }

  if (Configuration::Instance(engine_name_).GetAutoTuneMode() == TUNE_MODE_NO_TUNE) {
    /* Get all nodes which are need re-pre-compiled and store their scope id to
     * find all nodes with that scope id. */
    std::unordered_set<int64_t> need_re_compile_scope_id;
    GetNodesNeedRePrcmpl(all_nodes, need_re_compile_scope_id, nodes_be_compiled, all_nodes_after_lx_fusion);

    /* Find all nodes using special scope id. Every node uses that scope id
     * need to be re-compiled because they will be fused as one node. */
    for (auto &node : all_nodes) {
      int64_t scope_id = 0;
      bool has_scope_id = GetFusionScopeAttr(node->GetOpDesc(), scope_id);
      if (has_scope_id && scope_id >= 0) {
        if (need_re_compile_scope_id.count(scope_id) != 0) {
          FE_LOGD("node %s with scope id %ld needs to be re-compiled.", node->GetName().c_str(), scope_id);
          nodes_be_compiled.emplace_back(node);
        }
      }
    }
  } else {
    // if auto tune is on, all nodes should be re-compiled
    for (auto &node : all_nodes) {
      nodes_be_compiled.push_back(node);
      all_nodes_after_lx_fusion.push_back(node);
    }
  }

  if (GetScopeNodeMap(graph, nodes_be_compiled, fusion_nodes_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][GetFusScope] GetScopeNodeMap failed, graph [%s].", graph.GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status OpCompiler::ReCompileOpAfterLxFusion(ge::ComputeGraph &graph, CompileInfoParam &compile_info,
                                            const LxFusionOptimizeResult &opt_ret) {
  Status ret = PreCompileOp(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Lx] PreCompileOp failed after buffer fusion.");
    return ret;
  }

  /* scope_json_map is not need to be cleared. Put two rounds of compilation result together and
   * parse them once. */
  compile_info.fusion_nodes_map.clear();
  compile_info.buff_fus_to_del_nodes.clear();
  compile_info.l1_fusion_failed_nodes.clear();

  vector<ge::NodePtr> nodes_be_re_compiled;
  vector<ge::NodePtr> all_nodes;

  GetFusionScope(graph, compile_info.fusion_nodes_map, nodes_be_re_compiled, all_nodes);

  /* Using the same scope json map and we will parse json for all nodes including
   * those which are compiled before lx-fusion(and are not changed in lx-fusion) and those
   * which are changed in lx-fusion and need re-compiling. */
  ret = CompileOpOnly(graph, compile_info);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][Lx] Failed to compile after lx-fusion for graph %s with strategy %u.",
        graph.GetName().c_str(), static_cast<uint32_t>(compile_info.compile_strategy));
    return ret;
  }

  // rollback l1 fusion failed nodes
  if (!compile_info.l1_fusion_failed_nodes.empty() && opt_ret == LxFusionOptimizeResult::BOTH_FUSION_STRATEGY) {
    FE_LOGI("Fail to compile l1 fusion nodes, try to do ub fusion compile for l1 fusion failed nodes.");
    compile_info.fusion_nodes_map.clear();
    if (GetScopeNodeMap(graph, compile_info.l1_fusion_failed_nodes, compile_info.fusion_nodes_map) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Lx] Failed to get scope map for l1 fusion failed nodes of graph[%s].",
                      graph.GetName().c_str());
    }
    ret = CompileOpOnly(graph, compile_info);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Lx] Failed to compile l1 fusion failed nodes of graph[%s].",
                      graph.GetName().c_str());
      return ret;
    }
  }

  ret = ParseJsonAndCompressOp(graph, compile_info.scope_json_map, all_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Lx] Failed to parse json for graph [%s] after lx-fusion.",
                    graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}
}  // namespace fe
