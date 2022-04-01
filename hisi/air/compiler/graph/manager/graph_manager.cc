/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/manager/graph_manager.h"

#include <pthread.h>
#include <algorithm>
#include <future>
#include <set>
#include <sstream>
#include <string>
#include <thread>

#include "common/math/math_util.h"
#include "common/thread_pool.h"
#include "common/dump/dump_manager.h"
#include "opt_info/ge_opt_info.h"
#include "analyzer/analyzer.h"
#include "common/ge_call_wrapper.h"
#include "common/local_context.h"
#include "common/transop_util.h"
#include "graph/ge_context.h"
#include "graph/ge_global_options.h"
#include "graph/manager/util/rt_context_util.h"
#include "graph/partition/pipeline_partition.h"
#include "graph/partition/dynamic_shape_partition.h"
#include "graph/passes/enter_pass.h"
#include "graph/partition/stage_partition.h"
#include "graph/passes/set_ffts_plus_attr_pass.h"
#include "graph/passes/addn_pass.h"
#include "graph/passes/bitcast_pass.h"
#include "graph/passes/assign_remove_pass.h"
#include "graph/passes/inplace_support_check_pass.h"
#include "graph/passes/atomic_addr_clean_pass.h"
#include "graph/passes/attach_stream_label_pass.h"
#include "graph/passes/cast_remove_pass.h"
#include "graph/passes/common_subexpression_elimination_pass.h"
#include "graph/passes/compile_nodes_pass.h"
#include "graph/passes/cond_remove_pass.h"
#include "graph/passes/constant_folding_pass.h"
#include "graph/passes/constant_fuse_same_pass.h"
#include "graph/passes/control_trigger_pass.h"
#include "graph/passes/ctrl_edge_transfer_pass.h"
#include "graph/passes/dimension_adjust_pass.h"
#include "graph/passes/dimension_compute_pass.h"
#include "graph/passes/data_flow_prepare_pass.h"
#include "graph/passes/flow_ctrl_pass.h"
#include "graph/passes/fuse_data_nodes_with_common_input_pass.h"
#include "graph/passes/hccl_tailing_optimization_pass.h"
#include "graph/passes/identity_pass.h"
#include "graph/passes/input_output_connection_identify_pass.h"
#include "graph/passes/iterator_op_pass.h"
#include "graph/passes/link_gen_mask_nodes_pass.h"
#include "graph/passes/mark_graph_unknown_status_pass.h"
#include "graph/passes/merge_pass.h"
#include "graph/passes/merge_input_memcpy_pass.h"
#include "graph/passes/merge_to_stream_merge_pass.h"
#include "graph/passes/merge_unknown_shape_n_pass.h"
#include "graph/passes/multi_batch_pass.h"
#include "graph/passes/subgraph_multi_dims_pass.h"
#include "graph/passes/next_iteration_pass.h"
#include "graph/passes/permute_pass.h"
#include "graph/passes/prune_pass.h"
#include "graph/passes/ref_identity_delete_op_pass.h"
#include "graph/passes/remove_same_const_pass.h"
#include "graph/passes/reshape_recovery_pass.h"
#include "graph/passes/reshape_remove_pass.h"
#include "graph/passes/same_transdata_breadth_fusion_pass.h"
#include "graph/passes/subgraph_pass.h"
#include "graph/passes/switch_data_edges_bypass.h"
#include "graph/passes/switch_dead_branch_elimination.h"
#include "graph/passes/switch_logic_remove_pass.h"
#include "graph/passes/switch_to_stream_switch_pass.h"
#include "graph/passes/start_of_sequence_pass.h"
#include "graph/passes/transop_breadth_fusion_pass.h"
#include "graph/passes/transop_nearby_allreduce_fusion_pass.h"
#include "graph/passes/transop_symmetry_elimination_pass.h"
#include "graph/passes/transop_without_reshape_fusion_pass.h"
#include "graph/passes/transpose_transdata_pass.h"
#include "graph/passes/useless_control_out_remove_pass.h"
#include "graph/passes/variable_op_pass.h"
#include "graph/passes/variable_ref_delete_op_pass.h"
#include "graph/passes/variable_ref_useless_control_out_delete_pass.h"
#include "graph/passes/end_of_sequence_add_control_pass.h"
#include "graph/passes/subexpression_migration_pass.h"
#include "graph/passes/subgraph_const_migration_pass.h"
#include "graph/passes/unused_args_clean_pass.h"
#include "graph/passes/global_step_insert_pass.h"
#include "graph/passes/memcpy_addr_async_pass.h"
#include "graph/passes/hccl_continuous_memcpy_pass.h"
#include "graph/passes/replace_with_empty_const_pass.h"
#include "graph/passes/parallel_group_pass.h"
#include "graph/passes/buffer_pool_memory_pass.h"
#include "graph/passes/mds_pass.h"
#include "graph/passes/swap_space_pass.h"
#include "graph/build/label_allocator.h"
#include "graph/utils/tensor_adapter.h"
#include "inc/pass_manager.h"
#include "init/gelib.h"
#include "ir_build/option_utils.h"
#include "common/local_context.h"
#include "common/omg_util.h"
#include "common/formats/utils/formats_trans_utils.h"
#include "register/custom_pass_helper.h"
#include "common/util/error_manager/error_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "pne/npu/npu_process_node_engine.h"
#include "pne/cpu/cpu_process_node_engine.h"
#include "pne/manager/process_node_engine_manager.h"

namespace {
const char *const kSummary = "Summary";
const char *const kSave = "Save";
const char *const kNetOutput = "NetOutput";
const char *const kVariable = "Variable";
const char *const kSend = "Send";
const char *const kRecv = "Recv";
const char *const kCheckPointForGetVar = "CheckPointGraphForGetVar";
const char *const kCheckPointGraph = "checkpoint_graph";
const char *const kVectorEngine = "VectorEngine";
const char *const kAIcoreEngine = "AIcoreEngine";
const int32_t kDynamicDimsTypeIsGetNext = 0;
const int32_t kDynamicDimsTypeIsData = 1;
const int32_t kBase = 10;
const char *const kGetNextName = "IteratorV2";
const uint32_t kInitGraphCount = 1;
const uint32_t kNotAdded = 0;
const uint32_t kStartAdd = 1;
const uint32_t kDoneAdded = 2;

bool IsTailingOptimization() {
  std::string is_tailing_optimization_option;
  auto ret = ge::GetContext().GetOption(ge::OPTION_EXEC_ENABLE_TAILING_OPTIMIZATION, is_tailing_optimization_option);
  if (ret == ge::GRAPH_SUCCESS) {
    GELOGI("Option ge.exec.isTailingOptimization is %s", is_tailing_optimization_option.c_str());
    // "1" means it's True from frontend option
    return is_tailing_optimization_option == "1";
  }
  GELOGW("OPTION_EXEC_ENABLE_TAILING_OPTIMIZATION not set, use BFSTopologicalSorting by default.");
  return false;
}

ge::Status CheckFpCeilingMode() {
  static const std::set<std::string> kValidFpCeilingMode = {"0", "1", "2"};
  std::string mode;
  auto ret = ge::GetContext().GetOption("ge.fpCeilingMode", mode);
  if (ret == ge::GRAPH_SUCCESS) {
    if (kValidFpCeilingMode.count(mode) == 0) {
      REPORT_INNER_ERROR("E19999", "Option ge.fpCeilingMode is invalid, value:%s", mode.c_str());
      GELOGE(ge::GE_GRAPH_OPTIONS_INVALID, "[Get][Option] The fp_ceiling_mode %s is invalid, options are 0, 1, and 2.",
             mode.c_str());
      return ge::GE_GRAPH_OPTIONS_INVALID;
    }
    GELOGI("The parameter fp_ceiling_mode is set to %s.", mode.c_str());
    return ge::SUCCESS;
  }
  GELOGW("The parameter fp_ceiling_mode is not set");
  return ge::SUCCESS;
}

ge::Status ProcessNodeEnginePartion(const ge::GraphNodePtr &graph_node, const ge::ComputeGraphPtr &compute_graph,
                                    ge::FlowModelPtr &flow_model) {
  GE_CHECK_NOTNULL(compute_graph);
  GE_CHECK_NOTNULL(graph_node);
  // 1.orgion graph standardization and infershape
  // 2.process node engine partion
  // 3.flow model releation generation
  auto flow_root_compute_graph = ge::MakeShared<ge::ComputeGraph>(compute_graph->GetName());
  auto flow_root_model = ge::MakeShared<ge::FlowModel>(flow_root_compute_graph);
  GE_CHECK_NOTNULL(flow_root_compute_graph);
  GE_CHECK_NOTNULL(flow_root_model);

  std::string process_node_engine_id = ge::PNE_ID_NPU;
  if (ge::GetContext().GetHostExecFlag()) {
    process_node_engine_id = ge::PNE_ID_CPU;
  }
  compute_graph->SetName(compute_graph->GetName() + "_" + process_node_engine_id);
  (void)ge::AttrUtils::SetStr(compute_graph, ge::ATTR_NAME_PROCESS_NODE_ENGINE_ID, process_node_engine_id);
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  graph_node->SetComputeGraph(flow_root_compute_graph);
  graph_node->SetFlowModel(flow_root_model);
  flow_model = flow_root_model;
  return ge::SUCCESS;
}

ge::Status InitProcessNodeEngines(const std::map<std::string, std::string> &options,
                                  ge::GraphManager *impl,
                                  std::map<std::string, ge::ProcessNodeEnginePtr> &process_node_engines) {
  if (ge::ProcessNodeEngineManager::GetInstance().GetEngines().empty()) {
    GELOGW("[Initialize][ProcessNodeEngine] is not registereds.");
    REGISTER_PROCESS_NODE_ENGINE(ge::PNE_ID_NPU, NPUProcessNodeEngine);
  }

  for (auto process_node_engine_pair : ge::ProcessNodeEngineManager::GetInstance().GetEngines()) {
    GE_CHECK_NOTNULL(process_node_engine_pair.second);
    auto engine_id = process_node_engine_pair.first;
    // every graph manger has one ProcessNodeEngine instance
    auto pne = ge::ProcessNodeEngineManager::GetInstance().CloneEngine(engine_id);
    if (pne != nullptr) {
      process_node_engines[engine_id] = pne;
      const ge::Status ret = pne->Initialize(options);
      if (ret != ge::SUCCESS) {
        GELOGE(ret, "[Initialize][ProcessNodeEngine] %s failed.", pne->GetEngineName().c_str());
        return ret;
      }
      // special ProcessNodeEngine process
      auto npu_pne = std::dynamic_pointer_cast<ge::NPUProcessNodeEngine>(pne);
      if (npu_pne != nullptr) {
        npu_pne->SetImpl(impl);
      } else {
        auto cpu_pne = std::dynamic_pointer_cast<ge::CPUProcessNodeEngine>(pne);
        if (cpu_pne != nullptr) {
          cpu_pne->SetImpl(impl);
        }
      }
    }
  }
  return ge::SUCCESS;
}
}  // namespace

namespace ge {
Status GraphManager::Initialize(const std::map<std::string, std::string> &options, Executor *executor) {
  ErrorManager::GetInstance().SetStage(error_message::kInitialize, error_message::kOther);
  if (init_flag_) {
    GELOGW("[Initialize] GraphManager already initialized.");
    return SUCCESS;
  }
  // graph context
  graph_context_ = MakeShared<GraphContext>();
  if (graph_context_ == nullptr) {
    REPORT_CALL_ERROR("E19999", "New GraphContext fail");
    GELOGE(MEMALLOC_FAILED, "[New][GraphContext] failed.");
    return MEMALLOC_FAILED;
  }

  // parse option parameters
  Status ret = ParseOptions(options);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Parse][Options] failed.");
    return ret;
  }

  ret = CheckFpCeilingMode();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Check][FpCeilingMode] failed.");
    return ret;
  }

  ret = graph_context_->Initialize(options);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Initialize][GraphContext] failed.");
    return ret;
  }

  ret = InitProcessNodeEngines(options, this, process_node_engines_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Initialize][ProcessNodeEngines] failed.");
    return ret;
  }

  executor_ = executor;
  init_flag_ = true;

  thread_run_flag_ = true;
  prerun_thread_ = std::thread(&GraphManager::PreRunThread, this);

  return SUCCESS;
}

Status GraphManager::UnloadModel(FlowModelPtr flow_model, uint32_t graph_id) {
  GE_CHECK_NOTNULL(executor_);
  return executor_->UnloadGraph(flow_model, graph_id);
}

Status GraphManager::Finalize() {
  if (!init_flag_) {
    GELOGW("GraphManager has not been initialized.");
    return SUCCESS;
  }

  StopQueue();
  if (prerun_thread_.joinable()) {
    prerun_thread_.join();
  }

  // check graph whether running or not
  Status unload_model_ret = SUCCESS;
  for (auto iter = graph_map_.cbegin(); iter != graph_map_.cend(); ++iter) {
    GraphNodePtr graph_node = iter->second;
    GE_CHECK_NOTNULL(graph_node);
    GE_CHECK_NOTNULL(graph_node->GetGraph());
    if (graph_node->GetRunFlag()) {
      GELOGW("[GraphManager] finalize failed, graphId=%u.", iter->first);
      unload_model_ret = GE_GRAPH_GRAPH_IS_RUNNING;
      continue;
    }
    // unload model
    auto flow_model = graph_node->GetFlowModel();
    if (CheckModelLoad(flow_model, graph_node->GetLoadFlag())) {
      Status ret = UnloadModel(flow_model, iter->first);
      if (ret != SUCCESS) {
        unload_model_ret = ret;
        GELOGW("[GraphManager] unload model failed, graph_id=%u.", iter->first);
      }
    }

    // clear analyzer saved info(graph level)
    auto compute_graph = GraphUtils::GetComputeGraph(*graph_node->GetGraph());
    GE_CHECK_NOTNULL(compute_graph);
    auto session_id = compute_graph->GetSessionID();
    auto graph_id = compute_graph->GetGraphID();
    Analyzer::GetInstance()->DestroyGraphJsonObject(session_id, graph_id);
  }
  graph_map_.clear();
  graph_count_.clear();

  // graph context
  if (graph_context_ != nullptr) {
    Status ret_final = graph_context_->Finalize();
    if (ret_final != SUCCESS) {
      GELOGE(ret_final, "[Finalize][GraphContext] failed!");
      unload_model_ret = ret_final;
    }
  }
  resource_context_mgr_.ClearContext();

  init_flag_ = false;
  return unload_model_ret;
}

Status GraphManager::InitDynamicParams(const ComputeGraphPtr &compute_graph) const {
  GE_CHECK_NOTNULL(compute_graph);
  for (const auto &node : compute_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    auto op_desc = node->GetOpDesc();
    if (op_desc == nullptr) {
      continue;
    }
    GetLocalOmgContext().need_multi_batch = false;
    std::string op_type;
    auto ret = GetOriginalType(node, op_type);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "GetOriginalType from op:%s fail", node->GetName().c_str());
      GELOGE(FAILED, "[Get][OriginalType] from op:%s failed.", node->GetName().c_str());
      return FAILED;
    }
    if ((op_desc->GetType() == DATA) || (op_type == kGetNextName)) {
      GELOGI("Maybe need to process multi batch for compute graph %s, op_type:%s.",
             compute_graph->GetName().c_str(), op_desc->GetType().c_str());
      GetLocalOmgContext().need_multi_batch = true;
      break;
    }
  }
  if (!options_.input_shape.empty() && !options_.dynamic_dims.empty()) {
    if (!ge::ParseInputShape(options_.input_shape, GetLocalOmgContext().input_dims,
                             GetLocalOmgContext().user_input_dims, true)) {
      GELOGE(GRAPH_PARAM_INVALID, "[Parse][InputShape] %s failed.", options_.input_shape.c_str());
      return GRAPH_PARAM_INVALID;
    }
    GetLocalOmgContext().dynamic_dims = options_.dynamic_dims;
  }
  if (options_.dynamic_node_type == kDynamicDimsTypeIsGetNext) {
    GetLocalOmgContext().dynamic_node_type = GETNEXT;
  }
  if (options_.dynamic_node_type == kDynamicDimsTypeIsData) {
    GetLocalOmgContext().dynamic_node_type = DATA;
  }
  return SUCCESS;
}

void GraphManager::SetAddGraphCondition(GraphId graph_id, uint32_t cond) {
  std::lock_guard<std::mutex> lock(add_graph_cond_mutex_);
  graph_id_to_add_graph_cond_[graph_id] = cond;
  GELOGD("Graph [id:%u] has been added.", graph_id);
}

uint32_t GraphManager::GetAddGraphCondition(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(add_graph_cond_mutex_);
  std::map<GraphId, uint32_t>::const_iterator it = graph_id_to_add_graph_cond_.find(graph_id);
  if (it != graph_id_to_add_graph_cond_.cend()) {
    return it->second;
  } else {
    GELOGD("Graph [id:%u] has not been added.", graph_id);
    return kNotAdded;
  }
}

void GraphManager::RemoveAddGraphCondition(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(add_graph_cond_mutex_);
  std::map<GraphId, uint32_t>::const_iterator it = graph_id_to_add_graph_cond_.find(graph_id);
  if (it != graph_id_to_add_graph_cond_.cend()) {
    (void)graph_id_to_add_graph_cond_.erase(it);
    GELOGD("Successfully remove add_graph_cond of graph [id:%u].", graph_id);
  } else {
    GELOGD("Graph [id:%u] has not been added, no need to be removed.", graph_id);
  }
}

Status GraphManager::CheckRepeatAdd(uint32_t graph_id, bool &is_added) {
  uint32_t count = 0;
  if (GetGraphCount(graph_id, count) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Get][GraphCount] failed, graph[id:%u] might have not been added.", graph_id);
    return INTERNAL_ERROR;
  }
  // previous thread owns same graph_id has been in the middle of the AddGraph procession
  if (count > 1 && GetAddGraphCondition(graph_id) == kStartAdd) {
    std::unique_lock<std::mutex> lock(add_graph_mutex_);
    GELOGD("Waitting for build end of previous thread.");
    while (GetAddGraphCondition(graph_id) != kDoneAdded) {
      add_graph_cv_.wait(lock);
    }
    GraphNodePtr graph_node;
    Status ret = GetGraphNode(graph_id, graph_node);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Get][GraphNode] failed, graph_id = %u.", graph_id);
      return ret;
    }
    is_added = true;
  }
  return SUCCESS;
}

