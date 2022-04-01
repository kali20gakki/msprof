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

#include "graph/execute/model_executor.h"

#include "graph/ge_context.h"
#include "common/ge_call_wrapper.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/load/graph_loader.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/profiling/profiling_manager.h"
#include "common/utils/executor_utils.h"
#include "exec_runtime/execution_runtime.h"

namespace {
constexpr int32_t kBase = 10;
constexpr uint8_t kNeverLoaded = 0U;

bool IsNeedHeterogeneousLoad(const ge::FlowModelPtr &flow_model) {
  if (ge::ExecutionRuntime::IsHeterogeneous()) {
    return true;
  }
  for (auto &pne_model_pair : flow_model->GetSubmodels()) {
    auto pne_model = pne_model_pair.second;
    if ((pne_model != nullptr) && (!pne_model->GetSubmodels().empty())) {
      return true;
    }
  }
  return false;
}
}

namespace ge {
///
/// @ingroup ge
/// @brief graph executor init
/// @param [in] options user config params
/// @return Status result of function
///
Status ModelExecutor::Initialize(const std::map<std::string, std::string> &options, const uint64_t session_id) {
  if (init_flag_) {
    GELOGW("ModelExecutor has already initialized.");
    return SUCCESS;
  }

  session_id_ = session_id;
  if (!ExecutionRuntime::IsHeterogeneous()) {
    size_t total_mem_size = 0U;
    GE_CHK_STATUS_RET_NOLOG(GetTotalMemorySize(total_mem_size));
    const auto status = VarManager::Instance(session_id)->SetMemoryMallocSize(options, total_mem_size);
    if (status != SUCCESS) {
      GELOGE(status, "[Set][MemoryMallocSize] failed.");
      REPORT_CALL_ERROR("E19999", "VarManager SetMemoryMallocSize failed, InnerSession:%lu.", session_id_);
      return status;
    }
  } else {
    const auto status = VarManager::Instance(session_id)->SetAllMemoryMaxValue(options);
    if (status != SUCCESS) {
      GELOGE(status, "[Set][MemoryMallocSize] failed.");
      REPORT_CALL_ERROR("E19999", "VarManager SetAllMemoryMaxValue failed, InnerSession:%lu.", session_id_);
      return status;
    }
  }

  train_graph_flag_ = ParseTrainGraphFlag();
  thread_run_flag_.store(true);
  run_thread_ = std::thread(&ModelExecutor::RunThread, this);

  init_flag_ = true;
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief graph executor finalize
/// @return Status result of function
///
Status ModelExecutor::Finalize() {
  if (!init_flag_) {
    GELOGW("ModelExecutor has not been initialized.");
    return SUCCESS;
  }

  StopQueue();
  if (run_thread_.joinable()) {
    run_thread_.join();
  }

  if (graph_executor_.FreeExecuteMemory() != SUCCESS) {
    GELOGW("Graph executor FreeExecuteMemory failed, resources may not be released correctly.");
  }

  GELOGI("VarManager free var memory.");
  (void)VarManager::Instance(session_id_)->FreeVarMemory();
  MemManager::Instance().FreeSessionMemory(session_id_);

  ModelManager::GetInstance().DestroyAicpuSession(session_id_);
  return SUCCESS;
}

Status ModelExecutor::GetTotalMemorySize(size_t &total_mem_size) {
  GE_CHK_STATUS_RET(MdsUtils::SetDevice(static_cast<int32_t>(GetContext().DeviceId())));
  size_t free_mem = 0U;
  GE_CHK_RT_RET(rtMemGetInfoEx(RT_MEMORYINFO_HBM, &free_mem, &total_mem_size));
  if (total_mem_size == 0U) {
    GE_CHK_RT_RET(rtMemGetInfoEx(RT_MEMORYINFO_DDR, &free_mem, &total_mem_size));
  }
  GE_CHK_RT_RET(rtDeviceReset(static_cast<int32_t>(GetContext().DeviceId())));

  return SUCCESS;
}

// OPTION_GRAPH_RUN_MODE is supposed to be a session-level option, but it used to be set to global-level in the past.
// If can not parse from session, it can parse from global by GetContext().
bool ModelExecutor::ParseTrainGraphFlag() {
  std::string run_mode;
  if ((GetContext().GetOption(OPTION_GRAPH_RUN_MODE, run_mode) == SUCCESS) && (!run_mode.empty())) {
    const GraphRunMode mode = static_cast<GraphRunMode>(std::strtol(run_mode.c_str(), nullptr, kBase));
    if (mode >= TRAIN) {
      GELOGI("Graph train flag set.");
      return true;
    }
  }
  return false;
}

void ModelExecutor::AddGraphNode(const GraphId graph_id, const GraphNodePtr &graph_node) {
  const std::lock_guard<std::mutex> lk(mutex_);
  graph_nodes_[graph_id] = graph_node;
}

void ModelExecutor::RemoveGraphNode(const GraphId graph_id) {
  const std::lock_guard<std::mutex> lk(mutex_);
  (void)graph_nodes_.erase(graph_id);
}

///
/// @ingroup ge
/// @brief Load mode for graph.
/// @param [in] FlowModel: root model of graph compiled.
/// @param [in] GraphNode: node of graph.
/// @return Status result of function
///
Status ModelExecutor::LoadGraph(const FlowModelPtr &flow_model, const GraphNodePtr &graph_node) {
  GE_CHECK_NOTNULL(graph_node);
  GE_CHECK_NOTNULL(flow_model);
  if (flow_model->GetSubmodels().empty()) {
    return SUCCESS;
  }
  return ModelLoad(flow_model, graph_node);
}

///
/// @ingroup ge
/// @brief Unload mode for graph.
/// @param [in] GeRootModel: root model of graph compiled.
/// @param [in] graph_id: graph identifier.
/// @return Status result of function
///
Status ModelExecutor::UnloadGraph(const FlowModelPtr &flow_model, const uint32_t graph_id) {
  GE_CHECK_NOTNULL(flow_model);
  GE_CHK_STATUS_RET(MdsUtils::SetDevice(static_cast<int32_t>(GetContext().DeviceId())));
  RemoveGraphNode(graph_id);
  const Status ret = UnloadModel(flow_model, graph_id);
  if (ret != SUCCESS) {
    GELOGW("[GraphExecutor] unload model failed, graph_id=%u.", graph_id);
  }

  GE_CHK_RT_RET(rtDeviceReset(static_cast<int32_t>(GetContext().DeviceId())));
  return ret;
}

Status ModelExecutor::UnloadModel(const FlowModelPtr &flow_model, const uint32_t graph_id) {
  if (ModelManager::GetInstance().IsHeterogeneous(flow_model->GetModelId())) {
    return GraphLoader::UnloadModel(flow_model->GetModelId());
  }

  for (auto &pne_model_pair : flow_model->GetSubmodels()) {
    auto pne_model = pne_model_pair.second;
    if (pne_model == nullptr) {
      continue;
    }

    auto ge_root_model = std::dynamic_pointer_cast<GeRootModel>(pne_model);
    if (ge_root_model == nullptr) {
      (void) GraphLoader::UnloadModel(pne_model->GetModelId());
      continue;
    }

    // GeRootModel process
    for (size_t i = 0U; i < ge_root_model->GetAllModelId().size(); ++i) {
      const uint32_t model_id = ge_root_model->GetAllModelId()[i];
      GELOGI("Unload model %u.", model_id);
      const Status ret = GraphLoader::UnloadModel(model_id);
      if (ret != SUCCESS) {
        GELOGE(ret, "[GraphExecutor] unload model failed, modelId=%u, graphId=%u.", model_id, graph_id);
        return ret;
      }
    }
  }

  return SUCCESS;
}

void ModelExecutor::StopQueue() {
  thread_run_flag_.store(false);
  run_args_q_.Stop();
}

void ModelExecutor::ReturnError(const RunAsyncCallback callback, const Status ret, const std::string &log_info) {
  GELOGE(ret, "%s.", log_info.c_str());
  if (callback != nullptr) {
    std::vector<Tensor> outputs;
    callback(ret, outputs);
  }
  StopQueue();
}

///
/// @ingroup ge
/// @brief Push model execution params to queue.
/// @param [in] RunArgs of for model execution.
/// @return Status result of function
///
Status ModelExecutor::PushRunArgs(const RunArgs &args) {
  return run_args_q_.Push(args) ? SUCCESS : FAILED;
}

void ModelExecutor::RunThread() {
  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  if (mmSetCurrentThreadName("GE_Run") != EN_OK) {
    GELOGW("Set thread name failed.");
  }

  RunArgs args;
  while (thread_run_flag_) {
    if (!run_args_q_.Pop(args)) {
      continue;
    }

    GELOGI("[RunThread] A new loop start, graph_id:%u.", args.graph_id);
    ErrorManager::GetInstance().SetErrorContext(args.error_context);
    GetContext().SetSessionId(args.session_id);
    GetThreadLocalContext() = args.context;
    GE_MAKE_GUARD(args, [args] { args.graph_node->Unlock(); });
    if ((args.flow_model == nullptr) || args.flow_model->GetSubmodels().empty()) {
      ReturnError(args.callback, PARAM_INVALID, "flow_model is invalid, thread exit.");
      return;
    }
    Status ret = SUCCESS;
    args.graph_node->UpdateLoadFlag();
    if (!args.graph_node->GetLoadFlag()) {
      ErrorManager::GetInstance().SetStage(error_message::kModelLoad, error_message::kModelLoad);
      args.graph_node->SetAsync(true);
      ret = ModelLoad(args.flow_model, args.graph_node);
      if (ret != SUCCESS) {
        ReturnError(args.callback, ret, "LoadGraphAsync failed, thread exit.");
        return;
      }
      // control the times of graph loading in multi-thread scenario
      args.graph_node->DecreaseLoadCount();
      args.graph_node->IncreaseLoadRecord();
      args.graph_node->SetLoadFlag(true);
      GELOGI("LoadGraph[%u], model[%u] success and set LoadFlag to true.", args.graph_node->GetGraphId(),
             args.flow_model->GetModelId());
    }

    ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
    const auto model_id = args.flow_model->GetModelId();
    if (ModelManager::GetInstance().IsHeterogeneous(model_id)) {
      GELOGD("Execute by HeterogeneousModelExecutor, graph id = %u, model_id = %u", args.graph_id, model_id);
      const auto executor = ModelManager::GetInstance().GetHeterogeneousModelExecutor(model_id);
      if (executor != nullptr) {
        ret = executor->ExecuteAsync(args.input_tensor, args.callback);
      }
    } else {
      auto ge_root_model = std::dynamic_pointer_cast<GeRootModel>(args.flow_model->GetSubmodels().begin()->second);
      ret = graph_executor_.ExecuteGraphAsync(args.graph_id, ge_root_model, args.input_tensor, args.callback);
    }
    args.graph_node->SetRunFlag(false);
    if (ret != SUCCESS) {
      ReturnError(args.callback, ret, "ExecuteGraphAsync failed, thread exit.");
      return;
    }
    GELOGI("[GraphExecutor] Run graph async success, graph_id=%u.", args.graph_id);
  }
}

///
/// @ingroup ge
/// @brief Run graph for synchronize model.
/// @param [in] graph_node: node of graph.
/// @param [in] graph_id: graph identifier.
/// @param [in] inputs: input data for the graph running.
/// @param [out] outputs: output data of the graph running
/// @return Status result of function
///
Status ModelExecutor::RunGraph(const GraphNodePtr &graph_node, const GraphId graph_id,
                               const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) {
  const auto &flow_model = graph_node->GetFlowModel();
  if ((flow_model == nullptr) || (flow_model->GetSubmodels().empty())) {
    GELOGW("ge_root_model is null, graph_id:%u", graph_id);
    return FAILED;
  }

  Status ret = SUCCESS;
  const auto model_id = flow_model->GetModelId();
  if (ModelManager::GetInstance().IsHeterogeneous(model_id)) {
    GELOGI("[ExecuteGraph] SyncExecuteModel via heterogeneous model executor, modelId=%u", model_id);
    ret = ModelManager::GetInstance().ExecuteHeterogeneousModel(model_id, inputs, outputs);
  } else {
    auto ge_root_model = std::dynamic_pointer_cast<GeRootModel>(flow_model->GetSubmodels().begin()->second);
    GE_CHECK_NOTNULL(ge_root_model);
    ret = graph_executor_.ExecuteGraph(graph_id, ge_root_model, inputs, outputs);
  }
  graph_node->SetRunFlag(false);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Execute][Graph] failed, graph_id = %u.", graph_id);
  }
  return ret;
}

