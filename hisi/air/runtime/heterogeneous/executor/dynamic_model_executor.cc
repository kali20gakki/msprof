/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "executor/dynamic_model_executor.h"
#include <future>
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/ge_context.h"
#include "common/utils/rts_api_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "aicpu/aicpu_schedule/aicpusd_interface.h"
#include "executor/cpu_sched_event_dispatcher.h"

#ifdef ONLY_COMPILE_OPEN_SRC
int32_t AicpuLoadModelWithQ(void *ptr) {
  (void) ptr;
  return 0;
}

int32_t AICPUModelDestroy(uint32_t modelId) {
  (void) modelId;
  return 0;
}
int32_t InitCpuScheduler(const CpuSchedInitParam *const initParam) {
  (void) initParam;
  return 0;
}
#endif

namespace ge {
DynamicModelExecutor::DynamicModelExecutor(bool is_host) : is_host_(is_host) {
}

DynamicModelExecutor::~DynamicModelExecutor() {
  CpuSchedEventDispatcher::GetInstance().Deregister(model_id_);
  if (rt_context_ != nullptr) {
    (void) rtCtxDestroy(rt_context_);
  }
}

Status DynamicModelExecutor::LoadModelWithQueue(const GeRootModelPtr &root_model,
                                                const vector<uint32_t> &input_queues,
                                                const vector<uint32_t> &output_queues) {
  GE_CHK_RT_RET(rtCtxCreate(&rt_context_, RT_CTX_NORMAL_MODE, device_id_));
  if (!GetContext().GetHostExecFlag()) {
    GE_CHK_RT_RET(rtStreamCreate(&stream_, 0));
  }
  num_inputs_ = input_queues.size();
  num_outputs_ = output_queues.size();
  input_queue_ids_ = input_queues;
  output_queue_ids_ = output_queues;
  input_mbuf_addresses_.resize(num_inputs_);
  output_mbuf_addresses_.resize(num_outputs_);
  output_runtime_tensor_descs_.resize(num_outputs_);
  GE_CHK_STATUS_RET_NOLOG(ParseModelDesc(root_model));
  GE_CHK_STATUS_RET_NOLOG(DoLoadModel(root_model));
  CpuSchedEventDispatcher::GetInstance().Register(model_id_, this);
  // load with aicpu-sd
  GE_CHK_STATUS_RET_NOLOG(LoadWithAiCpuSd());
  run_thread_ = std::thread([&]() {
    Run();
  });
  return SUCCESS;
}

void DynamicModelExecutor::UnloadModel() {
  Stop();
  (void) AICPUModelDestroy(model_id_);
  (void) ge_executor_.UnloadModel(model_id_);
}

Status DynamicModelExecutor::ExecuteAsync(const std::function<void(Status)> &callback) {
  GE_CHK_BOOL_RET_STATUS(task_queue_.Push(callback), FAILED, "Failed to enqueue task, model_id = %u", model_id_);
  GELOGD("Enqueue task successfully, model_id = %u", model_id_);
  return SUCCESS;
}

Status DynamicModelExecutor::ExecuteInternal() {
  GELOGD("Execute model started, model_id = %u", model_id_);
  // prepare inputs
  RunModelData model_inputs;
  std::vector<GeTensor> input_tensors;
  RunModelData model_outputs;
  GE_CHK_STATUS_RET_NOLOG(PrepareInputs(model_inputs));
  GELOGD("Inputs prepared successfully, model_id = %u", model_id_);
  GE_CHK_STATUS_RET_NOLOG(PrepareOutputs(model_outputs));
  GELOGD("Output buffers prepared successfully, model_id = %u", model_id_);
  GE_CHK_STATUS_RET(DoExecuteModel(model_inputs, model_outputs), "Failed to execute model");
  GELOGD("Model executed successfully, model_id = %u", model_id_);
  GE_CHK_STATUS_RET_NOLOG(UpdateOutputs(model_outputs));
  GELOGD("Outputs post processes done successfully, model_id = %u", model_id_);
  return SUCCESS;
}

Status DynamicModelExecutor::PrepareInputs(RunModelData &model_inputs) {
  for (size_t i = 0U; i < num_inputs_; ++i) {
    void *m_buf = input_mbuf_addresses_[i];
    void *buffer_data = nullptr;
    uint64_t buffer_size = 0;
    GE_CHK_STATUS_RET(RtsApiUtils::MbufGetBufferAddr(m_buf, &buffer_data));
    GE_CHK_STATUS_RET(RtsApiUtils::MbufGetBufferSize(m_buf, buffer_size));
    GELOGD("Inputs[%zu], buffer address = %p, buffer size = %zu", i, buffer_data, buffer_size);
    DataBuffer data_buffer;
    if (is_input_dynamic_[i]) {
      GE_CHECK_GE(buffer_size, sizeof(RuntimeTensorDesc));
      auto &tensor_desc = input_tensor_descs_[i];
      auto *runtime_tensor_desc = reinterpret_cast<const RuntimeTensorDesc *>(buffer_data);
      GE_CHK_STATUS_RET(UpdateTensorDesc(*runtime_tensor_desc, tensor_desc),
                        "Failed to update tensor desc, input index = %zu", i);
      GELOGD("Inputs[%zu] is dynamic, shape = [%s], original shape = [%s]",
             i,
             tensor_desc.GetShape().ToString().c_str(),
             tensor_desc.GetOriginShape().ToString().c_str());
      data_buffer.data = static_cast<uint8_t *>(buffer_data) + sizeof(RuntimeTensorDesc);
      data_buffer.length = buffer_size - sizeof(RuntimeTensorDesc);
    } else {
      data_buffer.data = buffer_data;
      data_buffer.length = buffer_size;
    }
    data_buffer.placement = is_host_ ? kPlacementHost : kPlacementDevice;
    model_inputs.blobs.emplace_back(data_buffer);
  }
  return SUCCESS;
}

Status DynamicModelExecutor::ParseModelDesc(const GeRootModelPtr &root_model) {
  GE_CHECK_NOTNULL(root_model);
  const auto &root_graph = root_model->GetRootGraph();
  input_tensor_descs_.resize(num_inputs_);
  is_input_dynamic_.resize(num_inputs_);
  std::map<int64_t, std::string> data_indices;
  for (const auto &node : root_graph->GetDirectNode()) {
    if (node->GetType() == DATA) {
      int64_t index = -1;
      GE_CHK_BOOL_RET_STATUS(AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_INDEX, index),
                             PARAM_INVALID,
                             "Failed to get attribute \"index\" from data node: %s", node->GetName().c_str());
      GE_CHK_BOOL_RET_STATUS(data_indices[index].empty(),
                             PARAM_INVALID,
                             "Duplicated data index [%ld], node name = %s, prev node name = %s",
                             index, node->GetName().c_str(), data_indices[index].c_str());
      data_indices[index] = node->GetName();
      GE_CHK_BOOL_RET_STATUS(static_cast<size_t>(index) < is_input_dynamic_.size(),
                             PARAM_INVALID,
                             "Data index of node %s out of range, index = %ld num_inputs = %zu",
                             node->GetName().c_str(), index, is_input_dynamic_.size());
      const auto &tensor_desc = node->GetOpDesc()->MutableOutputDesc(0U);
      GE_CHECK_NOTNULL(tensor_desc);
      input_tensor_descs_[index] = *tensor_desc;
      is_input_dynamic_[index] = tensor_desc->MutableShape().IsUnknownShape();
    } else if (node->GetType() == NETOUTPUT) {
      size_t output_index = 0;
      for (const auto &tensor_desc : node->GetOpDesc()->GetAllInputsDescPtr()) {
        is_output_dynamic_.emplace_back(tensor_desc->GetShape().IsUnknownShape());
        int64_t tensor_size = -1;
        GE_CHK_STATUS_RET_NOLOG(GetOutputTensorSize(*tensor_desc, tensor_size));
        output_tensor_sizes_.emplace_back(tensor_size);
        GELOGD("Output[%zu], shape = [%s], size = %ld",
               output_index,
               tensor_desc->GetShape().ToString().c_str(),
               tensor_size);
      }
    } else {
      // skip other nodes
    }
  }
  return SUCCESS;
}

