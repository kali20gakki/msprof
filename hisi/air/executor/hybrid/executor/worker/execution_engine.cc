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

#include "hybrid/executor/worker/execution_engine.h"
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/executor/worker/shape_inference_engine.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/profiling/profiling_properties.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling_definitions.h"

namespace ge {
namespace hybrid {
namespace {
constexpr int64_t kMaxPaddings = 63;
constexpr uint32_t kInvalidModelIds = 0xFFFFFFFFU;

Status LogInputs(const NodeItem &node_item, const TaskContext &task_context) {
  for (auto i = 0; i < task_context.NumInputs(); ++i) {
    const auto &input_tensor = task_context.GetInput(i);
    GE_CHECK_NOTNULL(input_tensor);
    const auto &tensor_desc = task_context.GetInputDesc(i);
    GE_CHECK_NOTNULL(tensor_desc);
    GELOGD("[%s] Print task args. input[%d] = %s, shape = [%s]",
           node_item.NodeName().c_str(),
           i,
           input_tensor->DebugString().c_str(),
           tensor_desc->GetShape().ToString().c_str());
  }

  return SUCCESS;
}

Status LogOutputs(const NodeItem &node_item, const TaskContext &task_context) {
  for (auto i = 0; i < task_context.NumOutputs(); ++i) {
    const auto &output_tensor = task_context.GetOutput(i);
    GE_CHECK_NOTNULL(output_tensor);
    const auto &tensor_desc = node_item.MutableOutputDesc(i);
    GE_CHECK_NOTNULL(tensor_desc);
    GELOGD("[%s] Print task args. output[%d] = %s, shape = [%s]",
           node_item.NodeName().c_str(),
           i,
           output_tensor->DebugString().c_str(),
           tensor_desc->MutableShape().ToString().c_str());
  }

  return SUCCESS;
}
}  // namespace

static void ReportModelIdMapData(const uint32_t model_id, const uint64_t session_id, const uint32_t graph_id,
                                 MsprofGeProfIdMapData &id_map_data) {
  id_map_data.modelId = model_id;
  id_map_data.sessionId = static_cast<uint32_t>(session_id);
  id_map_data.graphId = graph_id;
  const mmTimespec time_spec = mmGetTickCount();
  id_map_data.timeStamp =
      (static_cast<uint64_t>(time_spec.tv_sec) * 1000U * 1000U * 1000U) + static_cast<uint64_t>(time_spec.tv_nsec);
  int32_t logic_device_id = 0;
  const rtError_t rt_ret = rtGetDevice(&logic_device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][LogicDeviceId]Failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Get logic device id failed, ret 0x%X", rt_ret);
    return;
  }
  const std::string tag_name = "id_map_info";
  auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.ReportData(logic_device_id, &id_map_data, sizeof(id_map_data), tag_name);
}

NodeDoneCallback::NodeDoneCallback(GraphExecutionContext *const graph_context,
                                   const std::shared_ptr<TaskContext> task_context)
    : graph_context_(graph_context), context_(task_context) {
}

Status NodeDoneCallback::PrepareConstInputs(const NodeItem &node_item) const {
  for (const auto output_idx : node_item.to_const_output_id_list) {
    RECORD_CALLBACK_EVENT(graph_context_, node_item.NodeName().c_str(),
                          "[PrepareConstInputs] [index = %d] Start",
                          output_idx);

    const auto output_tensor = context_->GetOutput(output_idx);
    GE_CHECK_NOTNULL(output_tensor);
    const auto ge_tensor_desc = node_item.MutableOutputDesc(output_idx);
    GE_CHECK_NOTNULL(ge_tensor_desc);
    GeTensorPtr ge_tensor = MakeShared<GeTensor>(*ge_tensor_desc);
    GE_CHECK_NOTNULL(ge_tensor);

    int64_t tensor_size;
    GE_CHK_GRAPH_STATUS_RET(TensorUtils::GetTensorSizeInBytes(*ge_tensor_desc, tensor_size),
                            "[Get][TensorSize] In Bytes failed");

    if (output_tensor->GetSize() < static_cast<size_t>(tensor_size)) {
      GELOGE(INTERNAL_ERROR,
          "[Check][Size] [%s(%s)] Tensor size is not enough. output index = %d, required size = %ld, tensor = %s.",
          node_item.NodeName().c_str(), node_item.NodeType().c_str(), output_idx, tensor_size,
          output_tensor->DebugString().c_str());
      REPORT_INNER_ERROR("E19999",
                         "[%s(%s)] Tensor size is not enough. output index = %d, required size = %ld, tensor = %s.",
                         node_item.NodeName().c_str(), node_item.NodeType().c_str(), output_idx, tensor_size,
                         output_tensor->DebugString().c_str());
      return INTERNAL_ERROR;
    }

    std::vector<uint8_t> host_buffer(static_cast<size_t>(tensor_size));
    GELOGD("[%s] To cache output[%d] to host, size = %zu",
           node_item.NodeName().c_str(),
           output_idx,
           output_tensor->GetSize());
    if (tensor_size > 0) {
      GE_CHK_RT_RET(rtMemcpy(host_buffer.data(),
                             static_cast<uint64_t>(tensor_size),
                             output_tensor->GetData(),
                             static_cast<uint64_t>(tensor_size),
                             RT_MEMCPY_DEVICE_TO_HOST));
    }
    (void)ge_tensor->SetData(std::move(host_buffer));
    GE_CHK_STATUS_RET(graph_context_->runtime_context_.SetTensor(node_item.node_id, output_idx, std::move(ge_tensor)),
                      "[Set][Tensor] Failed, node = %s(%s), output_index = %d",
                      node_item.NodeName().c_str(), node_item.NodeType().c_str(), output_idx);
    GELOGD("[%s] Output[%d] cached successfully. node_id = %ld, shape = [%s]",
           node_item.NodeName().c_str(), output_idx, node_item.node_id, ge_tensor_desc->GetShape().ToString().c_str());

    RECORD_CALLBACK_EVENT(graph_context_, node_item.NodeName().c_str(),
                          "[PrepareConstInputs] [index = %d] End", output_idx);
  }

  return SUCCESS;
}

Status NodeDoneCallback::GetTaskDescInfo(TaskContext &context, const NodePtr node,
                                         std::vector<TaskDescInfo> &task_desc_info) {
  GE_CHECK_NOTNULL(node);

  // only report aicpu and aicore node
  const bool is_profiling_report = context.GetNodeItem().is_profiling_report;
  if (!is_profiling_report) {
    GELOGD("Node[%s] is not aicore or aicpu, and no need to report data.", node->GetName().c_str());
    return SUCCESS;
  }

  GELOGD("GetTaskDescInfo of node [%s] start.", node->GetName().c_str());
  const auto &prof_mgr = ProfilingManager::Instance();
  task_desc_info = context.GetProfilingTaskDescInfo();
  context.ClearProfilingTaskDescInfo();
  for (auto &tmp_task_desc : task_desc_info) {
    // save op input and output info
    const auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    prof_mgr.GetOpInputOutputInfo(op_desc, tmp_task_desc);
  }

  return SUCCESS;
}

Status NodeDoneCallback::ReportTaskDetailInfo(TaskContext &context,
                                              const GraphExecutionContext *const graph_context) {
  const auto node = context.GetNodeItem().node;
  if (node == nullptr) {
    GELOGE(PARAM_INVALID, "[Get][Node] value is nullptr.");
    REPORT_INNER_ERROR("E19999", "TaskContext GetNodeItem value is nullptr.");
    return PARAM_INVALID;
  }

  const auto &op_type = node->GetType();
  if (op_type == PARTITIONEDCALL) {
    return SUCCESS;
  }

  GE_CHECK_NOTNULL(graph_context);
  const HybridModel *const model = graph_context->model;
  GE_CHECK_NOTNULL(model);

  GELOGD("ProfilingReport of node [%s] model [%s] start.", node->GetName().c_str(), model->GetModelName().c_str());
  std::vector<TaskDescInfo> task_desc_info;
  const auto profiling_ret = GetTaskDescInfo(context, node, task_desc_info);
  if (profiling_ret != static_cast<uint32_t>(RT_ERROR_NONE)) {
    GELOGE(profiling_ret, "[Get][TaskDescInfo] of node:%s(%s) failed.",
           node->GetName().c_str(), node->GetType().c_str());
    REPORT_CALL_ERROR("E19999", "GetTaskDescInfo of node:%s(%s) failed.",
                      node->GetName().c_str(), node->GetType().c_str());
    return profiling_ret;
  }
  const uint32_t model_id = (model->IsSingleOp()) ? kInvalidModelIds : model->GetModelId();
  GELOGD("The model is online:%d, model id is %u",
         static_cast<int32_t>(domi::GetContext().is_online_model), model_id);
  auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.ReportProfilingData(model_id, task_desc_info);
  if (domi::GetContext().is_online_model) {
    // only online model need to report the map of graph id and model id
    MsprofGeProfIdMapData id_map_data{};
    const uint32_t graph_id = model->GetRootGraph()->GetGraphID();
    const uint64_t session_id = graph_context->session_id;
    ReportModelIdMapData(model_id, session_id, graph_id, id_map_data);
  }
  return SUCCESS;
}

Status NodeDoneCallback::DumpDynamicNode() {
  const auto &node = context_->GetNodeItem().node;
  if (node == nullptr) {
    GELOGE(PARAM_INVALID, "[Get][Node] value is nullptr.");
    REPORT_INNER_ERROR("E19999", "get node value is nullptr.");
    return PARAM_INVALID;
  }
  const auto &op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(graph_context_);
  const HybridModel *const model = graph_context_->model;
  GE_CHECK_NOTNULL(model);
  const std::string dynamic_model_name = model->GetModelName();
  const std::string dynamic_om_name = model->GetOmName();
  const uint32_t model_id = model->GetModelId();
  if (!context_->GetDumpProperties().IsLayerNeedDump(dynamic_model_name, dynamic_om_name, op_desc->GetName())) {
    GELOGI("[%s] is not in dump list, no need dump", op_desc->GetName().c_str());
    return SUCCESS;
  }
  dump_op_.SetDynamicModelInfo(dynamic_model_name, dynamic_om_name, model_id);

  const auto stream = context_->GetStream();
  std::vector<uintptr_t> input_addrs;
  std::vector<uintptr_t> output_addrs;
  for (int32_t i = 0; i < context_->NumInputs(); ++i) {
    const auto *const tensor_value = context_->GetInput(i);
    GE_CHK_BOOL_RET_STATUS(tensor_value != nullptr, PARAM_INVALID, "[Get][Tensor] value is nullptr.");
    input_addrs.emplace_back(PtrToValue(tensor_value->GetData()));
  }
  for (int32_t i = 0; i < context_->NumOutputs(); ++i) {
    const auto *const tensor_value = context_->GetOutput(i);
    GE_CHK_BOOL_RET_STATUS(tensor_value != nullptr, PARAM_INVALID, "[Get][Tensor] value is nullptr.");
    output_addrs.emplace_back(PtrToValue(tensor_value->GetData()));
  }

  dump_op_.SetDumpInfo(context_->GetDumpProperties(), op_desc, input_addrs, output_addrs, stream);

  const TensorValue *const variable_loop_iter = context_->GetVariable(NODE_NAME_FLOWCTRL_LOOP_PER_ITER);
  const uintptr_t loop_iter = (variable_loop_iter != nullptr) ? PtrToValue(variable_loop_iter->GetData()) : 0U;

  const TensorValue *const variable_loop_cond = context_->GetVariable(NODE_NAME_FLOWCTRL_LOOP_COND);
  const uintptr_t loop_cond = (variable_loop_cond != nullptr) ? PtrToValue(variable_loop_cond->GetData()) : 0U;

  dump_op_.SetLoopAddr(PtrToValue(context_->GetExecutionContext()->global_step), loop_iter, loop_cond);

  GE_CHK_STATUS_RET(dump_op_.LaunchDumpOp(), "[Launch][DumpOp] failed in hybird model.");

  const auto rt_ret = rtStreamSynchronize(stream);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Call][RtStreamSynchronize] failed, ret = %d.", rt_ret);
    REPORT_CALL_ERROR("E19999", "call rtStreamSynchronize failed, ret = %d.", rt_ret);
    return static_cast<uint32_t>(rt_ret);
  }
  return SUCCESS;
}