///
/// @ingroup ge
/// @brief Run graph for NN synchronize model.
/// @param [in] graph_node: node of graph.
/// @param [in] graph_id: graph identifier.
/// @param [in] stream: Stream for model running.
/// @param [in] inputs: input data for the graph running.
/// @param [out] outputs: output data of the graph running
/// @return Status result of function
///
Status ModelExecutor::RunGraphWithStream(const GraphNodePtr &graph_node, const GraphId graph_id,
                                         const rtStream_t stream, const std::vector<GeTensor> &inputs,
                                         const std::vector<GeTensor> &outputs) {
  if ((graph_node->GetFlowModel() == nullptr) || (graph_node->GetFlowModel()->GetSubmodels().empty())) {
    GELOGW("ge_root_model is null, graph_id:%u", graph_id);
    return FAILED;
  }
  auto ge_root_model =
      std::dynamic_pointer_cast<GeRootModel>(graph_node->GetFlowModel()->GetSubmodels().begin()->second);
  const auto ret = graph_executor_.ExecuteGraphWithStream(graph_id, stream,
                                                          ge_root_model,
                                                          inputs, outputs);
  graph_node->SetRunFlag(false);
  graph_node->SetIsSpecificStream(false);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Execute][Graph] With Stream failed, graph id = %u, stream = %p.", graph_id, stream);
    return ret;
  }
  GELOGI("[Run][GraphWithStreamAsync] run graph success, graph id = %u, stream = %p.", graph_id, stream);
  return SUCCESS;
}