Status DynamicModelExecutor::UpdateTensorDesc(const RuntimeTensorDesc &runtime_tensor_desc,
                                              GeTensorDesc &tensor_desc) {
  auto num_dims = runtime_tensor_desc.shape[0];
  auto num_ori_dims = runtime_tensor_desc.original_shape[0];
  GE_CHK_BOOL_RET_STATUS(num_dims <= kMaxDimSize,
                         UNSUPPORTED,
                         "shape dim number out of range, num_dims = %ld, max = %lu",
                         num_dims, kMaxDimSize);
  GE_CHK_BOOL_RET_STATUS(num_ori_dims <= kMaxDimSize,
                         UNSUPPORTED,
                         "original shape dim number out of range, num_dims = %ld, max = %lu",
                         num_dims, kMaxDimSize);
  GeShape shape(std::vector<int64_t>(&runtime_tensor_desc.shape[1], &runtime_tensor_desc.shape[1 + num_dims]));
  GeShape ori_shape(std::vector<int64_t>(&runtime_tensor_desc.shape[1], &runtime_tensor_desc.shape[1 + num_dims]));
  tensor_desc.MutableShape() = std::move(shape);
  tensor_desc.MutableShape() = std::move(ori_shape);
  return SUCCESS;
}

Status DynamicModelExecutor::UpdateRuntimeTensorDesc(const GeTensorDesc &tensor_desc,
                                                     RuntimeTensorDesc &runtime_tensor_desc) {
  GE_CHK_STATUS_RET_NOLOG(UpdateRuntimeShape(tensor_desc.GetShape(), runtime_tensor_desc.shape));
  GE_CHK_STATUS_RET_NOLOG(UpdateRuntimeShape(tensor_desc.GetOriginShape(), runtime_tensor_desc.original_shape));
  return SUCCESS;
}