void GraphManager::SetSessionGraphId(ComputeGraphPtr compute_graph, uint32_t graph_id) const {
  GE_CHECK_NOTNULL_EXEC(compute_graph, return);
  std::string session_graph_id;
  if (!AttrUtils::GetStr(*compute_graph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) || session_graph_id.empty()) {
    session_graph_id = "-1_" + to_string(graph_id);
    if (!AttrUtils::SetStr(*compute_graph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
      GELOGW("Set attribute of compute graph failed.");
    }
    for (auto &subgraph : compute_graph->GetAllSubgraphs()) {
      GE_CHECK_NOTNULL_EXEC(subgraph, return);
      (void)AttrUtils::SetStr(*subgraph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    }
    GELOGD("Get graph session_graph_id attr failed, set session id to default value: [0]");
  }
}

Status GraphManager::NotifyWaittingGraph(uint32_t graph_id) {
  uint32_t count = 0;
  if (GetGraphCount(graph_id, count) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Get][GraphCount] failed, graph[id:%u] might have not been added.", graph_id);
    return INTERNAL_ERROR;
  }
  GELOGD("Add graph finished, graph_id:%u", graph_id);
  if (count > 1) {
    GELOGD("Finish addgraph, graph_id:%u, graph_count:%u, start to notify.", graph_id, count);
    add_graph_cv_.notify_all();
  }
  return SUCCESS;
}

Status GraphManager::CreateGraphNode(uint32_t graph_id, const Graph &graph,
                                     const std::map<std::string, std::string> &options) {
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  GE_IF_BOOL_EXEC(graph_node == nullptr,
                  REPORT_CALL_ERROR("E19999", "New GraphNode fail, graph_id:%u", graph_id);
                  GELOGE(FAILED, "[New][GraphNode] fail, graph_id:%u", graph_id);
                  return FAILED);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  GE_IF_BOOL_EXEC(graph_ptr == nullptr,
                  REPORT_CALL_ERROR("E19999", "New Graph fail, graph_id:%u", graph_id);
                  GELOGE(FAILED, "[New][Graph] fail, graph_id:%u", graph_id);
                  return FAILED);
  // update option about tuning graph
  ParseOption(options, BUILD_MODE, options_.build_mode);
  ParseOption(options, BUILD_STEP, options_.build_step);
  ParseOption(options, TUNING_PATH, options_.tuning_path);
  graph_node->SetGraph(graph_ptr);
  graph_node->SetOptions(options);
  graph_node->IncreaseLoadCount();
  AddGraphNode(graph_id, graph_node);
  return SUCCESS;
}

Status GraphManager::SetStagesOptions(uint32_t graph_id) {
  CompilerStages &stages = GetCompilerStages(graph_id);
  auto refreshed_options = options_;
  RefreshOptionByGraph(graph_id, refreshed_options);
  stages.preparer.SetOptions(refreshed_options);
  Status status = stages.optimizer.SetOptions(refreshed_options);
  if (status != SUCCESS) {
    GELOGE(status, "[Set][Options] for Graph optimizer failed, graph id:%u.", graph_id);
    return status;
  }
  stages.builder.SetOptions(refreshed_options);
  return SUCCESS;
}

Status GraphManager::ModifyDataIndex(const Graph &graph, const std::map<std::string,
                                     std::string> &graph_option) const {
  std::vector<OpDescPtr> data_desc;
  std::set<int64_t> indexes;
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHECK_NOTNULL(compute_graph);
  for (auto &input_node : compute_graph->GetDirectNode()) {
    GE_CHECK_NOTNULL(input_node);
    auto op = input_node->GetOpDesc();
    GE_CHECK_NOTNULL(op);
    if (op->GetType() == DATA) {
      int64_t index = 0;
      (void) AttrUtils::GetInt(op, ATTR_NAME_INDEX, index);
      (void)indexes.insert(index);
      data_desc.emplace_back(op);
    }
  }
  if (!indexes.empty()) {
    auto first_iter = indexes.begin();
    auto end_iter = indexes.end();
    --end_iter;
    auto data_size = static_cast<int64_t>(data_desc.size());
    // The valid index starts with 0 and increases by 1, and num is equal to data_node.
    if (indexes.size() != data_desc.size() || *first_iter != 0 || *end_iter != data_size - 1) {
      auto iter = graph_option.find(OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE);
      if (iter != graph_option.end() && !iter->second.empty()) {
        // If data inputs shape range is set, user must set valid data index.
        std::string situation = "Data op index";
        std::string reason = "Data index must be set continuous from 0 when data shape range enabled!";
        REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"situation", "reason"}),
                           std::vector<std::string>({situation, reason}));
        GELOGE(GRAPH_PARAM_INVALID, "[COMP][AddGraph]Input data index is invalid when data shape range enabled.");
        return GRAPH_PARAM_INVALID;
      }
      GELOGI("Graph[%s] input data index is invalid, set data index by topo order.", compute_graph->GetName().c_str());
      int64_t index = 0;
      for (auto &op : data_desc) {
        (void) AttrUtils::SetInt(op, ATTR_NAME_INDEX, index++);
      }
    }
  }
  return SUCCESS;
}

Status GraphManager::AddGraph(const GraphId &graph_id, const Graph &graph,
                              const std::map<std::string, std::string> &options,
                              const OmgContext &omg_context) {
  IncreaseGraphCount(graph_id);
  // validation for adding graphs of same graph_id in multi-thread secenario
  // 1.previous thread owns same graph_id has finished the AddGraph procession
  if (GetAddGraphCondition(graph_id) == kDoneAdded) {
    GraphNodePtr graph_node;
    if (GetGraphNode(graph_id, graph_node) != SUCCESS) {
      GELOGE(GE_GRAPH_GRAPH_NOT_EXIST, "[Get][GraphNode] failed, Graph not exist while done adding previously, "
             "graph_id = %u.", graph_id);
      return GE_GRAPH_GRAPH_NOT_EXIST;
    }
    graph_node->IncreaseLoadCount();
    return SUCCESS;
  }
  // In multi-thread scenario, former thread owns same graph_id has been
  // in the middle of the AddGraph procession while following threads have to wait until
  // done adding graph of the former graph, avoiding repeatively adding same graph.
  bool is_added = false;
  if (CheckRepeatAdd(graph_id, is_added) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Check][RepeatAdd] for graph[id:%u] failed.", graph_id);
    return INTERNAL_ERROR;
  }
  // The former graph (from different thread) owns same graph id has been successfully added.
  if (is_added) {
    return SUCCESS;
  }
  // Do add graph
  SetAddGraphCondition(graph_id, kStartAdd);
  if (CheckGraphAdded(graph_id, graph) != SUCCESS) {
    GELOGE(FAILED, "[Check][GraphAdded] failed, graph id:%u.", graph_id);
    return FAILED;
  }
  GE_CHK_STATUS_RET(ModifyDataIndex(graph, options));
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHECK_NOTNULL(compute_graph);
  (void)AttrUtils::SetBool(*compute_graph, ATTR_NAME_GRAPH_HAS_BEEN_ADDED, true);
  SetSessionGraphId(compute_graph, graph_id);

  if (CreateGraphNode(graph_id, graph, options) != SUCCESS) {
    GELOGE(FAILED, "[Create][GraphNode] failed, graph id:%u.", graph_id);
    return FAILED;
  }

  AddLocalOmgContext(graph_id, omg_context);
  if (!options_.output_datatype.empty()) {
    GetLocalOmgContext().output_type = options_.output_datatype;
  }
  if (InitDynamicParams(compute_graph) != SUCCESS) {
    GELOGE(GRAPH_PARAM_INVALID, "[Init][Params] failed, when online infer is dynamic, graph id:%u.", graph_id);
    return GRAPH_PARAM_INVALID;
  }

  if (SetStagesOptions(graph_id) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Set][StagesOptions] failed, graph id:%u.", graph_id);
    return INTERNAL_ERROR;
  }

  graph_rebuild_state_ctrl_.AddGraph(graph_id, compute_graph);
  SetAddGraphCondition(graph_id, kDoneAdded);
  // There are threads waitting for adding same graph
  if (NotifyWaittingGraph(graph_id) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Notify][WaittingGraph] failed, graph id:%u.", graph_id);
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status GraphManager::CheckGraphAdded(const GraphId &graph_id, const Graph &graph) {
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  if (compute_graph != nullptr) {
    compute_graph->SetGraphID(graph_id);
    bool graph_has_been_added = false;
    if (AttrUtils::GetBool(*compute_graph, ATTR_NAME_GRAPH_HAS_BEEN_ADDED, graph_has_been_added)
        && graph_has_been_added) {
      REPORT_INNER_ERROR("E19999", "Get Attr:%s from graph:%u fail.",
                         ATTR_NAME_GRAPH_HAS_BEEN_ADDED.c_str(), graph_id);
      GELOGE(GE_GRAPH_GRAPH_ALREADY_EXIST,  "[Get][Attr] %s from graph:%u fail.",
             ATTR_NAME_GRAPH_HAS_BEEN_ADDED.c_str(), graph_id);
      return GE_GRAPH_GRAPH_ALREADY_EXIST;
    }
  } else {
    REPORT_INNER_ERROR("E19999", "compute_graph from graph:%u is nullptr, check invalid", graph_id);
    GELOGE(FAILED, "[Get][ComputeGraph] failed, compute graph from graph:%u is nullptr", graph_id);
    return FAILED;
  }
  return SUCCESS;
}

Status GraphManager::AddGraphWithCopy(const GraphId &graph_id, const Graph &graph,
                                      const std::map<std::string, std::string> &options,
                                      const OmgContext &omg_context) {
  if (HasGraphNode(graph_id)) {
    GELOGE(GE_GRAPH_GRAPH_ALREADY_EXIST, "[Has][GraphNode] graph exists, graph_id = %u", graph_id);
    return GE_GRAPH_GRAPH_ALREADY_EXIST;
  }
  if (CheckGraphAdded(graph_id, graph) != SUCCESS) {
    GELOGE(FAILED, "[Check][GraphAdded] failed, graph_id = %u", graph_id);
    return FAILED;
  }
  IncreaseGraphCount(graph_id);
  // Do add graph
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  std::vector<NodePtr> input_nodes;
  std::vector<NodePtr> output_nodes;
  auto new_compute_graph = GraphUtils::CloneGraph(compute_graph, "", input_nodes, output_nodes);
  GE_CHECK_NOTNULL(new_compute_graph);
  new_compute_graph->SetGraphID(graph_id);
  SetSessionGraphId(new_compute_graph, graph_id);
  std::shared_ptr<Graph> new_graph_ptr = GraphUtils::CreateGraphPtrFromComputeGraph(new_compute_graph);
  GE_CHECK_NOTNULL(new_graph_ptr);
  if (CreateGraphNode(graph_id, *new_graph_ptr, options) != SUCCESS) {
    GELOGE(FAILED, "[Create][GraphNode] failed, graph_id = %u", graph_id);
    return FAILED;
  }

  AddLocalOmgContext(graph_id, omg_context);
  if (!options_.output_datatype.empty()) {
    GetLocalOmgContext().output_type = options_.output_datatype;
  }
  if (InitDynamicParams(new_compute_graph) != SUCCESS) {
    GELOGE(GRAPH_PARAM_INVALID, "[Init][Params] failed, when online infer is dynamic, graph_id = %u", graph_id);
    return GRAPH_PARAM_INVALID;
  }

  if (SetStagesOptions(graph_id) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Set][StagesOptions] failed, graph_id = %u", graph_id);
    return INTERNAL_ERROR;
  }

  graph_rebuild_state_ctrl_.AddGraph(graph_id, new_compute_graph);
  return SUCCESS;
}

Status GraphManager::MergeSubGraph(ComputeGraphPtr &compute_graph, const ge::ComputeGraphPtr &original_compute_graph,
                                   GraphId root_graph_id) {
  std::shared_ptr<GELib> instance_ptr = ge::GELib::GetInstance();
  GraphPartitioner &partitioner = GetCompilerStages(root_graph_id).partitioner;
  if (instance_ptr != nullptr && instance_ptr->InitFlag()) {
    Status ret = partitioner.MergeAfterSubGraphOptimization(compute_graph, original_compute_graph);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Merge][SubGraph] merge end and placeholder after subGraph optimization failed.");
      return FAILED;
    }

    Status ret_topo = compute_graph->TopologicalSorting();
    if (ret_topo != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "TopologicalSorting fail, graph_id:%u", compute_graph->GetGraphID());
      GELOGE(ret_topo, "[Call][TopologicalSorting] for the merged graph failed, graph_id:%u",
             compute_graph->GetGraphID());
      return ret_topo;
    }
  } else {
    auto subgraph_list = partitioner.GetSubGraphMap();
    if (subgraph_list.find(original_compute_graph) != subgraph_list.end() &&
        !subgraph_list[original_compute_graph].empty() && subgraph_list[original_compute_graph][0] != nullptr) {
      compute_graph = subgraph_list[original_compute_graph][0]->GetSubGraph();
    }
  }

  return SUCCESS;
}

Status GraphManager::OptimizeSubGraphWithMultiThreads(ComputeGraphPtr compute_graph,
                                                      Graph2SubGraphInfoList &sub_graph_map, uint64_t session_id) {
  GE_CHECK_NOTNULL(compute_graph);
  // use default 16 multi thread
  uint32_t thread_num = 16;
  ThreadPool executor(thread_num);
  std::vector<std::future<Status>> vector_future;
  const auto &root_subgraph_list = sub_graph_map[compute_graph];
  std::string op_compile_strategy;
  (void)AttrUtils::GetStr(compute_graph, ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy);
  GELOGD("OptimizeSubGraphWithMultiThreads Process op_compile_strategy:%s", op_compile_strategy.c_str());
  for (const auto &subgraph : root_subgraph_list) {
    GE_CHECK_NOTNULL(subgraph);
    if (!op_compile_strategy.empty()) {
      (void) AttrUtils::SetStr(subgraph->GetSubGraph(), ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy);
    }
    std::future<Status> f = executor.commit(GraphManager::ProcessSubGraphWithMultiThreads, this,
                                            compute_graph->GetGraphID(), subgraph,
                                            compute_graph->GetName(), session_id,
                                            ErrorManager::GetInstance().GetErrorManagerContext(),
                                            GetThreadLocalContext());
    if (!f.valid()) {
      GELOGE(FAILED, "[Call][Commit] failed, Future is invalid, session_id:%lu", session_id);
      return FAILED;
    }
    vector_future.emplace_back(std::move(f));
  }
  for (auto &function_graph : compute_graph->GetAllSubgraphs()) {
    auto subgraph_list = sub_graph_map[function_graph];
    for (const auto &subgraph : subgraph_list) {
      GE_CHECK_NOTNULL(subgraph);
      if (!op_compile_strategy.empty()) {
        (void) AttrUtils::SetStr(subgraph->GetSubGraph(), ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy);
      }
      std::future<Status> f = executor.commit(GraphManager::ProcessSubGraphWithMultiThreads, this,
                                              compute_graph->GetGraphID(), subgraph,
                                              compute_graph->GetName(), session_id,
                                              ErrorManager::GetInstance().GetErrorManagerContext(),
                                              GetThreadLocalContext());
      if (!f.valid()) {
        GELOGE(FAILED, "[Call][Commit] failed, Future is invalid, session_id:%lu", session_id);
        return FAILED;
      }
      vector_future.emplace_back(std::move(f));
    }
  }
  GELOGD("All sub graph num is %zu", vector_future.size());
  for (size_t i = 0; i < vector_future.size(); ++i) {
    Status ret_status = vector_future[i].get();
    if (ret_status != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "subgraph %zu optimize failed", i);
      GELOGE(ret_status, "[Check][Param] subgraph %zu optimize failed", i);
      return ret_status;
    }
  }
  return SUCCESS;
}

Status GraphManager::SetSubgraph(uint64_t session_id, ComputeGraphPtr compute_graph, GraphPartitioner &partitioner) {
  GE_CHECK_NOTNULL(compute_graph);
  auto sub_graph_map = partitioner.GetSubGraphMap();
  GELOGD("Directly optimize subgraph with build mode:%s, and step:%s.",
         options_.build_mode.c_str(),
         options_.build_step.c_str());
  Status ret = OptimizeSubGraphWithMultiThreads(compute_graph, sub_graph_map, session_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][OptimizeSubGraphWithMultiThreads] failed, ret:%d, session_id:%lu", ret, session_id);
    return ret;
  }
  for (const auto &item : sub_graph_map) {
    for (const auto &subgraph_info : item.second) {
      GE_CHECK_NOTNULL(subgraph_info);
      const auto &subgraph = subgraph_info->GetSubGraph();
      GE_CHECK_NOTNULL(subgraph);
      for (const auto &new_graph : subgraph->GetAllSubgraphs()) {
        compute_graph->AddSubGraph(new_graph);
      }
    }
  }
  return SUCCESS;
}

#define GM_RUN_AND_DUMP_PERF(name, func, ...)                                                                    \
  do {                                                                                                           \
    GE_RUN_PERF(GraphManager, func, __VA_ARGS__);                                                                \
    GE_DUMP(compute_graph, "PreRunAfter" name);                                                                  \
    GELOGI("Run %s on graph %s(%u) success.", name, compute_graph->GetName().c_str(), graph_node->GetGraphId()); \
  } while (0)

Status GraphManager::PreRunOptimizeOriginalGraph(const GraphNodePtr &graph_node, const std::vector<GeTensor> &inputs,
                                                 ge::ComputeGraphPtr &compute_graph, uint64_t session_id) {
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kPrepareOptimize);
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);

  CompilerStages &stages = GetCompilerStages(graph_node->GetGraphId());
  GM_RUN_AND_DUMP_PERF("InitPreparation", stages.preparer.PrepareInitAndUpdateInput, graph_node, inputs,
                       session_id, &graph_rebuild_state_ctrl_, &resource_context_mgr_);
  GM_RUN_AND_DUMP_PERF("OptimizeGraphPrepare", stages.optimizer.OptimizeOriginalGraphForQuantize, compute_graph);

  int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
  (void)AttrUtils::GetInt(compute_graph, kGraphDumpStage, graph_stage);
  if (graph_stage == static_cast<int64_t>(GraphStage::GRAPH_STAGE_FUZZ)) {
    GELOGD("graph_stage:%d.", static_cast<int32_t>(graph_stage));
    return SUCCESS;
  }

  GM_RUN_AND_DUMP_PERF("HandleSummaryOp", stages.optimizer.HandleSummaryOp, compute_graph);
  GM_RUN_AND_DUMP_PERF("Prepare", stages.preparer.PrepareDynShape);
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOriginOptimize);
  GM_RUN_AND_DUMP_PERF("OptimizeOriginalGraph", stages.optimizer.OptimizeOriginalGraph, compute_graph);

  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kPrepareOptimize);
  GM_RUN_AND_DUMP_PERF("PrepareRunningFormatRefiner", stages.preparer.PrepareRunningFormatRefiner);
  GM_RUN_AND_DUMP_PERF("RefineRunningFormat", stages.optimizer.OptimizeOriginalGraphJudgeInsert, compute_graph);
  GM_RUN_AND_DUMP_PERF("SubexpressionMigration", SubexpressionMigration, compute_graph);
  GE_RUN(GraphManager, stages.preparer.RecordAIPPInfo, compute_graph);
  if (IsTailingOptimization()) {
    GM_RUN_AND_DUMP_PERF("OptimizeSwitchOp", stages.preparer.SwitchOpOptimize, compute_graph);
  }
  GM_RUN_AND_DUMP_PERF("Optimize1", OptimizeStage1, compute_graph);
  GM_RUN_AND_DUMP_PERF("OptimizeAfterStage1", stages.optimizer.OptimizeAfterStage1, compute_graph);
  GM_RUN_AND_DUMP_PERF("InferShape2", compute_graph->InferShapeInNeed);

  PassManager graph_pass;
  GE_CHK_STATUS_RET(graph_pass.AddPass("PreRun::CtrlEdgeTransferPass", new (std::nothrow) CtrlEdgeTransferPass));
  GE_CHK_STATUS_RET(graph_pass.Run(compute_graph));

  GE_CHK_STATUS_RET(stages.optimizer.IdentifyReference(compute_graph),
                    "[Identify][Reference] failed, graph:%s.", compute_graph->GetName().c_str());
  GEEVENT("PreRun:PreRunOptimizeOriginalGraph success.");
  return SUCCESS;
}

Status GraphManager::PreRunOptimizeSubGraph(const GraphNodePtr &graph_node,
                                            ge::ComputeGraphPtr &compute_graph,
                                            uint64_t session_id) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);
  GM_RUN_AND_DUMP_PERF("OptimizeSubgraph", OptimizeSubgraph, graph_node, compute_graph, session_id);

  // Dump graph to tuning path
  if ((GetBuildMode(graph_node) == BUILD_MODE_TUNING) && (GetBuildStep(graph_node) == BUILD_STEP_AFTER_UB_MATCH)) {
    std::string tuning_path;
    (void) GetContext().GetOption(TUNING_PATH, tuning_path);
    GELOGD("Dump path:%s.", tuning_path.c_str());
    GraphUtils::DumpGEGraph(compute_graph, "", true, tuning_path);
  }
  GEEVENT("PreRun:PreRunOptimizeSubGraph success.");
  return SUCCESS;
}