Status ModelExecutor::ModelLoad(const FlowModelPtr &flow_model, const GraphNodePtr &graph_node) {
  Status ret;
  uint32_t model_id = INVALID_MODEL_ID;
  if (IsNeedHeterogeneousLoad(flow_model)) {
    ret = FlowModelLoad(flow_model, graph_node);
  } else {
    auto ge_root_model = std::dynamic_pointer_cast<GeRootModel>(flow_model->GetSubmodels().begin()->second);
    GE_CHECK_NOTNULL(ge_root_model);
    if (!graph_node->IsAsync()) {
      ge_root_model->SetIsSpecificStream(graph_node->IsSpecificStream());
    }
    ge_root_model->SetTrainFlag(train_graph_flag_);
    const auto root_graph = ge_root_model->GetRootGraph();
    bool is_unknown_shape = false;
    GE_CHK_STATUS_RET(ge_root_model->CheckIsUnknownShape(is_unknown_shape));
    if (!is_unknown_shape) {
      char_t static_mem_env[MMPA_MAX_PATH] = {};
      if (mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH)) == EN_OK) {
        GELOGI("[LoadGraph] GE_USE_STATIC_MEMORY is seted.");
      } else {
        GE_CHECK_NOTNULL(root_graph);
        const auto &name_to_model = ge_root_model->GetSubgraphInstanceNameToModel();
        const auto it = name_to_model.find(root_graph->GetName());
        const GeModelPtr ge_model = (it != name_to_model.end()) ? it->second : nullptr;
        GE_CHECK_NOTNULL(ge_model);
        GE_CHK_STATUS_RET(CheckAndReleaseMemory(ge_model, graph_node));
      }
    }

    GE_TIMESTAMP_START(LoadModelOnline);
    std::vector<NamedAttrs> deploy_info;
    if (AttrUtils::GetListNamedAttrs(root_graph, ATTR_NAME_DEPLOY_INFO, deploy_info) && (!deploy_info.empty())) {
      ret = GraphLoader::MultiLoadModelOnline(ge_root_model, graph_node, deploy_info);
    } else {
      ret = GraphLoader::LoadModelOnline(model_id, ge_root_model, graph_node, GetContext().DeviceId(),
                                         ErrorManager::GetInstance().GetErrorManagerContext(),
                                         std::numeric_limits<int64_t>::max());
    }
    GE_TIMESTAMP_EVENT_END(LoadModelOnline, "GraphLoader::LoadModelOnline");
  }
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][ModelOnline] Failed, model_id:%u", model_id);
    graph_node->SetRunFlag(false);
    return ret;
  }
  graph_node->SetLoadFlag(true);
  AddGraphNode(graph_node->GetGraphId(), graph_node);
  return SUCCESS;
}

