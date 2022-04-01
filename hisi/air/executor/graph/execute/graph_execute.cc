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

#include "graph/execute/graph_execute.h"

#include "graph/ge_context.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/profiling/profiling_manager.h"
#include "common/thread_pool.h"

namespace {
constexpr size_t kMemAlignment = 64U;
}
namespace ge {
using Uint32Pair = std::pair<uint32_t, uint32_t>;
GraphExecutor::GraphExecutor() {}

GraphExecutor::~GraphExecutor() {
  outputs_desc_.clear();
  (void)FreeInOutBuffer();
}

Status GraphExecutor::SetDynamicSize(const uint32_t model_id, const std::vector<uint64_t> &batch_num,
                                     const int32_t dynamic_type) {
  const auto ret = ModelManager::GetInstance().SetDynamicSize(model_id, batch_num, dynamic_type);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][DynamicSize] failed, model_id:%u", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::FreeInOutBuffer(std::vector<BufferInfo> &buffers) {
  for (auto &buffer : buffers) {
    const rtError_t rt_ret = rtFreeHost(buffer.addr);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtFreeHost failed, ret:0x%X", rt_ret);
      GELOGE(RT_FAILED, "[Call][RtFreeHost] subgraph free buffer failed, ret: 0x%X", rt_ret);
      return GE_GRAPH_FREE_FAILED;
    }
    buffer.addr = nullptr;
  }
  return SUCCESS;
}

Status GraphExecutor::FreeInOutBuffer() {
  const std::unique_lock<std::mutex> lock(cache_mutex_);
  for (auto &buffer : buffer_cache_) {
    const auto ret = FreeInOutBuffer(buffer.second);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  buffer_cache_.clear();
  return SUCCESS;
}

Status GraphExecutor::MallocInOutBuffer(const uint32_t model_id, std::vector<BufferInfo> &buffer_info) {
  const std::unique_lock<std::mutex> lock(cache_mutex_);
  const auto iter = buffer_cache_.find(model_id);
  if (iter != buffer_cache_.end()) {
    if (iter->second == buffer_info) {
      buffer_info = iter->second;
      return SUCCESS;
    } else {
      const auto rt_ret = FreeInOutBuffer(iter->second);
      if (rt_ret != SUCCESS) {
        GELOGE(RT_FAILED, "[Free][Buffer] failed, ret: 0x%X", rt_ret);
        return RT_FAILED;
      }
      (void)buffer_cache_.erase(iter);
    }
  }

  rtError_t rt_ret;
  for (size_t i = 0U; i < buffer_info.size(); ++i) {
    void *tmp_buf = nullptr;
    rt_ret = rtMallocHost(&tmp_buf, buffer_info[i].size);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtMallocHost failed, size:%lu, ret:0x%X", buffer_info[i].size, rt_ret);
      GELOGE(RT_FAILED, "[Malloc][Buffer] failed, size:%lu, ret:0x%X", buffer_info[i].size, rt_ret);
      return GE_GRAPH_MALLOC_FAILED;
    }

    buffer_info[i].addr = tmp_buf;
  }
  buffer_cache_[model_id] = buffer_info;
  return SUCCESS;
}