Status GraphManager::PreRunAfterOptimizeSubGraph(const GraphNodePtr &graph_node, ComputeGraphPtr &compute_graph,
                                                 GeRootModelPtr &ge_root_model, uint64_t session_id) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);

  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kMergeGraphOptimize);
  CompilerStages &stages = GetCompilerStages(graph_node->GetGraphId());
  GM_RUN_AND_DUMP_PERF("OptimizeWholeGraph", stages.optimizer.OptimizeWholeGraph, compute_graph);
  GM_RUN_AND_DUMP_PERF("Optimize2", OptimizeStage2, compute_graph);
  GM_RUN_AND_DUMP_PERF("OptimizeGraphBeforeBuild",
                       GetCompilerStages(graph_node->GetGraphId()).optimizer.OptimizeGraphBeforeBuild,
                       compute_graph);

  Status ret = compute_graph->TopologicalSorting();
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "TopologicalSorting fail, graph_id:%u", compute_graph->GetGraphID());
    GELOGE(ret, "[Call][TopologicalSorting] fail, graph_id:%u", compute_graph->GetGraphID());
    return ret;
  }

  GM_RUN_AND_DUMP_PERF("Build", Build, graph_node, compute_graph, ge_root_model, session_id);

  const auto build_mode = GetBuildMode(graph_node);
  const auto tuning_path = GetTuningPath(graph_node);
  if ((build_mode.empty() || (build_mode == BUILD_MODE_BASELINE)) && !tuning_path.empty()) {
    GraphUtils::DumpGEGraph(compute_graph, "", true, tuning_path);
    GELOGD("Success to dump after build graph:%s to tuning path:%s.", compute_graph->GetName().c_str(),
           tuning_path.c_str());
  }

  GEEVENT("PreRun:PreRunAfterOptimizeSubGraph success.");
  return SUCCESS;
}

Status GraphManager::SetRtContext(rtContext_t rt_context, rtCtxMode_t mode, uint64_t session_id,
                                  uint32_t graph_id) const {
  GELOGD("Set rt_context: session id: %lu, graph id: %u, mode %d, device id:%u.",
         session_id, graph_id, static_cast<int32_t>(mode), ge::GetContext().DeviceId());
  GE_CHK_STATUS_RET(MdsUtils::CtxCreate(&rt_context, mode, ge::GetContext().DeviceId()));
  rtError_t rt_ret = rtCtxSetCurrent(rt_context);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtCtxSetCurrent failed, session_id:%lu, graph_id:%u, mode:%d",
                      session_id, graph_id, mode);
    GELOGE(FAILED, "[Call][RtCtxSetCurrent] failed, session_id:%lu, graph_id:%u, mode:%d", session_id, graph_id, mode);
    return FAILED;
  }
  RtContextUtil::GetInstance().AddRtContext(session_id, graph_id, rt_context);
  return SUCCESS;
}

Status GraphManager::RunCustomPass(const GraphNodePtr &graph_node) const {
  GE_CHECK_NOTNULL(graph_node);
  ConstGraphPtr const_graph = graph_node->GetGraph();
  GE_CHECK_NOTNULL(const_graph);
  auto comp_graph = GraphUtils::GetComputeGraph(*const_graph);
  GE_DUMP(comp_graph, "RunCustomPassBegin");

  GE_CHECK_NOTNULL(comp_graph);
  GE_TIMESTAMP_START(RunCustomPass);
  GraphPtr graph = std::const_pointer_cast<Graph>(const_graph);
  GE_CHK_STATUS_RET(CustomPassHelper::Instance().Run(graph), "[Call][Run] for Graph[%s] fail.",
                    comp_graph->GetName().c_str());
  GE_TIMESTAMP_END(RunCustomPass, "GraphBuilder::RunCustomPass");
  return SUCCESS;
}

Status GraphManager::PreRun(const GraphNodePtr &graph_node, const std::vector<GeTensor> &inputs,
                            FlowModelPtr &flow_model, uint64_t session_id) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(graph_node->GetGraph());
  GE_CHK_STATUS_RET_NOLOG(RunCustomPass(graph_node));
  auto compute_graph = GraphUtils::GetComputeGraph(*graph_node->GetGraph());
  GE_CHECK_NOTNULL(compute_graph);
  compute_graph->SetSessionID(session_id);
  auto analyzer_instance = Analyzer::GetInstance();
  GE_CHK_STATUS_RET(analyzer_instance->BuildJsonObject(session_id, compute_graph->GetGraphID()),
                    "[Build][JsonObject] Failed, session_id:%lu", session_id);

  GEEVENT("PreRun start: graph node size %zu, session id %lu, graph id %u, graph name %s.",
          compute_graph->GetDirectNodesSize(), session_id, compute_graph->GetGraphID(),
          compute_graph->GetName().c_str());
  GE_DUMP(compute_graph, "PreRunBegin");
  // rtContext_t
  Status ret = SetRtContext(rtContext_t(), RT_CTX_GEN_MODE, session_id, compute_graph->GetGraphID());
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][RtContext] failed, session_id:%lu, graph_id:%u.", session_id, compute_graph->GetGraphID());
    return ret;
  }

  ret = GeOptInfo::SetOptInfo();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][OptInfo] Set optional information failed.");
    return ret;
  }

  if (!process_node_engines_.empty()) {
    return ProcessNodeEngineBuild(graph_node, compute_graph, inputs, flow_model);
  }

  GEEVENT("[GEPERFTRACE] GE PreRun End");
  return FAILED;
}

Status GraphManager::SubexpressionMigration(ComputeGraphPtr &compute_graph) const {
  PassManager pass_manager;
  GE_CHK_STATUS_RET(pass_manager.AddPass("SubexpressionMigrationPass", new (std::nothrow) SubexpressionMigrationPass));
  GE_CHK_STATUS_RET(pass_manager.AddPass("UnusedArgsCleanPass", new (std::nothrow) UnusedArgsCleanPass));

  GE_TIMESTAMP_START(SubexpressionMigrationPass);
  auto ret = pass_manager.Run(compute_graph);
  GE_TIMESTAMP_END(SubexpressionMigrationPass, "GraphManager::SubexpressionMigration");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][SubexpressionMigrationPass] failed, ret:%u.", ret);
    return ret;
  }

  return SUCCESS;
}

Status GraphManager::StartForRunGraph(const GraphNodePtr &graph_node, const std::vector<GeTensor> &inputs,
                                      FlowModelPtr &ge_root_model, uint64_t session_id) {
  // it will not execute graph prreprocess, optimize, parition, build if the graph has built successful.
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(graph_node->GetGraph());
  Status ret = SUCCESS;
  if (IsGraphNeedBuild(graph_node)) {
    ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
    if (graph_node->GetBuildFlag()) {
      REPORT_INNER_ERROR("E19999", "Graph:%u has not build before, can't run directly, "
                         "check invalid", graph_node->GetGraphId());
      GELOGE(PARAM_INVALID,
             "[Get][BuildFlag] The graph %u need to re-build, you should remove it from GE "
             "first, then AddGraph again and rebuild it.",
             graph_node->GetGraphId());
      return PARAM_INVALID;
    }

    ret = PreRun(graph_node, inputs, ge_root_model, session_id);
    // release rts generate context
    RtContextUtil::GetInstance().DestroyRtContexts(session_id, graph_node->GetGraphId());
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][PreRun] Failed, graph_id:%u, session_id:%lu.", graph_node->GetGraphId(), session_id);
      return ret;
    }
    auto compute_graph = GraphUtils::GetComputeGraph(*(graph_node->GetGraph()));
    GE_CHECK_NOTNULL(compute_graph);
    int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
    (void)AttrUtils::GetInt(compute_graph, kGraphDumpStage, graph_stage);
    if (graph_stage == static_cast<int64_t>(GraphStage::GRAPH_STAGE_FUZZ)) {
      GELOGD("graph_stage:%d.", static_cast<int32_t>(graph_stage));
      return SUCCESS;
    }
    ret = LoadGraph(ge_root_model, graph_node);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Load][Graph] Failed, graph_id:%u.", graph_node->GetGraphId());
      return ret;
    }
    graph_node->SetBuildFlag(true);
    graph_rebuild_state_ctrl_.SetGraphBuildEnd(graph_node->GetGraphId());
  } else if (!graph_node->GetLoadFlag()) {
    auto flow_model_ptr = graph_node->GetFlowModel();
    GE_CHECK_NOTNULL(flow_model_ptr);
    ret = LoadGraph(flow_model_ptr, graph_node);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Load][Graph] Failed, graph_id:%u.", graph_node->GetGraphId());
      return ret;
    }
  }
  return ret;
}

Status GraphManager::LoadGraph(const FlowModelPtr &flow_root_model, const GraphNodePtr &graph_node) {
  GE_CHECK_NOTNULL(graph_node);
  GELOGI("[LoadGraph] run_graph_flag[%d], graph_id[%u]", options_.run_graph_flag, graph_node->GetGraphId());
  if (!options_.run_graph_flag) {
    return SUCCESS;
  }
  ErrorManager::GetInstance().SetStage(error_message::kModelLoad, error_message::kModelLoad);
  GE_CHECK_NOTNULL(executor_);
  Status ret = executor_->LoadGraph(flow_root_model, graph_node);
  if (ret != SUCCESS) {
    return ret;
  }
  return SUCCESS;
}

Status GraphManager::InnerRunGraph(const GraphNodePtr &graph_node, const GraphId &graph_id,
                                   const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) {
  GE_CHECK_NOTNULL(executor_);
  return executor_->RunGraph(graph_node, graph_id, inputs, outputs);
}

Status GraphManager::InnerRunGraphWithStream(const GraphNodePtr &graph_node, const GraphId &graph_id,
                                             rtStream_t stream, const std::vector<GeTensor> &inputs,
                                             std::vector<GeTensor> &outputs) {
  GE_CHECK_NOTNULL(executor_);
  return executor_->RunGraphWithStream(graph_node, graph_id, stream, inputs, outputs);
}

Status GraphManager::RunGraphWithStreamAsync(const GraphId &graph_id, const rtStream_t stream, uint64_t session_id,
                                             const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) {
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
  std::lock_guard<std::mutex> lock(run_mutex_);
  GELOGI("Start to run graph with stream async, graph id = %u, stream = %p.", graph_id, stream);

  if (inputs.empty()) {
    GELOGI("Run graph with stream async, initialize sub graph has no inputs.");
  }

  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "graph id = %u not exist in graph_map, check invalid.", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, Run graph with stream async, graph not exist, graph id = %u.", graph_id);
    return ret;
  }
  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph id = %u, check invalid.", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] Run graph with stream async, graph node is NULL, "
           "graph id = %u.", graph_id);
    return GE_GRAPH_GRAPH_NODE_NULL;
  }
  if (graph_node->GetRunFlag()) {
    REPORT_INNER_ERROR("E19999", "Graph is already running, can't be run again, graph id = %u, "
                       "check invalid.", graph_id);
    GELOGE(GE_GRAPH_ALREADY_RUNNING, "[Get][RunFlag] Run graph with stream async graph already running, "
           "graph id = %u.", graph_id);
    return GE_GRAPH_ALREADY_RUNNING;
  }

  UpdateLocalOmgContext(graph_id);
  // set graph's run flag
  graph_node->SetRunFlag(true);
  graph_node->SetIsSpecificStream(true);
  GE_CHECK_NOTNULL(graph_node->GetGraph());
  ComputeGraphPtr compute_graph_tmp = GraphUtils::GetComputeGraph(*(graph_node->GetGraph()));
  GE_CHECK_NOTNULL(compute_graph_tmp);

  if (options_.local_fmk_op_flag) {
    GetCompilerStages(graph_id).optimizer.TranFrameOp(compute_graph_tmp);
  }
  FlowModelPtr ge_root_model = nullptr;
  if (IsGraphNeedBuild(graph_node)) {
    GELOGI("Start to normalize input ge tensors of graph %s", compute_graph_tmp->GetName().c_str());
    std::vector<GeTensor> normalized_inputs;
    normalized_inputs.reserve(inputs.size());
    for (size_t i = 0; i < inputs.size(); i++) {
      normalized_inputs.emplace_back(TensorAdapter::NormalizeGeTensor(inputs[i]));
      GELOGI("Normalize graph %s input %lu %s[%s] -> %s[%s] addr %lu size %lu", compute_graph_tmp->GetName().c_str(),
             i, ge::TypeUtils::FormatToSerialString(inputs[i].GetTensorDesc().GetFormat()).c_str(),
             formats::JoinToString(inputs[i].GetTensorDesc().GetShape().GetDims()).c_str(),
             ge::TypeUtils::FormatToSerialString(normalized_inputs.back().MutableTensorDesc().GetFormat()).c_str(),
             formats::JoinToString(normalized_inputs.back().MutableTensorDesc().GetShape().GetDims()).c_str(),
             reinterpret_cast<uintptr_t>(normalized_inputs.back().GetData().GetData()),
             normalized_inputs.back().GetData().GetSize());
    }
    GELOGI("Start to normalize output ge tensors of graph %s", compute_graph_tmp->GetName().c_str());
    std::vector<GeTensor> normalized_outputs;
    normalized_outputs.reserve(outputs.size());
    for (size_t i = 0; i < outputs.size(); i++) {
      normalized_outputs.emplace_back(TensorAdapter::NormalizeGeTensor(outputs[i]));
      GELOGI("Normalize graph %s output %lu %s[%s] -> %s[%s] addr %lu size %lu", compute_graph_tmp->GetName().c_str(),
             i, ge::TypeUtils::FormatToSerialString(outputs[i].GetTensorDesc().GetFormat()).c_str(),
             formats::JoinToString(outputs[i].GetTensorDesc().GetShape().GetDims()).c_str(),
             ge::TypeUtils::FormatToSerialString(normalized_outputs.back().MutableTensorDesc().GetFormat()).c_str(),
             formats::JoinToString(normalized_outputs.back().MutableTensorDesc().GetShape().GetDims()).c_str(),
             reinterpret_cast<uintptr_t>(normalized_outputs.back().GetData().GetData()),
             normalized_outputs.back().GetData().GetSize());
    }
    if (compute_graph_tmp->GetGraphOutNodesInfo().size() != normalized_outputs.size()) {
      GELOGE(GE_GRAPH_GET_IN_OUT_FAILED, "Run graph with stream async output size mismatch expect %lu vs. %lu",
             compute_graph_tmp->GetGraphOutNodesInfo().size(), normalized_outputs.size());
      return GE_GRAPH_GET_IN_OUT_FAILED;
    }

    size_t output_index = 0;
    for (auto &retval : compute_graph_tmp->GetGraphOutNodesInfo()) {
      GE_CHECK_NOTNULL(retval.first);
      GE_CHECK_NOTNULL(retval.first->GetOpDesc());
      ret = retval.first->GetOpDesc()->UpdateOutputDesc(retval.second,
                                                        normalized_outputs[output_index++].MutableTensorDesc());
      if (ret != GRAPH_SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Update output desc size failed for op:%s(%s) index:0 ",
                          retval.first->GetOpDesc()->GetName().c_str(), retval.first->GetOpDesc()->GetType().c_str());
        GELOGE(FAILED, "[Update][OutputDesc] for graph mode graph data failed, op:%s.",
               retval.first->GetOpDesc()->GetName().c_str());
        return FAILED;
      }
    }
    ret = StartForRunGraph(graph_node, normalized_inputs, ge_root_model, session_id);
  } else {
    ret = StartForRunGraph(graph_node, inputs, ge_root_model, session_id);
  }
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][StartForRunGraph] failed, session_id:%lu", session_id);
    graph_node->SetRunFlag(false);
    return ret;
  }
  return InnerRunGraphWithStream(graph_node, graph_id, stream, inputs, outputs);
}

Status GraphManager::RunGraph(const GraphId &graph_id, const std::vector<GeTensor> &inputs,
                              std::vector<GeTensor> &outputs, uint64_t session_id) {
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
  std::lock_guard<std::mutex> lock(run_mutex_);
  GELOGI("[RunGraph] start to run graph, graph_id = %u, is_train_graph: %d", graph_id, GetTrainFlag());

  if (inputs.empty()) {
    GELOGI("[RunGraph] initialize sub graph has no inputs");
  }

  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, graph not exist, graph_id = %u.", graph_id);
    return ret;
  }

  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] graph node is NULL, graph_id = %u.", graph_id);
    return GE_GRAPH_GRAPH_NODE_NULL;
  }

  if (graph_node->GetRunFlag()) {
    REPORT_INNER_ERROR("E19999", "Graph is already running, can't be run again, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_ALREADY_RUNNING, "[Get][RunFlag] graph already running, graph id = %u", graph_id);
    return GE_GRAPH_ALREADY_RUNNING;
  }

  UpdateLocalOmgContext(graph_id);

  // set graph's run flag
  graph_node->SetRunFlag(true);
  GE_CHECK_NOTNULL(graph_node->GetGraph());
  ComputeGraphPtr compute_graph_tmp = GraphUtils::GetComputeGraph(*(graph_node->GetGraph()));

  GE_IF_BOOL_EXEC(GetTrainFlag(),
                  GE_IF_BOOL_EXEC(compute_graph_tmp == nullptr,
                                  REPORT_CALL_ERROR("E19999", "compute_graph is nullptr in graph_node, graph_id:%u, "
                                                    "check invalid", graph_id);
                                  GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Get][ComputeGraph] failed, "
                                         "compute_graph_tmp is NULL, graph id = %u.", graph_id);
                                  return GE_GRAPH_GRAPH_NODE_NULL;))

  if (options_.local_fmk_op_flag) {
    GetCompilerStages(graph_id).optimizer.TranFrameOp(compute_graph_tmp);
  }

  FlowModelPtr flow_model = nullptr;
  ret = StartForRunGraph(graph_node, inputs, flow_model, session_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][StartForRunGraph] failed, session_id:%lu", session_id);
    graph_node->SetRunFlag(false);
    return ret;
  }

  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  // excute graph
  ret = InnerRunGraph(graph_node, graph_id, inputs, outputs);
  if (ret != SUCCESS) {
    return ret;
  }

  if (GetTrainFlag()) {
    if (compute_graph_tmp->IsSummaryGraph()) {
      ret = SummaryHandle(graph_id, outputs);
      if (ret != SUCCESS) {
        GELOGE(ret, "[Call][SummaryHandle] failed, graph_id:%u", graph_id);
      }
    }

    auto flow_root_model = graph_node->GetFlowModel();
    GE_CHECK_NOTNULL(flow_root_model);
    for (auto &root_model : flow_root_model->GetSubmodels()) {
      if (root_model.second != nullptr) {
        GELOGI("Start CheckpointHandle.");
        auto checkPointGraph = root_model.second->GetRootGraph();
        if (IsCheckpointGraph(checkPointGraph)) {
          ret = CheckpointHandle(graph_id, checkPointGraph, outputs);
          if (ret != SUCCESS) {
            GELOGE(ret, "[Check][PointHandle] failed, graph_id:%u", graph_id);
          }
        }
      }
    }
  }

  GELOGI("[RunGraph] run graph success, graph_id = %u.", graph_id);
  return SUCCESS;
}

Status GraphManager::GenerateInfershapeGraph(GraphId &graph_id) {
  GELOGI("[DumpInfershapeJson] start to DumpInfershapeJson graph, graph_id=%u.", graph_id);
  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, graph not exist, graph_id = %u.", graph_id);
    return ret;
  }

  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid",
                       graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Get][GraphNode] graph node is NULL, graphId = %u.", graph_id);
    return GE_GRAPH_GRAPH_NODE_NULL;
  }

  UpdateLocalOmgContext(graph_id);

  ret = GetCompilerStages(graph_id).preparer.GenerateInfershapeGraph(graph_node->GetGraph());
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][CompilerStages] failed");
    return ret;
  }

  GELOGI("[DumpInfershapeJson] Dump infershape json success, graph_id=%u.", graph_id);
  return ret;
}

