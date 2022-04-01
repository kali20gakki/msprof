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
#include "graph/load/model_manager/heterogeneous_model_executor.h"
#include "exec_runtime/execution_runtime.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/tensor_adapter.h"
#include "framework/common/types.h"
#include "common/model/ge_root_model.h"

namespace ge {
namespace {
constexpr size_t kDataOutputIndexVal0 = 0U;
constexpr size_t kAlignmentVal64 = 64U;

ge::PneModelPtr GetSubmodel(const PneModelPtr &root_model, const std::string &submodel_name) {
  auto submodel = root_model->GetSubmodel(submodel_name);
  if (submodel == nullptr) {
    for (const auto &pne_model_pair : root_model->GetSubmodels()) {
      submodel = pne_model_pair.second->GetSubmodel(submodel_name);
      if (submodel != nullptr) {
        break;
      }
    }
  }
  return submodel;
}

}  // namespace
HeterogeneousModelExecutor::HeterogeneousModelExecutor(const PneModelPtr &root_model,
                                                       const DeployResult &deploy_result)
    : root_model_(root_model),
      deployed_model_id_(deploy_result.model_id),
      input_queue_ids_(deploy_result.input_queue_ids),
      control_input_queue_ids_(deploy_result.control_input_queue_ids),
      output_queue_ids_(deploy_result.output_queue_ids) {
}

HeterogeneousModelExecutor::~HeterogeneousModelExecutor() {
  if (run_flag_) {
    GELOGW("Run thread is not stopped");
    (void)ModelRunStop();
  }
}

Status HeterogeneousModelExecutor::Initialize() {
  GELOGI("input_queues = %s, control_input_queues = %s, output_queues = %s",
         ToString(input_queue_ids_).c_str(),
         ToString(control_input_queue_ids_).c_str(),
         ToString(output_queue_ids_).c_str());
  GE_CHECK_NOTNULL(root_model_);
  if (root_model_->GetSubmodels().empty()) {
    return FAILED;
  }

  GE_CHK_STATUS_RET_NOLOG(WrapSingleModel());
  GE_CHECK_NOTNULL(root_model_->GetModelRelation());
  GE_CHK_STATUS_RET(ParseInputTensorInfo(), "Failed to parse input tensor info, model name = %s",
                    root_model_->GetModelName().c_str());
  GE_CHK_STATUS_RET(ParseOutputTensorInfo(), "Failed to parse output tensor info, model name = %s",
                    root_model_->GetModelName().c_str());
  const auto execution_runtime = ExecutionRuntime::GetInstance();
  GE_CHECK_NOTNULL(execution_runtime);
  exchange_service_ = &execution_runtime->GetExchangeService();
  GE_CHECK_NOTNULL(exchange_service_);
  return SUCCESS;
}

Status HeterogeneousModelExecutor::WrapSingleModel() {
  auto model_relation = ge::MakeShared<ModelRelation>();
  GE_CHECK_NOTNULL(model_relation);
  for (const auto &pne_model_pair : root_model_->GetSubmodels()) {
    PneModelPtr single_model = pne_model_pair.second;
    GE_CHECK_NOTNULL(single_model);
    if (single_model->GetRootGraph() != nullptr) {
      is_dynamic_ = single_model->GetRootGraph()->GetGraphUnknownFlag();
    }
    if (!single_model->GetSubmodels().empty()) {
      model_relation = single_model->GetModelRelation();
      break;
    }

    GE_CHK_STATUS_RET(ModelRelationBuilder().BuildForSingleModel(*single_model->GetRootGraph(),
                                                                 *model_relation),
                      "Failed to build model relation for single model");
  }
  root_model_->SetModelRelation(model_relation);
  return SUCCESS;
}

Status HeterogeneousModelExecutor::ParseInputTensorInfo() {
  const auto model_relation = root_model_->GetModelRelation();
  // 1. build queue name to tensor desc mapping
  std::map<std::string, GeTensorDescPtr> input_tensor_desc_map;
  GE_CHK_STATUS_RET_NOLOG(BuildInputTensorDescMapping(input_tensor_desc_map));
  GELOGD("Start to set tensor info for inputs, size = %zu",
         model_relation->root_model_queue_info.input_queue_names.size());
  GE_CHK_STATUS_RET_NOLOG(SetTensorInfo(input_tensor_desc_map, model_relation->root_model_queue_info.input_queue_names,
                                        input_is_no_tiling_, true));
  return SUCCESS;
}

Status HeterogeneousModelExecutor::BuildInputTensorDescMapping(std::map<std::string, GeTensorDescPtr> &mapping) {
  for (const auto &it : root_model_->GetModelRelation()->submodel_queue_infos) {
    const auto &submodel_name = it.first;
    const auto &input_queue_names = it.second.input_queue_names;
    auto submodel = GetSubmodel(root_model_, submodel_name);
    GE_CHECK_NOTNULL(submodel);
    std::map<int64_t, GeTensorDescPtr> indices_to_tensor_descs;
    for (const auto &node : submodel->GetRootGraph()->GetDirectNode()) {
      if (NodeUtils::GetNodeType(node) != DATA) {
        continue;
      }
      int64_t index = -1;
      if (!AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_INDEX, index)) {
        GELOGE(PARAM_INVALID, "Failed to get data index, node name = %s", node->GetName().c_str());
        return PARAM_INVALID;
      }
      GELOGD("Data index of node [%s] = [%ld]", node->GetName().c_str(), index);
      auto input_desc = node->GetOpDesc()->MutableOutputDesc(kDataOutputIndexVal0);
      GE_CHECK_NOTNULL(input_desc);
      (void)indices_to_tensor_descs.emplace(index, input_desc);
    }
    if (indices_to_tensor_descs.size() != input_queue_names.size()) {
      GELOGE(PARAM_INVALID, "Number of inputs(%zu) mismatches that of input queues(%zu), sub model name = %s",
             indices_to_tensor_descs.size(), input_queue_names.size(), submodel_name.c_str());
      return PARAM_INVALID;
    }

    for (size_t i = 0U; i < input_queue_names.size(); ++i) {
      const auto &queue_name = input_queue_names[i];
      auto tensor_desc = indices_to_tensor_descs[static_cast<int64_t>(i)];
      if (tensor_desc == nullptr) {
        GELOGE(PARAM_INVALID, "Failed to input[%zu] from submodel: %s", i, submodel_name.c_str());
        return PARAM_INVALID;
      }
      if (is_dynamic_) {
        GELOGD("Input[%zu] is %s", i, tensor_desc->MutableShape().IsUnknownShape() ? "dynamic" : "static");
        input_is_no_tiling_.push_back(tensor_desc->MutableShape().IsUnknownShape());
      } else {
        bool is_no_tiling = false;
        (void)AttrUtils::GetBool(tensor_desc, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, is_no_tiling);
        input_is_no_tiling_.push_back(is_no_tiling);
        GELOGD("Input tensor:%zu supports no-tiling is %d", i, static_cast<int32_t>(is_no_tiling));
      }

      mapping[queue_name] = std::move(tensor_desc);
    }
  }
  return SUCCESS;
}

