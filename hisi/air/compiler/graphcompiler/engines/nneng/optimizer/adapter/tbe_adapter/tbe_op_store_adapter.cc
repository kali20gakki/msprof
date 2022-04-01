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

#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include <thread>
#include <nlohmann/json.hpp>
#include "common/configuration.h"
#include "common/fe_error_code.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_type_utils.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "common/scope_allocator.h"
#include "ge/ge_api_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/tuning_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/string_utils.h"
#include "common/util/platform_info.h"
#include "common/ffts_plus_type.h"

namespace fe {
static const std::string TBE_SO_NAME = "libte_fusion.so";

const std::string VECTOR_FP_CEILING = "ge.fpCeilingMode";
const std::string SHAPE_NOT_SUPPORT = "The shape is not support now";

static const std::map<std::string, std::string> TBE_INIT_OPTION_KEY_MAP {
    {ge::AUTO_TUNE_MODE, TBE_AUTO_TILING_MODE},
    {ge::OPTION_EXEC_DEVICE_ID, TBE_DEVICE_ID},
    {ge::SOC_VERSION, ge::SOC_VERSION},
    {ge::CORE_TYPE, ge::CORE_TYPE},
    {ge::AICORE_NUM, ge::AICORE_NUM},
    {ge::OP_SELECT_IMPL_MODE, ge::OP_SELECT_IMPL_MODE},
    {ge::OPTYPELIST_FOR_IMPLMODE, ge::OPTYPELIST_FOR_IMPLMODE},
    {ge::OP_DEBUG_LEVEL, ge::OP_DEBUG_LEVEL},
    {ge::DEBUG_DIR, ge::DEBUG_DIR},
    {ge::OP_COMPILER_CACHE_DIR, ge::OP_COMPILER_CACHE_DIR},
    {ge::OP_COMPILER_CACHE_MODE, ge::OP_COMPILER_CACHE_MODE},
    {VECTOR_FP_CEILING, VECTOR_FP_CEILING},
    {ge::MDL_BANK_PATH_FLAG, ge::MDL_BANK_PATH_FLAG},
    {ge::OP_BANK_PATH_FLAG, ge::OP_BANK_PATH_FLAG},
    {ge::PERFORMANCE_MODE, ge::PERFORMANCE_MODE}};

static const std::map<CompileStrategy, std::string> kCompileStrategyStrMap {
        {CompileStrategy::COMPILE_STRATEGY_KEEP_OPTUNE, "set by fe: keep compiling in op tune"},
        {CompileStrategy::COMPILE_STRATEGY_NO_TUNE, "NoTune"},
};

Status TbeOpStoreAdapter::SerialPreCompileOp(vector<PreCompileNodePara> &compile_para_vec) {
  for (auto &comp_para : compile_para_vec) {
    FE_CHECK(comp_para.node == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][Compile][SerialPreComOp] compPara.node is nullptr."),
             return FAILED);
    FE_LOGD("TbeOpStoreAdapter::PreCompile Op begin, node name: %s, node type %s.",
            comp_para.node->GetOpDesc()->GetName().c_str(), comp_para.node->GetOpDesc()->GetType().c_str());

    TbeOpInfoPtr tbe_op_info_ptr = PreCompSetTbeOpInfo(comp_para);
    if (tbe_op_info_ptr == nullptr) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SerialPreComOp] Set TbeOpInfo Failed.");
      return FAILED;
    }

    string op_pattern_before_buff_fus;
    bool need_precompile_node = false;
    (void)ge::AttrUtils::GetBool(comp_para.node->GetOpDesc(), NEED_RE_PRECOMPILE, need_precompile_node);
    if (need_precompile_node) {
      if (!ge::AttrUtils::GetStr(comp_para.node->GetOpDesc(), comp_para.node->GetName() + "_pattern",
                                 op_pattern_before_buff_fus)) {
        FE_LOGW("Can't get BuffFus op %s pattern before precompile.", comp_para.node->GetName().c_str());
      }
    }

    FE_CHECK(PreBuildTbeOp == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][Compile][SerialPreComOp] PreBuildTbeOp is nullptr."), return FAILED);

    FE_TIMECOST_START(PreBuild);
    // call pre-compile func, and return pattern of op, such as reduction,
    bool result = PreBuildTbeOp(*tbe_op_info_ptr, 0, 0);
    if (!result) {
      std::map<std::string, std::string> error_key_map;
      // op_name,op_type,graph_id
      error_key_map[EM_OP_NAME] = comp_para.node->GetOpDesc()->GetName();
      error_key_map[EM_OP_TYPE] = comp_para.node->GetOpDesc()->GetType();
      error_key_map[EM_GRAPH_ID] = comp_para.session_graph_id;
      LogErrorMessage(EM_PRECOMPLIE_FAILED, error_key_map);
    }

    if (!result) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SerialPreComOp] Pre-build Tbe op failed.");
      return FAILED;
    }
    FE_TIMECOST_END(PreBuild, "PreBuildTbe during FEGraphOptimizer::OptimizeFusedGraph");

    if (SetPreCompilePattern(comp_para.node->GetOpDesc(), *tbe_op_info_ptr, op_pattern_before_buff_fus) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SerialPreComOp] %s set op pattern failed.",
                      comp_para.node->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status TbeOpStoreAdapter::SetPreCompilePattern(ge::OpDescPtr op_desc, te::TbeOpInfo &op_info,
                                               const string &op_pattern_before_buff_fus) const {
  string op_pattern;
  op_info.GetPattern(op_pattern);
  if (op_pattern.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetPtn] %s's pattern is empty", op_desc->GetName().c_str());
    return FAILED;
  }

  FE_LOGD("op %s, pattern after precompile is %s, before precompile is %s.", op_desc->GetName().c_str(),
          op_pattern.c_str(), op_pattern_before_buff_fus.c_str());
  if (!op_pattern_before_buff_fus.empty() && (op_pattern_before_buff_fus != op_pattern)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetPtn] %s's pattern (%s to %s) is changed during buffer fusion.",
                    op_desc->GetName().c_str(), op_pattern_before_buff_fus.c_str(), op_pattern.c_str());
    return FAILED;
  }

  // set op pattern to op's desc
  if (!ge::AttrUtils::SetStr(op_desc, op_desc->GetName() + "_pattern", op_pattern)) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetPtn] %s set pattern attr failed.", op_desc->GetName().c_str());
    return FAILED;
  }

  string lx_core_type;
  op_info.GetOpCoreType(lx_core_type);
  (void)ge::AttrUtils::SetStr(op_desc, kSgtCubeVectorCoreType, lx_core_type);

  FE_LOGD("TbeOpStoreAdapter::PreCompile Op success. Node name: %s, type: %s, pattern: %s, core type %s.",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), op_pattern.c_str(),
          lx_core_type.c_str());
  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessFailPreCompTask(CompileTaskPara &task_para) const {
  for (auto &fin_task_pair : task_para.failed_tasks) {
    auto task_id = fin_task_pair.first;
    auto task_iter = task_para.task_node_map.find(task_id);
    if (task_iter == task_para.task_node_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp] thread[%lu], not find task[%ld]", GetCurThreadId(), task_id);
      return FAILED;
    }

    ge::Node *node = task_para.task_node_map[task_id];
    REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp][Node %s] Failed to pre-compile. Tid is [%lu], TaskId is [%lu] ",
                    node->GetName().c_str(), GetCurThreadId(), task_id);
  }

  if (!task_para.failed_tasks.empty()) {
    FE_LOGD("process failed task_num[%zu]. tid[%lu]", task_para.failed_tasks.size(), GetCurThreadId());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessSuccPreCompTask(CompileTaskPara &task_para) const {
  for (auto &fin_task_pair : task_para.succ_tasks) {
    auto task_id = fin_task_pair.first;
    auto task_iter = task_para.task_node_map.find(task_id);
    if (task_iter == task_para.task_node_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProSucTask] Thread[%lu], not find task[%ld]", GetCurThreadId(),
                      task_id);
      return FAILED;
    }

    ge::Node *node = task_para.task_node_map[task_id];
    TbeOpInfoPtr tbe_op_info_ptr = task_para.task_tbe_info_map[task_id];
    FE_LOGD("Thread[%lu], get task[%lu], node[%s]", GetCurThreadId(), task_id, node->GetName().c_str());

    string op_pattern_before_buff_fus;
    bool need_precompile_node = false;
    (void)ge::AttrUtils::GetBool(node->GetOpDesc(), NEED_RE_PRECOMPILE, need_precompile_node);
    if (need_precompile_node) {
      if (!ge::AttrUtils::GetStr(node->GetOpDesc(), node->GetName() + "_pattern", op_pattern_before_buff_fus)) {
        FE_LOGW("Can't get buff_fus op[%s] pattern before precompile.", node->GetName().c_str());
      }
    }

    if (SetPreCompilePattern(node->GetOpDesc(), *tbe_op_info_ptr, op_pattern_before_buff_fus) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProSucTask] %s set op pattern failed", node->GetName().c_str());
      return FAILED;
    }
  }

  FE_LOGD("process success task_num[%zu]. tid[%lu]", task_para.succ_tasks.size(), GetCurThreadId());
  return SUCCESS;
}

TbeOpInfoPtr TbeOpStoreAdapter::PreCompSetTbeOpInfo(PreCompileNodePara &comp_para) {
  if (comp_para.op_dsl_file_path.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompSetInfo] Op dsl path is invalid.");
    return nullptr;
  }

  auto op_desc = comp_para.node->GetOpDesc();
  string op_name = op_desc->GetName();
  if (!comp_para.session_graph_id.empty()) {
    op_name = comp_para.session_graph_id + "_" + op_desc->GetName();
  }

  string opFuncName = op_desc->GetType();

  TbeOpInfoPtr tbe_op_info_ptr;
  FE_MAKE_SHARED(tbe_op_info_ptr =
                     std::make_shared<te::TbeOpInfo>(op_name, comp_para.op_dsl_file_path, opFuncName, "", engine_name_),
                 return nullptr);
  tbe_op_info_ptr->SetRealName(op_desc->GetName());

  bool is_dynamic_impl = IsOpDynamicImpl(op_desc);
  tbe_op_info_ptr->SetDynamicImpl(is_dynamic_impl);

  if (op_desc->HasAttr(ge::ATTR_NAME_UNREGST_OPPATH)) {
    if (tbe_single_op_info_assembler_ptr_->AssembleSingleTbeInfo(comp_para.node, *tbe_op_info_ptr, engine_name_) !=
        SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompSetInfo] AssembleTbeInfo failed.");
      return nullptr;
    }
  } else {
    if (tbe_info_assembler_ptr_->AssembleTbeInfo(comp_para.node, comp_para.op_kernel_info_ptr, *tbe_op_info_ptr,
                                                 engine_name_) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][PreCompSetInfo] AssembleTbeInfo failed.");
      return nullptr;
    }
  }

  if (IsFeSupportedDynamicOp(*comp_para.node->GetOpDesc())) {
    tbe_op_info_ptr->SetIsUnknownShape(true);
  }
  string op_slice_info_str;
  te::LX_QUERY_STATUS status = GetOpInfo(*tbe_op_info_ptr, op_slice_info_str);
  if (status == te::LX_QUERY_SUCC) {
    (void)ge::AttrUtils::SetStr(comp_para.node->GetOpDesc(), OP_SLICE_INFO, op_slice_info_str);
    FE_LOGD("Obtain slice info %s from tbe api for node[%s].", op_slice_info_str.c_str(),
            comp_para.node->GetName().c_str());
  } else {
    FE_LOGD("Not obtain slice info from tbe api for node[%s].", comp_para.node->GetName().c_str());
  }

  // set custom flag to node
  SetOpDescCustomOp(comp_para.node->GetOpDesc());

  tbe_op_info_ptr->SetNode(comp_para.node->shared_from_this());

  return tbe_op_info_ptr;
}
/*
 *  @ingroup fe
 *  @brief   pre-compile and return pattern of op
 *  @param   [in]  node        node pointer
 *  @param   [in]  info_store   op info store pointer
 *  @param   [in] imply_type_str  op imply type
 *  @param   [in] op_dsl_file_path  python DSL file for op
 *  @return  SUCCESS or FAILED
 */
Status TbeOpStoreAdapter::PreCompileOp(vector<PreCompileNodePara> &compile_para_vec) {
  if (!support_parallel_compile) {
    return SerialPreCompileOp(compile_para_vec);
  } else {
    return ParallelPreCompileOp(compile_para_vec);
  }
}