Status NodeDoneCallback::SaveDumpOpInfo() {
  GE_CHECK_NOTNULL(graph_context_);
  GE_CHECK_NOTNULL(graph_context_->model);

  const auto node = context_->GetNodeItem().node;
  if (node == nullptr) {
    GELOGE(PARAM_INVALID, "[Save][DumpOpInfo] Get node is nullptr.");
    return PARAM_INVALID;
  }
  const auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);

  ExtraOpInfo extra_op_info = context_->MutableExtraOpInfo();
  for (int32_t i = 0; i < context_->NumInputs(); ++i) {
    const auto tensor_value = context_->MutableInput(i);
    GE_CHK_BOOL_RET_STATUS(tensor_value != nullptr, PARAM_INVALID, "[Save][DumpOpInfo] Tensor value is nullptr.");
    extra_op_info.input_addrs.emplace_back(tensor_value->MutableData());
  }
  for (int32_t i = 0; i < context_->NumOutputs(); ++i) {
    const auto tensor_value = context_->MutableOutput(i);
    GE_CHK_BOOL_RET_STATUS(tensor_value != nullptr, PARAM_INVALID, "[Save][DumpOpInfo] Tensor value is nullptr.");
    extra_op_info.output_addrs.emplace_back(tensor_value->MutableData());
  }

  const uint32_t stream_id = context_->GetStreamId();
  const uint32_t task_id = context_->GetTaskId();
  graph_context_->exception_dumper.SaveDumpOpInfo(op_desc, task_id, stream_id, extra_op_info);
  return SUCCESS;
}