Status GraphExecutor::PrepareInputData(const std::vector<GeTensor> &input_tensor, InputData &graph_input_data,
                                       OutputData &graph_output_data,
                                       const std::vector<InputOutputDescInfo> &output_desc) {
  // Preprocessing input data
  graph_input_data.index = 0U;
  graph_input_data.timeout = 0U;
  graph_input_data.timestamp = 0U;
  const std::size_t inputSize = input_tensor.size();
  const std::size_t output_size = output_desc.size();
  std::vector<BufferInfo> buffer_info;

  for (size_t i = 0U; i < inputSize; ++i) {
    const GeTensor &in_tensor = input_tensor[i];
    buffer_info.push_back({nullptr, in_tensor.GetData().size()});
  }

  for (const auto &desc : output_desc) {
    buffer_info.push_back({nullptr, desc.size});
  }

  const Status ret = MallocInOutBuffer(graph_input_data.model_id, buffer_info);
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_MALLOC_FAILED, "[Malloc][Mem] failed");
    return GE_GRAPH_MALLOC_FAILED;
  }

  for (size_t i = 0U; (i < input_tensor.size()) && (i < buffer_info.size()); ++i) {
    const GeTensor &in_tensor = input_tensor[i];
    if ((buffer_info[i].addr != nullptr) && (in_tensor.GetData().data() != nullptr)) {
      const rtError_t rt_ret = rtMemcpy(buffer_info[i].addr, buffer_info[i].size, in_tensor.GetData().data(),
                                        in_tensor.GetData().size(), RT_MEMCPY_HOST_TO_HOST);
      if (rt_ret != RT_ERROR_NONE) {
        REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, dst_size:%lu, src_size:%zu, ret:0x%X",
                          buffer_info[i].size, in_tensor.GetData().size(), rt_ret);
        GELOGE(RT_FAILED, "[Call][RtMemcpy] failed, dst_size:%lu, src_size:%zu, ret:0x%X",
               buffer_info[i].size, in_tensor.GetData().size(), rt_ret);
        return RT_FAILED;
      }
    }

    DataBuffer in_data_buf;
    in_data_buf.data = buffer_info[i].addr;
    in_data_buf.length = in_tensor.GetData().size();
    in_data_buf.isDataSupportMemShare = false;
    graph_input_data.blobs.push_back(in_data_buf);
  }

  graph_output_data.index = 0U;

  for (size_t j = 0U; j < output_size; j++) {
    const auto &desc = output_desc[j];
    const uint64_t buffer_size = desc.size;

    DataBuffer out_data_buf;
    out_data_buf.data = buffer_info[inputSize + j].addr;
    out_data_buf.length = buffer_size;
    out_data_buf.isDataSupportMemShare = false;
    graph_output_data.blobs.push_back(out_data_buf);
  }

  return SUCCESS;
}