Status GraphManager::BuildGraphForUnregisteredOp(const GraphId &graph_id, const std::vector<GeTensor> &inputs,
                                                 FlowModelPtr &flow_model, uint64_t session_id) {
  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid",
                       graph_id);
    GELOGE(ret, "[Get][GraphNode] graph not exist, graph_id = %u.", graph_id);
    return ret;
  }

  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] graph node is NULL, graphId = %u.", graph_id);
    return GE_GRAPH_GRAPH_NODE_NULL;
  }

  UpdateLocalOmgContext(graph_id);

  GE_CHECK_NOTNULL(graph_node->GetGraph());
  auto compute_graph = GraphUtils::GetComputeGraph(*graph_node->GetGraph());
  GE_CHECK_NOTNULL(compute_graph);

  GM_RUN_AND_DUMP_PERF("PrepareInit", GetCompilerStages(graph_id).preparer.PrepareInitAndUpdateInput,
                       graph_node, inputs, session_id);
  GM_RUN_AND_DUMP_PERF("Prepare", GetCompilerStages(graph_id).preparer.PrepareDynShape);

  for (auto &node : compute_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    OpDescPtr op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if (op_desc->HasAttr(ATTR_NAME_UNREGST_OPPATH)) {
      std::vector<ge::NodePtr> node_vec = {node};

      auto instance_ptr = ge::GELib::GetInstance();
      if (instance_ptr == nullptr || !instance_ptr->InitFlag()) {
        REPORT_INNER_ERROR("E19999", "GELib is not init before, graph_id:%u, check invalid", graph_id);
        GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Get][GELib] GELib is not init before, graph_id:%u", graph_id);
        return GE_CLI_GE_NOT_INITIALIZED;
      }

      OpsKernelInfoStorePtr kernel_info =
          instance_ptr->OpsKernelManagerObj().GetOpsKernelInfoStore(op_desc->GetOpKernelLibName());
      if (kernel_info == nullptr) {
        REPORT_INNER_ERROR("E19999", "GetOpsKernelInfoStore fail for op:%s(%s), kernel_lib_name:%s, graph_id:%u, "
                           "check invalid", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                           op_desc->GetOpKernelLibName().c_str(), graph_id);
        GELOGE(FAILED, "[Get][OpsKernelInfoStore] fail for op:%s(%s), kernel_lib_name:%s, graph_id:%u",
               op_desc->GetName().c_str(), op_desc->GetType().c_str(),
               op_desc->GetOpKernelLibName().c_str(), graph_id);
        return FAILED;
      }

      ret = kernel_info->CompileOp(node_vec);
      if (ret != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Call CompileOp fail for op:%s(%s), kernel_lib_name:%s, graph_id:%u, "
                          "check invalid", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                          op_desc->GetOpKernelLibName().c_str(), graph_id);
        GELOGE(ret, "[Compile][Op] failed, op = %s, graph_id = %u.", op_desc->GetName().c_str(), graph_id);
        return ret;
     }
    }
  }
  GeRootModelPtr ge_root_model = nullptr;
  auto flow_root_model = ge::MakeShared<ge::FlowModel>(compute_graph);
  GE_CHECK_NOTNULL(flow_root_model);
  GM_RUN_AND_DUMP_PERF("Build", Build, graph_node, compute_graph, ge_root_model, session_id);
  (void)flow_root_model->AddSubModel(ge_root_model);
  graph_node->SetFlowModel(flow_root_model);
  flow_model = flow_root_model;
  return SUCCESS;
}

Status GraphManager::BuildGraph(const GraphId &graph_id, const std::vector<GeTensor> &inputs,
                                FlowModelPtr &flow_model, uint64_t session_id, bool async) {
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
  GELOGD("[BuildGraph] start to build graph, graph_id:%u", graph_id);
  if (inputs.empty()) {
    GELOGW("[BuildGraph] BuildGraph warning: empty GeTensor inputs");
  }

  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, graph not exist, graph_id = %u.", graph_id);
    return ret;
  }

  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] graph node is NULL, graphId = %u.", graph_id);
    return GE_GRAPH_GRAPH_NODE_NULL;
  }

  if (graph_node->GetRunFlag()) {
    REPORT_INNER_ERROR("E19999", "Graph is already running, can't be run again, graph_id:%u, "
                       "check invalid", graph_id);
    GELOGE(GE_GRAPH_ALREADY_RUNNING, "[Get][RunFlag] graph already running, graph id = %u", graph_node->GetGraphId());
    return GE_GRAPH_ALREADY_RUNNING;
  }

  UpdateLocalOmgContext(graph_id);

  graph_node->SetAsync(async);
  // set graph's run flag
  graph_node->SetRunFlag(true);

  ret = StartForRunGraph(graph_node, inputs, flow_model, session_id);
  graph_node->SetRunFlag(false);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][StartForRunGraph] failed! graph_id:%u.", graph_id);
    return ret;
  }

  GELOGI("[BuildGraph] build graph success, graph_id=%u.", graph_id);
  return ret;
}

///
/// @ingroup ge_graph
/// @brief Save extra attribute to Model
/// @param [in] model: Model attribues will save to.
/// @param [in] type: type of OpDesc.
/// @param [in] attrs: attributes of OpDesc.
/// @param [in] inputs: inputs tensor.
/// @param [in] outputs: outputs tensor.
/// @return: Status
///
Status GraphManager::SaveParams(ge::GeModel &model, const std::string &type, const std::map<std::string,
    GeAttrValue> &attrs, const std::vector<GeTensor> &inputs, const std::vector<GeTensor> &outputs) const {
  GE_CHK_BOOL_EXEC(ge::AttrUtils::SetStr(&model, "ATTR_MODEL_OP_TYPE", type), return FAILED,
                   "[Set][Str] model type[%s] fail", type.c_str());

  for (const auto &it : attrs) {
    GE_CHK_BOOL_EXEC(model.SetAttr("ATTR_MODEL_" + it.first, it.second) == GRAPH_SUCCESS, return FAILED,
                     "[Set][Attr] OpDesc attribute[%s] fail", it.first.c_str());
  }

  GE_CHK_BOOL_EXEC(ge::AttrUtils::SetListTensor(&model, "ATTR_MODEL_TENSOR_INPUTS", inputs), return FAILED,
                   "[Set][InputsTensor] list fail");
  GE_CHK_BOOL_EXEC(ge::AttrUtils::SetListTensor(&model, "ATTR_MODEL_TENSOR_OUTPUTS", outputs), return FAILED,
                   "[Set][OutputsTensor] list fail");

  return SUCCESS;
}

bool GraphManager::CheckModelLoad(const FlowModelPtr &flow_model, bool load_flag) const {
  return ((flow_model != nullptr) && load_flag);
}

Status GraphManager::RemoveGraph(const GraphId &graph_id) {
  std::set<GraphId>::const_iterator it = to_be_deleted_graphs_.find(graph_id);
  if (it != to_be_deleted_graphs_.cend()) {
    (void)to_be_deleted_graphs_.erase(it);
  }
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS || graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NOT_EXIST, "[Get][GraphNode] Id %u does not exists.", graph_id);
    return GE_GRAPH_GRAPH_NOT_EXIST;
  }
  if (graph_node->GetRunFlag()) {
    // only put graph into to-be-deleted list when exceptional scenario
    (void)to_be_deleted_graphs_.insert(graph_id);
    GELOGI("[GraphManager] Trying to remove running graph[Id:%u], added into to_be_deleted_graphs_.", graph_id);
    return SUCCESS;
  }

  std::lock_guard<std::mutex> lock(unload_model_mutex_);

  graph_rebuild_state_ctrl_.RemoveGraph(graph_id);
  RemoveGraphNode(graph_id);
  auto flow_root_model = graph_node->GetFlowModel();
  if (CheckModelLoad(flow_root_model, graph_node->GetLoadFlag())) {
    Status middle_ret = UnloadModel(flow_root_model, graph_id);
    if (middle_ret != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "UnloadModel for graph:%u failed, check invalid", graph_id);
      GELOGE(middle_ret, "[Unload][Model] model failed, graph_id=%u.", graph_id);
      ret = middle_ret;
    }
  }

  RemoveCompilerStages(graph_id);
  RemoveGraphCount(graph_id);
  RemoveAddGraphCondition(graph_id);

  GE_CHK_STATUS_RET(ret, "[Remove][Graph] failed, graph_id=%u.", graph_id);
  GELOGI("[GraphManager] remove graph success, graph_id=%u.", graph_id);
  return SUCCESS;
}

Status GraphManager::ParseOptions(const std::map<std::string, std::string> &options) {
  Status ret;

  ParseOption(options, "ge.INPUT_NODES_SET_FP16", options_.input_nodes_set_fp16);
  // parse streams max parallel num
  ret = ParseOption(options, STREAM_MAX_PARALLEL_NUM, options_.stream_max_parallel_num);
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_OPTIONS_INVALID,
           "[Parse][Option] %s value failed, it must be same format as DNN_V100:2,DNN_HCCL:3",
           STREAM_MAX_PARALLEL_NUM.c_str());
    return GE_GRAPH_OPTIONS_INVALID;
  }

  // get stream num
  ret = ParseOption(options, STREAM_NUM, options_.stream_num);
  if ((ret != SUCCESS) || (options_.stream_num == 0)) {
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.stream_num, its value %d is invalid, "
           "must be not equal zero.", options_.stream_num);
    return GE_GRAPH_OPTIONS_INVALID;
  }

  // get perf level, its value please see enum PerfLevel
  ret = ParseOption(options, PERF_LEVEL, options_.perf_level);
  if ((ret != SUCCESS) || IsPerfLevelInvalid(options_.perf_level)) {
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.perfLevel, its value %d is invalid, "
           "must be enum PerfLevel type.", options_.perf_level);
    return GE_GRAPH_OPTIONS_INVALID;
  }

  // get encrypt mode
  ret = ParseOption(options, ENCRYPT_MODE, options_.encrypt_mode);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.encryptMode value invalid.");
                  return GE_GRAPH_OPTIONS_INVALID);

  // get ek file
  ParseOption(options, EK_FILE, options_.ek_file);

  // get cert file
  ParseOption(options, CERT_FILE, options_.cert_file);

  // get hw key file
  ParseOption(options, HW_KEY_FILE, options_.hw_key_file);

  // get private file
  ParseOption(options, PRIVATE_KEY_FILE, options_.private_key_file);

  // get framework type, its value please see enum FrameworkType
  ret = ParseOption(options, FRAMEWORK_TYPE, options_.framework_type);
  if (ret != SUCCESS) {
    // print error log in ParseOption
    return GE_GRAPH_OPTIONS_INVALID;
  }

  // get calibration info file
  ParseOption(options, CALIBRATION_CONF_FILE, options_.calibration_conf_file);

  // get insert op info file
  ParseOption(options, INSERT_OP_FILE, options_.insert_op_file);

  // get output node name
  ParseOption(options, OUTPUT_NODE_NAME, options_.output_node_name);

  // get function bin path
  ParseOption(options, "ge.func_bin_path", options_.func_bin_path);

  // get core type
  ParseOption(options, CORE_TYPE, options_.core_type);

  // get weight compress flag
  ret = ParseOption(options, COMPRESS_FLAG, options_.compress_flag);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.compressFlag value is invalid, "
                         "must be 0 or 1.");
                  return GE_GRAPH_OPTIONS_INVALID);
  // Set Build model and step
  ParseOption(options, BUILD_MODE, options_.build_mode);
  ParseOption(options, BUILD_STEP, options_.build_step);
  ParseOption(options, TUNING_PATH, options_.tuning_path);

  // ge.graphType.
  options_.run_graph_flag = true;
  ret = ParseOption(options, RUN_FLAG, options_.run_graph_flag);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.runFlag value is invalid, must be 0 or 1.");
                  return GE_GRAPH_OPTIONS_INVALID);

  // ge.graphType
  ret = ParseTrainGraphFlag(options_.train_graph_flag);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][TrainGraphFlag] Key:ge.runFlag value is invalid");
                  return GE_GRAPH_OPTIONS_INVALID);

  // parse FmkOp
  options_.local_fmk_op_flag = false;
  ret = ParseOption(options, LOCAL_FMKOP_FLAG, options_.local_fmk_op_flag);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.localFmkopFlag value is invalid, "
                         "must be 0 or 1.");
                  return GE_GRAPH_OPTIONS_INVALID);
  options_.enable_print_op_pass = true;
  ret = ParseOption(options, ENABLE_PRINT_OP_PASS, options_.enable_print_op_pass);

  options_.is_single_op = false;
  ret = ParseOption(options, SINGLE_OP_FLAG, options_.is_single_op);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.enablePrintOpPass value is invalid, "
                         "must be 0 or 1.");
                  return GE_GRAPH_OPTIONS_INVALID);
  // parse hcom parallel
  options_.hcom_parallel = false;
  ret = ParseOption(options, HCOM_PARALLEL, options_.hcom_parallel);
  GE_IF_BOOL_EXEC(ret != SUCCESS,
                  GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][Option] Key:ge.hcomParallel value is invalid, "
                         "must be 0 or 1.");
                  return GE_GRAPH_OPTIONS_INVALID);
  // net output node dataType
  ParseOption(options, OUTPUT_DATATYPE, options_.output_datatype);

  // Set save_original_model flag (ge.save_original_model)
  ParseOption(options, SAVE_ORIGINAL_MODEL, options_.save_original_model);
  // Original model file name
  ParseOption(options, ORIGINAL_MODEL_FILE, options_.original_model_file);

  ParseOption(options, INPUT_SHAPE, options_.input_shape);
  ParseOption(options, kDynamicDims, options_.dynamic_dims);
  ParseOption(options, DYNAMIC_NODE_TYPE, options_.dynamic_node_type);
  GELOGD("Dynamic dims params: input shape is %s, dynamic dims is %s, dynamic node type is %d",
         options_.input_shape.c_str(), options_.dynamic_dims.c_str(), options_.dynamic_node_type);

  return SUCCESS;
}

// OPTION_GRAPH_RUN_MODE is supposed to be a session-level option, but it used to be set to global-level in the past.
// If can not parse from session, it can parse from global by GetContext().
Status GraphManager::ParseTrainGraphFlag(bool &train_flag) {
  train_flag = false;
  std::string run_mode;
  if (GetContext().GetOption(ge::OPTION_GRAPH_RUN_MODE, run_mode) == SUCCESS && !run_mode.empty()) {
    if (GraphRunMode(std::strtol(run_mode.c_str(), nullptr, kBase)) >= TRAIN) {
      train_flag = true;
    }
  }
  domi::GetContext().train_flag = train_flag;
  GELOGI("Is train flag: %d.", train_flag);
  return SUCCESS;
}

bool GraphManager::IsPerfLevelInvalid(int32_t perf_level) {
  return ((perf_level != static_cast<int32_t>(PerfLevel::GEN_TASK_WITHOUT_L2FUSION)) &&
          (perf_level != static_cast<int32_t>(PerfLevel::GEN_TASK_WITHOUT_FUSION)) &&
          (perf_level != -1));
}

void GraphManager::ParseOption(const std::map<std::string, std::string> &options, const std::string &key,
                               std::string &option) {
  auto iter = options.find(key);
  if (iter != options.end()) {
    GELOGD("Set option %s from value %s to value%s", key.c_str(), option.c_str(), iter->second.c_str());
    option = iter->second;
  }
}

Status GraphManager::ParseOption(const std::map<std::string, std::string> &options, const std::string &key,
                                 bool &option) {
  auto iter = options.find(key);
  if (iter != options.end()) {
    std::string flag = iter->second;
    if (flag == "0") {
      option = false;
    } else if (flag == "1") {
      option = true;
    } else {
      REPORT_INNER_ERROR("E19999", "Option:%s value:%s must be 0 or 1, check invalid", key.c_str(), flag.c_str());
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] Key:%s, its value %s is invalid, it must be 0 or 1.",
             key.c_str(), flag.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }
  }
  return SUCCESS;
}

Status GraphManager::ParseOption(const std::map<std::string, std::string> &options, const std::string &key,
                                 int32_t &option) {
  const int32_t kDecimal = 10;
  char *ptr = nullptr;
  auto iter = options.find(key);
  if (iter != options.end()) {
    option = static_cast<int32_t>(std::strtol(iter->second.c_str(), &ptr, kDecimal));
    if (ptr != nullptr && *ptr != '\0') {
      REPORT_INNER_ERROR("E19999", "Option:%s value:%s must be int32_t type, check invalid",
                         key.c_str(), iter->second.c_str());
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] Key:%s, its value %s is invalid, must be int32_t type.",
             key.c_str(), iter->second.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }
  }
  return SUCCESS;
}

void GraphManager::Trim(std::string &str) {
  if (!str.empty()) {
    auto it = str.find_first_not_of(" ");
    if (it != std::string::npos) {
      (void)str.erase(0, it);
    }
    it = str.find_last_not_of(" ");
    if (it != std::string::npos) {
      (void)str.erase(it + 1);
    }
  }
}

Status GraphManager::ParseOption(const std::map<std::string, std::string> &options, const std::string &key,
                                 std::map<std::string, int32_t> &option) {
  auto iter = options.find(key);
  if (iter == options.end()) {
    return SUCCESS;
  }
  GELOGI("Start to parse %s", key.c_str());
  option.clear();
  std::string op_num = iter->second;

  // split std::string by ','
  std::vector<std::string> split;
  std::istringstream f(op_num);
  std::string str_tmp;
  while (getline(f, str_tmp, ',')) {
    split.push_back(str_tmp);
  }

  for (const std::string &engine_parallel : split) {
    // split engine and num by :
    size_t pos = engine_parallel.find(':');
    if (pos == std::string::npos) {
      REPORT_INNER_ERROR("E19999", "Option:%s, value:%s, engine and num must be connected by :, check invalid",
                         key.c_str(), engine_parallel.c_str());
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] engine and num must be connected by :, "
             "while your input is %s", engine_parallel.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }
    std::string engine_name = engine_parallel.substr(0, pos);
    std::string parallel_num = engine_parallel.substr(pos + 1);
    Trim(engine_name);
    Trim(parallel_num);

    Status ret = CheckEngineName(engine_name, key, option);
    if (ret != SUCCESS) {
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][EngineName] %s failed", engine_name.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }

    int32_t num = 0;
    ret = ParseParallelNum(parallel_num, key, num);
    if (ret != SUCCESS) {
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Parse][ParallelNum] %s failed", parallel_num.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }

    (void)option.insert(std::make_pair(engine_name, num));
  }
  GELOGI("Parse %s successfully", key.c_str());
  return SUCCESS;
}

Status GraphManager::CheckEngineName(const std::string &engine_name, const std::string &key,
                                     const std::map<std::string, int32_t> &option) {
  if (engine_name.empty()) {
    REPORT_INNER_ERROR("E19999", "Option:%s, param engine_name:%s is empty, check invalid",
                       key.c_str(), engine_name.c_str());
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] engine name of %s is empty", key.c_str());
    return GE_GRAPH_OPTIONS_INVALID;
  }
  // judge whether exist in engine list
  GE_CHECK_NOTNULL(GELib::GetInstance());
  if (!GELib::GetInstance()->DNNEngineManagerObj().IsEngineRegistered(engine_name)) {
    GELOGW("engine : %s is not registered in %s", engine_name.c_str(), key.c_str());
  }

  auto it_stream_repeat = option.find(engine_name);
  if (it_stream_repeat != option.end()) {
    REPORT_INNER_ERROR("E19999", "Option:%s, param engine_name:%s is repeated, check invalid",
                       key.c_str(), engine_name.c_str());
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] engine:%s of %s is repeated", engine_name.c_str(), key.c_str());
    return GE_GRAPH_OPTIONS_INVALID;
  }
  return SUCCESS;
}

Status GraphManager::ParseParallelNum(const std::string &parallel_num, const std::string &key, int32_t &num) {
  if (parallel_num.empty()) {
    REPORT_INNER_ERROR("E19999", "Option:%s, param parallel num:%s is empty, check invalid",
                       key.c_str(), parallel_num.c_str());
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] parallel num of %s is empty", key.c_str());
    return GE_GRAPH_OPTIONS_INVALID;
  }
  for (char c : parallel_num) {
    if (!isdigit(c)) {
      REPORT_INNER_ERROR("E19999", "Option:%s, param parallel num:%s is not digit, check invalid",
                         key.c_str(), parallel_num.c_str());
      GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] Option:%s, param parallel num:%s is not digit, check invalid",
             key.c_str(), parallel_num.c_str());
      return GE_GRAPH_OPTIONS_INVALID;
    }
  }

  GE_CHK_STATUS_RET_NOLOG(ConvertToInt32(parallel_num, num));
  if (num < 1) {
    REPORT_INNER_ERROR("E19999", "Option:%s, param parallel num:%s < 1, check invalid",
                       key.c_str(), parallel_num.c_str());
    GELOGE(GE_GRAPH_OPTIONS_INVALID, "[Check][Param] parallel num:%s of %s must bigger than 0",
           parallel_num.c_str(), key.c_str());
    return GE_GRAPH_OPTIONS_INVALID;
  }
  return SUCCESS;
}


void GraphManager::AddGraphNode(GraphId graph_id, const GraphNodePtr &graph_node) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  graph_map_.emplace(graph_id, graph_node);
}

void GraphManager::RemoveGraphNode(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  (void)graph_map_.erase(graph_id);
}

bool GraphManager::HasGraphNode(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  return graph_map_.find(graph_id) != graph_map_.end();
}