Status NodeDoneCallback::OnNodeDone() {
  auto &node_item = context_->GetNodeItem();
  GELOGI("[%s] Start callback process.", node_item.NodeName().c_str());
  RECORD_CALLBACK_EVENT(graph_context_, context_->GetNodeName(), "[Compute] End");
  RECORD_CALLBACK_EVENT(graph_context_, context_->GetNodeName(), "[Callback] Start");

  const DumpProperties &dump_properties = context_->GetDumpProperties();
  if (dump_properties.IsDumpOpen() || context_->IsOverFlow()) {
    GELOGI("Start to dump dynamic shape op");
    GE_CHK_STATUS_RET(DumpDynamicNode(), "[Call][DumpDynamicNode] Failed.");
  }

  if (ModelManager::GetInstance().IsDumpExceptionOpen()) {
    GE_CHK_STATUS_RET(SaveDumpOpInfo(), "[Save][DumpOpInfo] Failed to dump op info.");
  }

  if (ProfilingManager::Instance().ProfilingModelLoadOn() || ProfilingManager::Instance().ProfilingSubscribeOn()) {
    GE_CHK_STATUS_RET(ReportTaskDetailInfo(*(context_.get()), graph_context_),
                      "[Report][Profiling] of node[%s(%s)] failed.",
                      node_item.NodeName().c_str(), node_item.NodeType().c_str());
  }

  // release workspace
  context_->ReleaseWorkspace();
  // release inputs
  for (int32_t i = 0; i < context_->NumInputs(); ++i) {
    context_->ReleaseInput(i);
  }

  GE_CHK_STATUS_RET_NOLOG(PrepareConstInputs(node_item));
  GE_CHK_STATUS_RET_NOLOG(node_item.OnNodeDone());
  // PropagateOutputs for type == DEPEND_COMPUTE
  if (node_item.shape_inference_type == DEPEND_COMPUTE) {
    if (graph_context_->trace_enabled) {
      (void)LogOutputs(node_item, *context_);
    }

    GE_CHK_STATUS_RET(context_->PropagateOutputs(), "[Propagate][Outputs] of [%s(%s)] failed.",
                      node_item.NodeName().c_str(), node_item.NodeType().c_str());

    RECORD_CALLBACK_EVENT(graph_context_, context_->GetNodeName(), "[PropagateOutputs] End");
  }

  // release condition variable
  if (node_item.has_observer) {
    GELOGI("[%s] Notify observer. node_id = %ld", node_item.NodeName().c_str(), node_item.node_id);
    context_->NodeDone();
  }

  RECORD_CALLBACK_EVENT(graph_context_, context_->GetNodeName(), "[Callback] End");
  return SUCCESS;
}