Status GraphExecutor::SyncMultiExecuteModel(const GeRootModelPtr &ge_root_model,
                                            const std::vector<GeTensor> &input_tensor,
                                            std::vector<GeTensor> &output_tensor) {
  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  GE_CHECK_NOTNULL(ge_root_model);
  const auto root_graph = ge_root_model->GetRootGraph();
  GE_CHECK_NOTNULL(root_graph);
  std::vector<NamedAttrs> deploy_info;
  if ((!AttrUtils::GetListNamedAttrs(root_graph, ATTR_NAME_DEPLOY_INFO, deploy_info)) || deploy_info.empty()) {
    REPORT_CALL_ERROR("E19999", "Sync execute model fail, graph %s has invalid deploy attr %s",
                      root_graph->GetName().c_str(), ATTR_NAME_DEPLOY_INFO.c_str());
    GELOGE(FAILED, "[SyncMultiExecuteModel] graph %s has invalid deploy attr %s", root_graph->GetName().c_str(),
           ATTR_NAME_DEPLOY_INFO.c_str());
    return FAILED;
  }

  std::vector<size_t> deploy_id;
  const auto thread_instances_size = ModelUtils::GetDeployNum(ge_root_model, deploy_id);
  const auto model_ids = ge_root_model->GetAllModelId();
  if (model_ids.size() != thread_instances_size) {
    GELOGE(FAILED,
           "[SyncMultiExecuteModel] something wrong, attr deploy numbers %zu should be equal to loaded models %zu",
           thread_instances_size, model_ids.size());
    return FAILED;
  }

  ThreadPool executor(static_cast<size_t>(thread_instances_size));
  std::vector<std::future<Status>> vector_future;
  std::vector<std::vector<GeTensor>> graph_inputs;
  graph_inputs.reserve(thread_instances_size);
  for (size_t i = 0U; i < thread_instances_size; ++i) {
    std::vector<GeTensorPtr> attr_inputs;
    graph_inputs.push_back(input_tensor);
    if (AttrUtils::MutableListTensor(deploy_info[i], ATTR_NAME_DEPLOY_GRAPH_INPUTS, attr_inputs)) {
      for (const auto &ge_tensor_ptr : attr_inputs) {
        graph_inputs[i].push_back(*ge_tensor_ptr);
      }
    }
    SyncExecuteModelFunc execute_model_func(&GraphExecutor::SyncExecuteModel);
    std::future<Status> f;
    bool need_return_result = false;
    if ((AttrUtils::GetBool(deploy_info[i], ATTR_NAME_DEPLOY_NEED_RETURN_RESULT, need_return_result) &&
        need_return_result)) {
      GELOGI("[SyncMultiExecuteModel] model[%u] will return result.", model_ids[i]);
      f = executor.commit(execute_model_func, this, model_ids[i], cref(graph_inputs[i]),
                          ref(output_tensor), ErrorManager::GetInstance().GetErrorManagerContext());
    } else {
      std::vector<GeTensor> output_tensor_no_use;
      f = executor.commit(execute_model_func, this, model_ids[i], cref(graph_inputs[i]),
                          ref(output_tensor_no_use), ErrorManager::GetInstance().GetErrorManagerContext());
    }
    if (!f.valid()) {
      GELOGE(FAILED, "[Call][Commit] failed, Future is invalid");
      return FAILED;
    }
    vector_future.emplace_back(std::move(f));
  }

  Status last_status = SUCCESS;
  for (size_t i = 0U; i < vector_future.size(); ++i) {
    const Status ret_status = vector_future[i].get();
    if (ret_status != SUCCESS) {
      REPORT_CALL_ERROR("E19999", " Execute multi model %zu  failed", i);
      GELOGE(ret_status, "[SyncMultiExecuteModel] Execute multi model[%zu] failed", i);
      last_status = ret_status;
    }
  }
  return last_status;
}