Status TbeOpStoreAdapter::ParallelPreCompileOp(vector<PreCompileNodePara> &compile_para_vec) {
  uint64_t thread_id = GetCurThreadId();
  CompileTaskPara task_para;
  task_para.task_num = 0;
  for (auto &comp_para : compile_para_vec) {
    FE_CHECK(comp_para.node == nullptr, REPORT_FE_ERROR("compPara.node is nullptr."), return FAILED);
    comp_para.tbe_op_info_ptr = PreCompSetTbeOpInfo(comp_para);
    if (comp_para.tbe_op_info_ptr == nullptr) {
      REPORT_FE_ERROR("[SubGraphOpt] [Pre-Comp] Set TbeOpInfo Failed.");
      return FAILED;
    }
  }

  for (auto &comp_para : compile_para_vec) {
    te::BUILD_TYPE build_type;
    if (IsFuzzBuild()) {
      build_type = te::FUZZILY_BUILD;
    } else {
      build_type = te::ACCURATELY_BUILD;
    }
    task_para.task_num++;
    comp_para.tbe_op_info_ptr->SetBuildType(build_type);

    uint64_t taskId = GetAtomicId();
    task_para.task_node_map.insert(make_pair(taskId, comp_para.node));
    task_para.task_tbe_info_map.insert(make_pair(taskId, comp_para.tbe_op_info_ptr));

    bool result = PreBuildTbeOp(*comp_para.tbe_op_info_ptr, taskId, thread_id);
    if (!result) {
      std::map<std::string, std::string> error_key_map;
      // op_name,op_type,graph_id,thread_id,task_id
      error_key_map[EM_OP_NAME] = comp_para.node->GetOpDesc()->GetName();
      error_key_map[EM_OP_TYPE] = comp_para.node->GetOpDesc()->GetType();
      error_key_map[EM_GRAPH_ID] = comp_para.session_graph_id;
      error_key_map[EM_TASK_ID] = std::to_string(taskId);
      error_key_map[EM_THREAD_ID] = std::to_string(thread_id);

      LogErrorMessage(EM_PRECOMPLIE_TASK_FAILED, error_key_map);
    }

    if (!result) {
      REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to pre-compile node %s. thread id is [%lu], task is [%lu].",
                      comp_para.node->GetName().c_str(), thread_id, taskId);
      return FAILED;
    }
    FE_LOGD("Set precompile task[%s] success, tid[%lu], taskId[%lu].", comp_para.node->GetName().c_str(), thread_id,
            taskId);
  }

  FE_LOGD("Thread[%lu], to set %lu tasks to pre-compile.", thread_id, task_para.task_num);

  if (WaitTaskFinish(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to wait thread[%lu]'s task finish.", thread_id);
    return FAILED;
  }

  if (ProcessFailPreCompTask(task_para) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to process failed task. Thread_id is [%lu].", thread_id);
    return FAILED;
  }

  if (ProcessSuccPreCompTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Pre-Comp]Failed to process successful task. Thread_id is [%lu].", thread_id);
    return FAILED;
  }

  return SUCCESS;
}

void TbeOpStoreAdapter::SetCustomFlag(ScopeNodeIdMap &fusion_nodes_map) const {
  for (auto &iter : fusion_nodes_map) {
    for (ge::Node *node : iter.second) {
      if (node == nullptr) {
        continue;
      }
      string name = node->GetName();
      auto op_desc = node->GetOpDesc();
      int tmp_imply_type = 0;
      if (!ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, tmp_imply_type)) {
        FE_LOGW("Node[%s]: get fe_imply_type failed.", name.c_str());
        continue;
      }

      bool is_custom_op = true;
      if (BUILT_IN_IMPLY_TYPE.count(tmp_imply_type) != 0) {
        is_custom_op = false;
      }

      if (!ge::AttrUtils::SetBool(op_desc, IS_CUSTOM_OP, is_custom_op)) {
        FE_LOGW("Node[%s]: set is_custom_op[%d] failed.", name.c_str(), is_custom_op);
      }
    }
  }
}

void TbeOpStoreAdapter::GetAutoMode(ScopeNodeIdMap &fusion_nodes_map, bool &auto_mode) const {
  for (auto &iter : fusion_nodes_map) {
    for (ge::Node *node : iter.second) {
      if (node == nullptr) {
        continue;
      }
      uint32_t thread_mode = 0;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kThreadMode, thread_mode);
      if (thread_mode) {
        auto_mode = true;
        break;
      }
    }
  }
}

Status TbeOpStoreAdapter::CompileOp(ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string> &json_path_map,
                                    std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                    const std::vector<ge::NodePtr> &buff_fus_to_del_nodes) {
  FE_LOGD("TbeOpStoreAdapter::Compile Op begin.");
  // If the map is empty, then there is no fusion op.
  if (fusion_nodes_map.empty()) {
    FE_LOGD("Call Fusion Engine success, but there is no fusion op.");
    return SUCCESS;
  }

  SetCustomFlag(fusion_nodes_map);
  bool auto_mode = false;
  GetAutoMode(fusion_nodes_map, auto_mode);
  FE_LOGD("TbeOpStoreAdapter::Compile Op auto_mode:%d.", auto_mode);
  if (GetPlatformSCubeVecSplitFlag() == 0 || (GetPlatformAICoreMode() == FFTS_MODE_FFTS_PLUS && !auto_mode)) {
    std::vector<ge::NodePtr> l1_fusion_failed_nodes;
    return ParallelCompileOp(fusion_nodes_map, json_path_map, buff_fus_compile_failed_nodes, l1_fusion_failed_nodes,
                             buff_fus_to_del_nodes, CompileStrategy::COMPILE_STRATEGY_OP_SPEC);
  } else {
    return CompileMultiKernelSliceOp(fusion_nodes_map, json_path_map, buff_fus_compile_failed_nodes,
                                     buff_fus_to_del_nodes);
  }
}

/*
 *  @ingroup fe
 *  @brief   compile fused op and single op, and generate .o and json files
 *  @param   [in]  fusion_nodes_map  op id and fused sub-graph
 *  @ptaram  [out] json_path_map    keep path of .o and json of each op
 *  @return  SUCCESS or FAILED
 */
Status TbeOpStoreAdapter::CompileOp(CompileInfoParam &compile_info) {
  FE_LOGD("TbeOpStoreAdapter::Compile Op begin.");
  // If the map is empty, then there is no fusion op.
  if (compile_info.fusion_nodes_map.empty()) {
    FE_LOGD("Call Fusion Engine success, but there is no fusion op.");
    return SUCCESS;
  }

  SetCustomFlag(compile_info.fusion_nodes_map);
  bool auto_mode = false;
  GetAutoMode(compile_info.fusion_nodes_map, auto_mode);
  FE_LOGD("TbeOpStoreAdapter::Compile Op auto_mode:%d.", auto_mode);
  if (GetPlatformSCubeVecSplitFlag() == 0 || (GetPlatformAICoreMode() == FFTS_MODE_FFTS_PLUS && !auto_mode)) {
    return ParallelCompileOp(compile_info.fusion_nodes_map, compile_info.scope_json_map,
                             compile_info.buff_fus_compile_failed_nodes, compile_info.l1_fusion_failed_nodes,
                             compile_info.buff_fus_to_del_nodes, compile_info.compile_strategy);
  } else {
    return CompileMultiKernelSliceOp(compile_info.fusion_nodes_map, compile_info.scope_json_map,
                                     compile_info.buff_fus_compile_failed_nodes, compile_info.buff_fus_to_del_nodes);
  }
}

bool TbeOpStoreAdapter::IsL1FusionOptimizedNodes(const std::vector<ge::Node *> &nodes) const {
  if (nodes.empty()) {
    return false;
  }
  ge::Node *first_node = nodes[0];
  return ScopeAllocator::HasL1ScopeAttr(first_node->GetOpDesc());
}

bool TbeOpStoreAdapter::IsBuffFusOptimizedNodes(const std::vector<ge::Node *> &scope_op) const {
  bool need_precompile_node = false;
  Status ret_lx;
  for (auto &op : scope_op) {
    if (op == nullptr) {
      continue;
    }
    ret_lx = ge::AttrUtils::GetBool(op->GetOpDesc(), NEED_RE_PRECOMPILE, need_precompile_node);
    if (!ret_lx) {
      return false;
    }
    if (!need_precompile_node) {
      return false;
    }
  }
  return true;
}

void TbeOpStoreAdapter::SetFusionFailedId(const vector<ge::Node *> &fusion_nodes,
                                          const int64_t &fusion_failed_id) const {
  for (ge::Node *node : fusion_nodes) {
    if (node == nullptr) {
      continue;
    }
    string name = node->GetName();
    if (ge::AttrUtils::SetInt(node->GetOpDesc(), FUSION_FAILED_ID_ATTR, fusion_failed_id)) {
      FE_LOGD("Node[%s]: set failed_id[%ld] success.", name.c_str(), fusion_failed_id);
    }
  }
}

bool TbeOpStoreAdapter::StopCompileOpInTuningAndAfterUBMatchMode() const {
  std::string build_mode_value = Configuration::Instance(engine_name_).GetGeContextOptionValue(ge::BUILD_MODE);
  std::string step_mode_value = Configuration::Instance(engine_name_).GetGeContextOptionValue(ge::BUILD_STEP);
  if (build_mode_value == ge::BUILD_MODE_TUNING && step_mode_value == ge::BUILD_STEP_AFTER_UB_MATCH) {
    FE_LOGI("No need to try recovery if build_mode is [%s] and step is [%s].", build_mode_value.c_str(),
            step_mode_value.c_str());
    return true;
  }
  return false;
}

inline bool TbeOpStoreAdapter::IsFuzzCompileStrategy(const CompileStrategy &compile_strategy) const {
  return compile_strategy == CompileStrategy::COMPILE_STRATEGY_ONLINE_FUZZ;
}

bool TbeOpStoreAdapter::StopWaitTaskFinishInTuningAndAfterBuilderMode(const CompileStrategy &compile_strategy) const {
  std::string build_mode_value = Configuration::Instance(engine_name_).GetGeContextOptionValue(ge::BUILD_MODE);
  std::string step_mode_value = Configuration::Instance(engine_name_).GetGeContextOptionValue(ge::BUILD_STEP);
  bool no_need_to_wait_task_finish =
      ((build_mode_value == ge::BUILD_MODE_TUNING && step_mode_value == ge::BUILD_STEP_AFTER_BUILDER) ||
       (build_mode_value == ge::BUILD_MODE_TUNING && step_mode_value == ge::BUILD_STEP_AFTER_BUILDER_SUB));
  if (compile_strategy == CompileStrategy::COMPILE_STRATEGY_OP_SPEC && no_need_to_wait_task_finish) {
    FE_LOGI("No need to wait task finish if build_mode is [%s] and step is [%s] and flag is %d.",
            build_mode_value.c_str(), step_mode_value.c_str(), static_cast<int32_t>(compile_strategy));
    return true;
  }
  return false;
}

Status TbeOpStoreAdapter::SetSgtOpJsonPath(const ge::OpDescPtr &compile_op_desc,
                                           map<int64_t, std::string> &json_path_map,
                                           const int &scope_idx) const {
  string json_file_path;
  // get json file path
  if (!(ge::AttrUtils::GetStr(compile_op_desc, "json_file_path", json_file_path))) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetSgtOpJsonPath] Get json_file_path failed.");
    return FAILED;
  }

  if (json_file_path.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetSgtOpJsonPath] JsonFilePath.size is invalid.");
    return FAILED;
  }
  // keep json path
  if (json_path_map[scope_idx].empty()) {
    json_path_map[scope_idx] = json_file_path;
  } else {
    json_path_map[scope_idx] = json_path_map[scope_idx] + ";" + json_file_path;
  }

  return SUCCESS;
}

Status TbeOpStoreAdapter::SetTaskToTeFusion(CompileTaskPara &task_para,
                                            const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                                            const CompileStrategy &compile_strategy) {
  for (auto &iter : *task_para.fusion_nodes_map) {
    task_para.task_num++;
    uint64_t taskId = GetAtomicId();
    task_para.task_scope_id_map.insert(std::make_pair(taskId, iter.first));
    FE_LOGD("%lu, taskId %lu , scope_id %ld, set compile %s task", GetCurThreadId(), taskId, iter.first,
            iter.second[0]->GetName().c_str());
    // set compile task
    if (SetTeTask(iter.second, taskId, buff_fus_to_del_nodes, compile_strategy) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][FillTaskPara] The op[%s] set compile task failed",
                      iter.second[0]->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessLxFusionFailCompileTasks(CompileTaskPara &task_para,
                                                          std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                                                          std::vector<ge::NodePtr> &buff_fus_failed_nodes) const {
  if (task_para.failed_tasks.empty()) {
    return SUCCESS;
  }
  for (auto iter = task_para.failed_tasks.begin(); iter != task_para.failed_tasks.end();) {
    uint64_t task_id = iter->first;
    std::unordered_map<uint64_t, int64_t>::const_iterator scope_id_iter = task_para.task_scope_id_map.find(task_id);
    if (scope_id_iter == task_para.task_scope_id_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcessLxFusionFailTask] can not find scope id by task id[%lu]",
                      task_id);
      return FAILED;
    }

    int64_t scope_id = scope_id_iter->second;
    ScopeNodeIdMap::const_iterator nodes_iter = task_para.fusion_nodes_map->find(scope_id);
    if (nodes_iter == task_para.fusion_nodes_map->end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcessLxFusionFailTask] can not find fusion nodes by scope id[%ld]",
                      scope_id);
      return FAILED;
    }
    // l1 fuson failed nodes
    if (IsL1FusionOptimizedNodes(nodes_iter->second)) {
      FE_LOGI("Failed to compile L1 fusion optimized nodes. Scope id[%ld], task id[%lu].", scope_id, task_id);
      iter = task_para.failed_tasks.erase(iter);
      for (auto &node : nodes_iter->second) {
        RemoveL1FusionScopeAttr(node->GetOpDesc());
        l1_fusion_failed_nodes.push_back(node->shared_from_this());
        FE_LOGD("L1 fusion compile failed node[%s, %s].", node->GetName().c_str(), node->GetType().c_str());
      }
      continue;
    }
    // other nodes after lxfusion
    if (IsBuffFusOptimizedNodes(nodes_iter->second)) {
      FE_LOGI("Fail to compile nodes who are optimized by lxfusion. Scope id[%ld], task id[%lu].", scope_id, task_id);
      iter = task_para.failed_tasks.erase(iter);
      for (auto &node : nodes_iter->second) {
        buff_fus_failed_nodes.push_back(node->shared_from_this());
        FE_LOGD("Other lxfusion compile failed node[%s, %s].", node->GetName().c_str(), node->GetType().c_str());
      }
      continue;
    }
    iter++;
  }
  return SUCCESS;
}

