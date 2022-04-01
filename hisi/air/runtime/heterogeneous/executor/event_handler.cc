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

#include "executor/event_handler.h"
#include "executor/executor_context.h"
#include "mmpa/mmpa_api.h"

namespace ge {
Status EventHandler::Initialize() {
  context_ = MakeUnique<ExecutorContext>();
  GE_CHECK_NOTNULL(context_);
  return SUCCESS;
}

void EventHandler::HandleEvent(deployer::ExecutorRequest &request,
                               deployer::ExecutorResponse &response) {
  if (request.has_pre_download_message()) {
    HandlePreDownloadRequest(request, response);
  } else if (request.has_download_model_message()) {
    HandleDownloadRequest(request, response);
  } else if (request.has_load_model_message()) {
    HandleLoadRequest(request, response);
  } else if (request.has_unload_model_message()) {
    HandleUnloadRequest(request, response);
  } else if (request.has_multi_var_manager_info()) {
    HandleInitVarManagerRequest(request, response);
  } else if (request.has_shared_content_desc()) {
    HandleFillSharedContent(request, response);
  } else if (request.has_deploy_rank_table_message()) {
    HandleDeployRankTable(request, response);
  } else {
    GELOGE(PARAM_INVALID, "[Handle][Event] failed, request has no content");
    response.set_status_code(UNSUPPORTED);
    response.set_error_message("request has no content");
  }
}

void EventHandler::HandlePreDownloadRequest(deployer::ExecutorRequest &request,
                                            deployer::ExecutorResponse &response) {
  auto &pre_download_req = request.pre_download_message();
  auto sub_model_id = pre_download_req.model_id();
  auto root_model_id = pre_download_req.root_model_id();
  auto model_size = pre_download_req.model_size();
  auto ret = context_->AddModel(root_model_id, sub_model_id, model_size);
  response.set_status_code(ret);
  GELOGD("[Handle][PreDownloadModel] success");
}

void EventHandler::HandleDownloadRequest(deployer::ExecutorRequest &request,
                                         deployer::ExecutorResponse &response) const {
  auto &download_req = request.download_model_message();
  auto sub_model_id = download_req.model_id();
  auto root_model_id = download_req.root_model_id();
  ExecutorContext::ModelHandle *handle = nullptr;
  if (context_->GetModel(root_model_id, sub_model_id, &handle) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Failed to get model");
    return;
  }

  auto offset = download_req.offset();
  auto &model_data = download_req.model_data();
  if (handle->ParsePartialModel(offset, model_data.data(), model_data.size()) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Parse model failed");
    return;
  }

  response.set_status_code(SUCCESS);
  GELOGD("[Handle][DownloadModel] success");
}

void EventHandler::HandleLoadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) const {
  auto &load_req = request.load_model_message();
  auto sub_model_id = load_req.model_id();
  auto root_model_id = load_req.root_model_id();
  ExecutorContext::ModelHandle *handle = nullptr;
  if (context_->GetModel(root_model_id, sub_model_id, &handle) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Failed to get model");
    return;
  }

  std::vector<uint32_t> input_queues(load_req.input_queues().begin(), load_req.input_queues().end());
  std::vector<uint32_t> output_queues(load_req.output_queues().begin(), load_req.output_queues().end());
  if (handle->LoadModel(input_queues, output_queues) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Load model failed");
    return;
  }

  response.set_status_code(SUCCESS);
  GELOGD("[Handle][LoadModel] success");
}

void EventHandler::HandleUnloadRequest(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) const {
  auto &unload_req = request.unload_model_message();
  auto root_model_id = unload_req.model_id();
  std::map<uint32_t, std::unique_ptr<ExecutorContext::ModelHandle>> *submodel_map = nullptr;
  if (context_->GetModel(root_model_id, submodel_map) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Failed to get model");
    return;
  }

  std::vector<uint32_t> failed;
  for (auto it = submodel_map->begin(); it != submodel_map->end();) {
    ExecutorContext::ModelHandle *handle = it->second.get();
    if (handle->UnloadModel() != SUCCESS) {
      failed.emplace_back(it->first);
    }
    it = submodel_map->erase(it);
  }

  if (!failed.empty()) {
    response.set_status_code(FAILED);
    const std::string error_msg = "Unload model failed, failed model id = " + ToString(failed);
    response.set_error_message(error_msg);
    GELOGW("%s", error_msg.c_str());
    return;
  }

  response.set_status_code(SUCCESS);
  GELOGD("[Handle][UnloadModel] success");
}

void EventHandler::HandleInitVarManagerRequest(deployer::ExecutorRequest &request,
                                               deployer::ExecutorResponse &response) {
  GELOGD("[Handle][Init VarManager] begin.");
  auto &multi_var_manager_info = request.multi_var_manager_info();
  if (context_->InitVarManager(multi_var_manager_info) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Failed to init VarManger");
    return;
  }
  response.set_status_code(SUCCESS);
  GELOGD("[Handle][Init VarManager] success.");
}

void EventHandler::HandleFillSharedContent(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) {
  GELOGD("[Handle][Fill Shared_content] begin.");
  auto &shared_content_desc = request.shared_content_desc();
  if (context_->FillSharedContent(shared_content_desc) != SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Failed to fill shared content.");
    return;
  }
  response.set_status_code(SUCCESS);
  GELOGD("[Handle][Fill Shared_content] success.");
}

void EventHandler::HandleDeployRankTable(deployer::ExecutorRequest &request, deployer::ExecutorResponse &response) {
  auto &deploy_rank_table_req = request.deploy_rank_table_message();
  const auto &rank_table = deploy_rank_table_req.rank_table();
  auto rank_id = deploy_rank_table_req.rank_id();

  GELOGD("[Handle][RankTable] begin. rank table=%s", rank_table.c_str());
  const std::string hccl_path = "/usr/local/Ascend/lib64/libhccl.so";
  const auto handle = mmDlopen(hccl_path.c_str(), static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                                                       static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  GE_CHECK_NOTNULL_JUST_RETURN(handle);
  GE_MAKE_GUARD(not_used_var, [&handle] {
    if (mmDlclose(handle) != 0) {
      GELOGW("Failed to close handle %s", mmDlerror());
    }
  });

  const auto hcom_init_by_string =
      reinterpret_cast<HcclResult(*)(const char*, const char*)>(mmDlsym(handle, "HcomInitByString"));
  if (hcom_init_by_string == nullptr) {
    response.set_status_code(FAILED);
    response.set_error_message("Symbol HcomInitByString is null.");
    return;
  }

  auto ret = hcom_init_by_string(rank_table.c_str(), rank_id.c_str());
  if (ret != HCCL_SUCCESS) {
    response.set_status_code(FAILED);
    response.set_error_message("Hcom init failed.");
    return;
  }

  response.set_status_code(SUCCESS);
  GELOGD("[Handle][RankTable] success. rank table=%s", rank_table.c_str());
}
}  // namespace ge