Status GraphExecutor::SyncExecuteModel(const uint32_t model_id, const std::vector<GeTensor> &input_tensor,
                                       std::vector<GeTensor> &output_tensor,
                                       const error_message::Context &error_context) {
  ErrorManager::GetInstance().SetErrorContext(error_context);
  const auto rt_ret = rtSetDevice(static_cast<int32_t>(GetContext().DeviceId()));
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtSetDevice failed, device_id:%u, ret:0x%X",
                      GetContext().DeviceId(), rt_ret);
    GELOGE(RT_FAILED, "[Call][RtSetDevice] failed, device_id:%u, ret:0x%X", GetContext().DeviceId(), rt_ret);
    return RT_FAILED;
  }

  if (ModelManager::GetInstance().IsDynamicShape(model_id)) {
    GELOGI("[ExecuteGraph] SyncExecuteModel via dynamic shape model executor, modelId=%u", model_id);
    return ModelManager::GetInstance().SyncExecuteModel(model_id, input_tensor, output_tensor);
  }

  // Prepare input and output
  std::vector<InputOutputDescInfo> inputs_desc;
  std::vector<InputOutputDescInfo> output_desc;

  GELOGI("[ExecuteGraph] GetInputOutputDescInfo via new ome begin.");
  Status ret = GetInputOutputDescInfo(model_id, inputs_desc, output_desc);
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_GET_IN_OUT_FAILED, "[Get][InputOutputDescInfo] failed, modelId=%u.", model_id);
    return GE_GRAPH_GET_IN_OUT_FAILED;
  }
  outputs_desc_.assign(output_desc.begin(), output_desc.end());

  InputData input_data;
  OutputData output_data;
  input_data.model_id = model_id;
  ret = PrepareInputData(input_tensor, input_data, output_data, output_desc);
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_PREPARE_FAILED, "[Prepare][InputData] failed, modelId=%u.", model_id);
    return GE_GRAPH_PREPARE_FAILED;
  }

  // Run mode synchronize
  GELOGI("[ExecuteGraph] SyncExecuteModel via new ome begin.");
  GE_CHK_STATUS_RET_NOLOG(ModelManager::GetInstance().SyncExecuteModel(input_data, output_data));

  for (size_t i = 0U; i < output_data.blobs.size(); ++i) {
    const DataBuffer &output_buffer = output_data.blobs[i];
    CHECK_FALSE_EXEC(output_buffer.length != 0U,
        REPORT_INNER_ERROR("E19999", "Param output_data.length is 0 in model:%u, check invalid", model_id);
        GELOGE(GE_GRAPH_EXECUTE_FAILED, "[Check][Param] Failed to allocate memory, length is 0, model:%u", model_id);
        return GE_GRAPH_EXECUTE_FAILED);
    const auto aligned_ptr = MakeShared<AlignedPtr>(output_buffer.length, kMemAlignment);
    GE_CHECK_NOTNULL(aligned_ptr);
    auto *const data_buf = aligned_ptr->MutableGet();
    GE_CHECK_NOTNULL(data_buf);
    GE_CHK_RT_RET(rtMemcpy(data_buf, output_buffer.length, output_buffer.data, output_buffer.length,
                           RT_MEMCPY_HOST_TO_HOST));

    std::vector<int64_t> shape_dims;
    (void)std::copy(output_desc[i].shape_info.dims.begin(), output_desc[i].shape_info.dims.end(),
                    std::back_inserter(shape_dims));

    GeTensor out_tensor;
    out_tensor.MutableTensorDesc().SetShape(GeShape(shape_dims));
    out_tensor.MutableTensorDesc().SetDataType(static_cast<DataType>(output_desc[i].data_type));
    (void)out_tensor.SetData(aligned_ptr, output_buffer.length);
    output_tensor.push_back(out_tensor);
  }

  GELOGI("[GraphExecutor] execute model success, modelId=%u.", model_id);

  return SUCCESS;
}

Status GraphExecutor::FreeExecuteMemory() {
  const auto ret = FreeInOutBuffer();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Free][InOutBuffer] Error!");
    return ret;
  }

  return SUCCESS;
}

Status GraphExecutor::ExecuteGraph(const GraphId graph_id, const GeRootModelPtr &ge_root_model,
                                   const std::vector<GeTensor> &input_tensor, std::vector<GeTensor> &output_tensor) {
  if (graph_id != last_graph_id_) {
    const auto ret = FreeExecuteMemory();
    if (ret != SUCCESS) {
      return ret;
    }
  }
  last_graph_id_ = graph_id;

  GE_CHECK_NOTNULL_EXEC(ge_root_model, return FAILED);
  GE_CHK_STATUS_RET_NOLOG(ModelManager::GetInstance().ModelSubscribe(graph_id));
  std::vector<NamedAttrs> deploy_info;
  Status ret = SUCCESS;
  if (AttrUtils::GetListNamedAttrs(ge_root_model->GetRootGraph(), ATTR_NAME_DEPLOY_INFO, deploy_info)) {
    ret = SyncMultiExecuteModel(ge_root_model, input_tensor, output_tensor);
  } else {
    ret = SyncExecuteModel(ge_root_model->GetModelId(), input_tensor, output_tensor,
                           ErrorManager::GetInstance().GetErrorManagerContext());
  }
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_SYNC_MODEL_FAILED, "[SyncExecute][Model] Error! graph id:%u", graph_id);
    return GE_GRAPH_SYNC_MODEL_FAILED;
  }
  return SUCCESS;
}