void TbeOpStoreAdapter::SaveMsTuneErrorMsg(CompileTaskPara &task_para) const {
  std::unordered_map<uint64_t, int64_t> &pre_scope_id_map = task_para.task_scope_id_map;
  for (auto &fin_task_pair : task_para.failed_tasks) {
    uint64_t task_id = fin_task_pair.first;
    std::unordered_map<uint64_t, int64_t>::const_iterator task_iter = pre_scope_id_map.find(task_id);
    if (task_iter == pre_scope_id_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SaveMsTuneErrorMsg] Thread[%lu], not find taskId[%ld]", GetCurThreadId(),
                      task_id);
      return;
    }
    int64_t scope_id = task_iter->second;
    ScopeNodeIdMap::const_iterator nodes_iter = task_para.fusion_nodes_map->find(scope_id);
    if (nodes_iter == task_para.fusion_nodes_map->end()) {
      FE_LOGD("Can not find fusion nodes by scope id[%ld]", scope_id);
      return;
    }
    FE_LOGD("save compile msg, taskId[%lu], tid[%lu]", task_id, GetCurThreadId());

    string node_name;
    for (auto &node : nodes_iter->second) {
      node_name += node->GetName();
      node_name += ", ";
    }
    FE_LOGI("Failed nodes are: {%s}", node_name.c_str());

    std::map<std::string, std::string> op_build_mapArgs = {
        {"S40000", nodes_iter->second.at(0)->GetName()}};
    ge::ComputeGraphPtr owner_graph = nodes_iter->second.at(0)->GetOwnerComputeGraph();
    std::string owner_graph_name;
    (void) ge::AttrUtils::GetStr(owner_graph, ge::ATTR_NAME_ROOT_GRAPH_NAME, owner_graph_name);
    SaveErrorMessage(owner_graph_name, op_build_mapArgs);
  }
}

Status TbeOpStoreAdapter::RetryCompileFailOp(CompileTaskPara &task_para) {
  if (ProcessFailCompileTask(task_para, CompileStrategy::COMPILE_STRATEGY_OP_SPEC) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][RetryCompFailOp] Thread[%lu] process fail task failed", GetCurThreadId());
    return FAILED;
  }
  // wait for finish
  if (WaitTaskFinish(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][RetryCompFailOp] Thread[%lu] wait task finish failed", GetCurThreadId());
    return FAILED;
  }

  if (!task_para.failed_tasks.empty()) {
    for (auto &fin_task_pair : task_para.failed_tasks) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][RetryCompFailOp] Thread[%lu] recompile single op[%s] failed",
                      GetCurThreadId(), fin_task_pair.second.teNodeOpDesc->GetName().c_str());
    }
    return FAILED;
  }

  // process successful sgt sliced task
  if (ProcessSuccSgtSliceTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][RetrySgtSliceOp] Thread[%lu] failed to process successful sgt task.",
                    GetCurThreadId());
  }
  // process successful task
  if (ProcessSuccCompileTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][RetryCompFailOp] Thread[%lu] failed to process successful task.",
                    GetCurThreadId());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessAllFailedCompileTasks(CompileTaskPara &task_para,
                                                       std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                                       std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                                                       const CompileStrategy &compile_strategy) {
  if (!task_para.failed_tasks.empty()) {
    SaveMsTuneErrorMsg(task_para);

    if (StopCompileOpInTuningAndAfterUBMatchMode()) {
      FE_LOGI("No need to try recovery fused op.");
      return FAILED;
    }

    if (IsFuzzCompileStrategy(compile_strategy)) {
      FE_LOGI("online fuzzy compile, no need retry.");
      return FAILED;
    }

    if (ProcessLxFusionFailCompileTasks(task_para, l1_fusion_failed_nodes, buff_fus_compile_failed_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcLxFusFailedCompTask] Thread[%lu] Fail to process lx failed tasks.",
                      GetCurThreadId());
      return FAILED;
    }

    if (ProcessFailCompileTask(task_para, compile_strategy) == FAILED) {
      REPORT_FE_ERROR("Thread[%lu] process fail task failed", GetCurThreadId());
      return FAILED;
    }
    // wait for finish
    if (WaitTaskFinish(task_para) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcFailedCompTask] Thread[%lu] wait task finish failed",
                      GetCurThreadId());
      return FAILED;
    }

    if (!task_para.failed_tasks.empty()) {
      for (auto &fin_task_pair : task_para.failed_tasks) {
        REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcFailedCompTask] Thread[%lu] recompile single op[%s] failed",
                        GetCurThreadId(), fin_task_pair.second.teNodeOpDesc->GetName().c_str());

      }
      return FAILED;
    }
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::ParallelCompileOp(ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string> &json_path_map,
                                            std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                            std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                                            const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                                            const CompileStrategy &compile_strategy) {
  FE_CHECK(TeFusion == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][ParalCompOp] TeFusion is nullptr."),
           return FAILED);
  FE_TIMECOST_START(TeFusion);
  CompileTaskPara task_para = {.task_num = 0, .json_path_map = &json_path_map, .fusion_nodes_map = &fusion_nodes_map};
  if (SetTaskToTeFusion(task_para, buff_fus_to_del_nodes, compile_strategy) != SUCCESS) {
    return FAILED;
  }

  FE_LOGD("Thread[%lu], to set %lu tasks to comp", GetCurThreadId(), task_para.task_num);
  if (StopWaitTaskFinishInTuningAndAfterBuilderMode(compile_strategy)) {
    FE_LOGI("No need to wait task finish.");
    return SUCCESS;
  }
  // wait for finish
  if (WaitTaskFinish(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ParalCompOp] Thread[%lu] wait task finish failed", GetCurThreadId());
    return FAILED;
  }
  // process success task
  if (ProcessSuccCompileTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ParalCompOp] Thread[%lu] process success, task failed", GetCurThreadId());
    return FAILED;
  }
  // process failed task
  if (ProcessAllFailedCompileTasks(task_para, buff_fus_compile_failed_nodes, l1_fusion_failed_nodes,
                                   compile_strategy) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ParalCompOp] Thread[%lu] process fail task failed", GetCurThreadId());
    return FAILED;
  }

  // process success task
  if (ProcessSuccCompileTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ParalCompOp] Thread[%lu] process success, task failed", GetCurThreadId());
    return FAILED;
  }

  FE_TIMECOST_END(TeFusion, "TeFusion during FEGraphOptimizer::OptimizeFusedGraph");
  FE_LOGI("TbeOpStoreAdapter::Compile Op success. tid:%lu", GetCurThreadId());

  return SUCCESS;
}

Status TbeOpStoreAdapter::SetOpJsonPath(const ge::OpDescPtr &compile_op_desc,
                                        map<int64_t, std::string> &json_path_map,
                                        int scope_idx) const {
  string json_file_path;
  // get json file path
  if (!(ge::AttrUtils::GetStr(compile_op_desc, "json_file_path", json_file_path))) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetJsonPath] Get json_file_path failed.");
    return FAILED;
  }

  if (json_file_path.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetJsonPath] Json path of node %s is empty.",
                    compile_op_desc->GetName().c_str());
    return FAILED;
  }
  FE_LOGW("Json path for node %s is %s", compile_op_desc->GetName().c_str(), json_file_path.c_str());
  // keep json path
  json_path_map[scope_idx] = json_file_path;
  return SUCCESS;
}

void TbeOpStoreAdapter::SetOpDescCustomOp(ge::OpDescPtr op_desc) const {
  int tmp_imply_type = 0;
  if (!ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, tmp_imply_type)) {
    FE_LOGD("Node[%s]: get fe_imply_type failed.", op_desc->GetName().c_str());
  }

  bool is_custom_op = true;
  if (BUILT_IN_IMPLY_TYPE.count(tmp_imply_type) != 0) {
    is_custom_op = false;
  }
  if (!ge::AttrUtils::SetBool(op_desc, IS_CUSTOM_OP, is_custom_op)) {
    FE_LOGD("Node[%s]: set is_custom_op[%d] failed.", op_desc->GetName().c_str(), is_custom_op);
  }
}

Status TbeOpStoreAdapter::DoFuzzBuildTbeOp(std::vector<ge::Node *> &node_vec, uint64_t taskId, uint64_t thread_id) {
  if (node_vec.size() != 1) {
    return NOT_CHANGED;
  }

  ge::Node *node = node_vec[0];
  auto op_desc = node->GetOpDesc();
  if (!IsFuzzBuildOp(*op_desc)) {
    FE_LOGD("[SubGraphOpt][DoFuzzBuild]No Need to do fuzzy build tbe op.");
    return NOT_CHANGED;
  }

  FE_LOGD("Start to do fuzz build tbe op[%s].", node->GetName().c_str());
  te::OpBuildResCode result = FuzzBuildTbeOp(taskId, thread_id, *node);
  if (result == te::OP_BUILD_FAIL) {
    std::map<std::string, std::string> error_key_map;
    error_key_map[EM_OP_NAME] = op_desc->GetName();
    error_key_map[EM_OP_TYPE] = op_desc->GetType();
    std::string session_graph_id = "";
    (void)ge::AttrUtils::GetStr(op_desc, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    error_key_map[EM_GRAPH_ID] = session_graph_id;
    error_key_map[EM_THREAD_ID] = std::to_string(thread_id);
    error_key_map[EM_TASK_ID] = std::to_string(taskId);
    LogErrorMessage(EM_COMPLIE_TASK_FAILED, error_key_map);
    REPORT_FE_ERROR("[SubGraphOpt][Compile][DoFuzzBuild] Fuzz compile te fusion op %s failed, tid:%lu, taskId:%lu.",
        op_desc->GetName().c_str(), thread_id, taskId);
    return FAILED;
  }
  FE_LOGD("Set op[%s] success, thread[%lu], taskId[%lu].", op_desc->GetName().c_str(), thread_id, taskId);
  return SUCCESS;
}

Status TbeOpStoreAdapter::SetTeTask(std::vector<ge::Node *> &node_vec, uint64_t taskId,
                                    const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                                    const CompileStrategy &compile_strategy) {
  if (node_vec.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetTeTask] nodeVec in empty.");
    return FAILED;
  }

  std::shared_ptr<ge::OpDesc> op_desc_ptr = nullptr;
  FE_MAKE_SHARED(op_desc_ptr = std::make_shared<ge::OpDesc>(node_vec[0]->GetName(), ""), return FAILED);

  uint64_t thread_id = GetCurThreadId();

  std::string op_compile_strategy;
  for (ge::Node *node : node_vec) {
    if (ge::AttrUtils::GetStr(node->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy) &&
        !op_compile_strategy.empty()) {
      FE_LOGD("Compile strategy of node[%s, %s] is [%s].", node->GetName().c_str(), node->GetType().c_str(),
              op_compile_strategy.c_str());
      break;
    }
  }
  FE_LOGD("Get _op_compile_strategy attr from graph is %s.", op_compile_strategy.c_str());
  FE_LOGD("Flag of compile strategy is %d.", static_cast<int32_t>(compile_strategy));
  if (compile_strategy != CompileStrategy::COMPILE_STRATEGY_OP_SPEC) {
    auto compile_strategy_iter = kCompileStrategyStrMap.find(compile_strategy);
    if (compile_strategy_iter != kCompileStrategyStrMap.end()) {
      op_compile_strategy = compile_strategy_iter->second;
      FE_LOGD("Op compile strategy has been modified to %s due to compile strategy.", op_compile_strategy.c_str());
    }
  }

  // judge fuzz compile
  Status res = DoFuzzBuildTbeOp(node_vec, taskId, thread_id);
  if (res == SUCCESS) {
    FE_LOGD("Node: %s, do fuzz build tbe op success.", node_vec[0]->GetOpDesc()->GetName().c_str());
    return SUCCESS;
  } else if (res == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetTeTask] Node: %s, do fuzz build tbe op failed.",
                    node_vec[0]->GetOpDesc()->GetName().c_str());
    return FAILED;
  }

  te::OpBuildResCode result =
      TeFusion(node_vec, op_desc_ptr, buff_fus_to_del_nodes, taskId, thread_id, op_compile_strategy);
  if (result == te::OP_DYNSHAPE_NOT_SUPPORT) {
    if (SetSupportDynamicShape(node_vec) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SetTeTask] Op : %s set support_dynamicshape failed",
                      node_vec[0]->GetName().c_str());
      return FAILED;
    }
  } else if (result == te::OP_BUILD_FAIL) {
    std::map<std::string, std::string> error_key_map;

    // op_name,op_type,graph_id,thread_id,task_id
    error_key_map[EM_OP_NAME] = node_vec[0]->GetOpDesc()->GetName();
    error_key_map[EM_OP_TYPE] = node_vec[0]->GetOpDesc()->GetType();

    std::string session_graph_id = "";
    (void)ge::AttrUtils::GetStr(node_vec[0]->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    error_key_map[EM_GRAPH_ID] = session_graph_id;
    error_key_map[EM_THREAD_ID] = std::to_string(thread_id);
    error_key_map[EM_TASK_ID] = std::to_string(taskId);

    LogErrorMessage(EM_COMPLIE_TASK_FAILED, error_key_map);
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SetTeTask] Compile te fusion op %s failed, tid:%lu, taskId:%lu.",
                    op_desc_ptr->GetName().c_str(), thread_id, taskId);
    return FAILED;
  }

  FE_LOGD("set op[%s] success, thread[%lu], taskId[%lu].", op_desc_ptr->GetName().c_str(), thread_id, taskId);
  return SUCCESS;
}