Status DynamicModelExecutor::UpdateRuntimeShape(const GeShape &shape, int64_t (&shape_buffer)[33]) {
  auto num_dims = static_cast<int64_t>(shape.GetDimNum());
  GE_CHK_BOOL_RET_STATUS(num_dims <= kMaxDimSize,
                         UNSUPPORTED,
                         "shape dim number out of range, num_dims = %ld, max = %zu",
                         num_dims, kMaxDimSize);
  GE_CHECK_LE(num_dims, kMaxDimSize);
  shape_buffer[0] = num_dims;
  for (size_t i = 0; i < shape.GetDimNum(); ++i) {
    shape_buffer[1 + i] = shape.GetDim(i);
  }
  return SUCCESS;
}

Status DynamicModelExecutor::PrepareOutputs(RunModelData &model_outputs) {
  for (size_t i = 0; i < num_outputs_; ++i) {
    auto tensor_size = output_tensor_sizes_[i];
    if (tensor_size < 0) { // no valid range
      GELOGD("Output[%zu] is dynamic and cannot get a valid size by range", i);
      output_mbuf_addresses_[i] = nullptr;
      model_outputs.blobs.emplace_back(DataBuffer{});
      continue;
    }

    DataBuffer data_buffer;
    data_buffer.length = tensor_size;
    uint64_t buffer_size = is_output_dynamic_[i] ? tensor_size + sizeof(RuntimeTensorDesc) : tensor_size;
    GE_CHK_RT_RET(rtMbufAlloc(&output_mbuf_addresses_[i], buffer_size));
    GE_CHK_STATUS_RET(RtsApiUtils::MbufGetBufferAddr(output_mbuf_addresses_[i], &data_buffer.data));
    if (is_output_dynamic_[i]) {
      data_buffer.data = static_cast<uint8_t *>(data_buffer.data) + sizeof(RuntimeTensorDesc);
    }
    GELOGD("Output[%zu] is dynamic = %d, buffer addr = %p, buffer size = %zu",
           i,
           static_cast<int32_t>(is_output_dynamic_[i]),
           data_buffer.data,
           data_buffer.length);
    model_outputs.blobs.emplace_back(data_buffer);
  }
  return SUCCESS;
}

Status DynamicModelExecutor::GetOutputTensorSize(const GeTensorDesc &tensor_desc, int64_t &tensor_size) {
  if (tensor_desc.GetShape().IsUnknownShape()) {
    std::vector<std::pair<int64_t, int64_t>> shape_range;
    (void) tensor_desc.GetShapeRange(shape_range);
    if (shape_range.empty()) {
      GELOGD("dynamic shape tensor without range. shape = [%s]", tensor_desc.GetShape().ToString().c_str());
      tensor_size = -1;
      return SUCCESS;
    }
  }

  GE_CHK_STATUS_RET(TensorUtils::CalcTensorMemSizeForNoTiling(tensor_desc,
                                                              tensor_desc.GetFormat(),
                                                              tensor_desc.GetDataType(),
                                                              tensor_size),
                    "Failed to calc output size, shape = [%s]",
                    tensor_desc.GetShape().ToString().c_str());
  return SUCCESS;
}

