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

#include "hybrid/executor/hybrid_model_executor.h"
#include "graph/ge_context.h"
#include "graph/runtime_inference_context.h"
#include "graph/utils/tensor_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/dump/dump_manager.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling_definitions.h"
#include "hybrid/executor/host_cpu_callback_manager.h"
#include "hybrid/executor/rt_callback_manager.h"

namespace ge {
namespace hybrid {
HybridModelExecutor::HybridModelExecutor(HybridModel *const model, const uint32_t device_id, const rtStream_t stream,
                                         ThreadPool *const thread_pool)
    : model_(model),
      device_id_(device_id),
      stream_(stream),
      executor_(model_->GetRootGraphItem(), &context_, false, thread_pool) {}

HybridModelExecutor::~HybridModelExecutor() {
}

Status HybridModelExecutor::Init(CallbackManager *const callback_manager) {
  GELOGD("Start to init HybridGraphEngine.");
  GE_CHK_STATUS_RET_NOLOG(InitExecutionContext(callback_manager));
  GE_CHK_STATUS_RET_NOLOG(executor_.Init());
  GELOGD("HybridGraphEngine initialized successfully.");
  return SUCCESS;
}

Status HybridModelExecutor::ExecuteForSingleOp(const HybridModelExecutor::ExecuteArgs &args) {
  const auto ret = executor_.ExecuteAsync(args.inputs, args.input_desc, args.outputs);

  if (context_.has_observer) {
    PROFILING_SCOPE(-1, profiling::kInitInferShapeContext);
    context_.runtime_context_.Release();
  }
  HYBRID_CHK_STATUS_RET(ret, "[Run][Execute] Single op model execute Failed.");

  // When dump on, must wait for the callback function to finish executing.
  if (context_.IsDumpEnabled()) {
    GE_CHK_STATUS_RET(context_.callback_manager->Destroy(), "[Destroy][Callback] for failed.");
    GE_CHK_STATUS_RET(context_.callback_manager->Init(), "[Init][Callback] for failed.");
  }
  executor_.Reset();
  return SUCCESS;
}

Status HybridModelExecutor::Execute(HybridModelExecutor::ExecuteArgs &args) {
  GELOGD("Start to execute model.");
  const auto root_graph_item = model_->GetRootGraphItem();
  GE_CHECK_NOTNULL(root_graph_item);

  // one-node-multiple-bin mode does not need to check shape range which will be modified in fuzz compile
  if (root_graph_item->IsDynamic() && model_->GetNodeBinMode() == kOneNodeSingleBinMode) {
    GE_CHK_STATUS_RET(CheckInputShapeByShapeRange(root_graph_item, args),
                      "[Check][InputShape] By ShapeRange for [%s] failed.", root_graph_item->GetName().c_str());
  }

  if (context_.global_step != nullptr) {
    GE_CHK_RT_RET(rtMemcpyAsync(context_.global_step, sizeof(uint64_t), &context_.iteration,
                                sizeof(uint64_t), RT_MEMCPY_HOST_TO_DEVICE_EX, context_.stream));
  }
  const auto ret = ExecuteGraphInternal(executor_, args);
  (void)Cleanup();
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[Cleanup] End");
  GELOGD("Model executed successfully.");
  if (context_.profiler != nullptr) {
    context_.profiler->Dump(std::cout);
    context_.profiler->Reset();
  }

  context_.iteration += 1;
  executor_.Reset();
  if (ret == END_OF_SEQUENCE) {
    args.is_eos = true;
  } else {
    if (ret != ge::SUCCESS) {
      const auto exception_infos = ModelManager::GetInstance().GetExceptionInfos();
      if (!exception_infos.empty()) {
        HYBRID_CHK_STATUS_RET(context_.DumpExceptionInfo(exception_infos),
                              "[Execute][GraphInternal] Dump exception info failed.");
      }
      GELOGE(ret, "[Invoke][ExecuteGraphInternal] Failed, ret:%u.", ret);
      return ret;
    }
  }
  return SUCCESS;
}

Status HybridModelExecutor::ExecuteGraphInternal(SubgraphExecutor &executor,
                                                 HybridModelExecutor::ExecuteArgs &args) {
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[InitContext] Start");
  GE_CHK_STATUS_RET_NOLOG(ResetExecutionContext(context_));
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[InitContext] End");
  GE_CHECK_NOTNULL(model_->GetRootGraph());

  const uint64_t index_id = static_cast<uint64_t>(context_.iteration) + 1U;
  auto &prof_mgr = ProfilingManager::Instance();
  // tag_id 0 means step begin, 1 meas step end.
  GE_CHK_STATUS_RET_NOLOG(prof_mgr.ProfileStepInfo(index_id, model_->GetModelId(), 0U, stream_, device_id_));
  HYBRID_CHK_STATUS_RET(executor.ExecuteAsync(args.inputs, args.input_desc, args.outputs),
                        "[Call][ExecuteAsync] Failed to execute partitioned call.");
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[ExecuteAsync] End");
  GE_CHK_STATUS_RET_NOLOG(prof_mgr.ProfileStepInfo(index_id, model_->GetModelId(), 1U, stream_, device_id_));

  const Status ret = executor.Synchronize();
  if (ret != ge::SUCCESS) {
    if (ret == ge::END_OF_SEQUENCE) {
      GELOGD("Got end of sequence");
    } else {
      GELOGE(ret, "[Execute][GraphInternal] Synchronize failed.");
    }
    return ret;
  }
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[Synchronize] End");

  args.outputs.clear();
  HYBRID_CHK_STATUS_RET(executor.GetOutputs(args.outputs, args.output_desc), "[Get][Outputs] failed");
  RECORD_MODEL_EXECUTION_EVENT(&context_, "[GetOutput] End");
  return SUCCESS;
}

Status HybridModelExecutor::Cleanup() {
  GELOGD("Start to cleanup.");
  (void)context_.callback_manager->Destroy();
  context_.runtime_context_.Release();
  GELOGD("Cleanup successfully.");
  return SUCCESS;
}

Status HybridModelExecutor::InitExecutionContext(CallbackManager *const callback_manager) {
  GE_CHK_RT_RET(rtCtxGetCurrent(&context_.rt_context));
  GE_CHK_RT_RET(rtCtxSetCurrent(context_.rt_context));

  context_.is_host_cpu = ::ge::GetContext().GetHostExecFlag();
  context_.global_step = model_->GetGlobalStep();
  context_.stream = stream_;
  context_.model = model_;
  context_.is_eos_ = false;
  context_.session_id = ::ge::GetContext().SessionId();
  context_.ge_context = &GetThreadLocalContext();
  GELOGD("session id from model = %lu, from context = %lu", model_->GetSessionId(), context_.session_id);
  context_.allocator = NpuMemoryAllocator::GetAllocator(device_id_, stream_);
  GE_CHECK_NOTNULL(context_.allocator);
  if (callback_manager != nullptr) {
    context_.callback_manager = callback_manager;
  } else {
    if (context_.is_host_cpu) {
      context_.callback_manager = new (std::nothrow) HostCpuCallbackManager();
    } else {
      context_.callback_manager = new (std::nothrow) RtCallbackManager();
    }
    GE_CHECK_NOTNULL(context_.callback_manager);
    context_.own_callback_manager = true;
  }

  context_.dump_properties = DumpManager::GetInstance().GetDumpProperties(context_.session_id);

  GE_CHK_STATUS_RET_NOLOG(context_.InitProfiler());
  if (IsLogEnable(GE_MODULE_NAME, DLOG_DEBUG)) {
    context_.trace_enabled = true;
  }
  context_.has_observer = model_->HasObserver();
  GE_CHK_STATUS_RET_NOLOG(context_.res_manager.Init(model_->GetRootGraphItem()));

  return SUCCESS;
}

Status HybridModelExecutor::ResetExecutionContext(GraphExecutionContext &context) {
  GE_CHK_STATUS_RET_NOLOG(context.callback_manager->Init());

  context.runtime_context_.Release();
  for (auto &host_tensor : context.model->GetHostTensors()) {
    const auto node_id = host_tensor.first;
    for (const auto &output_idx_and_tensor : host_tensor.second) {
      const auto output_idx = output_idx_and_tensor.first;
      GELOGD("Preload const host tensor, node_id = %ld, output id = %d", node_id, output_idx);
      (void)context.runtime_context_.SetTensor(node_id, output_idx, output_idx_and_tensor.second);
    }
  }
  context.res_manager.ClearDataFlowResources();
  return SUCCESS;
}

Status HybridModelExecutor::CheckInputShapeByShapeRange(const GraphItem *const graph_item,
                                                        const HybridModelExecutor::ExecuteArgs &args) {
  GE_CHECK_NOTNULL(graph_item);
  const auto input_nodes = graph_item->GetInputNodes();
  for (size_t i = 0U; i < input_nodes.size(); ++i) {
    const auto &input_node = input_nodes[i];
    if (input_node == nullptr) {
      GELOGD("[%s] Input[%zu] is not needed by graph, skip it.", graph_item->GetName().c_str(), i);
      continue;
    }
    if (!input_node->is_dynamic) {
      GELOGD("[%s] Input[%zu] is not dynamic, skip it.", graph_item->GetName().c_str(), i);
      continue;
    }
    const GeTensorDescPtr model_input_desc = input_node->MutableInputDesc(0);
    GE_CHECK_NOTNULL(model_input_desc);
    std::vector<std::pair<int64_t, int64_t>> shape_range;
    if (model_input_desc->GetShapeRange(shape_range) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "[%s] Input[%zu] get shape range failed", graph_item->GetName().c_str(), i);
      GELOGE(INTERNAL_ERROR, "[Get][ShapeRange] [%s] Input[%zu] get shape range failed",
             graph_item->GetName().c_str(), i);
      return INTERNAL_ERROR;
    }
    if (shape_range.empty()) {
      GELOGD("[%s] Input[%zu] shape is not needed to check by shape range, skip it.", graph_item->GetName().c_str(), i);
      continue;
    }
    if (i >= args.input_desc.size()) {
      REPORT_INNER_ERROR("E19999", "[%s] Inputs[%zu] is greater than or equal to input desc size[%zu].",
                         graph_item->GetName().c_str(), i, args.input_desc.size());
      GELOGE(INTERNAL_ERROR, "[Check][Param] [%s] inputs[%zu] is greater than or equal to input desc size[%zu].",
             graph_item->GetName().c_str(), i, args.input_desc.size());
      return INTERNAL_ERROR;
    }
    const ConstGeTensorDescPtr args_tensor_desc = args.input_desc[i];
    GE_CHECK_NOTNULL(args_tensor_desc);
    const GeShape shape = args_tensor_desc->GetShape();
    if (shape.IsUnknownShape()) {
      REPORT_INNER_ERROR("E19999", "[%s] Input desc shape [%zu] designed by user must be static.",
                         graph_item->GetName().c_str(), i);
      GELOGE(INTERNAL_ERROR, "[Check][Param] [%s] Input desc shape [%zu] designed by user must be static.",
             graph_item->GetName().c_str(), i);
      return INTERNAL_ERROR;
    }

    if (TensorUtils::CheckShapeByShapeRange(shape, shape_range) != SUCCESS) {
      GELOGE(PARAM_INVALID, "[Check][InputShape] [%s] check input [%zu] shape failed by shape range.",
             graph_item->GetName().c_str(), i);
      return PARAM_INVALID;
    }
  }

  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