void ModelExecutor::ReleaseMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                                  const std::vector<uint32_t> &model_ids, const uint32_t graph_id,
                                  uint64_t session_id) {
  for (const auto &model_id : model_ids) {
    uint64_t max_memory_size = 0U;
    Status result = ModelManager::GetInstance().GetMaxUsedMemory(model_id, max_memory_size);
    if (result != SUCCESS) {
      continue;
    }
    GELOGI("try to UnloadGraph[%u], model[%u] which MaxUsedMemory[%lu].", graph_id, model_id, max_memory_size);
    if (model_ids.size() > 1U) {
      result = ge_model->GetSessionId(model_id, session_id);
      if (result != SUCCESS) {
        GELOGW("[GraphExecutor:] get session failed when dynamic memory, modelId=%u, graphId=%u.", model_id,
               graph_id);
        continue;
      }
    }
    result = ModelManager::GetInstance().DestroyAicpuKernel(session_id, model_id, 0U);
    if (result != SUCCESS) {
      GELOGW("[GraphExecutor:] destroy aicpu kernel failed when dynamic memory, modelId=%u, graphId=%u.", model_id,
             graph_id);
    }
    result = GraphLoader::UnloadModel(model_id);
    if (result != SUCCESS) {
      GELOGW("[GraphExecutor:] unload model failed, modelId=%u, graphId=%u.", model_id, graph_id);
    }
    GELOGI("UnloadGraph[%u], model[%u] success.", graph_id, model_id);
  }
  graph_node->SetLoadFlag(false);
  // Allow model to be loaded agagin without adding graph again
  graph_node->SetLoadCount(graph_node->GetLoadRecord());
  graph_node->SetLoadRecord(kNeverLoaded);
  if ((graph_node->GetFlowModel() == nullptr) || (graph_node->GetFlowModel()->GetSubmodels().empty())) {
    GELOGW("ge_root_model is null, graph_id:%u", graph_id);
    return;
  }
  const GeRootModelPtr ge_root_model =
      std::dynamic_pointer_cast<GeRootModel>(graph_node->GetFlowModel()->GetSubmodels().begin()->second);
  if (ge_root_model == nullptr) {
    GELOGW("ge_root_model is null, graph_id:%u", graph_id);
    return;
  }
  ge_root_model->ClearAllModelId();
}