void TbeOpStoreAdapter::SgtGetCompileStrategy(std::vector<ge::Node *> &node_vec, std::string &op_compile_strategy,
                                              const CompileStrategy &compile_strategy) const {
  (void)ge::AttrUtils::GetStr(node_vec.at(0)->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy);
  FE_LOGD("Get _op_compile_strategy attr from graph is %s.", op_compile_strategy.c_str());
  FE_LOGD("Flag of compile strategy is %d.", static_cast<int32_t>(compile_strategy));
  if (compile_strategy != CompileStrategy::COMPILE_STRATEGY_OP_SPEC) {
    auto compile_strategy_iter = kCompileStrategyStrMap.find(compile_strategy);
    if (compile_strategy_iter != kCompileStrategyStrMap.end()) {
      op_compile_strategy = compile_strategy_iter->second;
      FE_LOGD("Op compile strategy has been modified to %s due to compile strategy.", op_compile_strategy.c_str());
    }
  }
}

Status TbeOpStoreAdapter::SgtSetTeTask(std::vector<ge::Node *> &node_vec, uint64_t taskId,
                                       const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                                       const CompileStrategy &compile_strategy, uint64_t slice_shape_index) {
  if (node_vec.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SgtSetTeTask] nodeVec in empty.");
    return FAILED;
  }

  std::shared_ptr<ge::OpDesc> op_desc_ptr = nullptr;
  FE_MAKE_SHARED(op_desc_ptr = std::make_shared<ge::OpDesc>(node_vec[0]->GetName(), ""), return FAILED);

  uint64_t thread_id = GetCurThreadId();
  std::string op_compile_strategy;
  SgtGetCompileStrategy(node_vec, op_compile_strategy, compile_strategy);

  // judge fuzz compile
  Status res = DoFuzzBuildTbeOp(node_vec, taskId, thread_id);
  if (res == SUCCESS) {
    FE_LOGD("Node: %s, do fuzz build tbe op success.", node_vec[0]->GetOpDesc()->GetName().c_str());
    return SUCCESS;
  } else if (res == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SgtSetTeTask] Node: %s, do fuzz build tbe op failed.",
                    node_vec[0]->GetOpDesc()->GetName().c_str());
    return FAILED;
  }

  te::OpBuildResCode result =
      TeFusionV(node_vec, op_desc_ptr, buff_fus_to_del_nodes, taskId,
                thread_id, slice_shape_index, op_compile_strategy);
  if (result == te::OP_DYNSHAPE_NOT_SUPPORT) {
    if (SetSupportDynamicShape(node_vec) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SgtSetTeTask] Op : %s set support_dynamicshape failed",
                      node_vec[0]->GetName().c_str());
      return FAILED;
    }
  } else if (result == te::OP_BUILD_FAIL) {
    std::map<std::string, std::string> error_key_map;

    // op_name,op_type,graph_id,thread_id,task_id
    error_key_map[EM_OP_NAME] = node_vec[0]->GetOpDesc()->GetName();
    error_key_map[EM_OP_TYPE] = node_vec[0]->GetOpDesc()->GetType();

    std::string session_graph_id = "";
    (void)ge::AttrUtils::GetStr(node_vec[0]->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    error_key_map[EM_GRAPH_ID] = session_graph_id;
    error_key_map[EM_THREAD_ID] = std::to_string(thread_id);
    error_key_map[EM_TASK_ID] = std::to_string(taskId);

    LogErrorMessage(EM_COMPLIE_TASK_FAILED, error_key_map);
    REPORT_FE_ERROR("[SubGraphOpt][Compile][SgtSetTeTask] Compile te fusion op %s failed, tid:%lu, taskId:%lu.",
                    op_desc_ptr->GetName().c_str(), thread_id, taskId);
    return FAILED;
  }

  FE_LOGD("set op[%s] success, thread[%lu], taskId[%lu].", op_desc_ptr->GetName().c_str(), thread_id, taskId);
  return SUCCESS;
}

Status TbeOpStoreAdapter::WaitTaskFinish(CompileTaskPara &task_para) const {
  vector<te::FinComTask> fin_com_task;
  task_para.succ_tasks.clear();
  task_para.failed_tasks.clear();

  uint64_t thread_id = GetCurThreadId();
  uint64_t task_num = task_para.task_num;
  while (task_num > 0) {
    fin_com_task.clear();
    bool ret = WaitAllFinished(thread_id, fin_com_task);
    if (!ret) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Wait] wait for compile task finish failed. thread[%lu]", thread_id);
      return FAILED;
    }
    // not get task
    if (fin_com_task.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    for (auto &task : fin_com_task) {
      if (task.status == SUCCESS) {
        task_para.succ_tasks.emplace(task.taskId, task);
      } else {
        task_para.failed_tasks.emplace(task.taskId, task);
      }
      FE_LOGD("tid[%lu], taskId[%lu], task_num[%lu], fin_task_num[%lu], status[%d]", thread_id, task.taskId, task_num,
              fin_com_task.size(), task.status);
    }

    if (task_num < fin_com_task.size()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Wait] taskNum %lu is less than fin size %zu", task_num,
                      fin_com_task.size());
      return FAILED;
    }
    task_num -= fin_com_task.size();
  }

  FE_LOGD("tid:%lu, total_num[%lu], succ_task_num[%zu], fail_task_num[%lu]", thread_id, task_para.task_num,
          task_para.succ_tasks.size(), task_para.failed_tasks.size());
  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessSuccCompileTask(CompileTaskPara &task_para) {
  for (auto &fin_task_pair : task_para.succ_tasks) {
    FE_LOGD("Process task with first node %s.", fin_task_pair.second.teNodeOpDesc->GetName().c_str());
    auto task_id = fin_task_pair.first;
    auto task_iter = task_para.task_scope_id_map.find(task_id);
    if (task_iter == task_para.task_scope_id_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProSucCmplTask] %lu, not find taskId[%ld]", GetCurThreadId(), task_id);
      return FAILED;
    }

    int64_t scope_id = task_para.task_scope_id_map[task_id];
    FE_LOGD("tid[%lu], get taskId[%lu], scope_id[%ld]", GetCurThreadId(), task_id, scope_id);
    if (SetOpJsonPath(fin_task_pair.second.teNodeOpDesc, *task_para.json_path_map, scope_id) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProSucCmplTask] %s set op json path failed",
                      (*task_para.fusion_nodes_map)[scope_id][0]->GetName().c_str());
      return FAILED;
    }
    if (SetOpCompileInfo((*task_para.fusion_nodes_map)[scope_id], fin_task_pair.second.teNodeOpDesc) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProSucCmplTask] Op : %s set op_compile_info failed",
                      (*task_para.fusion_nodes_map)[scope_id][0]->GetName().c_str());
      return FAILED;
    }
  }
  task_para.succ_tasks.clear();
  FE_LOGD("process success task_num[%zu]. tid[%lu]", task_para.succ_tasks.size(), GetCurThreadId());
  return SUCCESS;
}

void TbeOpStoreAdapter::RollBackAttributes(std::vector<ge::Node *> &failed_nodes) const {
  for (auto node : failed_nodes) {
    std::vector<string> roll_back_attrs;
    auto op_desc = node->GetOpDesc();
    (void)ge::AttrUtils::GetListStr(op_desc, ROLLBACK_IF_FAILED, roll_back_attrs);
    FE_LOGD("remove attr: node name %s size %zu", node->GetName().c_str(), roll_back_attrs.size());
    for (auto &attr : roll_back_attrs) {
      if (ge::AttrUtils::HasAttr(op_desc, attr)) {
        op_desc->DelAttr(attr);
      }
      if (attr == "reuse_input") {
        for (size_t i = 0; i < op_desc->GetAllOutputsDescSize(); i++) {
          auto out_desc = op_desc->MutableOutputDesc(i);
          if (out_desc == nullptr) {
            continue;
          }
          ge::TensorUtils::SetReuseInput(*out_desc.get(), false);
        }
        FE_LOGD("remove reuse_input for node %s.", node->GetName().c_str());
      }
    }
  }
}

Status TbeOpStoreAdapter::SetFailedOpCompileTask(ge::Node* node, CompileTaskPara &task_para,
                                                 const CompileStrategy &compile_strategy) {
  int tmp_imply_type = 0;
  if (!ge::AttrUtils::GetInt(node->GetOpDesc(), FE_IMPLY_TYPE, tmp_imply_type)) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][ProcFailedCompTask] get imply type failed, op[%s, type %s], op_imply_type[%d].",
        node->GetOpDesc()->GetName().c_str(), node->GetType().c_str(), tmp_imply_type);
    return FAILED;
  }

  vector<ge::Node *> node_vec = {node};
  int64_t scope_id = ScopeAllocator::Instance().AllocateNegScopeId();
  task_para.fusion_nodes_map->insert(make_pair(scope_id, node_vec));
  if (!(ScopeAllocator::SetScopeAttr(node->GetOpDesc(), scope_id))) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcFailedCompTask] set op[%s] scope id failed.",
                    node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Op[name:%s,type:%s] fusion failed, now compile it as single op, scopeid is %ld.",
          node->GetOpDesc()->GetName().c_str(), node->GetType().c_str(), scope_id);

  // set compile task
  std::vector<ge::NodePtr> buff_fus_to_del_nodes;

  Status result = SetTaskForOneScope(node_vec, scope_id, buff_fus_to_del_nodes, task_para,
                                     compile_strategy);
  if (result != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcFailedCompTask] failed to re-compile single op %s.",
                    node->GetName().c_str());
    return result;
  }
  return SUCCESS;
}

void TbeOpStoreAdapter::ClearTaskPara(CompileTaskPara &task_para) const {
  task_para.task_num = 0;
  task_para.task_scope_id_map.clear();
  task_para.scope_task_ids_map.clear();
  task_para.failed_task_able_to_delete.clear();
}
Status TbeOpStoreAdapter::ProcessFailCompileTask(CompileTaskPara &task_para,
                                                 const CompileStrategy &compile_strategy) {
  if (task_para.failed_tasks.empty()) {
    ClearTaskPara(task_para);
    return SUCCESS;
  }

  std::unordered_map<uint64_t, int64_t> pre_scope_id_map = task_para.task_scope_id_map;
  ClearTaskPara(task_para);
  for (auto &fin_task_pair : task_para.failed_tasks) {
    auto task_id = fin_task_pair.first;
    auto task_iter = pre_scope_id_map.find(task_id);
    if (task_iter == pre_scope_id_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcFailedCompTask] tid[%lu], not find taskId[%ld]", GetCurThreadId(),
                      task_id);
      return FAILED;
    }

    int64_t pre_scope_id = pre_scope_id_map[task_id];
    FE_LOGD("retry compile %s, taskId[%lu], tid[%lu]",
            (*task_para.fusion_nodes_map)[pre_scope_id][0]->GetName().c_str(),
            task_id, GetCurThreadId());

    std::vector<ge::Node *> &failed_nodes = (*task_para.fusion_nodes_map)[pre_scope_id];
    RollBackAttributes(failed_nodes);

    for (auto &node : failed_nodes) {
      if (SetFailedOpCompileTask(node, task_para, compile_strategy) != SUCCESS) {
        return FAILED;
      }
    }
    // When every op compile successful, setting failed_id for ops of fusion failed.
    std::vector<ge::Node *> fusion_failed_nodes = failed_nodes;
    SetFusionFailedId(fusion_failed_nodes, pre_scope_id);
    task_para.fusion_nodes_map->erase(pre_scope_id);
  }
  FE_LOGD("tid[%lu], retry task_num[%zu].", GetCurThreadId(), task_para.failed_tasks.size());
  return SUCCESS;
}