Status GraphExecutor::ExecuteGraphAsync(const GraphId graph_id, const GeRootModelPtr &ge_root_model,
                                        const std::vector<Tensor> &input_tensor,
                                        const RunAsyncCallback& callback) {
  GELOGI("[GraphExecutor] Start to async execute graph, graph_id=%u", graph_id);
  if (graph_id != last_graph_id_) {
    const auto ret = FreeExecuteMemory();
    if (ret != SUCCESS) {
      return ret;
    }
  }
  last_graph_id_ = graph_id;
  GE_CHECK_NOTNULL_EXEC(ge_root_model, return FAILED);
  std::vector<NamedAttrs> deploy_info;
  Status ret = SUCCESS;
  if (AttrUtils::GetListNamedAttrs(ge_root_model->GetRootGraph(), ATTR_NAME_DEPLOY_INFO, deploy_info)) {
    ret = AsyncMultiExecuteModel(ge_root_model, input_tensor, callback);
  } else {
    ret = AsyncExecuteModel(ge_root_model, GetExecuteModelId(ge_root_model), input_tensor,
                            ErrorManager::GetInstance().GetErrorManagerContext(), callback);
  }
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_SYNC_MODEL_FAILED, "[AsyncExecute][Model] Error! graph id:%u", graph_id);
    return GE_GRAPH_SYNC_MODEL_FAILED;
  }

  GELOGI("[GraphExecutor] Async execute graph success, graph_id=%u", graph_id);
  return SUCCESS;
}

Status GraphExecutor::ExecuteGraphWithStream(const GraphId graph_id,
                                             const rtStream_t stream,
                                             const GeRootModelPtr &ge_root_model,
                                             const std::vector<GeTensor> &input_tensor,
                                             const std::vector<GeTensor> &output_tensor) {
  GELOGI("[GraphExecutor] Start to execute graph with stream, graph id = %u, stream = %p.", graph_id, stream);
  if (graph_id != last_graph_id_) {
    const auto ret = FreeExecuteMemory();
    if (ret != SUCCESS) {
      return ret;
    }
  }
  last_graph_id_ = graph_id;

  GE_CHECK_NOTNULL_EXEC(ge_root_model, return FAILED);
  const auto model_id = ge_root_model->GetModelId();
  const auto ret = ModelManager::GetInstance().ExecuteModel(model_id, stream, true, input_tensor, output_tensor);
  if (ret != SUCCESS) {
    return ret;
  }

  GELOGI("[GraphExecutor] Async execute graph with stream success graph id = %u, stream = %p.", graph_id, stream);
  return SUCCESS;
}

uint32_t GraphExecutor::GetExecuteModelId(const GeRootModelPtr &ge_root_model) {
  const std::vector<uint32_t> &model_ids = ge_root_model->GetAllModelId();
  if (model_ids.empty()) {
    return kInvalidModelId;
  }
  if (model_ids.size() == 1U) {
    return ge_root_model->GetModelId();
  }
  std::vector<Uint32Pair> model_id_to_loads;
  for (auto model_id : model_ids) {
    const uint32_t input_load = ModelManager::GetInstance().GetDataInputerSize(model_id);
    const uint32_t running_load = ModelManager::GetInstance().GetRunningFlag(model_id);
    const uint32_t load = input_load + running_load;
    if (load == 0U) {
      return model_id;
    }
    model_id_to_loads.emplace_back(model_id, load);
  }
  std::sort(model_id_to_loads.begin(), model_id_to_loads.end(), [](const Uint32Pair &lhs, const Uint32Pair &rhs) {
    return lhs.second < rhs.second;
  });

  if (model_id_to_loads.empty()) {
    return kInvalidModelId;
  }
  return model_id_to_loads.begin()->first;
}