Status HeterogeneousModelExecutor::BuildOutputTensorDescMapping(std::map<std::string, GeTensorDescPtr> &mapping) {
  GE_CHECK_NOTNULL(root_model_);
  for (const auto &it : root_model_->GetModelRelation()->submodel_queue_infos) {
    const auto &submodel_name = it.first;
    const auto &output_queue_names = it.second.output_queue_names;
    auto submodel = GetSubmodel(root_model_, submodel_name);
    GE_CHECK_NOTNULL(submodel);
    const auto &subgraph = submodel->GetRootGraph();
    const auto net_output = subgraph->FindFirstNodeMatchType(NETOUTPUT);
    GE_CHECK_NOTNULL(net_output);
    const auto op_desc = net_output->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    auto output_desc_list = op_desc->GetAllInputsDescPtr();
    if (output_desc_list.size() != output_queue_names.size()) {
      GELOGE(PARAM_INVALID, "Number of outputs(%zu) mismatches that of output queues(%zu), sub model name = %s",
             output_desc_list.size(), output_queue_names.size(), submodel_name.c_str());
      return PARAM_INVALID;
    }
    for (size_t i = 0U; i < output_desc_list.size(); ++i) {
      const auto &tensor_desc = output_desc_list.at(i);
      if (is_dynamic_) {
        GELOGD("Output[%zu] is %s", i, tensor_desc->MutableShape().IsUnknownShape() ? "dynamic" : "static");
        output_is_no_tiling_.push_back(tensor_desc->MutableShape().IsUnknownShape());
      } else {
        bool is_no_tiling = false;
        (void)AttrUtils::GetBool(tensor_desc, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, is_no_tiling);
        output_is_no_tiling_.push_back(is_no_tiling);
        GELOGD("Output tensor:%zu support no-tiling is %d", i, static_cast<int32_t>(is_no_tiling));
      }
      const auto &queue_name = output_queue_names[i];
      mapping[queue_name] = std::move(output_desc_list.at(i));
    }
  }
  return SUCCESS;
}