Status GraphManager::GetGraphNode(const GraphId &graph_id, GraphNodePtr &out) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  auto iter = graph_map_.find(graph_id);
  if (iter == graph_map_.end()) {
    out = nullptr;
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NOT_EXIST, "[Check][Param] graph not exist, graph_id= %u.", graph_id);
    return GE_GRAPH_GRAPH_NOT_EXIST;
  }
  out = iter->second;
  return SUCCESS;
}

Status GraphManager::GetVariable(const std::string &name, Tensor &val) const {
  GeTensorPtr ge_tensor_ptr = TensorAdapter::AsGeTensorPtr(val);
  GE_CHECK_NOTNULL(ge_tensor_ptr);
  GE_CHECK_NOTNULL(GetGraphContext());
  return GetGraphContext()->GetVariableTensor(name, *(ge_tensor_ptr.get()));
}

Status GraphManager::SummaryHandle(const GraphId &graph_id, std::vector<GeTensor> &outputs) {
  std::vector<GeTensor> without_summary_outputs;
  std::set<int32_t> summary_output_index;
  GELOGI("[GraphManager] SummaryHandle, outputsSize=%zu.", outputs.size());
  const std::map<uint32_t, std::map<std::string, size_t>> &whole_summary_output_indexes =
      GetCompilerStages(graph_id).optimizer.GetSummaryOutputIndexes();
  if (whole_summary_output_indexes.find(graph_id) == whole_summary_output_indexes.end()) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in whole_summary_output_indexes, check invalid", graph_id);
    GELOGE(FAILED, "[Check][Param] Graph:%u not exist in whole_summary_output_indexes", graph_id);
    return FAILED;
  }
  const std::map<std::string, size_t> &summary_output_indexes = whole_summary_output_indexes.at(graph_id);
  GELOGI("[GraphManager] SummaryHandle, summaryOutputIndexesSize=%zu.", summary_output_indexes.size());
  std::map<std::string, Tensor> summary_results;
  for (auto iter = summary_output_indexes.begin(); iter != summary_output_indexes.end(); ++iter) {
    GELOGI("[GraphManager] SummaryHandle, summaryName=%s, outputIndex=%zu.", iter->first.c_str(), iter->second);
    summary_results.emplace(iter->first, TensorAdapter::AsTensor(outputs.at(iter->second)));
    summary_output_index.emplace(iter->second);
  }

  // remove summary data from outputs
  if (!summary_output_index.empty()) {
    for (size_t j = 0; j < outputs.size(); ++j) {
      if (summary_output_index.count(j) == 0) {
        without_summary_outputs.emplace_back(outputs.at(j));
      }
    }
    outputs.swap(without_summary_outputs);
    GELOGI("[GraphManager] SummaryHandle, after swap outputsSize=%zu.", outputs.size());
  }

  if (!summary_results.empty()) {
    return PushSummaryData2ME(graph_id, summary_results);
  }

  return SUCCESS;
}

Status GraphManager::CheckpointHandle(const GraphId &graph_id, const ComputeGraphPtr &compute_graph,
                                      const std::vector<GeTensor> &outputs) {
  GELOGI("[GraphManager] CheckpointHandle, outputsSize=%zu.", outputs.size());

  GE_CHECK_NOTNULL(compute_graph);
  std::map<std::string, Tensor> save_results;
  NodePtr netoutput = nullptr;
  for (const auto &node : compute_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    if (node->GetType() == kNetOutput) {
      netoutput = node;
      break;
    }
  }
  if (netoutput == nullptr) {
    REPORT_INNER_ERROR("E19999", "No netoutput node in graph:%u, check invalid", graph_id);
    GELOGE(FAILED, "[Check][Param] No netoutput node in graph:%u", graph_id);
    return FAILED;
  }
  for (const auto &in : netoutput->GetAllInDataAnchors()) {
    std::string desc_name;
    GE_CHECK_NOTNULL(in);
    auto out_anchor = in->GetPeerOutAnchor();
    if (out_anchor == nullptr) {
      REPORT_INNER_ERROR("E19999", "Peer anchor of op:%s(%s), in_index:%u is nullptr, graph_id:%u, check invalid",
                         netoutput->GetName().c_str(), netoutput->GetType().c_str(), in->GetIdx(), graph_id);
      GELOGE(FAILED, "[Get][PeerOutAnchor] Peer anchor of op:%s(%s), in_index:%u is nullptr, graph_id:%u",
             netoutput->GetName().c_str(), netoutput->GetType().c_str(), in->GetIdx(), graph_id);
      return FAILED;
    }
    ge::NodePtr peer_node = out_anchor->GetOwnerNode();
    // find the variable node in graph
    while (peer_node != nullptr && peer_node->GetType() != kVariable) {
      if (peer_node->GetAllInDataAnchors().size() != 1) {
        REPORT_INNER_ERROR("E19999", "More than one prior nodes of peer_node:%s(%s) in checkpoint Graph:%u, "
                           "check invalid", peer_node->GetName().c_str(), peer_node->GetType().c_str(), graph_id);
        GELOGE(FAILED, "[Check][Param] More than one prior nodes of peer_node:%s(%s) in checkpoint Graph:%u.",
               peer_node->GetName().c_str(), peer_node->GetType().c_str(), graph_id);
        return FAILED;
      }
      auto peer_node_in = peer_node->GetAllInDataAnchors().at(0);
      GE_CHECK_NOTNULL(peer_node_in);
      auto peer_node_out_anchor = peer_node_in->GetPeerOutAnchor();
      if (peer_node_out_anchor != nullptr) {
        peer_node = peer_node_out_anchor->GetOwnerNode();
        GE_CHECK_NOTNULL(peer_node);
        if (peer_node->GetType() == kVariable) {
          break;
        }
      }
    }
    if (peer_node == nullptr) {
      REPORT_INNER_ERROR("E19999", "Peer anchor node of op:%s(%s), in_index:%u is nullptr, graph_id:%u, check invalid",
                         netoutput->GetName().c_str(), netoutput->GetType().c_str(), in->GetIdx(), graph_id);
      GELOGE(FAILED, "[Check][Param] Peer anchor node of op:%s(%s), in_index:%u is nullptr, graph_id:%u",
             netoutput->GetName().c_str(), netoutput->GetType().c_str(), in->GetIdx(), graph_id);
      return FAILED;
    }
    desc_name = peer_node->GetName();
    GELOGI("[GraphManager] CheckpointHandle, descName=%s.", desc_name.c_str());
    if (in->GetIdx() >= static_cast<int32_t>(outputs.size())) {
      REPORT_INNER_ERROR("E19999", "in index:%u of op:%s(%s) is out of outputs.size:%zu range, graph_id:%u, "
                         "check invalid", in->GetIdx(), netoutput->GetName().c_str(),
                         netoutput->GetType().c_str(), outputs.size(), graph_id);
      GELOGE(FAILED, "[Check][Param] in index:%u of op:%s(%s) is out of outputs.size:%zu range, graph_id:%u",
             in->GetIdx(), netoutput->GetName().c_str(), netoutput->GetType().c_str(), outputs.size(), graph_id);
      return FAILED;
    }
    save_results.emplace(desc_name, TensorAdapter::AsTensor(outputs.at(in->GetIdx())));
  }

  if (!save_results.empty()) {
    return PushSaveData2ME(graph_id, save_results);
  }

  return SUCCESS;
}

Status GraphManager::RegisterCallBackFunc(
    const std::string &key,
    const std::function<Status(uint32_t, const std::map<std::string, ge::Tensor> &)> &callback) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  GELOGI("[GraphManager] RegisterCallBackFunc, key=%s.", key.c_str());
  me_callback_map_[key] = callback;
  return SUCCESS;
}

Status GraphManager::RegisterCallBackFunc(
    const std::string &key,
    const std::function<Status(uint32_t, const std::map<AscendString, ge::Tensor> &)> &callback) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  GELOGI("[GraphManager] RegisterCallBackFunc, key=%s.", key.c_str());
  callback_map_[key] = callback;
  return SUCCESS;
}

Status GraphManager::PushSummaryData2ME(const GraphId &graph_id,
                                        const std::map<std::string, ge::Tensor> &summary_data) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  GELOGI("[GraphManager] PushSummaryData2ME, dataSize=%zu.", summary_data.size());
  auto itr = me_callback_map_.find(kSummary);
  if (itr == me_callback_map_.end()) {
    auto iter = callback_map_.find(kSummary);
    if (iter != callback_map_.end()) {
      std::map<AscendString, ge::Tensor> tmp_summary_data;
      for (auto &data : summary_data) {
        AscendString tmp(data.first.c_str());
        tmp_summary_data[tmp] = data.second;
      }
      return iter->second(graph_id, tmp_summary_data);
    }
    REPORT_INNER_ERROR("E19999", "No summary callback found, graph_id:%u, check invalid", graph_id);
    GELOGE(FAILED, "[Check][Param] No summary callback found, graph_id:%u", graph_id);
    return FAILED;
  }
  return itr->second(graph_id, summary_data);
}

Status GraphManager::PushSaveData2ME(const GraphId &graph_id, const std::map<std::string, ge::Tensor> &save_data) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  GELOGI("[GraphManager] PushSaveData2ME, dataSize=%zu.", save_data.size());
  auto itr = me_callback_map_.find(kSave);
  if (itr == me_callback_map_.end()) {
    auto iter = callback_map_.find(kSave);
    if (iter != callback_map_.end()) {
      std::map<AscendString, ge::Tensor> tmp_save_data;
      for (auto &data : save_data) {
        AscendString tmp(data.first.c_str());
        tmp_save_data[tmp] = data.second;
      }
      return iter->second(graph_id, tmp_save_data);
    }
    REPORT_INNER_ERROR("E19999", "No checkpoint callback found, graph_id:%u, check invalid", graph_id);
    GELOGE(FAILED, "[Check][Param] No checkpoint callback found, graph_id:%u", graph_id);
    return FAILED;
  }
  return itr->second(graph_id, save_data);
}

bool GraphManager::CheckNetOutputForCheckpointGraph(const NodePtr &node) const {
  GE_RT_FALSE_CHECK_NOTNULL(node);
  size_t in_data_anchor_size = node->GetAllInDataAnchors().size();
  for (size_t i = 0; i < in_data_anchor_size; ++i) {
    auto in = node->GetInDataAnchor(i);
    if (in == nullptr) {
      return false;
    }
    auto peerin = in->GetPeerOutAnchor();
    GE_RT_FALSE_CHECK_NOTNULL(peerin);
    GE_RT_FALSE_CHECK_NOTNULL(peerin->GetOwnerNode());
    if (peerin->GetOwnerNode()->GetType() != kVariable && (!TransOpUtil::IsTransOp(peerin->GetOwnerNode()))) {
      return false;
    }
  }
  return true;
}

bool GraphManager::CheckVariableForCheckpointGraph(const NodePtr &node) const {
  if ((node == nullptr) || (node->GetOpDesc() == nullptr) || !node->GetOpDesc()->HasAttr(kCheckPointForGetVar)) {
    return false;
  }
  auto out = node->GetOutDataAnchor(0);
  if (out == nullptr) {
    REPORT_INNER_ERROR("E19999", "anchor index:0 of op:%s(%s) is nullptr, check invalid",
                       node->GetName().c_str(), node->GetType().c_str());
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "[Get][OutDataAnchor] anchor index:0 of op:%s(%s) is nullptr",
           node->GetName().c_str(), node->GetType().c_str());
    return false;
  }
  auto peer_out = out->GetPeerInDataAnchors();
  for (size_t i = 0; i < peer_out.size(); ++i) {
    GE_RT_FALSE_CHECK_NOTNULL(peer_out.at(i));
    GE_RT_FALSE_CHECK_NOTNULL(peer_out.at(i)->GetOwnerNode());
    if (peer_out.at(i)->GetOwnerNode()->GetType() != kNetOutput &&
        (!TransOpUtil::IsTransOp(peer_out.at(i)->GetOwnerNode()))) {
      return false;
    }
  }
  return true;
}

bool GraphManager::CheckTransOpForCheckpointGraph(const NodePtr &node) const {
  GE_RT_FALSE_CHECK_NOTNULL(node);
  for (const auto &out_node : node->GetOutAllNodes()) {
    GE_RT_FALSE_CHECK_NOTNULL(out_node);
    if ((!TransOpUtil::IsTransOp(out_node)) && (out_node->GetType() != kNetOutput) && (out_node->GetType() != kSend)) {
      return false;
    }
  }

  for (const auto &in_node : node->GetInAllNodes()) {
    GE_RT_FALSE_CHECK_NOTNULL(in_node);
    if ((!TransOpUtil::IsTransOp(in_node)) && (in_node->GetType() != kVariable) && (in_node->GetType() != kRecv)) {
      return false;
    }
  }
  return true;
}

static inline bool CheckConstanOpForCheckpointGraph(const NodePtr &node) { return node->GetOutDataNodes().empty(); }

bool GraphManager::IsCheckpointGraph(ComputeGraphPtr &compute_graph) {
  if (compute_graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param compute_graph is nullptr, check invalid");
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "[Check][Param] computeGraph is nullptr.");
    return false;
  }
  for (auto &node : compute_graph->GetAllNodes()) {
    GE_RT_FALSE_CHECK_NOTNULL(node);
    OpDescPtr op = node->GetOpDesc();
    GE_RT_FALSE_CHECK_NOTNULL(op);
    if (op->GetType() == kNetOutput) {
      if (!CheckNetOutputForCheckpointGraph(node)) {
        return false;
      }
    } else if (op->GetType() == kVariable) {
      if (!CheckVariableForCheckpointGraph(node)) {
        return false;
      }
    } else if ((TransOpUtil::IsTransOp(node))) {
      if (!CheckTransOpForCheckpointGraph(node)) {
        return false;
      }
    } else if (op->GetType() == CONSTANTOP) {
      if (!CheckConstanOpForCheckpointGraph(node)) {
        return false;
      }
    } else if (op->GetType() != kSend && op->GetType() != kRecv) {
      GELOGI("this node is not allow in checkpoint sub graph, node_type: %s, node_name: %s.", op->GetType().c_str(),
             op->GetName().c_str());
      return false;
    }
  }
  GELOGI("current graph %s is checkpoint sub graph.", compute_graph->GetName().c_str());
  return true;
}

bool GraphManager::IsBroadCastOpData(const ge::NodePtr &var_node) const {
  GE_RT_FALSE_CHECK_NOTNULL(var_node);
  for (auto &out_anchor : var_node->GetAllOutDataAnchors()) {
    GE_RT_FALSE_CHECK_NOTNULL(out_anchor);
    for (auto &in_anchor : out_anchor->GetPeerInDataAnchors()) {
      GE_RT_FALSE_CHECK_NOTNULL(in_anchor);
      ge::NodePtr dst_node = in_anchor->GetOwnerNode();
      GE_RT_FALSE_CHECK_NOTNULL(dst_node);
      if (dst_node->GetType() == HCOMBROADCAST || dst_node->GetType() == HVDCALLBACKBROADCAST) {
        return true;
      }
    }
  }
  return false;
}

void GraphManager::SetAttrForHcomBroadCastOp(ge::ComputeGraphPtr &compute_graph) {
  GE_RT_VOID_CHECK_NOTNULL(compute_graph);
  // add variable attr for hccl broadcast,need to be removed after variable pass online
  for (const ge::NodePtr &node : compute_graph->GetDirectNode()) {
    GE_RT_VOID_CHECK_NOTNULL(node);
    GE_RT_VOID_CHECK_NOTNULL(node->GetOpDesc());
    if (node->GetOpDesc()->GetType() != ge::VARIABLE) {
      continue;
    }
    if (IsBroadCastOpData(node)) {
      AdjustBroadCastOpData(node);
    }
    if (IsAssignOpData(node)) {
      AdjustAssignOpData(node);
    }
  }
}

void GraphManager::AdjustBroadCastOpData(const ge::NodePtr &var_node) const {
  GE_RT_VOID_CHECK_NOTNULL(var_node);
  if (!ge::AttrUtils::SetStr(var_node->GetOpDesc(), VAR_ATTR_VAR_IS_BROADCAST, "var_is_restore")) {
    GELOGW("set var_is_restore failed");
  }
}

bool GraphManager::IsAssignOpData(const ge::NodePtr &var_node) {
  GE_RT_FALSE_CHECK_NOTNULL(var_node);
  GELOGD("IsAssignOpData var_node %s", var_node->GetName().c_str());
  std::map<std::string, std::set<int32_t>> assign_ops = {{ASSIGN, {0}}};

  ge::NodePtr assign_node = nullptr;
  if (ConfirmUseOpAndIndexByNode(var_node, assign_ops, assign_node)) {
    return true;
  }

  return false;
}

void GraphManager::AdjustAssignOpData(const ge::NodePtr &var_node) const {
  GE_RT_VOID_CHECK_NOTNULL(var_node);
  if (!ge::AttrUtils::SetStr(var_node->GetOpDesc(), VAR_ATTR_VAR_IS_RESTORE, "var_is_restore")) {
    GELOGW("SetStr var_is_restore failed");
  }
}

bool GraphManager::ConfirmUseOpAndIndexByAnchor(const InDataAnchorPtr &in_anchor,
                                                const std::map<std::string, std::set<int32_t>> &confirm_ops,
                                                NodePtr &use_node) const {
  GE_RT_FALSE_CHECK_NOTNULL(in_anchor);
  ge::NodePtr dst_node = in_anchor->GetOwnerNode();
  GE_RT_FALSE_CHECK_NOTNULL(dst_node);
  ge::OpDescPtr dst_op_desc = dst_node->GetOpDesc();
  GE_RT_FALSE_CHECK_NOTNULL(dst_op_desc);
  const std::string &dst_type = dst_op_desc->GetType();
  int32_t input_index = in_anchor->GetIdx();

  GELOGD("ConfirmUseOpAndIndex, var name %s, dst_type = %s, input index %d", dst_node->GetName().c_str(),
         dst_type.c_str(), input_index);

  if (confirm_ops.count(dst_type) > 0) {
    if (confirm_ops.at(dst_type).count(input_index) > 0) {
      use_node = dst_node;
      return true;
    }
  }
  return false;
}

bool GraphManager::ConfirmUseOpAndIndexByNode(const NodePtr &var_node,
                                              const std::map<std::string, std::set<int32_t>> &confirm_ops,
                                              NodePtr &use_node) const {
  GE_RT_FALSE_CHECK_NOTNULL(var_node);
  for (auto &out_anchor : var_node->GetAllOutDataAnchors()) {
    GE_RT_FALSE_CHECK_NOTNULL(out_anchor);
    for (auto &in_anchor : out_anchor->GetPeerInDataAnchors()) {
      GE_RT_FALSE_CHECK_NOTNULL(in_anchor);
      if (ConfirmUseOpAndIndexByAnchor(in_anchor, confirm_ops, use_node)) {
        return true;
      }
    }
  }
  return false;
}

Status GraphManager::RemoveIsolatedConstInThisGraph(const ge::ComputeGraphPtr &compute_graph) const {
  GE_CHECK_NOTNULL(compute_graph);
  for (ge::NodePtr &n : compute_graph->GetDirectNode()) {
    GE_CHECK_NOTNULL(n);
    if (n->GetOpDesc() == nullptr) {
      continue;
    }
    if (n->GetOpDesc()->GetType() == CONSTANT || n->GetOpDesc()->GetType() == CONSTANTOP) {
      // reset const type depend on train_flag
      options_.train_graph_flag ? n->GetOpDesc()->SetType(CONSTANTOP) : n->GetOpDesc()->SetType(CONSTANT);
      if (n->GetOutAllNodes().empty() && n->GetInAllNodes().empty()) {
        // it is an isolated constant, just remove it
        if (GraphUtils::RemoveJustNode(compute_graph, n) != GRAPH_SUCCESS) {
          REPORT_CALL_ERROR("E19999", "Remove constant op:%s(%s) failed", n->GetName().c_str(), n->GetType().c_str());
          GELOGE(FAILED, "[Call][RemoveJustNode] remove constant %s failed.", n->GetName().c_str());
          return FAILED;
        }
      }
    }
  }
  return SUCCESS;
}