Status ModelExecutor::GetMemoryInfo(size_t &free) {
  GE_CHK_STATUS_RET(MdsUtils::SetDevice(static_cast<int32_t>(GetContext().DeviceId())));
  size_t total_mem = 0U;
  size_t free_mem = 0U;
  GE_CHK_RT_RET(rtMemGetInfo(&free_mem, &total_mem));
  GE_CHK_RT_RET(rtDeviceReset(static_cast<int32_t>(GetContext().DeviceId())));

  // Add small page memory size
  const size_t limited_max_size = VarManager::Instance(GetContext().SessionId())->GetUseMaxMemorySize();
  free = ((free_mem + limited_max_size) > total_mem) ? (free_mem + limited_max_size - total_mem) : 0U;
  GELOGI("GetMemoryInfo free[%zu], total[%zu], return free[%zu]", free_mem, total_mem, free);
  return SUCCESS;
}

Status ModelExecutor::GetMemorySizeAfterReuse(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                                              int64_t &sum_size, bool &reuse) {
  int64_t value = 0;
  const int64_t memory_size = AttrUtils::GetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, value) ? value : 0;
  const int64_t weight_size = AttrUtils::GetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, value) ? value : 0;
  const int64_t zero_copy_size = AttrUtils::GetInt(ge_model, ATTR_MODEL_ZERO_COPY_MEMORY_SIZE, value) ? value : 0;

  const auto &mem_instance = SessionMemAllocator::Instance().GetMemAllocator(session_id_);
  const int64_t malloced_feature_mem_size = mem_instance.MemorySize(kFeatureMemoryKey, GetContext().DeviceId());

  int64_t non_zero_copy_mem_size = memory_size;
  const bool is_reuse_zero_copy_memory = ExecutorUtils::IsReuseZeroCopyMemory();
  if (is_reuse_zero_copy_memory) {
    if (memory_size < zero_copy_size) {
      REPORT_CALL_ERROR("E19999", "total mem size[%ld] is less than zero copy size[%ld] ",
                        memory_size, zero_copy_size);
      GELOGE(FAILED, "[Check] failed, total mem size[%ld] is less than zero copy size[%ld]",
             memory_size, zero_copy_size);
      return FAILED;
    }
    non_zero_copy_mem_size = memory_size - zero_copy_size;
  }

  if (CheckInt64AddOverflow(memory_size, weight_size) != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "memory_size[%ld] and weight_size[%ld] will overflow after add, check invalid",
                       memory_size, weight_size);
    GELOGE(INTERNAL_ERROR, "[Check][Param] non_zero_copy_mem_size[%ld] and weight_size[%ld] will overflow after add",
           memory_size, weight_size);
    return INTERNAL_ERROR;
  }

  reuse = false;
  if (non_zero_copy_mem_size <= malloced_feature_mem_size) {
    reuse = true;
  }
  sum_size = weight_size + (reuse ? 0 : non_zero_copy_mem_size);

  GELOGI("Graph[%u] reuse[%d], non_zero_copy_mem_size[%ld], malloced_feature_mem_size[%ld], sum_size[%ld],"
         " memory_size[%ld], zero_copy_size[%ld], is_reuse_zero_copy_memory[%d], "
         " weight_size[%ld], Device[%u]", graph_node->GetGraphId(), reuse, non_zero_copy_mem_size,
         malloced_feature_mem_size, sum_size, memory_size, zero_copy_size, is_reuse_zero_copy_memory,
         weight_size, GetContext().DeviceId());
  return SUCCESS;
}