Status TbeOpStoreAdapter::InitTbeFunctions(const PluginManagerPtr &plugin_manager_ptr) {
  const string TBE_SELECT_FORMAT_FUNC_NAME = "SelectTbeOpFormat";
  Status ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const te::TbeOpInfo &, std::string &>(
      TBE_SELECT_FORMAT_FUNC_NAME, SelectTbeOpFormat);

  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_CHECK_SUPPORTED_WITH_REASON_FUNC_NAME = "CheckTbeSupported";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, te::TbeOpInfo &, te::CheckSupportedResult &, std::string &>(
      TBE_CHECK_SUPPORTED_WITH_REASON_FUNC_NAME, CheckTbeSupported);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_PRE_COMPILER_FUNC_NAME = "PreBuildTbeOp";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, te::TbeOpInfo &, uint64_t, uint64_t>(
      TBE_PRE_COMPILER_FUNC_NAME, PreBuildTbeOp);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_GET_OP_INFO_FUNC_NAME = "GetOpInfo";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<te::LX_QUERY_STATUS, const te::TbeOpInfo &, std::string &>(
      TBE_GET_OP_INFO_FUNC_NAME, GetOpInfo);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_COMPILER_FUNC_NAME = "TeFusion";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<te::OpBuildResCode, std::vector<ge::Node *>, ge::OpDescPtr,
      const std::vector<ge::NodePtr> &, uint64_t, uint64_t, const std::string &>(
      TBE_COMPILER_FUNC_NAME, TeFusion);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_COMPILER_FUNC_NAME_V = "TeFusionV";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<te::OpBuildResCode, std::vector<ge::Node *>, ge::OpDescPtr,
      const std::vector<ge::NodePtr> &, uint64_t, uint64_t, uint64_t, const std::string &>(
      TBE_COMPILER_FUNC_NAME_V, TeFusionV);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_FUZZ_COMPILER_FUNC_NAME = "FuzzBuildTbeOp";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<te::OpBuildResCode, uint64_t, uint64_t, ge::Node &>(
      TBE_FUZZ_COMPILER_FUNC_NAME, FuzzBuildTbeOp);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_WAIT_FINISH_FUNC_NAME = "WaitAllFinished";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, uint64_t, vector<te::FinComTask> &>(
      TBE_WAIT_FINISH_FUNC_NAME, WaitAllFinished);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_INIT_FUNC_NAME = "TbeInitialize";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const std::map<std::string, std::string> &, bool *>(
      TBE_INIT_FUNC_NAME, TbeInitialize);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TBE_FINALIZE_FUNC_NAME = "TbeFinalize";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool>(TBE_FINALIZE_FUNC_NAME, TbeFinalize);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string CHECK_IS_TBE_GENERALIZEFUNC_REGISTERED = "CheckIsTbeGeneralizeFuncRegistered";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const te::TbeOpInfo &, bool &>(
      CHECK_IS_TBE_GENERALIZEFUNC_REGISTERED, CheckIsTbeGeneralizeFuncRegistered);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string TE_GENERALIZE = "TeGeneralize";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const te::TbeOpInfo &, \
      const te::TE_GENERALIZE_TYPE &, const ge::NodePtr &>(TE_GENERALIZE, TeGeneralize);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string GET_SPECIFIC_INFO = "GetOpSpecificInfo";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const te::TbeOpInfo &, \
      std::string &>(GET_SPECIFIC_INFO, GetOpSpecificInfo);
  if (ret != SUCCESS) {
    return FAILED;
  }

  const string DYNAMIC_SHAPE_RANGE_CHECK = "DynamicShapeRangeCheck";
  ret = plugin_manager_ptr->GetFunctionFromTbePlugin<bool, const te::TbeOpInfo &, \
      bool &, std::vector<size_t> &, std::vector<size_t> &>(DYNAMIC_SHAPE_RANGE_CHECK, DynamicShapeRangeCheck);
  if (ret != SUCCESS) {
    return FAILED;
  }

  return SUCCESS;
}

Status TbeOpStoreAdapter::InitializeInnerHelp() {
  FE_MAKE_SHARED(tbe_info_assembler_ptr_ = std::make_shared<TbeInfoAssembler>(), return FAILED);
  FE_CHECK(tbe_info_assembler_ptr_ == nullptr,
           REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc] tbeInfoAssemblerPtr_ is null."),
           return FAILED);

  FE_MAKE_SHARED(tbe_single_op_info_assembler_ptr_ = std::make_shared<TbeSingleOpInfoAssembler>(), return FAILED);
  FE_CHECK(tbe_single_op_info_assembler_ptr_ == nullptr,
           REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc] tbeSingleOpInfoAssemblerPtr_ is null."),
           return FAILED);
  if (tbe_single_op_info_assembler_ptr_->Initialize() != SUCCESS) {
    return FAILED;
  }
  init_flag = true;
  FE_LOGI("Initialize tbe op store adapter success.");
  return SUCCESS;
}

Status TbeOpStoreAdapter::InitializeInner(const std::map<std::string, std::string> &options,
                                          const std::string &engine_name) {
  // return SUCCESS if graph optimizer has been initialized.
  if (init_flag) {
    FE_LOGW("TbeOpStoreAdapter has been initialized.");
    return SUCCESS;
  }
  /* set the engine name first */
  engine_name_ = engine_name;

  string root_path = Configuration::Instance(engine_name).GetRootPath();
  FE_LOGD("Start to initialize tbe compiler adapter.");
  const string real_path = root_path + TBE_SO_NAME;

  FE_MAKE_SHARED(plugin_manager_ptr = std::make_shared<PluginManager>(TBE_SO_NAME), return FAILED);
  FE_CHECK(plugin_manager_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc]pluginManagerPtr is nullptr."),
           return FAILED);

  if (plugin_manager_ptr->OpenPlugin(real_path) != SUCCESS) {
    REPORT_FE_ERROR("[FEInit][OpPluginSo] Failed to open plugin so.");
    return FAILED;
  }

  Status ret = InitTbeFunctions(plugin_manager_ptr);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc]: Failed to initialize TbeFunctions.");
    return FAILED;
  }

  std::map<std::string, std::string> new_options;
  for (auto key_map_iter = TBE_INIT_OPTION_KEY_MAP.begin(); key_map_iter != TBE_INIT_OPTION_KEY_MAP.end();
       key_map_iter++) {
    auto option_iter = options.find(key_map_iter->first);
    if (option_iter != options.end()) {
      new_options.insert(std::pair<string, string>(key_map_iter->second, option_iter->second));
      FE_LOGD("Options for TbeInitialize:[%s, %s]", key_map_iter->second.c_str(), option_iter->second.c_str());
    }
  }

  AppendArgsMode append_args_mode = Configuration::Instance(engine_name_).GetAppendArgsMode();
  if (append_args_mode < AppendArgsMode::NO_ARGS) {
    REPORT_FE_ERROR(
        "[GraphOpt][InitializeInner][InitTbeFunc] The append_args_mode:%d is invalid,The append_args_mode \
        must greater than 0.", static_cast<int>(append_args_mode));
    return FAILED;
  }
  string append_args_mode_str = std::to_string(static_cast<int>(append_args_mode));
  new_options.insert(std::pair<string, string>(TBE_APPEND_ARGS_MODE, append_args_mode_str));
  new_options[kFeEngineType] = engine_name_;
  ChangeBufferOptimize(options, new_options);

  // add op_precision_str
  std::string op_select_impl_mode_str;
  Configuration::Instance(engine_name_).GetOpSelectImplModeStr(op_select_impl_mode_str);
  FE_LOGD("The parameter value of op_precision_mode_str is [%s]", op_select_impl_mode_str.c_str());
  if (!op_select_impl_mode_str.empty()) {
    new_options.insert(std::pair<string, string>(kOpPrecisionModeStr, op_select_impl_mode_str));
  }

  if (!TbeInitialize(new_options, &support_parallel_compile)) {
    REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc] Failed to init tbe.");
    if (plugin_manager_ptr->CloseHandle() != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][InitializeInner][InitTbeFunc] Failed to close tbe plugin handle.");
    }
    return FAILED;
  }

  if (InitializeInnerHelp() != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}
/*
 *  @ingroup fe
 *  @brief   initial resources needed by TbeOpStoreAdapter, such as dlopen so
 * files
 *           and load function symbols etc.
 *  @return  SUCCESS or FAILED
 */
Status TbeOpStoreAdapter::Initialize(const std::map<std::string, std::string> &options,
                                     const std::string &engine_name) {
  // return SUCCESS if graph optimizer has been initialized.
  Status result = InitializeInner(options, engine_name);
  if (result != SUCCESS) {
    if (plugin_manager_ptr != nullptr) {
      (void)plugin_manager_ptr->CloseHandle();
    }
    return result;
  }
  return SUCCESS;
}

void TbeOpStoreAdapter::ChangeBufferOptimize(const std::map<std::string, std::string> &options,
                                             std::map<std::string, std::string> &new_options) {
  auto iter = options.find(ge::BUFFER_OPTIMIZE);
  if (iter != options.end()) {
    if (iter->second == L2_OPTIMIZE) {
      BufferFusionMode buffer_fusion_mode = Configuration::Instance(engine_name_).GetBufferFusionMode();
      if (buffer_fusion_mode == EN_L2_FUSION) {
        new_options.insert(std::pair<string, string>(ge::BUFFER_OPTIMIZE, L2_OPTIMIZE));
      } else {
        new_options.insert(std::pair<string, string>(ge::BUFFER_OPTIMIZE, OFF_OPTIMIZE));
      }
    } else {
      new_options.insert(std::pair<string, string>(ge::BUFFER_OPTIMIZE, iter->second));
    }
  } else {
    new_options.insert(std::pair<string, string>(ge::BUFFER_OPTIMIZE, OFF_OPTIMIZE));
  }
}

/*
 *  @ingroup fe
 *  @brief   finalize resources initialized in Initialize function,
 *           such as dclose so files etc.
 *  @return  SUCCESS or FAILED
 */
Status TbeOpStoreAdapter::Finalize() {
  // return SUCCESS if graph optimizer has been initialized.
  if (!init_flag) {
    REPORT_FE_ERROR("[GraphOpt][Finalize] TbeOpStoreAdapter not allowed to finalize before initialized.");
    return FAILED;
  }

  FE_LOGD("Start to finalize tbe compiler adapter.");

  // release TBE resources
  if (!TbeFinalize()) {
    REPORT_FE_ERROR("[GraphOpt][Finalize] Release tbe resources failed.");
    return FAILED;
  }

  // close dlopen handler
  if (plugin_manager_ptr != nullptr) {
    if (plugin_manager_ptr->CloseHandle() != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Finalize] Failed to close tbe plugin handle.");
      return FAILED;
    }
  }

  init_flag = false;
  FE_LOGI("Finalize tbe op store adapter success.");
  return SUCCESS;
}

// we reset intput or output dtype when precision mode is allow_fp32_tofp16 and intput or
// output dtype is all supporrted fp16 in op store.
// input0.dtype=float16,int8,float16   ======>  update input dtype from fp32 to fp16
// input0.dtype=float,int8,float16   ======>  do not update input dtype from fp32 to fp16
bool TbeOpStoreAdapter::UpdateInputOrOutputDtype(const ge::OpDescPtr &op_desc, const ge::GeTensorDescPtr &tensor_desc,
                                                 const size_t input_or_output_index) const {
  if (tensor_desc == nullptr) {
    return false;
  }
  bool need_update_dtype_when_op_checksupport_flag = false;
  (void) ge::AttrUtils::GetBool(tensor_desc, NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT,
                                need_update_dtype_when_op_checksupport_flag);
  if (need_update_dtype_when_op_checksupport_flag) {
    FE_LOGD("Op[name=%s, type=%s]:Current precision mode is allow_fp32_tofp16, input_or_output_index[%zu] dtype \
            supported in op store include fp16 without fp32, modify dtype fp32 to fp16.",
            op_desc->GetName().c_str(), op_desc->GetType().c_str(), input_or_output_index);
    tensor_desc->SetDataType(ge::DT_FLOAT16);
    return true;
  }
  return false;
}

void TbeOpStoreAdapter::UpdateDtypeByAllowFp32ToFp16(const ge::OpDescPtr &op_desc,
    size_t input_or_output_index, std::pair<std::vector<size_t>, std::vector<size_t>> &in_out_changed_idx_vec,
    const bool &isinput) const {
  FE_LOGD("Current precision mode is allow_fp32_tofp16, update dtype.");
  std::vector<size_t> input_idx_vec;
  std::vector<size_t> output_idx_vec;
  for (size_t index = 0; index < input_or_output_index; ++index) {
    if (isinput) {
      ge::GeTensorDescPtr input_tensor_desc = op_desc->MutableInputDesc(index);
      if (UpdateInputOrOutputDtype(op_desc, input_tensor_desc, index)) {
        input_idx_vec.emplace_back(index);
      }
    } else {
      ge::GeTensorDescPtr output_tensor_desc = op_desc->MutableOutputDesc(index);
      if (UpdateInputOrOutputDtype(op_desc, output_tensor_desc, index)) {
        output_idx_vec.emplace_back(index);
      }
    }
  }
  if (isinput) {
    in_out_changed_idx_vec.first = input_idx_vec;
  } else {
    in_out_changed_idx_vec.second = output_idx_vec;
  }
}
Status TbeOpStoreAdapter::UpdateTensorByMixPrecisionMode(const ge::OpDescPtr &op_desc,
    const OpKernelInfoPtr &op_kernel_info_ptr,
    std::pair<std::vector<size_t>, std::vector<size_t>> &in_out_changed_idx_vec) const {
  /* If the Auto Mix precision switch is on, we need to do the
   * checksupport in op by fp16, when the current datatype is fp32 and
   * the op is in white list or current precision mode is force fp16 */
  FE_CHECK_NOTNULL(op_kernel_info_ptr);
  if (IsDtypeSensitiveOp(op_desc->GetType())) {
    return SUCCESS;
  }
  bool white_list_op = op_kernel_info_ptr->GetOpStoreInfo().precision_policy == WHITE;
  bool fp16_flag = (Configuration::Instance(engine_name_).GetAutoMixPrecisionSwitch() &&
                    white_list_op) ||
                   Configuration::Instance(engine_name_).GetPrecisionModeStr() == FORCE_FP16;

  bool bf16_flag = (Configuration::Instance(engine_name_).GetAutoMixPrecisionBF16Switch() &&
                    white_list_op) ||
                   Configuration::Instance(engine_name_).GetPrecisionModeStr() == FORCE_BF16;

  ge::DataType final_dtype;
  if (fp16_flag) {
    FE_LOGI("Node %s is in white list and the mix precision switch is on.", op_desc->GetName().c_str());
    final_dtype = ge::DT_FLOAT16;
  } else if (bf16_flag) {
    FE_LOGI("Node %s is in white list and the mix precision_bf16 switch is on.", op_desc->GetName().c_str());
    final_dtype = ge::DT_BF16;
  } else if (Configuration::Instance(engine_name_).GetPrecisionModeStr() == ALLOW_FP32_TO_FP16) {
    UpdateDtypeByAllowFp32ToFp16(op_desc, op_desc->GetAllInputsSize(), in_out_changed_idx_vec, true);
    UpdateDtypeByAllowFp32ToFp16(op_desc, op_desc->GetAllOutputsDescSize(), in_out_changed_idx_vec, false);
    return SUCCESS;
  } else {
    return SUCCESS;
  }

  std::vector<size_t> input_idx_vec;
  std::vector<size_t> output_idx_vec;
  for (size_t i = 0; i < op_desc->GetAllInputsSize(); i++) {
    auto input_desc = op_desc->MutableInputDesc(i);
    if (input_desc != nullptr && input_desc->GetDataType() == ge::DT_FLOAT) {
      input_desc->SetDataType(final_dtype);
      input_idx_vec.emplace_back(i);
    }
  }
  in_out_changed_idx_vec.first = input_idx_vec;
  for (size_t i = 0; i < op_desc->GetAllOutputsDescSize(); i++) {
    auto output_desc = op_desc->MutableOutputDesc(i);
    if (output_desc != nullptr && output_desc->GetDataType() == ge::DT_FLOAT) {
      output_desc->SetDataType(final_dtype);
      output_idx_vec.emplace_back(i);
    }
  }
  in_out_changed_idx_vec.second = output_idx_vec;
  return SUCCESS;
}