Status GraphManager::RemoveIsolatedConst(ge::ComputeGraphPtr &compute_graph) {
  GE_CHECK_NOTNULL(compute_graph);
  GE_CHK_STATUS_RET(RemoveIsolatedConstInThisGraph(compute_graph));
  for (auto &sub_graph : compute_graph->GetAllSubgraphs()) {
    GE_CHK_STATUS_RET(RemoveIsolatedConstInThisGraph(sub_graph));
  }
  return SUCCESS;
}

Status GraphManager::OptimizeStage1(ge::ComputeGraphPtr &compute_graph) {
  std::string options = "default";
  if (GetContext().GetOption("ge.exec.variable_acc", options) != SUCCESS) {
    GELOGI("get ge.exec.variable_acc failed. set default value.");
  }
  PassManager after_merge_passes;
  GE_CHK_STATUS_RET(
      after_merge_passes.AddPass("OptimizeStage1_1::MergeInputMemcpyPass", new (std::nothrow) MergeInputMemcpyPass));
  GE_CHK_STATUS_RET(
      after_merge_passes.AddPass("OptimizeStage1_1::SwitchDataEdgesBypass", new (std::nothrow) SwitchDataEdgesBypass));
  GE_CHK_STATUS_RET(
      after_merge_passes.AddPass("OptimizeStage1_1::ConstantFuseSamePass", new (std::nothrow) ConstantFuseSamePass));
  /*
   * Do CSE before FuseDataNodesWithCommonInputPass to resolve the scene in bertlarge as following:
   *            const
   *    /        |        \
   * cast1      cast2     cast3
   *    \         |         /
   *             case
   * the node `const` is the fused const node after ConstantFuseSamePass
   * the nodes `cast1`, `cast2` and 'cast3' will be fused by CSE.
   * in order to eliminate hard code in FuseDataNodesWithCommonInputPass,
   * we do CSE before FuseDataNodesWithCommonInputPass
   * But it is a temp solution, this CSE will be deleted after change pass from graph pass to node pass
   */
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::CSEBeforeFuseDataNodesWithCommonInputPass",
                                               new (std::nothrow) CommonSubexpressionEliminationPass));
  // FuseDataNodesWithCommonInputPass: fuse same data with common input in same graph
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::FuseDataNodesWithCommonInputPass",
                                               new (std::nothrow) FuseDataNodesWithCommonInputPass));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::CommonSubexpressionEliminationPass",
                                               new (std::nothrow) CommonSubexpressionEliminationPass));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::PermutePass", new (std::nothrow) PermutePass));
  /*
   * The SameTransdataBreadthFusionPass should be called before VariableOpPass, because of the scene following:
   *   node3
   *    |
   * transdata1   node2
   *    |         |
   *   cast1  transdata2
   *      \    /
   *        var
   * the node `transdata1` should be moved to the front of the ndoe `cast1`,
   * to ensure that `transdata1` and `transdata2` can be fusion with `var`.
   * But it is a temp solution, because the `SameTransdataBreadthFusionPass`
   * can only move `TransData` but not `Cast` nodes.
   * So if we exchange Cast and TransData, the fusion mechanism will fail.
   */
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::SameTransdataBreadthFusionPass",
                                               new (std::nothrow) SameTransdataBreadthFusionPass));
  GE_IF_BOOL_EXEC(options == "default" || options == "1", GELOGI("turn on variable accelerator");
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::VariableOpPass",
                                               new (std::nothrow) VariableOpPass(&(graph_rebuild_state_ctrl_)))));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::TransOpWithoutReshapeFusionPass",
                                               new (std::nothrow) TransOpWithoutReshapeFusionPass));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::TransOpBreadthFusionPass",
                                               new (std::nothrow) TransOpBreadthFusionPass));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::DataFlowPreparePass",
                                               new (std::nothrow) DataFlowPreparePass));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage1_1::MergeUnknownShapeNPass",
                                               new (std::nothrow) MergeUnknownShapeNPass));

  GE_TIMESTAMP_START(after_merge_passes);
  auto ret = after_merge_passes.Run(compute_graph);
  GE_TIMESTAMP_END(after_merge_passes, "GraphManager::OptimizeStage1_1");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][Passes] when OptimizeStage1_1 failed, ret:%u.", ret);
    return ret;
  }

  GE_DUMP(compute_graph, "OptimizeStage1_1");

  NamesToPass names_to_passes;
  TransOpNearbyAllreduceFusionPass trans_op_nearby_allreduce_fusion_pass;
  ReshapeRemovePass reshape_remove_pass;
  ConstantFoldingPass constant_folding_pass;
  DimensionAdjustPass dimension_adjust_pass;
  EnterPass enter_pass;
  AddNPass addn_pass;
  SwitchDeadBranchElimination switch_dead_branch_elimination;
  SwitchLogicRemovePass switch_logic_remove_pass;
  MergePass merge_pass;
  CastRemovePass cast_remove_pass;
  TransposeTransDataPass transpose_transdata_pass;
  TransOpSymmetryEliminationPass symmetry_elimination_pass;
  DimensionComputePass dimension_compute_pass;
  UselessControlOutRemovePass useless_control_out_remove_pass;
  ReplaceWithEmptyConstPass replace_with_empty_const_pass;
  names_to_passes.emplace_back("EnterPass", &enter_pass);
  names_to_passes.emplace_back("AddNPass", &addn_pass);
  names_to_passes.emplace_back("SwitchDeadBranchElimination", &switch_dead_branch_elimination);
  names_to_passes.emplace_back("SwitchLogicRemovePass", &switch_logic_remove_pass);
  names_to_passes.emplace_back("MergePass", &merge_pass);
  names_to_passes.emplace_back("CastRemovePass", &cast_remove_pass);
  names_to_passes.emplace_back("TransposeTransDataPass", &transpose_transdata_pass);
  names_to_passes.emplace_back("ReshapeRemovePass", &reshape_remove_pass);
  names_to_passes.emplace_back("TransOpSymmetryEliminationPass", &symmetry_elimination_pass);
  names_to_passes.emplace_back("TransOpNearbyAllreduceFusionPass", &trans_op_nearby_allreduce_fusion_pass);
  names_to_passes.emplace_back("ReplaceWithEmptyConstPass", &replace_with_empty_const_pass);
  names_to_passes.emplace_back("DimensionComputePass", &dimension_compute_pass);
  names_to_passes.emplace_back("ConstantFoldingPass", &constant_folding_pass);
  names_to_passes.emplace_back("DimensionAdjustPass", &dimension_adjust_pass);
  names_to_passes.emplace_back("UselessControlOutRemovePass", &useless_control_out_remove_pass);
  GE_TIMESTAMP_START(names_to_passes);
  ret = GEPass(compute_graph).Run(names_to_passes);
  GE_TIMESTAMP_END(names_to_passes, "GraphManager::OptimizeStage1_2");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][Passes] when OptimizeStage1_2 failed, ret:%u.", ret);
    return ret;
  }
  // Calculate Op/Fe constantfolding cost
  uint64_t op_constant_folding_cost = 0;
  for (auto &it : constant_folding_pass.GetOpConstantFoldingPerfStatistic()) {
    GE_CHK_STATUS_RET(CheckUint64AddOverflow(op_constant_folding_cost, it.second.second));
    op_constant_folding_cost += it.second.second;
    GELOGI("The time cost of %s constant folding is [%lu] micro second, calls is %lu.",
           it.first.c_str(), it.second.second, it.second.first);
  }
  GEEVENT("[GEPERFTRACE] The time cost of extern constant folding is [%lu] micro second.", op_constant_folding_cost);
  for (auto &it : constant_folding_pass.GetGeConstantFoldingPerfStatistic()) {
    GE_CHK_STATUS_RET(CheckUint64AddOverflow(op_constant_folding_cost, it.second.second));
    op_constant_folding_cost += it.second.second;
    GELOGI("The time cost of %s constant folding is [%lu] micro second, calls is %lu.",
           it.first.c_str(), it.second.second, it.second.first);
  }

  GE_DUMP(compute_graph, "OptimizeStage1_2");
  PassManager graph_pass;
  // the prune pass should between SwitchPass and SwitchToStreamSwitchPass
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::Migration", new (std::nothrow) SubgraphConstMigrationPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::ArgsClean", new (std::nothrow) UnusedArgsCleanPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::PrunePass", new (std::nothrow) PrunePass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::NextIterationPass", new (std::nothrow) NextIterationPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::ControlTriggerPass", new (std::nothrow) ControlTriggerPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::MergeToStreamMergePass",
                                       new (std::nothrow) MergeToStreamMergePass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::SwitchToStreamSwitchPass",
                                       new (std::nothrow) SwitchToStreamSwitchPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::AttachStreamLabelPass",
                                       new (std::nothrow) AttachStreamLabelPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::MultiBatchPass", new (std::nothrow) MultiBatchPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::SubgraphMultiDimsPass",
                                       new (std::nothrow) SubgraphMultiDimsPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::IteratorOpPass", new (std::nothrow) IteratorOpPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::VariableRefUselessControlOutDeletePass",
                                       new (std::nothrow) VariableRefUselessControlOutDeletePass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::ReshapeRecoveryPass",
                                       new (std::nothrow) ReshapeRecoveryPass));
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::RemoveSameConstPass",
                                       new (std::nothrow) RemoveSameConstPass));
  if (options_.train_graph_flag) {
    // Priority: The GlobalStepInsertPass should work before graph partitioner.
    // Reason: Make sure that the var "global_step" can be partitioned to known sub graph and allocated memory
    GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::GlobalStepInsertPass",
                                         new (std::nothrow) GlobalStepInsertPass));

    std::string hccl_tailing_optimize;
    if (GetContext().GetOption("ge.exec.hccl_tailing_optimize", hccl_tailing_optimize) == SUCCESS &&
        hccl_tailing_optimize == "1") {
      GELOGI("Add hccl tailing optimize stage");
      GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeStage1_3::HcclTailingOptimizationPass",
                                           new (std::nothrow) HcclTailingOptimizationPass));
    }
  }
  GE_TIMESTAMP_START(graph_pass);
  ret = graph_pass.Run(compute_graph);
  GE_TIMESTAMP_END(graph_pass, "GraphManager::OptimizeStage1_3");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][Passes] when OptimizeStage1_3 failed, ret:%u.", ret);
    return ret;
  }
  NamesToPass node_pass;
  GE_TIMESTAMP_START(node_pass);
  IdentityPass identity_force_pass(false);  // after SwitchToStreamSwitchPass
  node_pass.emplace_back("IdentityPass", &identity_force_pass);
  ret = GEPass(compute_graph).Run(node_pass);
  GE_TIMESTAMP_END(node_pass, "GraphPrepare::node_pass");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][Identity] remove pass for preprocess failed, ret:%u.", ret);
    return ret;
  }
  return SUCCESS;
}

Status GraphManager::OptimizeStage2(ge::ComputeGraphPtr &compute_graph) {
  GELOGD("Start optimize after merge sub graph.");

  PassManager after_merge_passes;
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage2::AfterMergePasses::LinkGenMaskNodesPass",
                                               new (std::nothrow)
                                                   LinkGenMaskNodesPass(options_.stream_max_parallel_num)));
  GE_CHK_STATUS_RET(after_merge_passes.AddPass("OptimizeStage2::HcclContinuousMemcpyPass",
                                               new (std::nothrow) HcclContinuousMemcpyPass));

  GE_TIMESTAMP_START(after_merge_passes);
  auto ret = after_merge_passes.Run(compute_graph);
  GE_TIMESTAMP_END(after_merge_passes, "OptimizeStage2::AfterMergePasses");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][Passes] after merge sub graph failed, ret:%d.", ret);
    return ret;
  }
  SetAttrForHcomBroadCastOp(compute_graph);

  NamesToPass names_to_passes;
  ConstantFoldingPass constant_folding_pass;
  ReshapeRemovePass reshape_remove_pass;
  CondRemovePass condition_remove_pass;
  BitcastPass bitcast_pass;
  AssignRemovePass assign_remove_pass;
  InplaceSupportCheckPass inplace_support_check_pass;

  ReplaceWithEmptyConstPass replace_with_empty_const_pass;
  names_to_passes.emplace_back("ReplaceWithEmptyConstPass", &replace_with_empty_const_pass);
  names_to_passes.emplace_back("ConstantFoldingPass", &constant_folding_pass);
  names_to_passes.emplace_back("ReshapeRemovePass", &reshape_remove_pass);
  names_to_passes.emplace_back("CondRemovePass", &condition_remove_pass);
  names_to_passes.emplace_back("BitcastPass", &bitcast_pass);
  if (GetContext().GetHostExecFlag()) {
    names_to_passes.emplace_back("AssignRemovePass", &assign_remove_pass);
    names_to_passes.emplace_back("InplaceSupportCheckPass", &inplace_support_check_pass);
  }
  GE_TIMESTAMP_START(names_to_passes);
  ret = GEPass(compute_graph).Run(names_to_passes);
  GE_TIMESTAMP_END(names_to_passes, "OptimizeStage2::MergedGraphNameToPasses");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][GEPasses] optimize for OptimizeAfterMergeSubGraph failed, ret:%d.", ret);
    return ret;
  }

  ret = RemoveIsolatedConst(compute_graph);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Remove][IsolatedConst] failed, ret:%d.", ret);
    return ret;
  }

  PassManager pass_for_control_attr_optimize;
  if (options_.train_graph_flag) {
    GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::FlowCtrlPass",
                                                             new (std::nothrow) FlowCtrlPass));
  }

  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::MultiBatchPass",
                                                           new (std::nothrow) MultiBatchPass));
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::RefIdentityDeleteOpPass",
                                                           new (std::nothrow) RefIdentityDeleteOpPass));
  // the value of the attr is the original variable name the ref-variable ref from.
  // The attr will be used when allocating memory,
  // the node marked attr will be output to a variable instead of new-allocated memory.
  // Therefore, ComputeGraph should not delete nodes after `VariableRefDeleteOpPass`
  // to prevent unexpected deletion of nodes marked with attr
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::VariableRefDeleteOpPass",
                                                           new (std::nothrow) VariableRefDeleteOpPass));
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::CompileNodesPass",
                                                           new (std::nothrow) CompileNodesPass));
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass(
      "OptimizeStage2::AfterMergePasses::MarkGraphUnknownStatusPass", new(std::nothrow) MarkGraphUnknownStatusPass));
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::SwapSpacePass",
                                                           new (std::nothrow) SwapSpacePass));
  GE_CHK_STATUS_RET(
      pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::InputOutputConnectionIdentifyPass",
                                             new (std::nothrow) InputOutputConnectionIdentifyPass));
  // When the input node to be cleared is after a `Data` node, the atomic-clean-node should not be inserted.
  // So The ComputeGraph should not delete nodes after `AtomicAddrCleanPass`
  // to prevent unexpected deletion of nodes after a `Data` node
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::AtomicAddrCleanPass",
                                                           new (std::nothrow) AtomicAddrCleanPass));
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::"
                                                           "EndOfSequenceAddControlPass",
                                                           new (std::nothrow) EndOfSequenceAddControlPass));
#ifdef FWK_SUPPORT_TRAINING_TRACE
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::AfterMergePasses::StartOfSequencePass",
                                                           new (std::nothrow) StartOfSequencePass));
#endif
  // 'SubgraphPass' solves memory_assign_conflicts by insert MemcpyAsync node, which depends on multi attrs and
  // graph-structure. Passes after 'SubgraphPass' MUST NOT remove MemcpyAsync/Identity nodes in subgraphs.
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::SubgraphPass",
                                                           new (std::nothrow) SubgraphPass));
  // 'AttachStreamLabelPass' modifies attr without changing structure of compute_graph
  // All passes after 'AttachStreamLabelPass' MUST mark stream_label on new nodes by self.
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::AttachStreamLabelPass",
                                                           new (std::nothrow) AttachStreamLabelPass));

  // 'SetFftsPlusAttrPass' is temporary solution,
  // to solves FE can't recognize the new insertion operator in ffts_plus subgraph.
  GE_CHK_STATUS_RET(pass_for_control_attr_optimize.AddPass("OptimizeStage2::ControlAttrOptimize::SetFftsPlusAttrPass",
                                                           new (std::nothrow) SetFftsPlusAttrPass));

  GE_TIMESTAMP_START(pass_for_control_attr_optimize);
  ret = pass_for_control_attr_optimize.Run(compute_graph);
  GE_TIMESTAMP_END(pass_for_control_attr_optimize, "OptimizeStage2::ControlAttrOptimize");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][Passes] when optimize stage 2 failed");
    return ret;
  }

  // Assign functional op labels.
  GE_TIMESTAMP_START(AssignFunctionalLabels);
  LabelAllocator label_allocator(compute_graph);
  GE_CHK_STATUS_RET(label_allocator.AssignFunctionalLabels(), "[Assign][Label] failed.");
  GE_TIMESTAMP_END(AssignFunctionalLabels, "ModelBuilder::AssignFunctionalLabels");

  // Add memcpy addr asynchronous node.
  GE_TIMESTAMP_START(AddMemcpyAddrAsyncNode);
  MemcpyAddrAsyncPass memcpy_addr;
  GE_CHK_STATUS_RET(memcpy_addr.Run(compute_graph), "[Call][Run] Add memcpy_addr_async node failed.");
  GE_TIMESTAMP_END(AddMemcpyAddrAsyncNode, "MemcpyAddrAsyncPass::Run.");

  // Process offset and dependency for buffer pool memory assigner.
  GE_TIMESTAMP_START(BufferPoolMemoryPass);
  BufferPoolMemoryPass buffer_pool_mem_pass;
  GE_CHK_STATUS_RET(buffer_pool_mem_pass.Run(compute_graph),
                    "[Call][Run] Failed to process for buffer pool allocator.");
  GE_TIMESTAMP_END(BufferPoolMemoryPass, "BufferPoolMemoryPass::Run.");

  // Handle parallel group .
  GE_TIMESTAMP_START(ParallelGroup);
  ParallelGroupPass parallel_group_pass;
  GE_CHK_STATUS_RET(parallel_group_pass.Run(compute_graph), "[Handle][ParallelGroup] failed.");
  GE_TIMESTAMP_END(ParallelGroup, "ParallelGroupPass::Run.");

  // After while sub graph handle, mark all node rw type
  auto result = GetCompilerStages(compute_graph->GetGraphID()).optimizer.HandleMemoryRWConflict(compute_graph);
  if (result != SUCCESS) {
    GELOGW(
        "Mark node rw type failed. It will take some effect on memory_assign_conflicts handling."
        "Please pay attention to it.");
  }

  ChangeConstTypeWhenTraining(compute_graph);

  GELOGI("End optimize after merge sub graph.");
  return SUCCESS;
}

void GraphManager::ChangeConstTypeWhenTraining(const ComputeGraphPtr &compute_graph) const {
  // The constant for train is CONSTANTOP, and is CONSTANT for inference. They will be unified in future.
  GE_RT_VOID_CHECK_NOTNULL(compute_graph);
  if (options_.train_graph_flag) {
    for (NodePtr &n : compute_graph->GetAllNodes()) {
      GE_RT_VOID_CHECK_NOTNULL(n);
      GE_RT_VOID_CHECK_NOTNULL(n->GetOpDesc());
      // This can ensure that n is not a null pointer
      if (n->GetOpDesc()->GetType() == CONSTANT) {
        n->GetOpDesc()->SetType(CONSTANTOP);
      }
    }
  }
}

