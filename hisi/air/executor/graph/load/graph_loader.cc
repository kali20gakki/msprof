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

#include "graph/load/graph_loader.h"

#include "common/model_parser/model_parser.h"
#include "graph/ge_context.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/model_utils.h"
#include "common/thread_pool.h"

namespace ge {
Status GraphLoader::UnloadModel(const uint32_t model_id) {
  if (model_id == INVALID_MODEL_ID) {
    return SUCCESS;
  }
  GELOGI("UnLoad model begin, model id:%u.", model_id);
  GE_CHK_STATUS(ModelManager::GetInstance().Stop(model_id), "[Stop][Model] failed. model id:%u", model_id);

  GE_CHK_STATUS_RET(ModelManager::GetInstance().Unload(model_id), "[Unload][Model] failed. model id:%u", model_id);

  GELOGI("UnLoad model success, model id:%u.", model_id);
  return SUCCESS;
}

Status GraphLoader::LoadModelOnline(uint32_t &model_id, const GeRootModelPtr &ge_root_model,
                                    const GraphNodePtr &graph_node, const uint32_t device_id,
                                    const error_message::Context &error_context, const int64_t die_id) {
  ErrorManager::GetInstance().SetErrorContext(error_context);
  GELOGI("Load model online begin.");
  if (ge_root_model == nullptr) {
    REPORT_INNER_ERROR("E19999", "Check param ge_root_model_ptr nullptr, check invalid");
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "[LoadGraph][Check][Param] GE load graph model_ptr is nullptr.");
    return GE_GRAPH_PARAM_NULLPTR;
  }

  GE_CHK_STATUS_RET(ModelUtils::SetDevice(device_id, die_id), "[Call][SetDevice] failed, device_id:%u", device_id);
  GE_MAKE_GUARD(reset_device, [&device_id]() {
    GE_CHK_STATUS(ModelUtils::ResetDevice(device_id));
  });

  auto &model_mgr = ModelManager::GetInstance();
  GE_CHK_STATUS_RET_NOLOG(model_mgr.LoadModelOnline(model_id, ge_root_model, graph_node, device_id, die_id));

  ge_root_model->SetModelId(model_id);
  if (ge_root_model->IsSpecificStream()) {
    GELOGI("No need to start a new thread to run model in specific scene.");
    return SUCCESS;
  }
  const auto ret = model_mgr.Start(model_id);
  if (ret != SUCCESS) {
    GE_CHK_STATUS(model_mgr.Unload(model_id), "[Unload][Model] failed after start failed, model_id:%u.", model_id);
    GELOGE(ret, "[Start][Model] failed, model_id:%u.", model_id);
    return ret;
  }
  GELOGI("Load model online success, model_id:%u.", model_id);
  return SUCCESS;
}

Status GraphLoader::MultiLoadModelOnline(const GeRootModelPtr &ge_root_model, const GraphNodePtr &graph_node,
                                         const std::vector<NamedAttrs> &deploy_info) {
  // get deploy number of model instance
  const auto root_graph = ge_root_model->GetRootGraph();
  std::vector<size_t> deploy_id;
  const auto thread_instances_size = ModelUtils::GetDeployNum(ge_root_model, deploy_id);
  const auto device_id_fission_from = GetContext().DeviceId();
  GELOGI("Graph %s need to load model %zu times, and fission from device %u, multi deploy size %zu.",
         root_graph->GetName().c_str(), thread_instances_size, device_id_fission_from, deploy_info.size());

  ThreadPool executor(static_cast<size_t>(thread_instances_size));
  std::vector<std::future<Status>> vector_future;
  for (size_t i = 0U; i < thread_instances_size; ++i) {
    const auto thread_instance = deploy_info[deploy_id[i]];
    std::string device_type = "SingleMode";
    int64_t device_id_fissioned = std::numeric_limits<int64_t>::max();
    if (AttrUtils::GetStr(thread_instance, ATTR_NAME_DEPLOY_DEVICE_TYPE, device_type) && (device_type == "MultiMode")) {
      if ((!AttrUtils::GetInt(thread_instance, ATTR_NAME_DEPLOY_DEVICE_ID, device_id_fissioned)) ||
          (device_id_fissioned == std::numeric_limits<int64_t>::max())) {
        REPORT_CALL_ERROR("E19999", "graph %s has invalid deploy attr _device_id", root_graph->GetName().c_str());
        GELOGE(GRAPH_FAILED, "graph %s has invalid deploy attr _device_id", root_graph->GetName().c_str());
        return GRAPH_FAILED;
      }
    }

    uint32_t model_id = INVALID_MODEL_ID;
    const error_message::Context &error_context = ErrorManager::GetInstance().GetErrorManagerContext();
    std::future<Status> f = executor.commit(&GraphLoader::LoadModelOnline, model_id, ge_root_model, graph_node,
                                            device_id_fission_from, error_context, device_id_fissioned);

    if (!f.valid()) {
      GELOGE(FAILED, "[Call][Commit] failed, Future is invalid");
      return FAILED;
    }
    vector_future.emplace_back(std::move(f));
  }

  Status status = SUCCESS;
  for (size_t i = 0U; i < vector_future.size(); ++i) {
    GE_CHK_STATUS_EXEC(vector_future[i].get(), status = _chk_status, "Load multi model %zu failed", i);
  }
  return status;
}