Status GraphExecutor::AsyncMultiExecuteModel(const GeRootModelPtr &ge_root_model, const std::vector<Tensor> &inputs,
                                             const RunAsyncCallback &callback) {
  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  // get deploy number of model instance
  GE_CHECK_NOTNULL(ge_root_model);
  const auto root_graph = ge_root_model->GetRootGraph();
  GE_CHECK_NOTNULL(root_graph);
  std::vector<NamedAttrs> deploy_info;
  if ((!AttrUtils::GetListNamedAttrs(root_graph, ATTR_NAME_DEPLOY_INFO, deploy_info)) || deploy_info.empty()) {
    REPORT_CALL_ERROR("E19999", "Async execute model fail, graph %s has invalid deploy attr %s",
                      root_graph->GetName().c_str(), ATTR_NAME_DEPLOY_INFO.c_str());
    GELOGE(FAILED, "[AsyncMultiExecuteModel] graph %s has invalid deploy attr %s", root_graph->GetName().c_str(),
           ATTR_NAME_DEPLOY_INFO.c_str());
    return FAILED;
  }

  std::vector<size_t> deploy_id;
  const auto thread_instances_size = ModelUtils::GetDeployNum(ge_root_model, deploy_id);
  const auto model_ids = ge_root_model->GetAllModelId();
  if (model_ids.size() != thread_instances_size) {
    GELOGE(FAILED,
           "[AsyncMultiExecuteModel] something wrong, attr deploy numbers %zu should be equal to loaded models %zu",
           thread_instances_size, model_ids.size());
    return FAILED;
  }

  ThreadPool executor(static_cast<uint32_t>(thread_instances_size));
  std::vector<std::future<Status>> vector_future;
  std::vector<std::vector<Tensor>> graph_inputs;
  graph_inputs.reserve(thread_instances_size);
  for (size_t i = 0U; i < thread_instances_size; ++i) {
    graph_inputs.push_back(inputs);
    std::vector<GeTensorPtr> attr_inputs;
    if (AttrUtils::MutableListTensor(deploy_info[i], ATTR_NAME_DEPLOY_GRAPH_INPUTS, attr_inputs)) {
      for (const auto &ge_tensor_ptr : attr_inputs) {
        graph_inputs[i].push_back(TensorAdapter::AsTensor(*ge_tensor_ptr));
      }
    }
    AsyncExecuteModelFunc execute_model_func(&GraphExecutor::AsyncExecuteModel);
    std::future<Status> f;
    bool need_return_result = false;
    if ((AttrUtils::GetBool(deploy_info[i], ATTR_NAME_DEPLOY_NEED_RETURN_RESULT, need_return_result) &&
        need_return_result)) {
      f = executor.commit(execute_model_func, this, ge_root_model, model_ids[i], cref(graph_inputs[i]),
                          ErrorManager::GetInstance().GetErrorManagerContext(), cref(callback));
    } else {
      const RunAsyncCallback callback_stub = [](Status, std::vector<Tensor> &) {};
      f = executor.commit(execute_model_func, this, ge_root_model, model_ids[i], cref(graph_inputs[i]),
                          ErrorManager::GetInstance().GetErrorManagerContext(), cref(callback_stub));
    }
    if (!f.valid()) {
      GELOGE(FAILED, "[Call][Commit] failed, Future is invalid");
      return FAILED;
    }
    vector_future.emplace_back(std::move(f));
  }
  for (size_t i = 0U; i < vector_future.size(); ++i) {
    const Status ret_status = vector_future[i].get();
    if (ret_status != SUCCESS) {
      REPORT_CALL_ERROR("E19999", " Execute multi model %zu  failed", i);
      GELOGE(ret_status, "[AsyncMultiExecuteModel] Execute multi model[%zu] failed", i);
      return ret_status;
    }
  }
  return SUCCESS;
}