Status HeterogeneousModelExecutor::ParseOutputTensorInfo() {
  const auto model_relation = root_model_->GetModelRelation();
  std::map<std::string, GeTensorDescPtr> output_tensor_desc_map;
  GE_CHK_STATUS_RET_NOLOG(BuildOutputTensorDescMapping(output_tensor_desc_map));
  GELOGD("Start to set tensor info for outputs, size = %zu",
         model_relation->root_model_queue_info.output_queue_names.size());
  GE_CHK_STATUS_RET_NOLOG(SetTensorInfo(
      output_tensor_desc_map, model_relation->root_model_queue_info.output_queue_names, output_is_no_tiling_, false));
  return SUCCESS;
}

Status HeterogeneousModelExecutor::SetTensorInfo(std::map<std::string, GeTensorDescPtr> &mapping,
                                                 const std::vector<std::string> &queue_names,
                                                 const std::vector<bool> &is_no_tiling,
                                                 const bool is_input) {
  for (size_t i = 0U; i < queue_names.size(); ++i) {
    const auto &queue_name = queue_names[i];
    auto tensor_desc = mapping[queue_name];
    // actually this will not happen if the ModelRelation passes DeployPlanner
    if (tensor_desc == nullptr) {
      GELOGE(PARAM_INVALID, "queue name not found in submodels, name = %s", queue_name.c_str());
      return PARAM_INVALID;
    }

    int64_t tensor_size = -1;
    (void)TensorUtils::GetSize(*tensor_desc, tensor_size);
    int64_t tensor_raw_size = -1;
    GE_CHK_GRAPH_STATUS_RET(TensorUtils::CalcTensorMemSize(tensor_desc->GetShape(),
                                                           tensor_desc->GetFormat(),
                                                           tensor_desc->GetDataType(),
                                                           tensor_raw_size),
                            "Failed to calc tensor raw size, queue name = %s", queue_name.c_str());
    GELOGD("index = %zu, queue name = %s, shape = [%s], tensor raw size = %ld, padded size = %ld, notiling = %d",
           i,
           queue_name.c_str(),
           tensor_desc->GetShape().ToString().c_str(),
           tensor_raw_size,
           tensor_size,
           static_cast<int32_t>(is_no_tiling[i]));
    if (((!is_no_tiling[i]) && (tensor_raw_size < 0)) || (tensor_size < 0)) {
      GELOGE(UNSUPPORTED, "Dynamic shape is not supported yet, raw/padded size = %ld/%ld, shape of queue[%s] = [%s]",
             tensor_raw_size,
             tensor_size,
             queue_name.c_str(),
             tensor_desc->GetShape().ToString().c_str());
      return UNSUPPORTED;
    }
    if (is_input) {
      input_tensor_sizes_.emplace_back(tensor_size);
      input_tensor_raw_sizes_.emplace_back(tensor_raw_size);
      input_tensor_desc_.emplace_back(std::move(tensor_desc));
    } else {
      output_tensor_sizes_.emplace_back(tensor_size);
      output_tensor_raw_sizes_.emplace_back(tensor_raw_size);
      output_tensor_desc_.emplace_back(std::move(tensor_desc));
    }
  }
  return SUCCESS;
}