Status GraphManager::ProcessSubGraphWithMultiThreads(GraphManager *graph_manager, GraphId root_graph_id,
                                                     const SubGraphInfoPtr &sub_graph_info_ptr,
                                                     const std::string &root_graph_name,
                                                     uint64_t session_id,
                                                     const struct error_message::Context &error_context,
                                                     const GEThreadLocalContext &ge_context) {
  ErrorManager::GetInstance().SetErrorContext(error_context);
  if (sub_graph_info_ptr != nullptr && graph_manager != nullptr) {
    GetContext().SetSessionId(session_id);
    GetThreadLocalContext() = ge_context;
    graph_manager->UpdateLocalOmgContext(root_graph_id);
    ComputeGraphPtr compute_graph_tmp = sub_graph_info_ptr->GetSubGraph();
    const std::string &engine_name = sub_graph_info_ptr->GetEngineName();
    GELOGD("ProcessSubGraphWithMultiThreads start, graph name is %s, engine_name is %s, thread id is %lu",
           compute_graph_tmp != nullptr ? compute_graph_tmp->GetName().c_str() : "", engine_name.c_str(),
           pthread_self());
    GE_DUMP(compute_graph_tmp, "OptimizeSubGraphBefore");
    GE_CHECK_NOTNULL(compute_graph_tmp);
    if (!AttrUtils::SetInt(*compute_graph_tmp, ATTR_NAME_ROOT_GRAPH_ID, root_graph_id)) {
      REPORT_CALL_ERROR("E19999", "Set Attr:%s to graph:%u failed", ATTR_NAME_ROOT_GRAPH_ID.c_str(),
                        compute_graph_tmp->GetGraphID());
      GELOGE(FAILED, "[Set][Attr] %s to graph:%u failed", ATTR_NAME_ROOT_GRAPH_ID.c_str(),
             compute_graph_tmp->GetGraphID());
      return FAILED;
    }
    if (!AttrUtils::SetStr(*compute_graph_tmp, ATTR_NAME_ROOT_GRAPH_NAME, root_graph_name)) {
      REPORT_CALL_ERROR("E19999", "Set Attr:%s to graph:%u failed", ATTR_NAME_ROOT_GRAPH_NAME.c_str(),
                        compute_graph_tmp->GetGraphID());
      GELOGE(FAILED, "[Set][Attr] %s to graph:%u failed", ATTR_NAME_ROOT_GRAPH_NAME.c_str(),
             compute_graph_tmp->GetGraphID());
      return FAILED;
    }
    compute_graph_tmp->SetSessionID(session_id);
    Status ret = graph_manager->GetCompilerStages(root_graph_id).optimizer.OptimizeSubGraph(compute_graph_tmp,
                                                                                            engine_name);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Optimize][SubGraph] Failed, engine:%s, graph:%s",
             engine_name.c_str(), compute_graph_tmp->GetName().c_str());
      return ret;
    } else {
      GELOGD("SubGraph optimize success %s", engine_name.c_str());
    }
    GE_DUMP(compute_graph_tmp, "OptimizeSubGraphAfter");
    sub_graph_info_ptr->SetSubGraph(compute_graph_tmp);
    GELOGD("ProcessSubGraphWithMultiThreads end, graph name is %s, engine_name is %s, thread id is %lu",
           compute_graph_tmp != nullptr ? compute_graph_tmp->GetName().c_str() : "", engine_name.c_str(),
           pthread_self());
  } else {
    REPORT_INNER_ERROR("E19999", "Param sub_graph_info_ptr or graph_manager is nullptr");
    GELOGE(FAILED, "[Check][Param] graph_manager or sub_graph_info_ptr is nullptr");
    return FAILED;
  }

  return SUCCESS;
}

// run graph async on session
Status GraphManager::RunGraphAsync(const GraphId &graph_id, const std::vector<ge::Tensor> &inputs,
                                   uint64_t session_id, RunAsyncCallback callback) {
  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  GELOGI("[GraphManager] Start to run graph async, graph_id=%u, inputsSize=%zu.", graph_id, inputs.size());

  bool ret = prerun_args_q_.Push(PreRunArgs({graph_id, inputs, session_id,
    ErrorManager::GetInstance().GetErrorManagerContext(),
    GetThreadLocalContext(), callback}));
  if (!ret) {
    GELOGE(FAILED, "[Call][Push] failed, graph_id:%u.", graph_id);
    return FAILED;
  }

  GELOGI("[GraphManager] Run graph async success, graph_id=%u.", graph_id);
  return SUCCESS;
}

Status GraphManager::CheckIncreBuildAndPreRun(const PreRunArgs &args,
                                              GraphNodePtr &graph_node, FlowModelPtr &flow_model) {
  GE_CHECK_NOTNULL(graph_node);
  if (!IsGraphNeedBuild(graph_node)) {
    flow_model = graph_node->GetFlowModel();
    return SUCCESS;
  }
  if (graph_node->GetBuildFlag()) {
    ReturnError(args.callback, PARAM_INVALID,
                "[Check][Param] The graph " + std::to_string(graph_node->GetGraphId()) +
                " need to re-build, you should remove it"
                " from GE first, then AddGraph again and rebuild it.");
    return PARAM_INVALID;
  }
  // check need incre build.
  std::vector<GeTensor> ge_inputs;
  for (const auto &item: args.input_tensor) {
    ge_inputs.emplace_back(TensorAdapter::AsGeTensor(item));
  }
  Status ret = PreRun(graph_node, ge_inputs, flow_model, args.session_id);
  // release rts generate context
  RtContextUtil::GetInstance().DestroyRtContexts(args.session_id, graph_node->GetGraphId());
  if (ret != SUCCESS) {
    ReturnError(args.callback, ret, "[Call][PreRun] Failed.");
    return ret;
  }

  graph_node->SetBuildFlag(true);
  graph_rebuild_state_ctrl_.SetGraphBuildEnd(graph_node->GetGraphId());
  return SUCCESS;
}

void GraphManager::PreRunThread() {
  if (prctl(PR_SET_NAME, ("GE_PreRun")) != 0) {
    GELOGW("Set thread name failed.");
  }

  PreRunArgs args;
  while (thread_run_flag_) {
    if (!prerun_args_q_.Pop(args)) {
      continue;
    }

    GELOGI("[PreRunThread] A new loop start, graph_id:%u.", args.graph_id);
    ErrorManager::GetInstance().SetErrorContext(args.error_context);
    ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
    GetContext().SetSessionId(args.session_id);
    GetThreadLocalContext() = args.context;
    UpdateLocalOmgContext(args.graph_id);

    // find graph
    GraphNodePtr graph_node = nullptr;
    Status ret = GetGraphNode(args.graph_id, graph_node);
    if (ret != SUCCESS) {
      ReturnError(args.callback, GE_GRAPH_GRAPH_NODE_NULL,
                  "[Run][Graph] graph not exist, graph_id=" + std::to_string(args.graph_id));
      return;
    }
    // more than one graph owns same graph_id
    uint32_t count = 0;
    if (GetGraphCount(args.graph_id, count) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Get][GraphCount] failed, graph id:%u.", args.graph_id);
      return;
    }
    // Avoid repeatively prerun for graphs owns same graph_id in online inference concurrency
    if (count > 1 && graph_node->GetBuildFlag()) {
      GELOGD("Avoid repeatively prerun, graph_id:%u.", args.graph_id);
      // In online inference concurrency senario, graph_node is allowed to be locked for 'count' times
      graph_node->SetSemSize(count);
      graph_node->Lock();
      auto flow_model = graph_node->GetFlowModel();
      PushRunArgs(RunArgs({ graph_node, args.graph_id, args.session_id, args.error_context,
          args.input_tensor, flow_model, GetThreadLocalContext(), args.callback }));
      GELOGI("[PreRunThread] Loop end. Start to run with cached build model.");
      continue;
    }
    // Cannot be put ahead of the repeatively prerun judgement
    graph_node->Lock();

    if (graph_node->GetRunFlag()) {
      ReturnError(args.callback, GE_GRAPH_ALREADY_RUNNING,
                  "[Check][Param] graph already running, graph id=" + std::to_string(args.graph_id));
      graph_node->Unlock();
      return;
    }

    // set graph's run flag
    graph_node->SetRunFlag(true);

    if (graph_node->GetGraph() == nullptr) {
      ReturnError(args.callback, GE_GRAPH_GRAPH_NODE_NULL,
                  "[Get][Graph] from graph_node is NULL");
      graph_node->Unlock();
      return;
    }
    ComputeGraphPtr compute_graph_tmp = GraphUtils::GetComputeGraph(*(graph_node->GetGraph()));
    if (compute_graph_tmp == nullptr) {
      ReturnError(args.callback, GE_GRAPH_GRAPH_NODE_NULL,
                  "[Get][ComputeGraph] compute_graph_tmp is NULL.");
      graph_node->Unlock();
      return;
    }

    if (options_.local_fmk_op_flag) {
      GetCompilerStages(graph_node->GetGraphId()).optimizer.TranFrameOp(compute_graph_tmp);
    }

    // it will not execute graph preprocess, optimize, parition, build if the graph has built successful.
    GELOGI("Start for run graph async.");
    FlowModelPtr flow_model = nullptr;

    ret = CheckIncreBuildAndPreRun(args, graph_node, flow_model);
    if (ret != SUCCESS) {
      graph_node->SetRunFlag(false);
      if (!ge::Analyzer::GetInstance()->IsEnableNetAnalyzeDebug()) {
        GELOGE(ret, "[Check][IncreBuildAndPreRun] Failed, thread exit..");
        graph_node->Unlock();
        return;
      } else {
        GELOGE(ret, "[Check][IncreBuildAndPreRun] Failed, keep geop continue!");
        graph_node->Unlock();
        continue;
      }
    }

    PushRunArgs(RunArgs({ graph_node, args.graph_id, args.session_id, args.error_context,
        args.input_tensor, flow_model, GetThreadLocalContext(), args.callback }));
    GELOGI("[PreRunThread] Loop end.");
  }
}

void GraphManager::PushRunArgs(const RunArgs &args) const {
  if (executor_ == nullptr) {
    GELOGW("Just compile model, not support execute.");
    return;
  }

  (void)executor_->PushRunArgs(args);
}

Status GraphManager::SetRunContext(const GraphNodePtr &graph_node) const {
  OmeContext ome_context;
  if (!GetLocalOmgContext().dynamic_dims.empty()) {
    std::string dims_str(GetLocalOmgContext().dynamic_dims);
    if (dims_str[dims_str.size() - 1] == ';') {
      dims_str = dims_str.substr(0UL, dims_str.size() - 1UL);
    }
    const auto dynamic_dims = StringUtils::Split(dims_str, ';');
    for (const auto &dims : dynamic_dims) {
      const auto dim_vct = StringUtils::Split(dims, ',');
      std::vector<int32_t> cur_dynamic_dims;
      for (const auto &dim : dim_vct) {
        int32_t val = -1;
        GE_CHK_STATUS_RET_NOLOG(ConvertToInt32(dim, val));
        cur_dynamic_dims.emplace_back(val);
      }
      ome_context.dynamic_shape_dims.emplace_back(cur_dynamic_dims);
    }
  }

  ome_context.dynamic_node_type = GetLocalOmgContext().dynamic_node_type;
  ome_context.user_input_dims = GetLocalOmgContext().user_input_dims;

  ome_context.data_nodes = GetLocalOmgContext().data_nodes;
  ome_context.getnext_nosink_nodes = GetLocalOmgContext().getnext_nosink_nodes;

  GE_CHECK_NOTNULL(graph_node);
  graph_node->SetOmeContext(ome_context);
  return SUCCESS;
}

void GraphManager::StopQueue() {
  thread_run_flag_.store(false);
  prerun_args_q_.Stop();
}

void GraphManager::ReturnError(RunAsyncCallback callback, Status ret, const std::string &log) {
  GELOGE(ret, "%s.", log.c_str());
  std::vector<ge::Tensor> outputs;
  if (callback != nullptr) {
    callback(ret, outputs);
  }
  StopQueue();
}

bool GraphManager::IsGraphNeedRebuild(uint32_t graph_id) {
  // find graph
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, graph not exist, graph_id:%u.", graph_id);
    return true;
  }

  if (graph_node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] graph node is NULL, graph_id:%u.", graph_id);
    return true;
  }

  return IsGraphNeedBuild(graph_node);
}

bool GraphManager::IsGraphNeedBuild(const GraphNodePtr &graph_node) const {
  return !graph_node->GetBuildFlag() || graph_rebuild_state_ctrl_.IsGraphNeedRebuild(graph_node->GetGraphId());
}
const std::map<std::string, std::string> *GraphManager::GetGraphOptions(uint32_t graph_id) {
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(graph_id, graph_node);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Graph:%u not exist in graph_map, check invalid", graph_id);
    GELOGE(ret, "[Get][GraphNode] failed, graph not exist, graph_id:%u.", graph_id);
    return nullptr;
  }

  if (!graph_node) {
    REPORT_INNER_ERROR("E19999", "Graph node is nullptr in graph_map, graph_id:%u, check invalid", graph_id);
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] graph node is NULL, graph_id:%u.", graph_id);
    return nullptr;
  }
  return &(graph_node->GetOptions());
}

void GraphManager::SetOptionsRunGraphFlag(bool run_graph_flag) { options_.run_graph_flag = run_graph_flag; }

Status GraphManager::OptimizeSubgraph(const GraphNodePtr &graph_node, ComputeGraphPtr &compute_graph,
                                      uint64_t session_id) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);
  GE_TIMESTAMP_START(MDS);
  // mds pass
  PassManager graph_pass;
  GE_CHK_STATUS_RET(graph_pass.AddPass("OptimizeSubgraph::MDS", new (std::nothrow) ModelDeploySchedulerPass));
  GE_CHK_STATUS_RET(graph_pass.Run(compute_graph));
  GE_TIMESTAMP_EVENT_END(MDS, "OptimizeSubgraph::MDS");
  // graph partition
  // Stage partition, only for root graph
  GE_TIMESTAMP_START(StagePartition);
  StagePartitioner stage_partitioner(compute_graph);
  auto ret = stage_partitioner.Partition();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][Partition] for Graph:%s by stage Failed", compute_graph->GetName().c_str());
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(StagePartition, "OptimizeSubgraph::StagePartition");

  GE_TIMESTAMP_START(GraphPartitionPipeline);
  PipelinePartitioner pipeline_partitioner(compute_graph);
  ret = pipeline_partitioner.Partition();
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Call partition for graph:%s by pipeline failed",
                       compute_graph->GetName().c_str());
    GELOGE(ret, "[Call][Partition] for Graph:%s by pipeline Failed", compute_graph->GetName().c_str());
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(GraphPartitionPipeline, "OptimizeSubgraph::PipelinePartition");
  GE_DUMP(compute_graph, "AfterPipelinePartition");

  // all sub graph list of root graph and sub graph
  GE_TIMESTAMP_START(GraphPartitionDynamicShape);
  auto &engine_placer = GetCompilerStages(graph_node->GetGraphId()).partitioner.graph_info_.engine_placer_;
  engine_placer.SetComputeGraph(compute_graph);
  if (engine_placer.Run(false) != SUCCESS) {
    GELOGE(FAILED, "[Call][Run] Engine placer run failed, graph:%s.", compute_graph->GetName().c_str());
    return FAILED;
  }
  DynamicShapePartitioner dynamic_shape_partitioner(compute_graph);
  ret = dynamic_shape_partitioner.Partition();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][Partition] for Graph:%s by dynamic shape Failed", compute_graph->GetName().c_str());
    return ret;
  }
  if (engine_placer.ReAssignEngine() != SUCCESS) {
    GELOGE(FAILED, "[Call][Run] Engine placer reassign failed, graph:%s.", compute_graph->GetName().c_str());
    return FAILED;
  }
  GE_TIMESTAMP_EVENT_END(GraphPartitionDynamicShape, "OptimizeSubgraph::GraphPartitionDynamicShape");
  GE_DUMP(compute_graph, "AfterDynamicShapePartition");

  GE_TIMESTAMP_START(SubgraphPartitionAndOptimization_CompositeEngine);
  ret = SubgraphPartitionAndOptimization(graph_node, compute_graph, session_id,
                                         GraphPartitioner::kCompositeEnginePartitioning);
  if (ret != SUCCESS) {
    GELOGE(ret, "[SubgraphPartitionAndOptimization][CompositeEngine] for graph:%s failed",
           compute_graph->GetName().c_str());
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(SubgraphPartitionAndOptimization_CompositeEngine,
                         "OptimizeSubgraph::SubgraphPartitionAndOptimization::CompositeEngine");
  GE_DUMP(compute_graph, "MergedComputeGraphAfterCompositeEnginePartition");

  GE_TIMESTAMP_START(SubgraphPartitionAndOptimization_AtomicEngine);
  ret = SubgraphPartitionAndOptimization(graph_node, compute_graph, session_id,
                                         GraphPartitioner::kAtomicEnginePartitioning);
  if (ret != SUCCESS) {
    GELOGE(ret, "[SubgraphPartitionAndOptimization][AtomicEngine] for graph:%s failed",
           compute_graph->GetName().c_str());
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(SubgraphPartitionAndOptimization_AtomicEngine,
                         "OptimizeSubgraph::SubgraphPartitionAndOptimization::AtomicEngine");
  GE_DUMP(compute_graph, "MergedComputeGraphAfterAtomicEnginePartition");

  return SUCCESS;
}

Status GraphManager::SubgraphPartitionAndOptimization(const GraphNodePtr &graph_node, ComputeGraphPtr &compute_graph,
                                                      uint64_t session_id, GraphPartitioner::Mode mode) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);
  const bool composite_engine_not_register = (mode == GraphPartitioner::kCompositeEnginePartitioning) &&
                                             OpsKernelManager::GetInstance().GetCompositeEngines().empty();
  if (composite_engine_not_register) {
    GELOGI("No composite engine registers, ignore subgraph partition and optimization for composite engine");
    return SUCCESS;
  }
  GE_TIMESTAMP_START(GraphPartition);
  GraphPartitioner &partitioner = GetCompilerStages(graph_node->GetGraphId()).partitioner;
  Status ret = partitioner.Partition(compute_graph, mode);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][Partition] for Graph:%s Failed", compute_graph->GetName().c_str());
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(GraphPartition, "OptimizeSubgraph::Partition1");
  GE_TIMESTAMP_START(SetSubgraph);
  ret = SetSubgraph(session_id, compute_graph, partitioner);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][Subgraph] failed for graph:%s, session_id:%lu", compute_graph->GetName().c_str(), session_id);
    return ret;
  }
  GE_TIMESTAMP_EVENT_END(SetSubgraph, "OptimizeSubgraph::SetSubGraph");
  if (mode == GraphPartitioner::kAtomicEnginePartitioning) {
    std::set<std::string> build_steps = {BUILD_STEP_BEFORE_UB_MATCH, BUILD_STEP_AFTER_BUILDER,
                                         BUILD_STEP_AFTER_BUILDER_SUB};
    if ((GetBuildMode(graph_node) == BUILD_MODE_TUNING) && (build_steps.count(GetBuildStep(graph_node)) > 0)) {
      GE_TIMESTAMP_START(ConvertGraphToFile);
      std::string tuning_path;
      (void) GetContext().GetOption(TUNING_PATH, tuning_path);
      ret = ConvertGraphToFile(compute_graph, partitioner, tuning_path,
                               (GetBuildStep(graph_node) == BUILD_STEP_AFTER_BUILDER));
      if (ret != SUCCESS) {
        GELOGE(ret, "[Convert][Graph] [%s] to file failed", compute_graph->GetName().c_str());
        return ret;
      }
      GE_TIMESTAMP_EVENT_END(ConvertGraphToFile, "OptimizeSubgraph::ConvertGraphToFile");
      return SUCCESS;
    }
  }

  ComputeGraphPtr merged_compute_graph = nullptr;
  GE_TIMESTAMP_START(MergeSubgraph);
  ret = MergeSubGraph(merged_compute_graph, compute_graph, graph_node->GetGraphId());
  if (ret != SUCCESS) {
    GELOGE(ret, "[Merge][SubGraph] Failed, graph:%s(id:%u)",
           compute_graph->GetName().c_str(), graph_node->GetGraphId());
    return ret;
  }
  GE_CHECK_NOTNULL(merged_compute_graph);
  merged_compute_graph->SetSessionID(session_id);
  merged_compute_graph->SetGraphID(graph_node->GetGraphId());
  merged_compute_graph->SetNeedIteration(compute_graph->GetNeedIteration());
  for (auto &sub_graph : merged_compute_graph->GetAllSubgraphs()) {
    GE_CHECK_NOTNULL(sub_graph);
    sub_graph->SetSessionID(session_id);
    sub_graph->SetGraphID(graph_node->GetGraphId());
  }

  GE_TIMESTAMP_EVENT_END(MergeSubgraph, "OptimizeSubgraph::MergeSubGraph");
  GE_DUMP(merged_compute_graph, "mergedComputeGraph");
  compute_graph = merged_compute_graph;
  return SUCCESS;
}