Status ExecutionEngine::ExecuteAsync(const NodeState &node_state,
                                     const std::shared_ptr<TaskContext> &task_context,
                                     GraphExecutionContext &execution_context,
                                     const std::function<void()> &callback) {
  GELOGI("[%s] Node is ready for execution", task_context->GetNodeName());
  RECORD_EXECUTION_EVENT(&execution_context, task_context->GetNodeName(), "Start");
  GE_CHK_STATUS_RET_NOLOG(DoExecuteAsync(node_state, *task_context, execution_context, callback));
  GE_CHK_STATUS_RET_NOLOG(PropagateOutputs(node_state.GetNodeItem(), *task_context, execution_context));
  return SUCCESS;
}

Status ExecutionEngine::DoExecuteAsync(const NodeState &node_state,
                                       TaskContext &task_context,
                                       GraphExecutionContext &context,
                                       const std::function<void()> &callback) {
  const auto &task = node_state.GetKernelTask();
  if (task == nullptr) {
    GELOGE(INTERNAL_ERROR, "[Get][KernelTask] of [%s(%s)] is null.",
           node_state.GetName().c_str(), node_state.GetType().c_str());
    REPORT_INNER_ERROR("E19999", "GetKernelTask of %s(%s) failed.",
                       node_state.GetName().c_str(), node_state.GetType().c_str());
    return INTERNAL_ERROR;
  }

  // Wait for dependent nodes(DEPEND_COMPUTE), so that the input tensors are valid.
  RECORD_EXECUTION_EVENT(&context, task_context.GetNodeName(), "[AwaitDependents] Start");
  HYBRID_CHK_STATUS_RET(node_state.AwaitInputTensors(context),
                        "[Call][AwaitInputTensors] [%s(%s)] Failed to wait for dependent nodes.",
                        node_state.GetName().c_str(), node_state.GetType().c_str());

  const auto &node_item = node_state.GetNodeItem();
  const auto executor = node_item.node_executor;
  GE_CHECK_NOTNULL(executor);
  PROFILING_START(node_state.GetProfilingIndex(), profiling::kPrepareTask);
  RECORD_SHELL_PROFILING_START(kPrepareTask);
  node_state.UpdatePersistTensor();
  GE_CHK_STATUS_RET(executor->PrepareTask(*task, task_context), "[Prepare][Task] for [%s(%s)] failed.",
                    node_state.GetName().c_str(), node_state.GetType().c_str());
  RECORD_SHELL_PROFILING_END(profiling::kPrepareTask, node_state.GetProfilingIndex(), kPrepareTask);
  PROFILING_END(node_state.GetProfilingIndex(), profiling::kPrepareTask);
  GELOGD("[%s] Done task preparation successfully.", node_state.GetName().c_str());

  if (context.trace_enabled) {
    (void)LogInputs(node_item, task_context);
    if (node_item.shape_inference_type != DEPEND_COMPUTE) {
      (void)LogOutputs(node_item, task_context);
    }
  }

  PROFILING_START(node_state.GetProfilingIndex(), profiling::kValidateInputTensor);
  GE_CHK_STATUS_RET(ValidateInputTensors(node_state, task_context), "[Validate][InputTensors] for %s(%s) failed.",
                    node_state.GetName().c_str(), node_state.GetType().c_str());
  PROFILING_END(node_state.GetProfilingIndex(), profiling::kValidateInputTensor);
  RECORD_EXECUTION_EVENT(&context, task_context.GetNodeName(), "[ValidateInputTensors] End");

  PROFILING_START(node_state.GetProfilingIndex(), profiling::kLaunchTask);
  RECORD_SHELL_PROFILING_START(kLaunchTask);
  HYBRID_CHK_STATUS_RET(node_item.node_executor->ExecuteTask(*task, task_context, callback),
                        "[Call][ExecuteTask] [%s(%s)] Failed to execute task",
                        node_state.GetName().c_str(), node_state.GetType().c_str());
  RECORD_SHELL_PROFILING_END(profiling::kLaunchTask, node_state.GetProfilingIndex(), kLaunchTask);
  REPORT_TASK_PROFILING_DETAIL(task_context, &context);
  PROFILING_END(node_state.GetProfilingIndex(), profiling::kLaunchTask);

  GELOGD("[%s] Done task launch successfully.", node_state.GetName().c_str());
  return SUCCESS;
}