void RestoreDataType(const ge::OpDescPtr &op_desc,
    std::pair<std::vector<size_t>, std::vector<size_t>> &in_out_changed_idx_vec) {
  if (!in_out_changed_idx_vec.first.empty()) {
    for (auto &i : in_out_changed_idx_vec.first) {
      auto input_desc = op_desc->MutableInputDesc(i);
      input_desc->SetDataType(ge::DT_FLOAT);
    }
  }
  if (!in_out_changed_idx_vec.second.empty()) {
    for (auto &i : in_out_changed_idx_vec.second) {
      auto output_desc = op_desc->MutableOutputDesc(i);
      output_desc->SetDataType(ge::DT_FLOAT);
    }
  }
}

bool TbeOpStoreAdapter::AssembleTbeByMixPrecisionMode(const ge::NodePtr &node,
                                                      const OpKernelInfoPtr &op_kernel_info_ptr,
                                                      const bool &is_dynamic_impl,
                                                      te::TbeOpInfo &op_info) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  std::pair<std::vector<size_t>, std::vector<size_t>> in_out_changed_idx_vec;
  (void)UpdateTensorByMixPrecisionMode(op_desc_ptr, op_kernel_info_ptr, in_out_changed_idx_vec);
  op_info.SetDynamicImpl(is_dynamic_impl);
  if (tbe_info_assembler_ptr_->AssembleTbeInfo(node, op_kernel_info_ptr, engine_name_, op_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][CheckSupport][AssembleTbeInfo] Failed to Assemble tbe info for node %s tpye %s.",
                    op_name.c_str(), op_type.c_str());
    return false;
  }
  RestoreDataType(op_desc_ptr, in_out_changed_idx_vec);
  return true;
}
/*
 *  @ingroup fe
 *  @brief   check support something
 */
bool TbeOpStoreAdapter::CheckSupport(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                     const bool &is_dynamic_impl, std::string &reason) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  if (!op_kernel_info_ptr->GetNeedCheckSupportFlag()) {
    return true;
  }

  FE_LOGI("[ChkSpt][OpChk][Node %s, %s] Start to check in op implementation file.", op_name.c_str(), op_type.c_str());
  /* If this op is supported in ops store, we still need to check whether
   * it is supported by specific op plugin.
   * If the mix precision switch is on, we try the dtype float16 of this op if
   * the original dtype is fp32. */
  FEOpsStoreInfo op_store_info;
  if (GetTbeOpStoreInfo(op_desc, op_kernel_info_ptr, op_store_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][CheckSupport][GetTbeOpStoreInfo] Fail to get op store for op[%s].", op_name.c_str());
    return false;
  }

  bool custom_op_status = false;
  (void)ge::AttrUtils::GetBool(op_desc, NON_PERSISTENT_CUSTOM_OP_FLAG, custom_op_status);
  std::string op_dsl_file_path;
  bool ret_status = (custom_op_status && op_kernel_info_ptr != nullptr && !op_kernel_info_ptr->GetOpImpPath().empty());
  op_dsl_file_path = ret_status ? op_kernel_info_ptr->GetOpImpPath() : op_store_info.op_impl_file_path;

  te::TbeOpInfo op_info(op_name, op_dsl_file_path, op_type, "", engine_name_);
  if (!AssembleTbeByMixPrecisionMode(node, op_kernel_info_ptr, is_dynamic_impl, op_info)) {
    return false;
  }

  FE_CHECK(CheckTbeSupported == nullptr,
           REPORT_FE_ERROR("[GraphOpt][CheckSupport] Function CheckTbeSupported of TeFusion is nullptr."),
           return false);

  te::CheckSupportedResult is_supported = te::NOT_SUPPORTED;
  if (CheckTbeSupported(op_info, is_supported, reason)) {
    FE_LOGI("The result of check tbe supported of op[%s] is %s.", op_desc.GetName().c_str(),
            GetCheckSupportedString(is_supported).c_str());
    bool result = ConvertCheckSupportResult(node, reason, is_supported);
    if (result) {
      FE_LOGI("[ChkSpt][OpChk][Node %s, %s]This op is supported by implementation.", op_name.c_str(), op_type.c_str());
    } else {
      FE_LOGI("[ChkSpt][OpChk][Node %s, %s]This op is not supported by implementation. Reason is null.",
              op_name.c_str(), op_type.c_str());
    }
    return result;
  }
  FE_LOGI("Fail to invoke CheckTbeSupported of TeFusion.");
  return false;
}

bool TbeOpStoreAdapter::CheckUnsupportReason(const ge::NodePtr &node, const std::string &reason,
                                             te::CheckSupportedResult &is_supported) const {
  if (reason.find(SHAPE_NOT_SUPPORT) == 0) {
    is_supported = te::FULLY_SUPPORTED;
    (void)ge::AttrUtils::SetBool(node->GetOpDesc(), kOpShapeOrRangeUnsupport, true);
    FE_LOGD("Op[name=%s]:set attr shape_or_range_unsupport.", node->GetName().c_str());
    return true;
  }
  return false;
}

bool TbeOpStoreAdapter::ConvertCheckSupportResult(const ge::NodePtr &node,
                                                  const std::string &reason,
                                                  te::CheckSupportedResult &is_supported) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  if (is_supported == te::FULLY_SUPPORTED) {
    return true;
  } else if (is_supported == te::PARTIALLY_SUPPORTED) {
    (void)ge::AttrUtils::SetBool(op_desc_ptr, "partially_supported", true);
    FE_LOGD("Set attr partially supported to node[%s].", op_desc_ptr->GetName().c_str());
    return true;
  } else if (is_supported == te::NOT_SUPPORTED) {
    return CheckUnsupportReason(node, reason, is_supported);
  } else {
    return false;
  }
}

Status TbeOpStoreAdapter::GetLXOpCoreType(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                          const bool &is_dynamic_impl, std::string &lx_op_core_type_str) {
  string op_name = node->GetOpDesc()->GetName();
  string op_type = node->GetOpDesc()->GetType();
  FE_LOGD("Op[name=%s,type=%s]: start to GetLXOpCoreType.", op_name.c_str(), op_type.c_str());

  // 1. init the tbe_op_info
  FEOpsStoreInfo op_store_info;
  if (GetTbeOpStoreInfo(*(node->GetOpDesc().get()), op_kernel_info_ptr, op_store_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetLXOpCoreType][Op %s, type %s] fail to GetTbeOpStoreInfo.", op_name.c_str(),
                    op_type.c_str());
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

  te::TbeOpInfo tbe_op_info(op_name, op_dsl_file_path, op_type, "", engine_name_);
  tbe_op_info.SetDynamicImpl(is_dynamic_impl);
  // 2. assemble the information
  if (tbe_info_assembler_ptr_->AssembleTbeInfo(node, op_kernel_info_ptr, engine_name_, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetLXOpCoreType][Op %s, type %s] fail to assemble_tbe_info.", op_name.c_str(),
                    op_type.c_str());
    return FAILED;
  }

  // 3. call the function of TeFusion
  FE_CHECK(GetOpSpecificInfo == nullptr,
           REPORT_FE_ERROR("[GraphOpt][Setcheck][GetLXOpCoreType][Op %s, type %s] the function GetLXOpCoreType of \
           TeFusion is nullptr.", op_name.c_str(), op_type.c_str()),
           return FAILED);
  if (!GetOpSpecificInfo(tbe_op_info, lx_op_core_type_str)) {
    FE_LOGW("[GraphOpt][Setcheck][GetLXOpCoreType][Op %s, type %s] Fail to call op core type function.",
            op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  FE_LOGD("Op[name=%s,type=%s]: end to GetLXOpCoreType, lx_op_core_type_str=%s.", op_name.c_str(),
          op_type.c_str(), lx_op_core_type_str.c_str());
  return SUCCESS;
}

Status TbeOpStoreAdapter::SelectOpFormat(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                                         const bool &is_dynamic_impl, const HeavyFormatInfo &heavy_format_info,
                                         std::string &op_format_dtype_str) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  FE_LOGD("Op[name=%s,type=%s]: start to SelectOpFormat.", op_name.c_str(), op_type.c_str());

  // 1. init the tbe_op_info
  FEOpsStoreInfo op_store_info;
  if (GetTbeOpStoreInfo(op_desc, op_kernel_info_ptr, op_store_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][SeleFormat][Op %s, type %s] fail to GetTbeOpStoreInfo.", op_name.c_str(),
                    op_type.c_str());
    return FAILED;
  }

  bool is_custom_op = false;
  (void)ge::AttrUtils::GetBool(op_desc, NON_PERSISTENT_CUSTOM_OP_FLAG, is_custom_op);
  std::string op_dsl_file_path;
  bool ret_status = (is_custom_op && op_kernel_info_ptr != nullptr && !op_kernel_info_ptr->GetOpImpPath().empty());
  if (ret_status) {
    op_dsl_file_path = op_kernel_info_ptr->GetOpImpPath();
  } else {
    op_dsl_file_path = op_store_info.op_impl_file_path;
  }

  te::TbeOpInfo tbe_op_info(op_name, op_dsl_file_path, op_type, "", engine_name_);
  tbe_op_info.SetDynamicImpl(is_dynamic_impl);
  // 2. assemble the information
  if (tbe_info_assembler_ptr_->AssembleTbeInfo(op_desc, op_kernel_info_ptr, heavy_format_info, tbe_op_info,
                                               engine_name_) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][SeleFormat][Op %s, type %s] fail to assemble_tbe_info.", op_name.c_str(),
                    op_type.c_str());
    return FAILED;
  }

  // 3. call the function of TeFusion
  FE_CHECK(SelectTbeOpFormat == nullptr,
           REPORT_FE_ERROR("[GraphOpt][Setcheck][SeleFormat][Op %s, type %s] the function SelectTbeOpFormat of \
           TeFusion is nullptr.", op_name.c_str(), op_type.c_str()),
           return FAILED);
  if (!SelectTbeOpFormat(tbe_op_info, op_format_dtype_str)) {
    FE_LOGW("[GraphOpt][Setcheck][SeleFormat][Op %s, type %s] Fail to call op select format function.",
            op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  FE_LOGD("Op[name=%s,type=%s]: end to SelectOpFormat, op_format_dtype_str=%s.", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), op_format_dtype_str.c_str());
  return SUCCESS;
}

Status TbeOpStoreAdapter::GetTbeOpStoreInfo(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                                            FEOpsStoreInfo &op_store_info) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  OpImplType impl_type = op_kernel_info_ptr->GetOpStoreImplType();
  if (impl_type == EN_RESERVED) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetOpInfo][Op %s, type %s] the imply_type [%d] is invalid.", op_name.c_str(),
                    op_type.c_str(), impl_type);
    return FAILED;
  }

  if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(impl_type, op_store_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetOpInfo][Op %s, type %s] fail to get op store info by impl_type [%d].",
                    op_name.c_str(), op_type.c_str(), impl_type);
    return FAILED;
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::OpBuilder(ge::NodePtr node_ptr) { return SUCCESS; }