Status DynamicModelExecutor::UpdateOutputs(RunModelData &model_outputs) {
  for (size_t i = 0; i < num_outputs_; ++i) {
    if (!is_output_dynamic_[i]) {
      GELOGD("output[%zu] is static", i);  // mbuf is always allocated if is static
      continue;
    }

    auto &tensor_desc = output_tensor_descs_[i];
    void *buffer_addr = nullptr;
    if (output_mbuf_addresses_[i] == nullptr) {
      auto &data_buffer = model_outputs.blobs[i];
      auto buffer_size = sizeof(RuntimeTensorDesc) + data_buffer.length;
      GE_CHK_RT_RET(rtMbufAlloc(&output_mbuf_addresses_[i], buffer_size));
      GE_CHK_STATUS_RET(RtsApiUtils::MbufGetBufferAddr(output_mbuf_addresses_[i], &buffer_addr));
      GELOGD("output[%zu] was allocated by executor, Mbuf allocated, size = %zu, addr = %p",
             i,
             buffer_size,
             output_mbuf_addresses_[i]);
      GE_CHK_BOOL_RET_STATUS(memcpy_s(static_cast<uint8_t *>(buffer_addr) + sizeof(RuntimeTensorDesc),
                                      data_buffer.length,
                                      data_buffer.data,
                                      data_buffer.length) == EOK,
                             FAILED,
                             "Failed to copy output[%zu]", i);
      GELOGD("Copy output[%zu] succeeded, size = %zu", i, data_buffer.length);
    } else {
      GE_CHK_STATUS_RET(RtsApiUtils::MbufGetBufferAddr(output_mbuf_addresses_[i], &buffer_addr));
    }

    GE_CHK_STATUS_RET_NOLOG(UpdateRuntimeTensorDesc(tensor_desc, output_runtime_tensor_descs_[i]));
    GE_CHK_BOOL_RET_STATUS(memcpy_s(buffer_addr,
                                    sizeof(RuntimeTensorDesc),
                                    &output_runtime_tensor_descs_[i],
                                    sizeof(output_runtime_tensor_descs_[i])) == EOK,
                           FAILED,
                           "Failed to copy runtime tensor desc");
    GELOGD("Output[%zu] is dynamic, shape = [%s], original shape = [%s]",
           i,
           tensor_desc.GetShape().ToString().c_str(),
           tensor_desc.GetOriginShape().ToString().c_str());
  }
  return SUCCESS;
}
Status DynamicModelExecutor::LoadWithAiCpuSd() {
  CpuSchedModelBuilder builder(model_);
  builder.SetModelId(model_id_);
  for (size_t i = 0; i < num_inputs_; ++i) {
    auto queue_id = input_queue_ids_[i];
    auto mbuf_addr = reinterpret_cast<uintptr_t>(&input_mbuf_addresses_[i]);
    builder.AddInputQueue(queue_id, mbuf_addr);
    GELOGD("Input[%zu], queue_id = %u, mbuf_addr = %p", i, queue_id, &input_mbuf_addresses_[i]);
  }

  for (size_t i = 0; i < num_outputs_; ++i) {
    auto queue_id = output_queue_ids_[i];
    auto mbuf_addr = reinterpret_cast<uintptr_t>(&output_mbuf_addresses_[i]);
    builder.AddOutputQueue(queue_id, mbuf_addr);
    GELOGD("Output[%zu], queue_id = %u, mbuf_addr = %p", i, queue_id, &output_mbuf_addresses_[i]);
  }

  builder.Build();
  model_.LogModelDesc();
  int32_t ret = AicpuLoadModelWithQ(&model_.model_info_);
  if (ret != 0) {
    GELOGE(FAILED, "Failed to invoke AicpuLoadModelWithQ, ret = %d", ret);
    return FAILED;
  }
  return SUCCESS;
}
void DynamicModelExecutor::Run() {
  GELOGD("Run thread started, model_id = %u", model_id_);
  rtCtxSetCurrent(rt_context_);
  while (true) {
    std::function<void(Status)> callback;
    task_queue_.Pop(callback);
    if (callback == nullptr) {
      GELOGI("Got EOF, model_id = %u", model_id_);
      break;
    }

    GELOGD("Start to execute model, model_id = %u", model_id_);
    auto ret = ExecuteInternal();
    if (ret == SUCCESS) {
      GELOGD("Execute model successfully, model_id = %u", model_id_);
    } else {
      GELOGE(ret, "Failed to execute model, model_id = %u", model_id_);
    }
    callback(ret);
    GELOGD("callback finished");
  }
  GELOGD("Run thread exit");
}

void DynamicModelExecutor::Stop() {
  task_queue_.Push(nullptr);
  if (run_thread_.joinable()) {
    run_thread_.join();
  }
}

Status DynamicModelExecutor::DoLoadModel(const shared_ptr<GeRootModel> &root_model) {
  auto graph_node = MakeShared<GraphNode>(0U);
  GE_CHECK_NOTNULL(graph_node);
  graph_node->SetAsync(false);
  // Will be changed to flow model api, use inner api(ModelManager) for now
  GE_CHK_STATUS_RET(ModelManager::GetInstance().LoadModelOnline(model_id_, root_model, graph_node, device_id_, 0),
                    "Failed to load model");
  return SUCCESS;
}

Status DynamicModelExecutor::DoExecuteModel(const RunModelData &inputs, RunModelData &outputs) {
  output_tensor_descs_.clear();
  return ge_executor_.ExecModel(model_id_,
                                stream_,
                                inputs,
                                input_tensor_descs_,
                                outputs,
                                output_tensor_descs_,
                                false);
}
}  // namespace ge