void HeterogeneousModelExecutor::SetListener(const std::shared_ptr<ModelListener> &listener) {
  listener_ = listener;
}

uint32_t HeterogeneousModelExecutor::GetDeployedModelId() const {
  return deployed_model_id_;
}

Status HeterogeneousModelExecutor::ExecuteAsync(const std::vector<Tensor> &inputs, const RunAsyncCallback &callback) {
  GE_CHECK_NOTNULL(callback);
  if (!run_flag_) {
    GELOGE(FAILED, "Model is not running, model id = %u", model_id_);
    return FAILED;
  }
  auto request = MakeShared<RunAsyncRequest>();
  GE_CHECK_NOTNULL(request);
  request->callback = callback;
  vector<GeTensor> input_tensors;
  input_tensors.resize(inputs.size());
  (void)std::transform(inputs.begin(), inputs.end(), input_tensors.begin(),
                       [](const Tensor &input) { return TensorAdapter::AsGeTensor(input); });
  {
    GELOGD("Start to execute model async, model id = %u", model_id_);
    const std::lock_guard<std::mutex> lk(mu_);
    GE_CHK_STATUS_RET(EnqueueInputTensors(input_tensors), "Failed to enqueue input tensors");
    GELOGD("Start to execute model async, model id = %u", model_id_);
    GE_CHK_BOOL_RET_STATUS(input_queue_.Push(std::move(request)), INTERNAL_ERROR, "Failed to enqueue input");
  }
  GELOGD("Input enqueued successfully, model id = %u", model_id_);
  return SUCCESS;
}

Status HeterogeneousModelExecutor::Execute(const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) {
  GELOGD("Start to execute model, model id = %u", model_id_);
  GE_CHK_STATUS_RET(EnqueueInputTensors(inputs), "Failed to enqueue input tensors");
  GE_CHK_STATUS_RET(DequeueOutputTensors(outputs), "Failed to dequeue output tensors");
  GELOGD("Execute model successfully, model id = %u", model_id_);
  return SUCCESS;
}