Status TbeOpStoreAdapter::SetOpCompileInfo(std::vector<ge::Node *> &nodes, const ge::OpDescPtr &op_desc_ptr) const {
  for (auto &node : nodes) {
    ge::OpDescPtr cur_op_desc_ptr = node->GetOpDesc();
    FE_CHECK_NOTNULL(cur_op_desc_ptr);
    string cur_op_type = cur_op_desc_ptr->GetType();
    string cur_op_name = cur_op_desc_ptr->GetName();
    int64_t is_unknown_shape_value = 0;
    (void)ge::AttrUtils::GetInt(cur_op_desc_ptr, ATTR_NAME_IS_UNKNOWN_SHAPE_OP, is_unknown_shape_value);
    FE_LOGD("Op[name=%s,type=%s]: is_unknown_shape flag is %ld.", cur_op_name.c_str(), cur_op_type.c_str(),
            is_unknown_shape_value);
    if (IsFeSupportedDynamicOp(*(cur_op_desc_ptr.get())) || is_unknown_shape_value == IS_UNKNOWN_SHAPE_VALUE) {
      std::string op_compile_info_json;
      std::string op_compile_info_key;
      if (ge::AttrUtils::GetStr(op_desc_ptr, COMPILE_INFO_JSON, op_compile_info_json)) {
        FE_LOGD("Compile info json after compiling is:%s", op_compile_info_json.c_str());
        (void)ge::AttrUtils::SetStr(cur_op_desc_ptr, COMPILE_INFO_JSON, op_compile_info_json);
      } else {
        FE_LOGW("Can not find op[name:%s,type:%s] compile_info_json after compiling.", cur_op_name.c_str(),
                cur_op_type.c_str());
      }

      if (ge::AttrUtils::GetStr(op_desc_ptr, COMPILE_INFO_KEY, op_compile_info_key)) {
        FE_LOGI("Op[name:%s,type:%s], Compile info key after compiling is:%s", cur_op_name.c_str(), cur_op_type.c_str(),
                op_compile_info_key.c_str());
        bool temp = ge::AttrUtils::SetStr(cur_op_desc_ptr, COMPILE_INFO_KEY, op_compile_info_key);
        if (temp) {
          FE_LOGI("succeed to SetStr Op[name:%s,type:%s], Compile info key after compiling is:%s", cur_op_name.c_str(),
                  cur_op_type.c_str(), op_compile_info_key.c_str());
        } else {
          FE_LOGW("Failed to Setstr Op Compile info key[name:%s,type:%s]", cur_op_name.c_str(), cur_op_type.c_str());
        }
      } else {
        FE_LOGW("Can not find op[name:%s,type:%s] compile_info_key after compiling.", cur_op_name.c_str(),
                cur_op_type.c_str());
      }
    }
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::SetSupportDynamicShape(std::vector<ge::Node *> &nodes) const {
  for (auto node : nodes) {
    FE_CHECK_NOTNULL(node->GetOpDesc());
    if (IsUnKnownShapeOp(*(node->GetOpDesc().get()))) {
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    }
  }
  return SUCCESS;
}

/* 1. If one thread of the node is failed and the node is optimized by lx-fusion,
 * we add it into vector need_rollback_nodes. (Although sgt slicing is conflict with
 * lx-fusion, we still use a set to store all to-be-rolled-back nodes to remove
 * duplicates.)
 * 2. If this node is not optimized by lx-fusion, we remove all other duplicated
 * failed tasks.
 * When we re-compile single op, for one sgt-sliced node, we will separate it
 * into several(two) tasks. */
Status TbeOpStoreAdapter::GetSgtSliceTaskRollbackNode(CompileTaskPara &task_para,
                                                      std::vector<ge::NodePtr> &need_rollback_nodes) const {
  if (task_para.failed_tasks.empty()) {
    return SUCCESS;
  }

  auto &pre_scope_id_map = task_para.task_scope_id_map;
  auto &failed_tasks = task_para.failed_tasks;
  unordered_set<ge::NodePtr> del_nodes;
  unordered_set<uint64_t> all_tasks_related_to_failed_task;

  for (auto task_itr = failed_tasks.begin(); task_itr != failed_tasks.end();) {
    auto task_id = task_itr->first;
    if (pre_scope_id_map.find(task_id) == pre_scope_id_map.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][GetSgtSliceTaskRlb] tid[%lu], not find taskId[%ld]", GetCurThreadId(),
                      task_id);
      return FAILED;
    }

    int64_t scope_id = pre_scope_id_map[task_id];
    auto &scope_task_ids_map = task_para.scope_task_ids_map;
    // if not find in scope_task_ids_map, it is a no slice node(normal node)
    // only slice nodes can be set to scope_task_ids_map in function named SetSgtSliceTaskToTeFusion
    // slice scopeid has 2 tasks at least, normal scopeid does not need to be rollbacked, only for protect
    auto task_ids = scope_task_ids_map.find(scope_id);
    if (task_ids == scope_task_ids_map.end() || task_ids->second.size() == 1) {
      FE_LOGD("tid[%lu], not find scope_id[%ld]", GetCurThreadId(), scope_id);
      task_itr++;
      continue;
    }

    // record all failed tasks for deleting from task_para.succ_tasks
    all_tasks_related_to_failed_task.insert(task_ids->second.cbegin(), task_ids->second.cend());

    auto &all_nodes_in_one_task = (*task_para.fusion_nodes_map)[scope_id];
    bool is_optimized_by_lxfusion = IsBuffFusOptimizedNodes(all_nodes_in_one_task);

    if (is_optimized_by_lxfusion) {
      // all nodes(in this scope) optimized by lxfusion need to be rolled back
      for (auto &op : all_nodes_in_one_task) {
        if (del_nodes.find(op->shared_from_this()) == del_nodes.end()) {
          del_nodes.insert(op->shared_from_this());
          FE_LOGD("Delete op name : %s, type : %s.", op->GetOpDesc()->GetName().c_str(), op->GetType().c_str());
        }
      }
      task_itr = failed_tasks.erase(task_itr);
    } else {
      //
      auto scope_able_to_del = task_para.failed_task_able_to_delete.find(scope_id);
      if (scope_able_to_del != task_para.failed_task_able_to_delete.end() &&
          scope_able_to_del->second) {
        FE_LOGD("Delete task %lu for scope %ld", task_itr->first, scope_id);
        task_itr = failed_tasks.erase(task_itr);
      } else {
        FE_LOGD("Do not delete task %lu for scope %ld. Set task to true.", task_itr->first, scope_id);
        task_para.failed_task_able_to_delete[scope_id] = true;
        task_itr++;
      }
    }
  }

  // succ_task needs to delete relative failed tasks
  // can be optimized
  for (auto &task_id : all_tasks_related_to_failed_task) {
    if (task_para.succ_tasks.find(task_id) != task_para.succ_tasks.end()) {
      FE_LOGD("Delete task_id %lu because one of its peer tasks failed.", task_id);
      task_para.succ_tasks.erase(task_id);
    }
  }

  need_rollback_nodes.insert(need_rollback_nodes.cend(), del_nodes.cbegin(), del_nodes.cend());
  return SUCCESS;
}

void CalcSliceShapeByRange(const std::vector<ffts::DimRange> &dim_range, ge::GeShape &slice_shape) {
  vector<int64_t> dims;
  for (auto &range : dim_range) {
    dims.emplace_back(range.higher - range.lower);
  }
  slice_shape = ge::GeShape(dims);
}

void SetSgtSliceShaeForEachTensor(size_t tensor_idx, int32_t thread_idx, const ge::Node *node,
                                  const ge::OpDesc::Vistor<ge::GeTensorDescPtr> &tensors,
                                  const vector<vector<vector<ffts::DimRange>>> &slice_info,
                                  const vector<vector<vector<int64_t>>> &ori_slice_shape, const string &attr_name) {

  vector<vector<int64_t>> slice_dims_head_tail;
  /* The shape is an array in which the first one is head slice shape and
   * the second one is the tail slice shape */
  (void)ge::AttrUtils::GetListListInt(tensors.at(tensor_idx), attr_name, slice_dims_head_tail);

  ge::GeShape slice_shape;

  if (attr_name == ATTR_NAME_SGT_SLICE_SHAPE) {
    if (slice_info.empty()) {
      FE_LOGD("Slice info is empty.");
      return;
    }
    CalcSliceShapeByRange(slice_info[thread_idx][tensor_idx], slice_shape);
  } else {
    if (ori_slice_shape.empty()) {
      FE_LOGD("Slice info is empty.");
      return;
    }
    slice_shape = ge::GeShape(ori_slice_shape[thread_idx][tensor_idx]);
  }

  slice_dims_head_tail.emplace_back(slice_shape.GetDims());
  auto tensor = tensors.at(tensor_idx);
  (void)ge::AttrUtils::SetListListInt(tensor, attr_name, slice_dims_head_tail);
  FE_LOGD("optype:%s, opname:%s, set thread %d's slice shape %s for tensor %s, tensor index %zu",
          node->GetType().c_str(), node->GetName().c_str(), thread_idx,
          StringUtils::IntegerVecToString(slice_shape.GetDims()).c_str(),
          tensor->GetName().c_str(), tensor_idx);
  FE_LOGD("Original shape is %s shape is %s",
          StringUtils::IntegerVecToString(tensor->GetOriginShape().GetDims()).c_str(),
          StringUtils::IntegerVecToString(tensor->MutableShape().GetDims()).c_str());

}

Status TbeOpStoreAdapter::SetSgtTensorSliceInfoToNodes(std::vector<ge::Node*> &compile_nodes,
                                                       int32_t thread_idx) const {
  for (const auto &node : compile_nodes) {
    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
    if (slice_info_ptr == nullptr) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SetSgtTensSliceInfo] This node has no slice info. op_type:%s, op_name:%s",
                      node->GetType().c_str(), node->GetName().c_str());
      return FAILED;
    }
    // set slice shape to tensor
    auto input_tensors = node->GetOpDesc()->GetAllInputsDescPtr();
    FE_LOGD("Set input slice attribute for node %s.", node->GetName().c_str());
    for (size_t i = 0; i < input_tensors.size(); i++) {
      SetSgtSliceShaeForEachTensor(i, thread_idx, node, input_tensors,
          slice_info_ptr->input_tensor_slice,
          slice_info_ptr->ori_input_tensor_shape, ATTR_NAME_SGT_SLICE_SHAPE);
      SetSgtSliceShaeForEachTensor(i, thread_idx, node, input_tensors,
          slice_info_ptr->ori_input_tensor_slice,
          slice_info_ptr->ori_input_tensor_shape, ATTR_NAME_SGT_ORI_SLICE_SHAPE);
    }

    auto output_tensors = node->GetOpDesc()->GetAllOutputsDescPtr();
    FE_LOGD("Set output slice attribute for node %s.", node->GetName().c_str());
    for (size_t i = 0; i < output_tensors.size(); i++) {
      SetSgtSliceShaeForEachTensor(i, thread_idx, node, output_tensors,
          slice_info_ptr->output_tensor_slice,
          slice_info_ptr->ori_output_tensor_shape, ATTR_NAME_SGT_SLICE_SHAPE);
      SetSgtSliceShaeForEachTensor(i, thread_idx, node, output_tensors,
          slice_info_ptr->ori_output_tensor_slice,
          slice_info_ptr->ori_output_tensor_shape, ATTR_NAME_SGT_ORI_SLICE_SHAPE);
    }
  }
  return SUCCESS;
}

void ClearSgtAttr(std::vector<ge::Node *> &nodes) {
  for (auto node : nodes) {
    auto op_desc = node->GetOpDesc();
    size_t input_size = op_desc->GetAllInputsSize();
    for (size_t i = 0; i < input_size; i++) {
      auto input_desc =  op_desc->MutableInputDesc(i);
      if (input_desc == nullptr) {
        continue;
      }
      vector<vector<int64_t>> sgt_slice;
      vector<vector<int64_t>> empty_sgt_slice;
      if (ge::AttrUtils::GetListListInt(input_desc, ATTR_NAME_SGT_SLICE_SHAPE, sgt_slice)) {
        (void)ge::AttrUtils::SetListListInt(input_desc, ATTR_NAME_SGT_SLICE_SHAPE, empty_sgt_slice);
      }
    }

    size_t output_size = op_desc->GetAllOutputsDescSize();
    for (size_t i = 0; i < output_size; i++) {
      auto output_desc =  op_desc->MutableOutputDesc(i);
      if (output_desc == nullptr) {
        continue;
      }
      vector<vector<int64_t>> empty_sgt_slice;
      if (ge::AttrUtils::HasAttr(output_desc, ATTR_NAME_SGT_SLICE_SHAPE)) {
        (void)ge::AttrUtils::SetListListInt(output_desc, ATTR_NAME_SGT_SLICE_SHAPE, empty_sgt_slice);
      }
    }
  }
}

void SetThreadNodeName(const std::vector<ge::Node *> &nodes, vector<string> &old_names, const int32_t &i) {
  for (size_t j = 0; j < nodes.size(); j++) {
    string old_name = nodes[j]->GetOpDesc()->GetName();
    old_names.push_back(old_name);
    nodes[j]->GetOpDesc()->SetName(old_name + "_thread_" + to_string(i));
  }
}

void SetNameForNodes(const vector<ge::Node *> &nodes,
                     const vector<string> &names) {
  for (size_t i = 0; i < nodes.size(); i++) {
    nodes[i]->GetOpDesc()->SetName(names[i]);
  }
}