Status GraphExecutor::AsyncExecuteModel(const GeRootModelPtr &ge_root_model, const uint32_t model_id,
                                        const std::vector<Tensor> &inputs,
                                        const error_message::Context &error_context,
                                        const RunAsyncCallback &callback) const {
  ErrorManager::GetInstance().SetErrorContext(error_context);
  if (model_id == kInvalidModelId) {
    GELOGE(INTERNAL_ERROR, "No valid model id.");
    return INTERNAL_ERROR;
  }
  try {
    GELOGI("RunAsync begin.model_id %u", model_id);
    if (ModelManager::GetInstance().SetCallback(model_id, ge_root_model, callback) != SUCCESS) {
      GELOGE(FAILED, "[Set][CallBack] for model fail, model_id %u", model_id);
      return FAILED;
    }

    const auto ret = ModelManager::GetInstance().DataInputTensor(model_id, inputs);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][DataInputTensor] RunAsync: DataInput fail, model_id %u", model_id);
      return ret;
    }

    GELOGI("RunAsync success.");
  } catch (std::bad_alloc &) {
    REPORT_INNER_ERROR("E19999", "Bad memory allocation exception occur failed, model_id %u", model_id);
    GELOGE(MEMALLOC_FAILED, "[Run][Async] failed, bad memory allocation occur, model_id %u", model_id);
    return MEMALLOC_FAILED;
  } catch (...) {
    REPORT_INNER_ERROR("E19999", "Some exceptions occur failed, model_id %u", model_id);
    GELOGE(FAILED, "[Run][Async] failed, some exceptions occur, model_id %u", model_id);
    return FAILED;
  }

  return SUCCESS;
}

Status GraphExecutor::GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                             std::vector<InputOutputDescInfo> &output_desc) {
  try {
    const auto ret = ModelManager::GetInstance().GetInputOutputDescInfo(model_id, input_desc, output_desc);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Get][InputOutputDescInfo] failed, model_id:%u.", model_id);
      return ret;
    }
  } catch (std::bad_alloc &) {
    REPORT_INNER_ERROR("E19999", "Bad memory allocation exception occur failed, model_id:%u.", model_id);
    GELOGE(MEMALLOC_FAILED, "[Get][InputOutputDescInfo] failed, bad memory allocation occur, model_id:%u.", model_id);
    return MEMALLOC_FAILED;
  } catch (...) {
    REPORT_INNER_ERROR("E19999", "Some exceptions occur failed, model_id:%u.", model_id);
    GELOGE(FAILED, "[Get][InputOutputDescInfo] failed, some exceptions occur, model_id:%u.", model_id);
    return FAILED;
  }

  return SUCCESS;
}

Status GraphExecutor::GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                             std::vector<InputOutputDescInfo> &output_desc,
                                             std::vector<uint32_t> &input_formats, std::vector<uint32_t> &out_formats,
                                             const bool new_model_desc) {
  try {
    const auto ret = ModelManager::GetInstance().GetInputOutputDescInfo(model_id, input_desc, output_desc,
                                                                        input_formats, out_formats, new_model_desc);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Get][InputOutputDescInfo] failed, model_id:%u.", model_id);
      return ret;
    }
  } catch (std::bad_alloc &) {
    REPORT_INNER_ERROR("E19999", "Bad memory allocation exception occur failed, model_id:%u.", model_id);
    GELOGE(MEMALLOC_FAILED, "[Get][InputOutputDescInfo] failed, bad memory allocation occur, model_id:%u.", model_id);
    return MEMALLOC_FAILED;
  } catch (...) {
    REPORT_INNER_ERROR("E19999", "Some exceptions occur failed, model_id:%u.", model_id);
    GELOGE(FAILED, "[Get][InputOutputDescInfo] failed, some exceptions occur, model_id:%u.", model_id);
    return FAILED;
  }

  return SUCCESS;
}
///
/// @ingroup ge
/// @brief Get dynamic batch_info
/// @param [in] model_id
/// @param [out] batch_info
/// @param [out] dynamic_type
/// @return execute result
///
Status GraphExecutor::GetDynamicBatchInfo(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                          int32_t &dynamic_type) {
  const auto ret = ModelManager::GetInstance().GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][DynamicBatchInfo] failed, model_id:%u.", model_id);
    return ret;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get combined dynamic dims info