Status HeterogeneousModelExecutor::EnqueueInputTensors(const std::vector<GeTensor> &inputs) {
  GELOGD("Start to enqueue inputs, size = %zu", inputs.size());
  GE_CHK_STATUS_RET_NOLOG(ValidateInputTensors(inputs));
  for (size_t i = 0U; i < inputs.size(); ++i) {
    const auto &input = inputs[i];
    const uint32_t queue_id = input_queue_ids_[i];
    const auto tensor_data = input.GetData().GetData();
    const auto tensor_size = input.GetData().GetSize();
    // Enqueue tensor
    if (input_is_no_tiling_[i]) {
      const std::vector<int64_t> input_shape = input.GetTensorDesc().GetShape().GetDims();
      const DataType type = input.GetTensorDesc().GetDataType();
      const ExchangeService::FillFunc fill_func = [&input_shape, type, tensor_data, tensor_size](void *buffer,
                                                                                                 const size_t size) {
        RuntimeTensorDesc * const mbuf_tensor_desc = PtrToPtr<void, RuntimeTensorDesc>(buffer);
        mbuf_tensor_desc->shape[0U] = static_cast<int64_t>(input_shape.size());
        for (size_t j = 0U; j < input_shape.size(); ++j) {
          mbuf_tensor_desc->shape[j + 1U] = input_shape[j];
        }
        mbuf_tensor_desc->dtype = static_cast<int64_t>(type);
        uint8_t *const mbuf_tensor_data = PtrAdd<uint8_t>(PtrToPtr<void, uint8_t>(buffer),
                                                          sizeof(RuntimeTensorDesc) + 1UL, sizeof(RuntimeTensorDesc));
        if (memcpy_s(mbuf_tensor_data, size - sizeof(RuntimeTensorDesc), tensor_data, tensor_size) != EOK) {
          GELOGE(FAILED, "Failed to copy mbuf data, mbuf size:%zu, tensor size:%zu", size, tensor_size);
          return FAILED;
        }
        mbuf_tensor_desc->data_addr = PtrToValue(mbuf_tensor_data);
        return SUCCESS;
      };
      GE_CHK_STATUS_RET(exchange_service_->Enqueue(static_cast<int32_t>(device_id_), queue_id,
                                                   tensor_size + sizeof(RuntimeTensorDesc), fill_func),
                        "Failed to enqueue input[%zu], model id = %u, queue id = %u",
                        i, model_id_, queue_id);
    } else {
      GE_CHK_STATUS_RET(
          exchange_service_->Enqueue(static_cast<int32_t>(device_id_), queue_id, tensor_data, tensor_size),
          "Failed to enqueue input[%zu], model id = %u, queue id = %u", i, model_id_, queue_id);
    }
    GELOGD("Enqueue input[%zu] successfully, model id = %u, queue id = %u, size = %zu, notiling = %d",
           i, model_id_, queue_id, tensor_size, static_cast<int32_t>(input_is_no_tiling_[i]));
  }

  for (size_t i = 0U; i < control_input_queue_ids_.size(); ++i) {
    const uint32_t queue_id = control_input_queue_ids_[i];
    const int32_t control_value = 0;
    GE_CHK_STATUS_RET(
        exchange_service_->Enqueue(static_cast<int32_t>(device_id_), queue_id, &control_value, sizeof(control_value)),
        "Failed to enqueue control input[%zu], model id = %u, queue id = %u", i, model_id_, queue_id);
    GELOGD("Enqueue control input[%zu] successfully, model id = %u, queue id = %u", i, model_id_, queue_id);
  }
  return SUCCESS;
}

Status HeterogeneousModelExecutor::ValidateInputTensors(const std::vector<GeTensor> &inputs) {
  if (inputs.size() != input_queue_ids_.size()) {
    GELOGE(PARAM_INVALID,
           "Number of inputs (%zu) mismatches that of model inputs (%zu).",
           inputs.size(),
           input_queue_ids_.size());
    return PARAM_INVALID;
  }

  for (size_t i = 0U; i < inputs.size(); ++i) {
    const auto &input = inputs[i];
    const auto tensor_size = input.GetData().GetSize();
    GE_CHECK_LE(static_cast<int64_t>(tensor_size), INT64_MAX);
    if (static_cast<int64_t>(tensor_size) > input_tensor_sizes_[i]) {
      GELOGE(PARAM_INVALID,
             "Model[%u] validate input tensor[%zu] failed, expect = %zu, but given = %zu",
             model_id_,
             i,
             input_tensor_sizes_[i],
             tensor_size);
      return PARAM_INVALID;
    }
  }
  return SUCCESS;
}