Status ExecutionEngine::ValidateInputTensors(const NodeState &node_state, const TaskContext &task_context) {
  (void)node_state;
  for (auto i = 0; i < task_context.NumInputs(); ++i) {
    if (task_context.SkipSufficiencyOfInputCheck(i)) {
      GELOGD("[%s] Skipping input which no need to check, index:%d.", task_context.GetNodeName(), i);
      continue;
    }
    const auto &input_tensor = task_context.GetInput(i);
    GE_CHECK_NOTNULL(input_tensor);
    if (input_tensor->GetData() == nullptr) {
      GELOGD("[%s] Skipping null input, index = %d", task_context.GetNodeName(), i);
      continue;
    }

    const auto &tensor_desc = task_context.MutableInputDesc(i);
    GE_CHECK_NOTNULL(tensor_desc);
    if (tensor_desc->GetDataType() == DT_STRING) {
      GELOGD("[%s] Skipping DT_STRING input, index = %d", task_context.GetNodeName(), i);
      continue;
    }

    if (input_tensor->GetData() == nullptr) {
      GELOGD("[%s] Skipping null input, index = %d", task_context.GetNodeName(), i);
      continue;
    }

    int64_t expected_size = 0;
    (void)TensorUtils::GetSize(*tensor_desc, expected_size);
    GELOGD("[%s] Input[%d] expects [%ld] bytes.", task_context.GetNodeName(), i, expected_size);
    const auto size_diff = expected_size - static_cast<int64_t>(input_tensor->GetSize());
    if (size_diff > 0) {
      if (size_diff <= kMaxPaddings) {
        GELOGW("[%s] Input[%d]: tensor size mismatches. expected: %ld, but given %zu",
               task_context.GetNodeName(),
               i,
               expected_size,
               input_tensor->GetSize());
      } else {
        GELOGE(INTERNAL_ERROR,
               "[Check][Size] for [%s(%s)] Input[%d]: tensor size mismatches. expected: %ld, but given %zu.",
               task_context.GetNodeName(), task_context.GetNodeItem().NodeType().c_str(),
               i, expected_size, input_tensor->GetSize());
        REPORT_INNER_ERROR("E19999", "[%s(%s)] Input[%d]: tensor size mismatches. expected: %ld, but given %zu.",
                           task_context.GetNodeName(), task_context.GetNodeItem().NodeType().c_str(),
                           i, expected_size, input_tensor->GetSize());
        return INTERNAL_ERROR;
      }
    }
  }

  return SUCCESS;
}

Status ExecutionEngine::PropagateOutputs(const NodeItem &node_item,
                                         const TaskContext &task_context,
                                         const GraphExecutionContext &context) {
  if (node_item.shape_inference_type != DEPEND_COMPUTE) {
    PROFILING_START(-1, profiling::kPropgateOutputs);
    GE_CHK_STATUS_RET(task_context.PropagateOutputs(), "[Propagate][Outputs] for [%s(%s)] failed.",
                      node_item.NodeName().c_str(), node_item.NodeType().c_str());
    PROFILING_END(-1, profiling::kPropgateOutputs);
    RECORD_EXECUTION_EVENT(&context, task_context.GetNodeName(), "[PropagateOutputs] End");
    GELOGD("[%s(%s)] Done propagating outputs successfully.", node_item.NodeName().c_str(),
           node_item.NodeType().c_str());
  }

  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