/// @param [in] model_id
/// @param [out] batch_info
/// @return execute result
///
Status GraphExecutor::GetCombinedDynamicDims(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info) {
  const auto ret = ModelManager::GetInstance().GetCombinedDynamicDims(model_id, batch_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][GetCombinedDynamicDims] failed, model_id:%u.", model_id);
    return ret;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get user designate shape order
/// @param [in] model_id
/// @param [out] user_input_shape_order
/// @return execute result
///
Status GraphExecutor::GetUserDesignateShapeOrder(const uint32_t model_id,
                                                 std::vector<std::string> &user_input_shape_order) {
  const auto ret = ModelManager::GetInstance().GetUserDesignateShapeOrder(model_id, user_input_shape_order);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][UserDesignateShapeOrder] failed, model_id:%u.", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetCurrentShape(const uint32_t model_id, std::vector<int64_t> &batch_info,
                                      int32_t &dynamic_type) {
  const auto ret = ModelManager::GetInstance().GetCurrentShape(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][CurShape] failed, model_id:%u", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetNodeAttr(const uint32_t model_id, const std::string &op_name, const std::string &attr_name,
                                  std::string &attr_value) {
  const auto ret = ModelManager::GetInstance().GetNodeAttr(model_id, op_name, attr_name, attr_value);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][OpAttr]Get op:%s attr:%s failed.", op_name.c_str(), attr_name.c_str());
    REPORT_CALL_ERROR("E19999", "Get op:%s attr:%s failed.", op_name.c_str(), attr_name.c_str());
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetOutputShapeInfo(const uint32_t model_id, std::vector<std::string> &dynamic_output_shape_info) {
  const auto ret = ModelManager::GetInstance().GetOutputShapeInfo(model_id, dynamic_output_shape_info);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Get][ModelAttr] failed, model_id:%u", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetAippInfo(const uint32_t model_id, const uint32_t index, AippConfigInfo &aipp_info) {
  const auto ret = ModelManager::GetInstance().GetAippInfo(model_id, index, aipp_info);
  if (ret != SUCCESS) {
    GELOGW("GetAIPPInfo is not success.");
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetAippType(const uint32_t model_id, const uint32_t index, InputAippType &type,
                                  size_t &aipp_index) {
  const auto ret = ModelManager::GetInstance().GetAippType(model_id, index, type, aipp_index);
  if (ret != SUCCESS) {
    GELOGW("Get aipp type is not success.");
    return ret;
  }
  return SUCCESS;
}

Status GraphExecutor::GetOrigInputInfo(const uint32_t model_id, const uint32_t index,
                                       OriginInputInfo &orig_input_info) {
  const auto ret = ModelManager::GetInstance().GetOrigInputInfo(model_id, index, orig_input_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][OrigInputInfo] failed, model_id:%u, index:%u.", model_id, index);
    return ret;
  }

  return SUCCESS;
}

Status GraphExecutor::GetAllAippInputOutputDims(const uint32_t model_id, const uint32_t index,
                                                std::vector<InputOutputDims> &input_dims,
                                                std::vector<InputOutputDims> &output_dims) {
  const auto ret = ModelManager::GetInstance().GetAllAippInputOutputDims(model_id, index, input_dims, output_dims);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][AllAippInputOutputDims] failed, model_id:%u, index:%u.", model_id, index);
    return ret;
  }

  return SUCCESS;
}

Status GraphExecutor::GetOpDescInfo(const uint32_t device_id, const uint32_t stream_id, const uint32_t task_id,
                                    OpDescInfo &op_desc_info) {
  const auto ret = ModelManager::GetInstance().GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][OpDescInfo] failed, device_id:%u, stream_id:%u, task_id:%u.",
           device_id, stream_id, task_id);
    return ret;
  }
  return SUCCESS;
}
}  // namespace ge