Status ModelExecutor::CheckFreeMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node,
                                      bool &is_enough, bool &release_all) {
  is_enough = false;
  release_all = false;
  GELOGI("graph_id[%u]", graph_node->GetGraphId());
  size_t free_memory = 0U;
  GE_CHK_STATUS_RET(GetMemoryInfo(free_memory));

  int64_t sum_size = 0;
  if (ExecutorUtils::IsGeUseExtendSizeStaticMemory()) {
    bool reuse = false;
    GE_CHK_STATUS_RET(GetMemorySizeAfterReuse(ge_model, graph_node, sum_size, reuse));
    release_all = reuse ? false : true;
    GELOGI("GE_USE_STATIC_MEMORY is set, release_all[%d], reuse[%d]", release_all, reuse);
  } else {
    int64_t value = 0;
    const int64_t memory_size = AttrUtils::GetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, value) ? value : 0;
    const int64_t weight_size = AttrUtils::GetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, value) ? value : 0;

    GELOGI("Graph[%u] need memory_size[%ld], weight_size[%ld], Device[%u] free_memory_size[%zu]",
           graph_node->GetGraphId(), memory_size, weight_size, GetContext().DeviceId(), free_memory);
    if (CheckInt64AddOverflow(memory_size, weight_size) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "memory_size[%ld] and weight_size[%ld] will overflow after add, check invalid",
                         memory_size, weight_size);
      GELOGE(INTERNAL_ERROR, "[Check][Param] memory_size[%ld] and weight_size[%ld] will overflow after add",
             memory_size, weight_size);
      return INTERNAL_ERROR;
    }
    sum_size = memory_size + weight_size;
  }

  if (free_memory >= static_cast<size_t>(sum_size)) {
    is_enough = true;
  }
  return SUCCESS;
}

