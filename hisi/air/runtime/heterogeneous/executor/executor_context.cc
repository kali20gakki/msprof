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

#include "executor/executor_context.h"
#include "executor/cpu_sched_event_dispatcher.h"
#include "framework/executor/ge_executor.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/ge_context.h"
#include "graph/manager/graph_mem_manager.h"
#include "runtime/mem.h"
#include "runtime/dev.h"
#include "securec.h"

namespace ge {
std::unique_ptr<ExecutorContext::ModelHandle> ExecutorContext::CreateModelHandle(uint64_t model_size) const {
  return MakeUnique<ModelHandle>(model_size);
}

Status ExecutorContext::AddModel(uint32_t root_model_id, uint32_t sub_model_id, uint64_t model_size) {
  auto handle = CreateModelHandle(model_size);
  GE_CHECK_NOTNULL(handle);
  auto it = models_.find(root_model_id);
  if (it == models_.end()) {
    std::map<uint32_t, std::unique_ptr<ModelHandle>> submodel_map;
    submodel_map.emplace(sub_model_id, std::move(handle));
    models_.emplace(root_model_id, std::move(submodel_map));
  } else {
    auto &submodel_map = it->second;
    auto sub_iter = submodel_map.find(sub_model_id);
    if (sub_iter != submodel_map.end()) {
      GELOGE(FAILED, "[Add][Model] failed, model already added, sub_model_id = %u", sub_model_id);
      return FAILED;
    }
    submodel_map.emplace(sub_model_id, std::move(handle));
  }

  GELOGD("[Add][Model] succeeded, root_model_id = %u, sub_model_id = %u", root_model_id, sub_model_id);
  return SUCCESS;
}

Status ExecutorContext::GetModel(uint32_t root_model_id, uint32_t sub_model_id, ExecutorContext::ModelHandle **handle) {
  auto it = models_.find(root_model_id);
  if (it == models_.end()) {
    GELOGE(FAILED, "[Get][Model] failed, root model id = %u", root_model_id);
    return FAILED;
  }

  auto &submodel_map = it->second;
  auto iter = submodel_map.find(sub_model_id);
  if (iter == submodel_map.end()) {
    GELOGE(FAILED, "[Get][Model] failed, sub model id = %u", sub_model_id);
    return FAILED;
  }
  *handle = iter->second.get();
  return SUCCESS;
}

Status ExecutorContext::GetModel(uint32_t root_model_id,
                                 std::map<uint32_t, std::unique_ptr<ModelHandle>> *&submodel_map) {
  auto it = models_.find(root_model_id);
  if (it == models_.end()) {
    GELOGE(FAILED, "[Get][Model] failed, root model id = %u", root_model_id);
    return FAILED;
  }

  submodel_map = &(it->second);
  return SUCCESS;
}

ExecutorContext::ModelHandle::ModelHandle(uint64_t model_size) : model_parser_(model_size) {
}

ExecutorContext::ModelHandle::~ModelHandle() {
  if (loaded_) {
    (void) UnloadModel();
  }
}

Status ExecutorContext::ModelHandle::ParsePartialModel(uint64_t offset,
                                                       const void *model_buffer,
                                                       uint64_t buffer_size) {
  return model_parser_.ParseAndDeserialize(offset, model_buffer, buffer_size);
}

Status ExecutorContext::ModelHandle::LoadModel(const vector<uint32_t> &input_queues,
                                               const vector<uint32_t> &output_queues) {
  if (loaded_) {
    GELOGW("Already loaded");
    return SUCCESS;
  }

  GeRootModelPtr root_model;
  GE_CHK_STATUS_RET(model_parser_.GetModel(root_model), "Failed to get model from parser");
  GE_CHK_STATUS_RET_NOLOG(root_model->CheckIsUnknownShape(is_dynamic_));
  GE_CHK_STATUS_RET(DoLoadModel(root_model, input_queues, output_queues));
  loaded_ = true;
  return SUCCESS;
}

Status ExecutorContext::ModelHandle::UnloadModel() {
  if (dynamic_model_executor_ != nullptr) {
    dynamic_model_executor_->UnloadModel();
    dynamic_model_executor_.reset();
  } else {
    GE_CHK_STATUS_RET(DoUnloadModel(inner_model_id_),
                      "[Unload][Model] failed, model_id = %u", inner_model_id_);
    GELOGD("[Unload][Model] success, model_id = %u", inner_model_id_);
    inner_model_id_ = UINT32_MAX;
  }
  loaded_ = false;
  return SUCCESS;
}

Status ExecutorContext::ModelHandle::DoLoadModel(const std::shared_ptr<GeRootModel> &root_model,
                                                 const vector<uint32_t> &input_queues,
                                                 const vector<uint32_t> &output_queues) {
  bool is_host = GetContext().GetHostExecFlag();
  if (is_dynamic_ || is_host) {
    GELOGD("Load dynamic model");
    dynamic_model_executor_ = MakeUnique<DynamicModelExecutor>(is_host);
    GE_CHECK_NOTNULL(dynamic_model_executor_);
    GE_CHK_STATUS_RET_NOLOG(dynamic_model_executor_->LoadModelWithQueue(root_model, input_queues, output_queues));
    GELOGD("[LoadDynamicModelWithQ] success");
    return SUCCESS;
  }

  GELOGD("Load static model");
  GeExecutor executor;
  if (input_queues.empty() && output_queues.empty()) {
    // it is temporary solution for diagrams without input and output queue in helper.
    GE_CHK_STATUS_RET(executor.LoadModelWithoutQ(inner_model_id_, root_model), "[LoadModelWithoutQ] failed");
  } else {
    GE_CHK_STATUS_RET(executor.LoadModelWithQ(inner_model_id_, root_model, input_queues, output_queues),
                      "[LoadModelWithQ] failed");
  }
  GELOGD("[LoadModelWithQ] success, model_id = %u", inner_model_id_);
  return SUCCESS;
}

Status ExecutorContext::ModelHandle::DoUnloadModel(const uint32_t model_id) const {
  return GeExecutor().UnloadModel(model_id);
}

Status ExecutorContext::InitVarManager(const deployer::MultiVarManagerInfo &multi_var_manager) {
  GELOGI("Begin to init var manager, number is %d.", multi_var_manager.var_manager_info_size());
  for (int64_t i = 0; i < multi_var_manager.var_manager_info_size(); i++) {
    const deployer::VarManagerInfo &single_info = multi_var_manager.var_manager_info(i);
    uint64_t session_id = single_info.session_id();
    GELOGI("Init var manager, session id is %zu.", session_id);
    auto ret = VarManager::Instance(session_id)->VarManagerToDeserial(session_id, single_info);
    if (ret != SUCCESS) {
      GELOGE(ret, "Failed to init var manager, session id is %zu.", session_id);
      return ret;
    }
  }
  GELOGI("Success to init var manager.");
  return SUCCESS;
}

Status ExecutorContext::FillSharedContent(const deployer::SharedContentDescription &shared_content_desc) {
  uint64_t session_id = shared_content_desc.session_id();
  uint64_t head_offset = shared_content_desc.head_offset();
  uint64_t total_length = shared_content_desc.total_length();
  uint64_t current_offset = shared_content_desc.current_offset();
  uint32_t memory_type = shared_content_desc.mem_type();
  const std::string &node_name = shared_content_desc.node_name();

  const proto::TensorDescriptor &tensor_desc_proto = shared_content_desc.tensor_desc();
  GeTensorDesc tensor_desc;
  GeTensorSerializeUtils::AssembleGeTensorDescFromProto(&tensor_desc_proto, tensor_desc);

  const std::vector<rtMemType_t> mem_type {RT_MEMORY_HBM, RT_MEMORY_P2P_DDR};
  Status status = MemManager::Instance().Initialize(mem_type);
  if (status != SUCCESS) {
    GELOGE(status, "[Init][MemManager] MemoryAllocatorManager initialize failed.");
    REPORT_CALL_ERROR("E19999", "MemManager initialize failed.");
    return status;
  }

  VarManager::Instance(session_id)->SetMemManager(&MemManager::Instance());
  int32_t device_id = -1;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  GELOGD("Success to get device id = %u", device_id);
  const uint32_t dev_id = static_cast<uint32_t>(device_id);
  const uint8_t *var_mem_base = VarManager::Instance(session_id)->GetVarMemoryBase(RT_MEMORY_HBM, dev_id);
  if (var_mem_base == nullptr) {
    size_t var_mem_max_size  = VarManager::Instance(session_id)->GetVarMemMaxSize();
    status = VarManager::Instance(session_id)->MallocVarMemory(var_mem_max_size, dev_id);
    if (status != SUCCESS) {
      GELOGE(status, "MallocVarMemory fail, var_size is %zu, session id is %zu.", var_mem_max_size, session_id);
      REPORT_CALL_ERROR("E19999", "MemManager initialize failed.");
      return status;
    }
    GELOGI("Success to malloc hbm memory, size is %zu, session is %lu.", var_mem_max_size, session_id);
  }

  GELOGI("Begin to copy shared content to memory, var is %s.", node_name.c_str());
  uint8_t *logic_addr = reinterpret_cast<uint8_t *>(head_offset);
  uint8_t *dev_mem = VarManager::Instance(session_id)->GetVarMemoryAddr(logic_addr, memory_type, dev_id);
  if (dev_mem == nullptr) {
    GELOGE(INTERNAL_ERROR, "[GetVarMemoryAddr] Failed to get addr of var(%s).", node_name.c_str());
    REPORT_CALL_ERROR("E19999", "[GetVarMemoryAddr] Failed to get addr of var(%s).", node_name.c_str());
    return INTERNAL_ERROR;
  }

  uint32_t destMax =
    (total_length - current_offset <= SECUREC_MEM_MAX_LEN) ? (total_length - current_offset) : SECUREC_MEM_MAX_LEN;
  GE_CHK_RT_RET(rtMemcpy(dev_mem + current_offset, destMax, shared_content_desc.om_content().data(),
                         shared_content_desc.om_content().size(), RT_MEMCPY_HOST_TO_DEVICE));
  GELOGI("Success to copy shared content to memory, var is %s, size is %zu.", node_name.c_str(),
         shared_content_desc.om_content().size());
  if (current_offset + shared_content_desc.om_content().size() >= total_length) {
    VarManager::Instance(session_id)->SetVarIsReady(node_name, tensor_desc);
  }
  return SUCCESS;
}
}  // namespace ge