Status HeterogeneousModelExecutor::DequeueOutputTensors(std::vector<GeTensor> &outputs) {
  size_t end_of_sequence_num = 0UL;
  for (size_t i = 0U; i < output_queue_ids_.size(); ++i) {
    // Dequeue tensor
    const auto queue_id = output_queue_ids_[i];
    const auto device = static_cast<int32_t>(device_id_);
    auto output_tensor_raw_size = output_tensor_raw_sizes_[i];
    const auto &output_tensor_desc = output_tensor_desc_[i];
    GeTensor output_tensor(*output_tensor_desc);
    ControlInfo control_Info = {false};
    if (output_is_no_tiling_[i]) {
      GE_CHK_STATUS_RET(exchange_service_->DequeueTensor(device, queue_id, output_tensor, control_Info),
                        "Failed to dequeue no tiling output[%zu], model id = %u, queue id = %u", i, model_id_,
                        queue_id);
    } else {
      GELOGD("MakeShared start size=%ld", output_tensor_raw_size);
      auto aligned_ptr = MakeShared<AlignedPtr>(output_tensor_raw_size, kAlignmentVal64);
      GELOGD("MakeShared end size=%ld", output_tensor_raw_size);
      GE_CHECK_NOTNULL(aligned_ptr);
      GE_CHK_STATUS_RET(exchange_service_->Dequeue(device,
                                                   queue_id,
                                                   aligned_ptr->MutableGet(),
                                                   static_cast<size_t>(output_tensor_raw_size),
                                                   control_Info),
                        "Failed to dequeue output[%zu], model id = %u, queue id = %u",
                        i, model_id_, queue_id);
      output_tensor.SetData(aligned_ptr, static_cast<uint64_t>(output_tensor_raw_size));
    }
    GELOGD("Dequeue output[%zu] successfully, model id = %u, queue id = %u, size = %ld, notiling = %d",
           i, model_id_, queue_id, output_tensor_raw_size, static_cast<int32_t>(output_is_no_tiling_[i]));
    if (control_Info.end_of_sequence_flag) {
      end_of_sequence_num++;
      continue;
    }
    outputs.emplace_back(std::move(output_tensor));
  }
  if (end_of_sequence_num > 0U) {
    GELOGI("return end of sequence.");
    return END_OF_SEQUENCE;
  }
  return SUCCESS;
}

Status HeterogeneousModelExecutor::ModelRunStart() {
  GELOGI("model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
  const std::lock_guard<std::mutex> lk(mu_);
  run_flag_ = true;
  run_thread_ = std::thread([&]() {
    Run();
  });
  return SUCCESS;
}

void HeterogeneousModelExecutor::Run() {
  GELOGD("Run thread started, model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
  while (run_flag_) {
    std::shared_ptr<RunAsyncRequest> request;
    if ((!input_queue_.Pop(request)) || (request == nullptr)) {
      GELOGI("Got end of inputs, model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
      break;
    }

    std::vector<GeTensor> output_tensors;
    std::vector<Tensor> outputs;
    const auto ret = DequeueOutputTensors(output_tensors);
    if (ret == SUCCESS) {
      for (auto &output : output_tensors) {
        outputs.emplace_back(TensorAdapter::AsTensor(output));
      }
    } else if (ret == END_OF_SEQUENCE) {
      GELOGI("end of sequence is coming.");
    } else {
      GELOGE(ret, "Failed to execute model, model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
      run_flag_ = false;
    }
    request->callback(ret, outputs);
  }
  GELOGD("Run thread ended, model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
}

Status HeterogeneousModelExecutor::ModelRunStop() {
  GELOGI("model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
  const std::lock_guard<std::mutex> lk(mu_);
  if (!run_flag_) {
    GELOGD("Not started");
    return SUCCESS;
  }

  run_flag_ = false;
  (void) input_queue_.Push(nullptr);
  if (run_thread_.joinable()) {
    run_thread_.join();
  }
  GELOGI("Model run stopped, model id = %u, deployed model id = %u", model_id_, deployed_model_id_);
  return SUCCESS;
}

void HeterogeneousModelExecutor::SetModelId(const uint32_t model_id) {
  model_id_ = model_id;
}

void HeterogeneousModelExecutor::SetDeviceId(const uint32_t device_id) {
  device_id_ = device_id;
}
}  // namespace ge