Status ModelExecutor::CheckAndReleaseMemory(const GeModelPtr &ge_model, const GraphNodePtr &graph_node) {
  bool is_enough = false;
  bool release_all = false;
  GE_CHK_STATUS_RET(CheckFreeMemory(ge_model, graph_node, is_enough, release_all));
  if ((!release_all) && is_enough) {
    GELOGI("graph id[%u] no need to unload other models, release_all[%d], is_enough[%d]",
           graph_node->GetGraphId(), release_all, is_enough);
    return SUCCESS;
  }
  GELOGI("graph id[%u] need to unload other models, if have any, release_all[%d], is_enough[%d]",
         graph_node->GetGraphId(), release_all, is_enough);

  const std::lock_guard<std::mutex> lk(mutex_);
  for (const auto &it : graph_nodes_) {
    const auto graph_id = it.second->GetGraphId();
    if ((it.second->GetFlowModel() == nullptr) || (it.second->GetFlowModel()->GetSubmodels().empty())) {
      continue;
    }
    const auto
        model = std::dynamic_pointer_cast<GeRootModel>(it.second->GetFlowModel()->GetSubmodels().begin()->second);
    if (model == nullptr) {
      continue;
    }
    const auto &model_ids = model->GetAllModelId();
    // unload model not release
    bool is_unknown_shape = false;
    GE_CHK_STATUS_RET(model->CheckIsUnknownShape(is_unknown_shape));
    if (is_unknown_shape) {
      continue;
    }
    // not loaded,no need unload
    if (!it.second->GetLoadFlag()) {
      GELOGI("CheckAndReleaseMemory graph[%u] has not been loaded.", graph_id);
      continue;
    }
    // unload static shape model
    int64_t value = 0;
    const int64_t session_id = AttrUtils::GetInt(ge_model, MODEL_ATTR_SESSION_ID, value) ? value : 0;
    GEEVENT("Start to release static model memory.");
    GE_CHK_RT_RET(rtSetDevice(static_cast<int32_t>(GetContext().DeviceId())));
    ReleaseMemory(ge_model, it.second, model_ids, graph_id, static_cast<uint64_t>(session_id));
    if ((!release_all)) {
      GE_CHK_STATUS_RET(CheckFreeMemory(ge_model, graph_node, is_enough, release_all));
    }
    GE_CHK_RT_RET(rtDeviceReset(static_cast<int32_t>(GetContext().DeviceId())));
    if ((!release_all) && is_enough) {
      return SUCCESS;
    }
  }

  // unload unkonwn shape model
  GEEVENT("Start to release dynamic model memory.");
  (void)MemManager::Instance().CachingInstance(RT_MEMORY_HBM).TryFreeBlocks();
  (void)hybrid::NpuMemoryAllocator::FreeCachedMem();
  return SUCCESS;
}

Status ModelExecutor::FlowModelLoad(const FlowModelPtr &flow_model, const GraphNodePtr &graph_node) {
  uint32_t model_id = INVALID_MODEL_ID;
  auto &model_mgr = ModelManager::GetInstance();
  GE_CHK_STATUS(model_mgr.LoadFlowModelOnline(model_id, flow_model, graph_node));
  flow_model->SetModelId(model_id);
  const auto ret = model_mgr.Start(model_id);
  if (ret != SUCCESS) {
    GE_CHK_STATUS(model_mgr.Unload(model_id), "[Unload][Model] failed after start failed, model_id:%u.", model_id);
    GELOGE(ret, "[Start][Model] failed, model_id:%u.", model_id);
    return ret;
  }
  GELOGI("Load model online success, model_id:%u.", model_id);
  return SUCCESS;
}
} // namespace ge