Status TbeOpStoreAdapter::SetTaskForOneScope(std::vector<ge::Node *> &nodes,
                                             const int64_t scope_id,
                                             const std::vector<ge::NodePtr> &to_del_nodes,
                                             CompileTaskPara &task_para,
                                             const CompileStrategy &compile_strategy) {
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = nodes[0]->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  string first_node_name = nodes[0]->GetName();
  // normal nodes
  if (!OpIsAutoThread(slice_info_ptr)) { // normal op or manual mode
    task_para.task_num++;
    uint64_t taskId = GetAtomicId();
    task_para.task_scope_id_map.insert(std::make_pair(taskId, scope_id));
    FE_LOGD("%lu, taskId %lu , scope_id %ld, set compile %s task", GetCurThreadId(), taskId, scope_id,
            first_node_name.c_str());
    // set compile task
    if (SetTeTask(nodes, taskId, to_del_nodes, compile_strategy) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SetSgtSliceTask] The op[%s] set compile task failed",
                      first_node_name.c_str());
      return FAILED;
    }
  } else { // slice nodes
    // slice nodes need to clear attribute ATTR_NAME_SGT_SLICE_SHAPE
    // and ATTR_NAME_SGT_ORI_SLICE_SHAPE
    ClearSgtAttr(nodes);

    int32_t slice_size = static_cast<int32_t>(slice_info_ptr->input_tensor_slice.size());
    uint64_t slice_shape_index = 0;
    FE_LOGD("Set slice(size: %d) shape for scope %ld, first node %s.", slice_size, scope_id, first_node_name.c_str());

    for (auto &scope_task_ids : task_para.scope_task_ids_map) {
      FE_LOGD("%s", StringUtils::IntegerVecToString(scope_task_ids.second).c_str());
    }
    for (int32_t i = 0; i < slice_size; i++) {
      if (i != 0 && i != (slice_size - 1)) {
        /* Here we only care the head slice and the tail slice because all
         * middle slices are same. */
        continue;
      }
      // every thread slice needs to be compiled once
      SetSgtTensorSliceInfoToNodes(nodes, i);
      task_para.task_num++;
      uint64_t taskId = GetAtomicId();

      task_para.task_scope_id_map.insert(std::make_pair(taskId, scope_id));
      auto scope_task_iter = task_para.scope_task_ids_map.find(scope_id);
      if (scope_task_iter != task_para.scope_task_ids_map.end()) {
        scope_task_iter->second.emplace_back(taskId);
        FE_LOGI("Slice size is %d.", slice_size);
      } else {
        vector<uint64_t> task_id_vec = {taskId};
        task_para.scope_task_ids_map.emplace(scope_id, task_id_vec);
      }

      FE_LOGD("%lu, taskId %lu, scope_id %ld, set slice %d compile %s task", GetCurThreadId(), taskId,
              scope_id, i, first_node_name.c_str());

      // Before compilation, we need to give all nodes a thread node
      // name to find all the precomp information.
      vector<string> old_names;
      SetThreadNodeName(nodes, old_names, i);
      // set compile task
      if (SgtSetTeTask(nodes, taskId, to_del_nodes,
                       compile_strategy, slice_shape_index) != SUCCESS) {
        REPORT_FE_ERROR("[SubGraphOpt][Compile][SetSgtSliceTask] The op[%s] set compile task failed",
                        first_node_name.c_str());
        SetNameForNodes(nodes, old_names);
        return FAILED;
      }
      SetNameForNodes(nodes, old_names);
      slice_shape_index++;
    }
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::SetSgtSliceTaskToTeFusion(CompileTaskPara &task_para,
                                                    const std::vector<ge::NodePtr> &to_del_nodes) {
  for (auto &scope_task_ids : task_para.scope_task_ids_map) {
    FE_LOGD("start read scope_task_ids_map, first: %ld, second: %s.", scope_task_ids.first,
            StringUtils::IntegerVecToString(scope_task_ids.second).c_str());
  }
  // iter: {scope id, node vector}
  for (auto &iter : *task_para.fusion_nodes_map) {
    if (SetTaskForOneScope(iter.second, iter.first, to_del_nodes, task_para,
                           CompileStrategy::COMPILE_STRATEGY_OP_SPEC) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][SetTsk]Failed to set task for scope %ld with first node %s",
                      iter.first, iter.second[0]->GetName().c_str());
      return FAILED;
    }
  }

  for (auto &scope_task_ids : task_para.scope_task_ids_map) {
    FE_LOGD("End read scope_task_ids_map, first: %ld, second: %s.", scope_task_ids.first,
            StringUtils::IntegerVecToString(scope_task_ids.second).c_str());
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::ProcessSuccSgtSliceTask(CompileTaskPara &task_para) {
  auto &scope_task_ids_map = task_para.scope_task_ids_map;
  FE_LOGD("size of scope_task_ids_map is %zu.", scope_task_ids_map.size());
  for (auto &scope_tasks_pair : scope_task_ids_map) {
    int64_t scope_id = scope_tasks_pair.first;
    vector<uint64_t> &task_ids = scope_tasks_pair.second;
    FE_LOGD("All task id for scope id %lu is %s", scope_id, StringUtils::IntegerVecToString(task_ids).c_str());
    // filter failed tasks
    if (task_para.succ_tasks.find(task_ids.at(0)) == task_para.succ_tasks.end()) {
      continue;
    }

    // set every json path and compileinfo
    for (auto &task_id : task_ids) {
      auto fin_task_itr = task_para.succ_tasks.find(task_id);
      if (fin_task_itr == task_para.succ_tasks.end()) {
        REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcSucSgtSliceTask] Thread[%lu], not find taskId[%ld] in success \
                        tasks", GetCurThreadId(), task_id);
        return FAILED;
      }
      FE_LOGD("Process sgt task with first node %s.", fin_task_itr->second.teNodeOpDesc->GetName().c_str());

      FE_LOGD("tid[%lu], get taskId[%lu], scope_id[%ld]", GetCurThreadId(), task_id, scope_id);
      if (SetSgtOpJsonPath(fin_task_itr->second.teNodeOpDesc, *task_para.json_path_map, scope_id) == FAILED) {
        REPORT_FE_ERROR("[SubGraphOpt][Compile][ProcSucSgtSliceTask] %s set op json path failed",
                        (*task_para.fusion_nodes_map)[scope_id][0]->GetName().c_str());
        return FAILED;
      }
      task_para.succ_tasks.erase(task_id);
    }
  }
  FE_LOGD("process sgt success task_num[%zu]. tid[%lu]", task_para.succ_tasks.size(), GetCurThreadId());
  return SUCCESS;
}

Status TbeOpStoreAdapter::CompileMultiKernelSliceOp(ScopeNodeIdMap &fusion_nodes_map,
                                                    map<int64_t, std::string> &json_path_map,
                                                    std::vector<ge::NodePtr> &compile_failed_nodes,
                                                    const std::vector<ge::NodePtr> &to_del_nodes) {
  FE_CHECK(TeFusionV == nullptr, REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] TeFusionV is nullptr."),
           return FAILED);
  FE_TIMECOST_START(TeFusionV);
  CompileTaskPara task_para = {.task_num = 0, .json_path_map = &json_path_map, .fusion_nodes_map = &fusion_nodes_map};

  if (SetSgtSliceTaskToTeFusion(task_para, to_del_nodes) != SUCCESS) {
    return FAILED;
  }

  FE_LOGD("Thread[%lu], to set %lu tasks to comp", GetCurThreadId(), task_para.task_num);

  // wait for finish
  if (WaitTaskFinish(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] Thread[%lu] wait task finish failed", GetCurThreadId());
    return FAILED;
  }
  // get need recovered nodes
  if (GetSgtSliceTaskRollbackNode(task_para, compile_failed_nodes) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] Thread[%lu] get recover task failed", GetCurThreadId());
    return FAILED;
  }

  // process success slice task
  if (ProcessSuccSgtSliceTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] Thread[%lu] process success, task failed",
                    GetCurThreadId());
    return FAILED;
  }

  // process success normal task
  if (ProcessSuccCompileTask(task_para) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] Thread[%lu] process success, task failed",
                    GetCurThreadId());
    return FAILED;
  }
  // failed tasks with no slicemap need to be recompiled
  if (!task_para.failed_tasks.empty()) {
    SaveMsTuneErrorMsg(task_para);
    if (RetryCompileFailOp(task_para) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][CompSgtSliceOp] Thread[%lu] retry compile fail op failed",
                      GetCurThreadId());
      return FAILED;
    }
  }

  FE_TIMECOST_END(TeFusionV, "TeFusion during FEGraphOptimizer::OptimizeFusedGraph");
  FE_LOGI("TbeOpStoreAdapter::Compile Op success. tid:%lu", GetCurThreadId());

  return SUCCESS;
}
Status TbeOpStoreAdapter::SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) {
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);

  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();

  FE_LOGI("[ChkSpt][OpChk][Node %s type %s] Start to check in op implementation file.", op_name.c_str(),
          op_type.c_str());

  FEOpsStoreInfo op_store_info;
  if (GetTbeOpStoreInfo(*op_desc_ptr, op_kernel_info_ptr, op_store_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][CheckSupport][GetTbeOpStoreInfo] Op[%s,optype[%s]]: fail to GetTbeOpStoreInfo.",
                    op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }

  bool is_custom_op = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, NON_PERSISTENT_CUSTOM_OP_FLAG, is_custom_op);
  std::string op_dsl_file_path;
  bool ret_status = (is_custom_op && op_kernel_info_ptr != nullptr && !op_kernel_info_ptr->GetOpImpPath().empty());
  if (ret_status) {
    op_dsl_file_path = op_kernel_info_ptr->GetOpImpPath();
  } else {
    op_dsl_file_path = op_store_info.op_impl_file_path;
  }

  te::TbeOpInfo op_info(op_name, op_dsl_file_path, op_type, "", engine_name_);
  if (tbe_info_assembler_ptr_->AssembleTbeInfo(node_ptr, op_kernel_info_ptr, engine_name_, op_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][CheckSupport][AssembleTbeInfo] Failed to Assemble tbe info for node %s tpye %s.",
                    op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }

  if (IsFeSupportedDynamicOp(*op_desc_ptr)) {
    op_info.SetIsUnknownShape(true);
  }
  bool is_dynamic_impl = IsOpDynamicImpl(op_desc_ptr);
  op_info.SetDynamicImpl(is_dynamic_impl);
  FE_CHECK(GetOpInfo == nullptr,
           REPORT_FE_ERROR("[GraphOpt][CheckSupport] Function CheckTbeSupported of TeFusion is nullptr."),
           return SUCCESS);

  string op_slice_info_str;
  Status status = GetOpInfo(op_info, op_slice_info_str);
  if (status == te::LX_QUERY_SUCC) {
    (void)ge::AttrUtils::SetStr(op_desc_ptr, OP_SLICE_INFO, op_slice_info_str);
    FE_LOGD("Obtain slice info %s from tbe api for node[%s].", op_slice_info_str.c_str(),
            op_name.c_str());
  } else {
    FE_LOGD("Not obtain slice info from tbe api for node[%s].", op_name.c_str());
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::GeneralizeNode(const ge::NodePtr &node, const te::TbeOpInfo &op_info,
    te::TE_GENERALIZE_TYPE generalize_type) {
  FE_LOGI("Begin to generalize node[%s, %s].", node->GetName().c_str(), node->GetType().c_str());
  auto op_desc = node->GetOpDesc();
  FE_CHECK(op_desc == nullptr,
           FE_LOGW("[GraphOptimizePrepare][ShapeAndValueGeneralize][GeneralizeGraph]\
                   Thread[%lu] Get op_desc from node failed.", GetCurThreadId()),
           return FAILED);

  FE_LOGD("Begin to run function[TeGeneralize], node[%s, %s].", node->GetName().c_str(), node->GetType().c_str());
  if (!TeGeneralize(op_info, generalize_type, node)) {
    FE_LOGW("[GraphOptimizePrepare][ShapeAndValueGeneralize][GeneralizeGraph]\
            GeneralizeNode failed, op_type is [%s].", op_desc->GetType().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::GetRangeLimitType(const ge::NodePtr &node_ptr, const te::TbeOpInfo &tbe_op_info,
                                            bool &is_limited) {
  std::string node_name = node_ptr->GetName();
  std::string specific_info_str;
  if (!GetOpSpecificInfo(tbe_op_info, specific_info_str)) {
    FE_LOGW("[GraphOptimizePrepare][Generalize] GetOpSpecificInfo for node[%s] failed.", node_name.c_str());
    return FAILED;
  }

  try {
    nlohmann::json specific_info = nlohmann::json::parse(specific_info_str);
    std::string limit_type = specific_info.at("rangeLimit").get<std::string>();
    if (limit_type == "limited") {
      is_limited = true;
    } else if (limit_type == "unlimited") {
      is_limited = false;
    } else {
      FE_LOGW("[GraphOptimizePrepare][Generalize] node[%s] rangeLimit[%s] from tbe is invalid.",
              node_name.c_str(), limit_type.c_str());
      is_limited = false;
    }
  } catch (std::exception &e) {
    FE_LOGW("[GraphOptimizePrepare][Generalize] parse json_str failed, string is %s and the reason is %s.",
            specific_info_str.c_str(), e.what());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::LimitedNodesCheck(bool &is_support, const te::TbeOpInfo &tbe_op_info,
    std::vector<size_t> &upper_limited_input_indexs, std::vector<size_t> &lower_limited_input_indexs) {
  if (!DynamicShapeRangeCheck(tbe_op_info, is_support, upper_limited_input_indexs, lower_limited_input_indexs)) {
    return FAILED;
  }
  return SUCCESS;
}

Status TbeOpStoreAdapter::IsGeneralizeFuncRegistered(bool &is_registered, const te::TbeOpInfo &op_info) {
  FE_CHECK(!CheckIsTbeGeneralizeFuncRegistered(op_info, is_registered),
           FE_LOGW("[GraphOptimizePrepare][ShapeAndValueGeneralize][GeneralizeGraph]\
                   Thread[%lu] check isRegistered by op_type failed.", GetCurThreadId()),
           return FAILED);
  return SUCCESS;
}
}  // namespace fe