Status GraphManager::ConvertGraphToFile(ComputeGraphPtr &compute_graph, GraphPartitioner &partitioner, std::string path,
                                        bool exe_flag) const {
  GE_CHECK_NOTNULL(compute_graph);
  GELOGI("compute_graph [%s] path [%s] Enter ConvertGraphToFile.", compute_graph->GetName().c_str(), path.c_str());
  std::vector<ComputeGraphPtr> non_tuning_subgraphs;
  auto input_node_sub_graph_map = partitioner.graph_2_input_subgraph_;
  auto iter = input_node_sub_graph_map.find(compute_graph);
  if (iter != input_node_sub_graph_map.end()) {
    GE_CHECK_NOTNULL(iter->second);
    non_tuning_subgraphs.push_back(iter->second->GetSubGraph());
  }
  auto sub_graph_map = partitioner.GetSubGraphMap();
  const auto &subgraph_infos = sub_graph_map[compute_graph];
  std::vector<ComputeGraphPtr> tuning_subgraphs;
  for (const auto &sub_graph_info_ptr: subgraph_infos) {
    GE_CHECK_NOTNULL(sub_graph_info_ptr);
    ComputeGraphPtr sub_graph_tmp = sub_graph_info_ptr->GetSubGraph();
    // need to tuning
    if (sub_graph_info_ptr->GetEngineName() == kVectorEngine || sub_graph_info_ptr->GetEngineName() == kAIcoreEngine) {
      tuning_subgraphs.push_back(sub_graph_tmp);
    } else {
      non_tuning_subgraphs.push_back(sub_graph_tmp);
    }
  }
  // for function graphs to tune
  for (auto &function_graph : compute_graph->GetAllSubgraphs()) {
    auto subgraph_list = sub_graph_map[function_graph];
    auto it = input_node_sub_graph_map.find(function_graph);
    if (it != input_node_sub_graph_map.end()) {
      GE_CHECK_NOTNULL(it->second);
      non_tuning_subgraphs.push_back(it->second->GetSubGraph());
    }

    for (const auto &sub_graph_info_ptr : subgraph_list) {
      GE_CHECK_NOTNULL(sub_graph_info_ptr);
      ComputeGraphPtr sub_graph_tmp = sub_graph_info_ptr->GetSubGraph();
      // need to tuning
      if (sub_graph_info_ptr->GetEngineName() == kVectorEngine ||
          sub_graph_info_ptr->GetEngineName() == kAIcoreEngine) {
        tuning_subgraphs.push_back(sub_graph_tmp);
      } else {
        non_tuning_subgraphs.push_back(sub_graph_tmp);
      }
    }
  }
  return TuningUtils::ConvertGraphToFile(tuning_subgraphs, non_tuning_subgraphs, exe_flag, path);
}

Status GraphManager::Build(const GraphNodePtr &graph_node, ComputeGraphPtr &compute_graph,
                           GeRootModelPtr &ge_root_model, uint64_t session_id) {
  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kOther);
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(compute_graph);
  // build
  if (compute_graph != nullptr) {
    std::string graph_name = compute_graph->GetName();
    graph_name.append("_");
    graph_name.append(std::to_string(graph_node->GetGraphId()));
    compute_graph->SetName(graph_name);
  }

  auto ret = GetCompilerStages(graph_node->GetGraphId()).builder.Build(compute_graph, ge_root_model, session_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][Build] failed, session_id:%lu.", session_id);
    return ret;
  }

  bool is_always_dump = false;
  if (!DumpManager::GetInstance().GetDumpProperties(session_id).GetDumpPath().empty()) {
    is_always_dump = true;
  }

  GraphUtils::DumpGEGraph(compute_graph, "Build", is_always_dump);
  GraphUtils::DumpGEGraphToOnnx(*compute_graph, "Build");
  return SetRunContext(graph_node);
}

Status GraphManager::GenCheckPointGraph(const std::map<std::string, GeTensorDesc> &all_variables,
                                        Graph &graph) const {
  ge::ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>(kCheckPointGraph);
  GE_CHECK_NOTNULL(compute_graph);
  OpDescPtr save_desc = MakeShared<ge::OpDesc>(compute_graph->GetName() + "_" + kSave, kSave);
  GE_CHECK_NOTNULL(save_desc);
  uint32_t save_index = 0;
  for (auto iter = all_variables.begin(); iter != all_variables.end(); ++iter) {
    GE_CHK_GRAPH_STATUS_RET(save_desc->AddInputDesc(save_index, iter->second));
    save_index++;
  }
  NodePtr save_node = compute_graph->AddNode(save_desc);

  uint32_t index = 0;
  for (auto iter = all_variables.begin(); iter != all_variables.end(); ++iter) {
    OpDescPtr var_desc = MakeShared<ge::OpDesc>(iter->first, VARIABLE);
    GE_CHECK_NOTNULL(var_desc);
    if (!AttrUtils::SetBool(var_desc, kCheckPointForGetVar, true)) {
      GELOGW("Set check point graph attr failed.");
    }
    GE_CHK_GRAPH_STATUS_RET(var_desc->AddOutputDesc(iter->second));
    NodePtr var_node = compute_graph->AddNode(var_desc);
    GE_CHK_STATUS(GraphUtils::AddEdge(var_node->GetOutDataAnchor(0), save_node->GetInDataAnchor(index)),
                  "[Add][Edge][%s->%s] fail.", var_node->GetName().c_str(), save_node->GetName().c_str());
    index++;
  }
  compute_graph->Dump();
  graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  return SUCCESS;
}

Status GraphManager::SaveVariables(const Graph &graph, const std::vector<std::string> &var_names,
                                   const std::vector<Tensor> &outputs, std::vector<Tensor> &var_values) const {
  std::map<std::string, Tensor> var_results;
  GE_CHK_STATUS_RET(SaveCheckPointResult(graph, outputs, var_results), "[Save][CheckPointResult] failed.");
  if (!var_names.empty()) {
    for (const auto &var_name : var_names) {
      if (var_results.count(var_name) == 0) {
        REPORT_INNER_ERROR("E19999", "Fetch Var:%s result value fail", var_name.c_str());
        GELOGE(FAILED, "[Check][Param] Fetch var[%s] value failed.", var_name.c_str());
        return FAILED;
      } else {
        auto var_tensor = var_results[var_name].GetTensorDesc();
        var_tensor.SetName(var_name.c_str());
        var_results[var_name].SetTensorDesc(var_tensor);
        var_values.emplace_back(var_results[var_name]);
      }
    }
  } else {
    for (auto iter = var_results.begin(); iter != var_results.end(); ++iter) {
      std::string var_name = iter->first;
      auto var_tensor = iter->second.GetTensorDesc();
      var_tensor.SetName(var_name.c_str());
      iter->second.SetTensorDesc(var_tensor);
      var_values.emplace_back(iter->second);
    }
  }
  return SUCCESS;
}

Status FindVarNodeFromNetoutputIn(NodePtr &peer_node) {
  while (peer_node->GetType() != VARIABLE) {
    if (peer_node->GetAllInDataAnchors().size() != 1) {
      REPORT_INNER_ERROR("E19999", "peer node:%s(%s) of netoutput has more than 1 input in checkpoint Graph, "
                         "check invalid", peer_node->GetName().c_str(), peer_node->GetType().c_str());
      GELOGE(FAILED, "[Check][Param] node:%s has more than 1 input.", peer_node->GetName().c_str());
      return FAILED;
    }
    auto peer_node_in_anchor = peer_node->GetAllInDataAnchors().at(0);
    GE_CHECK_NOTNULL(peer_node_in_anchor);
    auto peer_node_out_anchor = peer_node_in_anchor->GetPeerOutAnchor();
    if (peer_node_out_anchor != nullptr) {
      peer_node = peer_node_out_anchor->GetOwnerNode();
      GE_CHECK_NOTNULL(peer_node);
      if (peer_node->GetType() == VARIABLE) {
        break;
      }
    }
  }
  return SUCCESS;
}

Status GraphManager::SaveCheckPointResult(const Graph &graph, const std::vector<Tensor> &outputs,
                                          std::map<std::string, Tensor> &var_results) const {
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHECK_NOTNULL(compute_graph);
  NodePtr netoutput_node = nullptr;
  for (const auto &node : compute_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    if (node->GetType() == NETOUTPUT) {
      netoutput_node = node;
      break;
    }
  }
  GE_CHECK_NOTNULL(netoutput_node);
  for (const auto &in : netoutput_node->GetAllInDataAnchors()) {
    GE_CHECK_NOTNULL(in);
    auto out_anchor = in->GetPeerOutAnchor();
    GE_CHECK_NOTNULL(out_anchor);
    auto peer_node = out_anchor->GetOwnerNode();
    GE_CHECK_NOTNULL(peer_node);
    if (FindVarNodeFromNetoutputIn(peer_node) != SUCCESS) {
      return FAILED;
    }
    if (peer_node->GetType() != VARIABLE) {
      REPORT_INNER_ERROR("E19999", "peer node:%s(%s) of netoutput is not variable in checkpoint Graph, "
                         "check invalid", peer_node->GetName().c_str(), peer_node->GetType().c_str());
      GELOGE(FAILED, "[Check][Param] peer_node %s is not variable in checkpoint Graph.", peer_node->GetName().c_str());
      return FAILED;
    }
    auto var_name = peer_node->GetName();
    GELOGI("[GraphManager] SaveVariables, varName is %s.", var_name.c_str());
    if (in->GetIdx() >= static_cast<int32_t>(outputs.size())) {
      REPORT_INNER_ERROR("E19999", "In index:%u of netoutput is out of outputs.size:%zu range in checkpoint Graph, "
                         "check invalid", in->GetIdx(), outputs.size());
      GELOGE(FAILED, "[Check][Param] variable index[%d] out of range[%zu].", in->GetIdx(), outputs.size());
      return FAILED;
    }
    var_results.emplace(var_name, outputs.at(in->GetIdx()));
  }
  return SUCCESS;
}

void GraphManager::AddLocalOmgContext(GraphId graph_id, const OmgContext &omg_context) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  omg_contexts_.emplace(graph_id, omg_context);
  SetLocalOmgContext(omg_contexts_[graph_id]);
}

void GraphManager::UpdateLocalOmgContext(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  auto iter = omg_contexts_.find(graph_id);
  if (iter != omg_contexts_.end()) {
    SetLocalOmgContext(iter->second);
  } else {
    GELOGW("OmgContext of graph %u is not found.", graph_id);
  }
}

GraphManager::CompilerStages &GraphManager::GetCompilerStages(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  return compiler_stages_[graph_id];
}

void GraphManager::RemoveCompilerStages(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(member_mutex_);
  (void)compiler_stages_.erase(graph_id);
}

void GraphManager::IncreaseGraphCount(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(graph_count_mutex_);
  std::map<GraphId, uint32_t>::const_iterator it = graph_count_.find(graph_id);
  if (it == graph_count_.cend()) {
    (void)graph_count_.insert({graph_id, kInitGraphCount});
    GELOGD("After increaseGraphCount, graph count of id[%u] is %u.", graph_id, graph_count_[graph_id]);
  } else {
    if (CheckUint32AddOverflow(graph_count_[graph_id], 1) == SUCCESS) {
      ++graph_count_[graph_id];
    }
    GELOGD("After increaseGraphCount, graph count of id[%u] is %u.", graph_id, graph_count_[graph_id]);
  }
}

void GraphManager::RemoveGraphCount(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(graph_count_mutex_);
  auto it = graph_count_.find(graph_id);
  if (it == graph_count_.end()) {
    GELOGW("Graph of id: %u has not been added, count cannot be decreased", graph_id);
  } else {
    GELOGD("RemoveGraphCount success, graph count of id[%u] is %u", graph_id, graph_count_[graph_id]);
    (void)graph_count_.erase(it);
  }
}

void GraphManager::DecreaseGraphCount(GraphId graph_id) {
  std::lock_guard<std::mutex> lock(graph_count_mutex_);
  auto it = graph_count_.find(graph_id);
  if (it == graph_count_.end()) {
    GELOGW("Graph of id: %u has not been added, count cannot be decreased.", graph_id);
  } else {
    if (CheckUint32SubOverflow(it->second, 1) == SUCCESS) {
      --it->second;
    }
    GELOGD("After DecreaseGraphCount, graph count of id[%u] is %u.", graph_id, graph_count_[graph_id]);
  }
}

Status GraphManager::GetGraphCount(GraphId graph_id, uint32_t &count) {
  std::lock_guard<std::mutex> lock(graph_count_mutex_);
  std::map<GraphId, uint32_t>::const_iterator it = graph_count_.find(graph_id);
  if (it == graph_count_.cend()) {
    GELOGW("Graph [id:%u] has not been added.", graph_id);
    return FAILED;
  }
  count = it->second;
  return SUCCESS;
}

Status GraphManager::OptimizeGraph(const std::vector<GeTensor> &inputs, ComputeGraphPtr &compute_graph) {
  GE_CHECK_NOTNULL(compute_graph);
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(compute_graph->GetGraphID(), graph_node);
  if (ret != SUCCESS) {
    return ret;
  }
  auto session_id = compute_graph->GetSessionID();
  /// 1. BUILD_MODE_TUNING with BUILD_STEP_AFTER_UB_MATCH no need PreRunOptimizeOriginalGraph;
  /// 2. BUILD_MODE_TUNING with BUILD_STEP_AFTER_MERGE no need PreRunOptimizeOriginalGraph.
  /// 3. BUILD_MODE_TUNING with BUILD_STEP_AFTER_BUILDER_SUB no need PreRunOptimizeOriginalGraph.
  const auto build_step = GetBuildStep(graph_node);
  bool run_optimize_original_graph = !((GetBuildMode(graph_node) == BUILD_MODE_TUNING) &&
      ((build_step == BUILD_STEP_AFTER_UB_MATCH) ||
       (build_step == BUILD_STEP_AFTER_MERGE) ||
       (build_step == BUILD_STEP_AFTER_BUILDER_SUB)));
  if (run_optimize_original_graph) {
    ret = PreRunOptimizeOriginalGraph(graph_node, inputs, compute_graph, session_id);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Run][PreRunOptimizeOriginalGraph] failed for graph:%s, session_id:%lu",
             compute_graph->GetName().c_str(), session_id);
      return ret;
    }
    int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
    (void)AttrUtils::GetInt(compute_graph, kGraphDumpStage, graph_stage);
    if (graph_stage == static_cast<int64_t>(GraphStage::GRAPH_STAGE_FUZZ)) {
      GELOGD("graph_stage:%d.", static_cast<int32_t>(graph_stage));
      return SUCCESS;
    }
  }

  ErrorManager::GetInstance().SetStage(error_message::kModelCompile, error_message::kSubGraphOptimize);
  ret = PreRunOptimizeSubGraph(graph_node, compute_graph, session_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][PreRunOptimizeSubGraph] failed for graph:%s, session_id:%lu.",
           compute_graph->GetName().c_str(), session_id);
    return ret;
  }

  return SUCCESS;
}

Status GraphManager::BuildGraph(ComputeGraphPtr &compute_graph, PneModelPtr &model) {
  GE_CHECK_NOTNULL(compute_graph);
  GraphNodePtr graph_node = nullptr;
  Status ret = GetGraphNode(compute_graph->GetGraphID(), graph_node);
  if (ret != SUCCESS) {
    return ret;
  }

  GeRootModelPtr ge_root_model = nullptr;

  /// 1. BUILD_MODE_TUNING with BUILD_STEP_BEFORE_UB_MATCH no need PreRunAfterOptimizeSubGraph;
  /// 2. BUILD_MODE_TUNING with BUILD_STEP_AFTER_BUILDER no need PreRunAfterOptimizeSubGraph.
  /// 3. BUILD_MODE_TUNING with BUILD_STEP_AFTER_BUILDER_SUB no need PreRunAfterOptimizeSubGraph.
  const auto build_step = GetBuildStep(graph_node);
  bool run_after_optimize_subgraph =
      !((GetBuildMode(graph_node) == BUILD_MODE_TUNING) &&
        ((build_step == BUILD_STEP_BEFORE_UB_MATCH) || (build_step == BUILD_STEP_AFTER_BUILDER) ||
         (build_step == BUILD_STEP_AFTER_BUILDER_SUB)));
  if (run_after_optimize_subgraph) {
    ret = PreRunAfterOptimizeSubGraph(graph_node, compute_graph, ge_root_model, compute_graph->GetSessionID());
    model = ge_root_model;
  }
  return ret;
}

Status GraphManager::ProcessNodeEngineBuild(const GraphNodePtr &graph_node,
                                            ComputeGraphPtr &compute_graph,
                                            const std::vector<GeTensor> &inputs,
                                            FlowModelPtr &flow_model) {
  GE_CHECK_NOTNULL(compute_graph);
  GE_CHECK_NOTNULL(graph_node);
  uint64_t session_id = compute_graph->GetSessionID();
  Status ret = SUCCESS;

  ret = ProcessNodeEnginePartion(graph_node, compute_graph, flow_model);
  if (ret != SUCCESS) {
    return ret;
  }

  for (auto pne_compute_graph : graph_node->GetComputeGraph()->GetAllSubgraphs()) {
    GE_CHECK_NOTNULL(pne_compute_graph);
    PneModelPtr pne_root_model = nullptr;
    std::string pne_id;
    (void)AttrUtils::GetStr(pne_compute_graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_id);
    auto process_node_engine = process_node_engines_[pne_id];
    if (process_node_engine != nullptr) {
      ret = process_node_engine->OptimizeGraph(inputs, pne_compute_graph);
      if (ret != SUCCESS) {
        GELOGE(ret, "[Run][OptimizeSubGraph] failed for graph:%s, session_id:%lu.",
               pne_compute_graph->GetName().c_str(), session_id);
        return ret;
      }
      int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
      (void)AttrUtils::GetInt(pne_compute_graph, kGraphDumpStage, graph_stage);
      if (graph_stage == static_cast<int64_t>(GraphStage::GRAPH_STAGE_FUZZ)) {
        GELOGD("graph_stage:%d.", static_cast<int32_t>(graph_stage));
        continue;
      }
      ret = process_node_engine->BuildGraph(pne_compute_graph, pne_root_model);
      if (ret != SUCCESS) {
        GELOGE(ret, "[Run][BuildGraph] failed for graph:%s, session_id:%lu.", pne_compute_graph->GetName().c_str(),
               session_id);
        return ret;
      }
      // in case no build pne_root_model is nullptr
      if (pne_root_model != nullptr) {
        GE_CHECK_NOTNULL(graph_node->GetFlowModel());
        (void)ge::AttrUtils::SetStr(pne_root_model->GetRootGraph(), ge::ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_id);
        ret = graph_node->GetFlowModel()->AddSubModel(pne_root_model, pne_id);
        if (ret != SUCCESS) {
          return ret;
        }
      }
    } else {
      GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Run][GetEngine] failed for graph:%s, session_id:%lu, engine:%s.",
             pne_compute_graph->GetName().c_str(), session_id, pne_id.c_str());
      return GE_CLI_GE_NOT_INITIALIZED;
    }
  }
  GEEVENT("[GEPERFTRACE] GE PreRun End");
  return SUCCESS;
}

std::string GraphManager::GetBuildMode(const GraphNodePtr &graph_node) const {
  const auto it = graph_node->GetOptions().find(BUILD_MODE);
  if (it != graph_node->GetOptions().end()) {
    return it->second;
  }
  return options_.build_mode;
}

std::string GraphManager::GetBuildStep(const GraphNodePtr &graph_node) const {
  const auto it = graph_node->GetOptions().find(BUILD_STEP);
  if (it != graph_node->GetOptions().end()) {
    return it->second;
  }
  return options_.build_step;
}

std::string GraphManager::GetTuningPath(const GraphNodePtr &graph_node) const {
  const auto it = graph_node->GetOptions().find(TUNING_PATH);
  if (it != graph_node->GetOptions().end()) {
    return it->second;
  }
  return options_.tuning_path;
}

void GraphManager::RefreshOptionByGraph(uint32_t graph_id, GraphManagerOptions &refreshed_options) {
  GraphNodePtr graph_node;
  if (GetGraphNode(graph_id, graph_node) != SUCCESS) {
    GELOGW("[Get][GraphNode] failed, Graph not exist while done adding previously, graph_id = %u", graph_id);
    return;
  }
  refreshed_options.build_mode = GetBuildMode(graph_node);
  refreshed_options.build_step = GetBuildStep(graph_node);
  refreshed_options.tuning_path = GetTuningPath(graph_node);
}
}  // namespace ge