Status GraphLoader::LoadDataFromFile(const std::string &path, const int32_t priority, ModelData &model_data) {
  if (!CheckInputPathValid(path, "model_file")) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID, "[Check][Param] model path is invalid:%s", path.c_str());
    return ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID;
  }

  GELOGI("Load model begin, model path is: %s", path.c_str());

  const Status ret = ModelParserBase::LoadFromFile(path.c_str(), priority, model_data);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][LoadFromFile] failed. ret = %u, path:%s", ret, path.c_str());
    if (model_data.model_data != nullptr) {
      delete[] static_cast<char_t *>(model_data.model_data);
      model_data.model_data = nullptr;
    }
  }
  return ret;
}

Status GraphLoader::CommandHandle(const Command &command) {
  try {
    const Status ret = ModelManager::GetInstance().HandleCommand(command);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Handle][Command] failed, module_index:%lu.", command.module_index);
      return ret;
    }
  } catch (std::bad_alloc &) {
    REPORT_INNER_ERROR("E19999", "Bad memory allocation occur");
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Handle][Command] failed, "
           "bad memory allocation occur, module_index:%lu.", command.module_index);

    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  } catch (...) {
    REPORT_INNER_ERROR("E19999", "Some exceptions occur");
    GELOGE(FAILED, "[Handle][Command] failed, some exceptions occur, module_index:%lu.", command.module_index);

    return FAILED;
  }

  return SUCCESS;
}

Status GraphLoader::LoadModelFromData(uint32_t &model_id, const ModelData &model_data, const uintptr_t mem_ptr,
                                      const size_t mem_size, const uintptr_t weight_ptr, const size_t weight_size) {
  GELOGI("Load model begin, model_id:%u.", model_id);
  // For ACL, Open Device from App.
  const auto ret = ModelManager::GetInstance().LoadModelOffline(model_id, model_data, nullptr,
                                                                mem_ptr, mem_size, weight_ptr, weight_size);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] failed, model_id:%u.", model_id);
    return ret;
  }
  GELOGI("Load model success, model_id:%u.", model_id);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Load task list from ModelData with queue.
/// @param [out] model_id: model id allocate from manager.
/// @param [in] model_data: Model data load from offline model.
/// @param [in] input_queue_ids: input queue ids create from user.
/// @param [in] output_queue_ids: input queue ids create from user.
/// @return: 0 for success / others for fail
///
Status GraphLoader::LoadModelWithQ(uint32_t &model_id, const ModelData &model_data,
                                   const std::vector<uint32_t> &input_queue_ids,
                                   const std::vector<uint32_t> &output_queue_ids) {
  GELOGI("Load model with queue begin, model_id:%u.", model_id);

  // For ACL, Open Device from App.
  const auto ret = ModelManager::GetInstance().LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] with queue failed, model_id:%u.", model_id);
    return ret;
  }

  GELOGI("Load model with queue success, model_id:%u.", model_id);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Load task list from root model without input queue and output queue.
/// @param [out] model_id: model id allocate from manager.
/// @param [in] root_model: instance of GeRootModel.
/// @return: 0 for success / others for fail
///
Status GraphLoader::LoadModelWithoutQ(uint32_t &model_id, const GeRootModelPtr &root_model) {
  GELOGI("Load model without queue begin, model_id: %u.", model_id);

  const auto ret = ModelManager::GetInstance().LoadModelWithoutQ(model_id, root_model);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] without queue failed, model_id:%u.", model_id);
    return ret;
  }

  GELOGI("Load model without queue success, model_id: %u.", model_id);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Load task list from ModelData with queue.
/// @param [out] model_id: model id allocate from manager.
/// @param [in] root_model: instance of GeRootModel.
/// @param [in] input_queue_ids: input queue ids create from user.
/// @param [in] output_queue_ids: input queue ids create from user.
/// @return: 0 for success / others for fail
///
Status GraphLoader::LoadModelWithQ(uint32_t &model_id,
                                   const GeRootModelPtr &root_model,
                                   const vector<uint32_t> &input_queue_ids,
                                   const vector<uint32_t> &output_queue_ids,
                                   const bool need_update_session_id) {
  GELOGI("Load model with queue begin, model_id:%u.", model_id);
  // For ACL, Open Device from App.
  const auto ret = ModelManager::GetInstance().LoadModelWithQ(model_id, root_model, input_queue_ids, output_queue_ids,
                                                              0, need_update_session_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] with queue failed, model_id:%u.", model_id);
    return ret;
  }

  GELOGI("Load model with queue success, model_id:%u.", model_id);
  return SUCCESS;
}

///
/// @ingroup domi_ome
/// @brief  execute model
/// @param [in] model_id  model id
/// @param [in] stream   stream to execute model on
/// @param [in] async_mode  is asynchronize mode.
/// @param [in] input_data  model input data
/// @param [in] input_desc  description of model input data
/// @param [out] output_data  model output data
/// @param [out] output_desc  description of model output data
///
Status GraphLoader::ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                                 const InputData &input_data, const std::vector<GeTensorDesc> &input_desc,
                                 OutputData &output_data, std::vector<GeTensorDesc> &output_desc) {
  const auto ret = ModelManager::GetInstance().ExecuteModel(model_id, stream, async_mode,
                                                            input_data, input_desc, output_data, output_desc);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Execute][Model] failed, model_id:%u.", model_id);
    return ret;
  }

  GELOGD("Execute model success, model_id:%u.", model_id);
  return SUCCESS;
}
}  // namespace ge
