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

#include "framework/executor/ge_executor.h"

#include <ctime>
#include "framework/common/debug/log.h"
#include "common/plugin/ge_util.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling/profiling_properties.h"
#include "common/dump/dump_manager.h"
#include "exec_runtime/execution_runtime.h"
#include "graph/execute/graph_execute.h"
#include "graph/load/graph_loader.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/host_mem_manager.h"
#include "single_op/single_op_manager.h"
#include "common/plugin/plugin_manager.h"
#include "graph/opsproto_manager.h"
#include "local_engine/engine/host_cpu_engine.h"
#include "runtime/base.h"
#include "runtime/config.h"
#include "common/profiling/command_handle.h"
#include "runtime/local_execution_runtime.h"
#include "common/profiling_definitions.h"
#include "common/ge_inner_attrs.h"
#include "hybrid/common/npu_memory_allocator.h"
#include "opskernel_executor/ops_kernel_executor_manager.h"


namespace {
const size_t kDynamicBatchSizeVecSize = 1U;
const size_t kStaticBatchInfoSize = 1U;
const size_t kDynamicImageSizeVecSize = 2U;
const size_t kDynamicImageSizeInputSize = 2U;
const std::string kBatchLabel = "Batch_";

void GetGeTensorDescFromDomiInfo(std::vector<ge::TensorDesc> &ge_descs,
                                 const std::vector<ge::InputOutputDescInfo> &domi_descs,
                                 const std::vector<uint32_t> &formats_vec) {
  size_t idx = 0U;
  for (const auto &desc_item : domi_descs) {
    ge::TensorDesc ge_desc;
    ge_desc.SetName(desc_item.name.c_str());
    ge_desc.SetDataType(static_cast<ge::DataType>(desc_item.data_type));
    ge_desc.SetFormat(static_cast<ge::Format>(formats_vec[idx]));
    std::vector<int64_t> shape_dims;
    for (const auto &dim : desc_item.shape_info.dims) {
      shape_dims.push_back(dim);
    }
    const ge::Shape ge_shape(shape_dims);
    ge_desc.SetShape(ge_shape);
    ge_desc.SetSize(static_cast<int64_t>(desc_item.size));
    (void)ge_desc.SetShapeRange(desc_item.shape_info.shape_ranges);
    ge_descs.emplace_back(ge_desc);
    ++idx;
  }
}

void GetDomiInputData(const ge::RunModelData &input_data, ge::InputData &inputs) {
  inputs.index = input_data.index;
  inputs.model_id = input_data.modelId;
  inputs.timestamp = input_data.timestamp;
  inputs.timeout = input_data.timeout;
  inputs.request_id = input_data.request_id;
  for (const auto &data_item : input_data.blobs) {
    ge::DataBuffer dataBuf{data_item.data, data_item.length, data_item.isDataSupportMemShare};
    inputs.blobs.emplace_back(dataBuf);
  }
}

void GetDomiOutputData(const ge::RunModelData &output_data, ge::OutputData &outputs) {
  outputs.index = output_data.index;
  outputs.model_id = output_data.modelId;
  for (const auto &data_item : output_data.blobs) {
    ge::DataBuffer dataBuf(data_item.data, data_item.length, data_item.isDataSupportMemShare);
    outputs.blobs.emplace_back(dataBuf);
  }
}

void SetDynamicInputDataFlag(const ge::RunModelData &input_data, const std::vector<std::vector<int64_t>> &batch_info,
                             ge::InputData &inputs) {
  inputs.is_dynamic_batch = true;
  std::string batch_label;
  size_t match_idx = 0U;
  for (size_t i = 0U; i < batch_info.size(); ++i) {
    // dynamic_dims
    if (input_data.dynamic_dims.size() != 0U) {
      bool is_match = true;
      for (size_t j = 0U; j < static_cast<size_t>(input_data.dynamic_dims.size()); ++j) {
        if (static_cast<uint64_t>(batch_info[i][j]) != input_data.dynamic_dims[j]) {
          is_match = false;
          break;
        }
      }
      if (is_match) {
        match_idx = i;
        break;
      }
    } else {
      // dynamic_batch_size
      if ((batch_info[i].size() == kDynamicBatchSizeVecSize) &&
          (batch_info[i][0U] == static_cast<int64_t>(input_data.dynamic_batch_size))) {
        match_idx = i;
        break;
      }
      // dynamic_image_size
      if ((batch_info[i].size() == kDynamicImageSizeVecSize) &&
          (batch_info[i][0U] == static_cast<int64_t>(input_data.dynamic_image_height)) &&
          (batch_info[i][1U] == static_cast<int64_t>(input_data.dynamic_image_width))) {
        match_idx = i;
        break;
      }
    }
  }
  batch_label = kBatchLabel + std::to_string(match_idx);
  inputs.batch_label = batch_label;
  GELOGI("current batch label:%s", batch_label.c_str());
}

bool IsDynamicBatchSizeMatchModel(const uint64_t batch_size, const std::vector<std::vector<int64_t>> &batch_info) {
  if (batch_info.empty()) {
    REPORT_INNER_ERROR("E19999", "param Dynamic batch info is empty, check invalid.");
    GELOGE(ge::FAILED, "[Check][Param] Dynamic batch info is empty.");
    return false;
  }

  for (const auto &batch : batch_info) {
    if (batch.size() != kDynamicBatchSizeVecSize) {
      REPORT_INNER_ERROR("E19999", "Dynamic batch param num is %zu, current batch size is %zu.",
                         kDynamicBatchSizeVecSize, batch.size());
      GELOGE(ge::FAILED, "[Check][Param] Dynamic batch param num is %zu, current batch size is %zu.",
             kDynamicBatchSizeVecSize, batch.size());
      return false;
    }
    if (batch[0U] == static_cast<int64_t>(batch_size)) {
      return true;
    }
  }
  REPORT_INNER_ERROR("E19999", "Dynamic batch %lu can not match the gear of model.", batch_size);
  GELOGE(ge::FAILED, "[Check][Param] Dynamic batch %lu can not match the gear of model.", batch_size);
  return false;
}

bool IsDynamicImageSizeMatchModel(const uint64_t image_height, const uint64_t image_width,
                                  const std::vector<std::vector<int64_t>> &batch_info) {
  if (batch_info.empty()) {
    REPORT_INNER_ERROR("E19999", "ParamDynamic batch info is empty. check invalid");
    GELOGE(ge::FAILED, "[Check][Param] Dynamic batch info is empty.");
    return false;
  }

  for (const auto &resolution : batch_info) {
    if (resolution.size() != kDynamicImageSizeVecSize) {
      REPORT_INNER_ERROR("E19999", "Dynamic resolution param num is %zu, current resolution size is %zu.",
                         kDynamicImageSizeVecSize, resolution.size());
      GELOGE(ge::FAILED, "[Check][Param] Dynamic resolution param num is %zu, current resolution size is %zu.",
             kDynamicImageSizeVecSize, resolution.size());
      return false;
    }
    if ((resolution[0U] == static_cast<int64_t>(image_height)) &&
        (resolution[1U] == static_cast<int64_t>(image_width))) {
      return true;
    }
  }
  REPORT_INNER_ERROR("E19999", "Dynamic resolution (%lu,%lu) can not match the gear of model.",
                     image_height, image_width);
  GELOGE(ge::FAILED, "[Check][Param]Dynamic resolution (%lu,%lu) can not match the gear of model.",
         image_height, image_width);
  return false;
}

bool IsDynmaicDimsSizeMatchModel(const std::vector<uint64_t> &cur_dynamic_dims,
                                 const std::vector<std::vector<int64_t>> &batch_info) {
  if (batch_info.empty()) {
    REPORT_INNER_ERROR("E19999", "param batch_info is empty, check invalid");
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] Dynamic batch info is empty.");
    return false;
  }

  bool find_match = false;
  for (const auto &resolution : batch_info) {
    if (cur_dynamic_dims.size() != resolution.size()) {
      REPORT_INNER_ERROR("E19999", "Cur dynamic dims param num is %zu, current resolution size is %zu.",
                         cur_dynamic_dims.size(), resolution.size());
      GELOGE(ACL_ERROR_GE_PARAM_INVALID,
             "[Check][Param] Cur dynamic dims param num is %zu, current resolution size is %zu.",
             cur_dynamic_dims.size(), resolution.size());
      return false;
    }
    bool flag = true;
    for (size_t i = 0U; i < resolution.size(); ++i) {
      if (cur_dynamic_dims[i] != static_cast<uint64_t>(resolution[i])) {
        flag = false;
        break;
      }
    }
    if (flag) {
      find_match = true;
      break;
    }
  }
  if (!find_match) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] choose dynamic dims can not match the gear of model.");
  }
  return find_match;
}

Status InitializeExecutionRuntime(const std::map<std::string, std::string> &options) {
  // check the need for loading helper execution runtime by option
  int32_t heterogeneous_flag = 0;
  if (ge::ExecutionRuntime::IsHeterogeneousEnabled() &&
      (rtGetIsHeterogenous(&heterogeneous_flag) == RT_ERROR_NONE) &&
      (heterogeneous_flag == ge::ExecutionRuntime::kRuntimeTypeHeterogeneous)) {
    GE_CHK_STATUS_RET_NOLOG(ge::ExecutionRuntime::InitHeterogeneousRuntime(options));
  } else {
    const auto local_execution_runtime = ge::MakeShared<ge::LocalExecutionRuntime>();
    GE_CHECK_NOTNULL(local_execution_runtime);
    ge::ExecutionRuntime::SetExecutionRuntime(local_execution_runtime);
  }
  return ge::SUCCESS;
}

// With a dynamic-shaped model, a caller cannot allocate the buffer if no valid output size can be calculated.
// In this case the buffer can be set to NULL and the executor would allocate and cache it till the next invocation.
// Update output buffer back to run_output_data
void UpdateOutputBuffer(const bool is_async,
                        const std::vector<ge::DataBuffer> &execute_outputs,
                        std::vector<ge::DataBuffer> &user_outputs) {
  if (is_async) {
    return;
  }
  if (execute_outputs.size() != user_outputs.size()) {
    GELOGW("Output number mismatches, before execute: %zu, after execute: %zu",
           user_outputs.size(), execute_outputs.size());
    return;
  }
  for (size_t i = 0U; i < execute_outputs.size(); ++i) {
    auto &data_buffer = user_outputs[i];
    if (data_buffer.data == nullptr) {
      data_buffer.data = execute_outputs[i].data;
      data_buffer.length = execute_outputs[i].length;
    }
  }
}
}  // namespace

namespace ge {
std::atomic_bool GeExecutor::is_inited_{false};

static void InitOpsProtoManager() {
  std::string opsproto_path;
  char_t path_env[MMPA_MAX_PATH] = {};
  const auto env_res = mmGetEnv("ASCEND_OPP_PATH", &path_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (env_res == EN_OK) {
    const std::string path = path_env;
    const std::string file_path = RealPath(path.c_str());
    if (file_path.empty()) {
      GELOGE(FAILED, "[Check][EnvPath]ASCEND_OPP_PATH path [%s] is invalid.", path.c_str());
      REPORT_INPUT_ERROR("E68016", {"ASCEND_OPP_PATH", path});
      return;
    }
    opsproto_path = (path + "/op_proto/custom/" + ":") + (path + "/op_proto/built-in/");
    GELOGI("Get opsproto so path from env : %s", path.c_str());
  } else {
    std::string path_base = GetModelPath();
    GELOGI("path_base is %s", path_base.c_str());
    path_base = path_base.substr(0U, path_base.rfind('/'));
    path_base = path_base.substr(0U, path_base.rfind('/') + 1U);
    opsproto_path = (path_base + "ops/op_proto/custom/" + ":") + (path_base + "ops/op_proto/built-in/");
  }
  GELOGI("Get opsproto path is %s", opsproto_path.c_str());
  std::map<std::string, std::string> option_tmp;
  (void)option_tmp.emplace(std::pair<std::string, std::string>(std::string("ge.opsProtoLibPath"), opsproto_path));
  (void)OpsProtoManager::Instance()->Initialize(option_tmp);
}

GeExecutor::GeExecutor() {}

Status GeExecutor::Initialize(const std::map<std::string, std::string> &options) {
  if (is_inited_) {
    GELOGW("Already initialized, no need to be initialized again.");
    return SUCCESS;
  }

  GELOGI("Init GeExecutor begin.");
  OpTilingManager::GetInstance().LoadSo();

  const std::string path_base = GetModelPath();
  const Status init_hostcpu_engine_status = HostCpuEngine::GetInstance().Initialize(path_base);
  if (init_hostcpu_engine_status != SUCCESS) {
    GELOGE(init_hostcpu_engine_status, "[initialize][HostCpuEngine] failed");
    return init_hostcpu_engine_status;
  }

  GE_CHK_STATUS_RET_NOLOG(OpsKernelExecutorManager::GetInstance().Initialize(options));
  InitOpsProtoManager();

  GE_CHK_STATUS_RET(HostMemManager::Instance().Initialize());

  const std::vector<rtMemType_t> mem_type{RT_MEMORY_HBM, RT_MEMORY_P2P_DDR};
  Status status = MemManager::Instance().Initialize(mem_type);
  if (status != SUCCESS) {
    GELOGE(status, "[Init][MemManager] MemoryAllocatorManager initialize failed.");
    REPORT_CALL_ERROR("E19999", "MemManager initialize failed.");
    return status;
  }

  status = ModelManager::GetInstance().EnableExceptionDump(options);
  if (status != SUCCESS) {
    return status;
  }

  ProfilingProperties::Instance().SetExecuteProfiling(options);
  GE_CHK_STATUS_RET(InitializeExecutionRuntime(options), "Failed to initialize execution runtime");
  // Start profiling
  Options profiling_options;
  profiling_options.device_id = 0;
  // job id need to be set, the value is meaningless;
  profiling_options.job_id = "1";

  is_inited_.store(true);
  GELOGI("Init GeExecutor over.");
  return SUCCESS;
}

Status GeExecutor::FinalizeEx() {
  if (!is_inited_) {
    GELOGW("GeExecutor has not been initialized.");
    return SUCCESS;
  }

  GELOGI("Uninit GeExecutor begin.");
  ExecutionRuntime::FinalizeExecutionRuntime();

  MemManager::Instance().Finalize();
  hybrid::NpuMemoryAllocator::Finalize();
  // Stop profiling
  if (ProfilingProperties::Instance().ProfilingOn()) {
    ProfilingManager::Instance().PluginUnInit();
    ProfilingProperties::Instance().ClearProperties();
  }

  OpsKernelExecutorManager::GetInstance().Finalize();
  HostMemManager::Instance().Finalize();

  is_inited_.store(false);
  GELOGI("Uninit GeExecutor over.");
  return SUCCESS;
}

Status GeExecutor::Initialize() {
  // job id need to be set, the value is meaningless;
  const std::map<std::string, std::string> options({
      {OPTION_EXEC_JOB_ID, "1"}, {OPTION_EXEC_PROFILING_MODE, ""}, {OPTION_EXEC_PROFILING_OPTIONS, ""}
  });

  const rtProfCtrlHandle callback = &ge::ProfCtrlHandle;
  const rtError_t rt_ret = rtProfRegisterCtrlCallback(GE, callback);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "register func failed");
    return FAILED;
  }

  return GeExecutor::Initialize(options);
}

Status GeExecutor::Finalize() {
  return GeExecutor::FinalizeEx();
}

Status GeExecutor::SetDynamicBatchSize(const uint32_t model_id, void *const dynamic_input_addr, const uint64_t length,
                                       const uint64_t batch_size) {
  if (dynamic_input_addr == nullptr) {
    REPORT_INNER_ERROR("E19999", "param dynamic_input_addr is nullptr, check invalid, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID,
           "[Check][Param] Dynamic input addr is nullptr, model id:%u", model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID;
  }

  uint64_t size = sizeof(uint32_t);
  if (length < size) {
    REPORT_INNER_ERROR("E19999", "Dynamic input size [%lu] is less than [%lu], check invalid, model id:%u",
                       length, size, model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
           "[Check][Param] Dynamic input size [%lu] is less than [%lu], model id:%u", length, size, model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
  }
  if (length >= sizeof(uint64_t)) {
    size = sizeof(uint64_t);
  }

  // Verify whether the input dynamic batch matches the model gear
  std::vector<std::vector<int64_t>> batch_info;
  const std::vector<uint64_t> batch_num{batch_size};
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::FIXED);
  Status ret = GraphExecutor::GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "get dynamic batch info failed, model id:%u", model_id);
    GELOGE(ret, "[Get][DynamicBatchInfo] failed, model id:%u.", model_id);
    return ret;
  }

  if (!IsDynamicBatchSizeMatchModel(batch_size, batch_info)) {
    GELOGE(ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID,
           "[Check][Param] The current dynamic input does not match the gear of the model(id:%u).", model_id);
    return ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID;
  }

  ret = GraphExecutor::SetDynamicSize(model_id, batch_num, static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH));
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "set dynamic size failed, model id:%u, dynamic_type:1", model_id);
    GELOGE(ret, "[Set][DynamicSize] failed, model id:%u, dynamic_type:1", model_id);
    return ret;
  }
  // memcpy dynamic_batch_size from host to device
  const rtError_t rt_ret = rtMemcpy(dynamic_input_addr, length, &batch_size, size, RT_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpy, size:%lu ret:0x%X", length, rt_ret);
    GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy dynamic batch input data failed! size:%lu ret:0x%X", length, rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return SUCCESS;
}

Status GeExecutor::SetDynamicImageSize(const uint32_t model_id, void *const dynamic_input_addr, const uint64_t length,
                                       const uint64_t image_height, const uint64_t image_width) {
  if (dynamic_input_addr == nullptr) {
    REPORT_INNER_ERROR("E19999", "param dynamic_input_addr is nullptr, check invalid, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID,
           "[Check][Param] Dynamic input addr is nullptr, model id:%u", model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID;
  }

  const uint64_t dynamic_input_size = kDynamicImageSizeInputSize * sizeof(uint32_t);
  if (length < dynamic_input_size) {
    REPORT_INNER_ERROR("E19999", "Dynamic input size [%lu] is less than [%lu], check invalid, model id:%u",
                       length, dynamic_input_size, model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
           "[Check][Param] Dynamic input size [%lu] is less than [%lu], model id:%u",
           length, dynamic_input_size, model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
  }
  uint64_t size = sizeof(uint32_t);
  if (length >= (kDynamicImageSizeInputSize * sizeof(uint64_t))) {
    size = sizeof(uint64_t);
  }
  // Verify whether the input dynamic resolution matches the model gear
  std::vector<std::vector<int64_t>> batch_info;
  const std::vector<uint64_t> batch_num{image_height, image_width};
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::FIXED);
  Status ret = GraphExecutor::GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get dynamic input info failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][DynamicBatchInfo] failed, model id:%u.", model_id);
    return ret;
  }

  if (!IsDynamicImageSizeMatchModel(image_height, image_width, batch_info)) {
    GELOGE(ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID,
           "[Check][Param] The current dynamic input does not match the gear of the model, "
           "image_height:%lu, image_width:%lu.", image_height, image_width);
    return ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID;
  }

  ret = GraphExecutor::SetDynamicSize(model_id, batch_num, static_cast<int32_t>(DynamicInputType::DYNAMIC_IMAGE));
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Set dynamic size failed, model id:%u,", model_id);
    GELOGE(ret, "[Set][DynamicSize] failed, model id:%u", model_id);
    return ret;
  }

  // Memcpy dynamic resolution height from host to device
  rtError_t rt_ret =
      rtMemcpy(dynamic_input_addr, size, &image_height, size, RT_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed! size:%lu, ret:0x%X, model id:%u", size, rt_ret, model_id);
    GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy dynamic resolution input data failed! size:%lu, ret:0x%X, model id:%u",
           size, rt_ret, model_id);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  const uint64_t remain_size = length - size;
  // Memcpy dynamic resolution width from host to device
  rt_ret = rtMemcpy(ValueToPtr(PtrToValue(dynamic_input_addr) + size), remain_size, &image_width,
                    size, RT_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed! size:%lu, ret:0x%X, model id:%u",
                      remain_size, rt_ret, model_id);
    GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy dynamic resolution input data failed! size:%lu, ret:0x%X, model id:%u",
           remain_size, rt_ret, model_id);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return SUCCESS;
}

Status GeExecutor::SetDynamicDims(const uint32_t model_id, void *const dynamic_input_addr, const uint64_t length,
                                  const std::vector<uint64_t> &dynamic_dims) {
  if (dynamic_input_addr == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param dynamic_input_addr is nullptr, check invalid, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID,
           "[Check][Param] Dynamic input addr is nullptr, model id:%u", model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID;
  }

  std::vector<uint64_t> cur_dynamic_dims;
  Status ret = GetCurDynamicDims(model_id, dynamic_dims, cur_dynamic_dims);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][CurDynamicDims] failed, model id:%u", model_id);
    return ret;
  }
  std::vector<std::vector<int64_t>> batch_info;
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::FIXED);
  ret = GraphExecutor::GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get dynamic input info failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][DynamicBatchInfo] failed, model id:%u.", model_id);
    return ret;
  }

  if (!IsDynmaicDimsSizeMatchModel(cur_dynamic_dims, batch_info)) {
    GELOGE(ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID,
           "[Check][Param] The current dynamic input does not match the gear of the model, id:%u.", model_id);
    return ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID;
  }

  ret = GraphExecutor::SetDynamicSize(model_id, cur_dynamic_dims, static_cast<int32_t>(DYNAMIC_DIMS));
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Set dynamic size failed, model id:%u", model_id);
    GELOGE(ret, "[Set][DynamicSize] failed, model id:%u", model_id);
    return ret;
  }

  const size_t dynamic_dim_num = cur_dynamic_dims.size();
  const uint64_t dynamic_input_size = static_cast<uint64_t>(dynamic_dim_num * sizeof(uint32_t));
  if (length < dynamic_input_size) {
    REPORT_INNER_ERROR("E19999", "input dynamic size [%lu] is less than [%lu], model id:%u",
                       length, dynamic_input_size, model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
           "[Check][Param] Dynamic input size [%lu] is less than [%lu], model id:%u",
           length, dynamic_input_size, model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
  }
  uint64_t size = sizeof(uint32_t);
  if (length >= (dynamic_dim_num * sizeof(uint64_t))) {
    size = sizeof(uint64_t);
  }
  rtError_t rt_ret;
  for (size_t i = 0U; i < dynamic_dim_num; ++i) {
    // Memcpy dynamic dim[i] from host to device
    rt_ret = rtMemcpy(ValueToPtr(PtrToValue(dynamic_input_addr) + (size * i)),
                      length - (size * i), &cur_dynamic_dims[i], size, RT_MEMCPY_HOST_TO_DEVICE);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, size:%lu, ret:0x%X", (length - (size * i)), rt_ret);
      GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy dynamic resolution input data failed! size:%lu, ret:0x%X",
             (length - (size * i)), rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
  }
  return SUCCESS;
}

Status GeExecutor::GetCurDynamicDims(const uint32_t model_id, const std::vector<uint64_t> &dynamic_dims,
                                     std::vector<uint64_t> &cur_dynamic_dims) {
  cur_dynamic_dims.clear();
  std::vector<TensorDesc> input_desc;
  std::vector<TensorDesc> output_desc;
  auto ret = GetModelDescInfo(model_id, input_desc, output_desc);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][ModelDescInfo] failed, model id:%u.", model_id);
    return ret;
  }
  std::vector<std::string> user_designate_shape_order;
  std::vector<int64_t> all_data_dims;
  ret = GetUserDesignateShapeOrder(model_id, user_designate_shape_order);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][GetUserDesignateShapeOrder] failed, model id:%u.", model_id);
    return ret;
  }
  for (auto &data_name : user_designate_shape_order) {
    for (auto &desc : input_desc) {
      AscendString get_name;
      (void) desc.GetName(get_name);
      if (get_name.GetString() == data_name) {
        const auto dims = desc.GetShape().GetDims();
        (void)std::copy(dims.begin(), dims.end(), std::back_inserter(all_data_dims));
        break;
      }
    }
  }
  if (dynamic_dims.size() != all_data_dims.size()) {
    REPORT_INNER_ERROR("E19999", "Dynamic input size [%lu] is not equal with all data dims size [%lu]!",
                       dynamic_dims.size(), all_data_dims.size());
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
           "[Check][Param] Dynamic input size [%lu] is not equal with all data dims size [%lu]!",
           dynamic_dims.size(), all_data_dims.size());
    return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
  }
  for (size_t i = 0U; i < all_data_dims.size(); ++i) {
    if (all_data_dims[i] < 0) {
      cur_dynamic_dims.push_back(dynamic_dims[i]);
    } else {
      if (static_cast<uint64_t>(all_data_dims[i]) != dynamic_dims[i]) {
        REPORT_INNER_ERROR("E19999", "Static dims should be same, index:%zu value:%lu should be %ld",
                           i, dynamic_dims[i], all_data_dims[i]);
        GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
               "[Check][Param] Static dims should be same, index:%zu value:%lu should be %ld",
               i, dynamic_dims[i], all_data_dims[i]);
        return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
      }
    }
  }
  return SUCCESS;
}

Status GeExecutor::GetCurShape(const uint32_t model_id, std::vector<int64_t> &batch_info, int32_t &dynamic_type) {
  GELOGI("Begin to get current shape");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized, model id:%u", model_id);
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  const Status ret = GraphExecutor::GetCurrentShape(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get Cur Shape failed, model id:%u", model_id);
    GELOGE(ret, "[Get][CurShape] failed, model id:%u", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GeExecutor::SetDynamicAippData(const uint32_t model_id, void *const dynamic_input_addr, const uint64_t length,
                                      const std::vector<kAippDynamicBatchPara> &aipp_batch_para,
                                      const kAippDynamicPara &aipp_parms) {
  GELOGI("Enter to SetDynamicAippData.");
  if (dynamic_input_addr == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param dynamic_input_addr is nullptr, check invalid, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID,
           "[Check][Param] Dynamic aipp input addr is nullptr, model id:%u", model_id);
    return ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID;
  }
  if (aipp_batch_para.empty()) {
    REPORT_INNER_ERROR("E19999", "Param aipp_batch_para is empty, check invalid, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_AIPP_BATCH_EMPTY, "[Check][Param] aipp_batch_para is empty, model id:%u", model_id);
    return ACL_ERROR_GE_AIPP_BATCH_EMPTY;
  }
  const uint64_t batch_num = aipp_batch_para.size();
  const uint64_t real_aippParms_size = sizeof(kAippDynamicPara) - sizeof(kAippDynamicBatchPara);
  const uint64_t struct_len = (batch_num * sizeof(kAippDynamicBatchPara)) + real_aippParms_size;
  GELOGI("Get acl input dynamic aipp data, model_id is %u, length is %lu, batch num is %lu, struct_len is %lu",
         model_id, length, batch_num, struct_len);
  if (struct_len > length) {
    REPORT_INNER_ERROR("E19999", "input dynamic aipp param len:%lu is larger than aipp_data size:%lu",
                       struct_len, length);
    GELOGE(ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID,
           "[Check][Param] input dynamic aipp param len [%lu] is larger than aipp_data size [%lu]",
           struct_len, length);
    return ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID;
  }
  // Memcpy real kAippDynamicBatchPara from host to device
  rtError_t rt_ret = rtMemcpy(dynamic_input_addr, length, &aipp_parms, real_aippParms_size, RT_MEMCPY_HOST_TO_DEVICE);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, size:%lu, ret:0x%X", length, rt_ret);
    GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy aipp_parms failed! size:%lu, ret:0x%X", length, rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  const uint64_t remain_len = length - real_aippParms_size;
  const auto aipp_batch_para_dev = PtrToValue(dynamic_input_addr) + real_aippParms_size;

  for (uint64_t i = 0U; i < batch_num; ++i) {
    rt_ret = rtMemcpy(ValueToPtr(aipp_batch_para_dev + (i * sizeof(kAippDynamicBatchPara))),
                      (remain_len - (i * sizeof(kAippDynamicBatchPara))), &(aipp_batch_para[i]),
                      sizeof(kAippDynamicBatchPara), RT_MEMCPY_HOST_TO_DEVICE);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, ret:0x%X", rt_ret);
      GELOGE(RT_FAILED, "[Call][RtMemcpy] memcpy kAippDynamicBatchPara input data failed! ret:0x%X", rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
  }
  return SUCCESS;
}

Status GeExecutor::UnloadModel(const uint32_t model_id) {
  GELOGD("unload model %u begin.", model_id);
  Status ret = ModelManager::GetInstance().DestroyAicpuSessionForInfer(model_id);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Destroy Aicpu Session For Infer failed, model id:%u", model_id);
    GELOGE(ret, "[Destroy][AicpuSession] For Infer failed. model id:%u", model_id);
    return ret;
  }

  const auto hybrid_davinci_model = ModelManager::GetInstance().GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    const uint64_t session_id = hybrid_davinci_model->GetSessionId();
    VarManagerPool::Instance().RemoveVarManager(session_id);
  } else {
    const auto davinci_model = ModelManager::GetInstance().GetModel(model_id);
    if (davinci_model != nullptr) {
      const uint64_t session_id = davinci_model->GetSessionId();
      VarManagerPool::Instance().RemoveVarManager(session_id);
    }
  }
  ret = GraphLoader::UnloadModel(model_id);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "unload model failed, model id:%u", model_id);
    GELOGE(ret, "[Unload][Model] failed. model id:%u", model_id);
    return ret;
  }
  return SUCCESS;
}

// Get input and output descriptor
Status GeExecutor::GetModelDescInfo(const uint32_t model_id, std::vector<TensorDesc> &input_desc,
                                    std::vector<TensorDesc> &output_desc, const bool new_model_desc) {
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized, model id:%u", model_id);
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized, model id:%u", model_id);
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  std::vector<InputOutputDescInfo> input_desc_infos;
  std::vector<InputOutputDescInfo> output_desc_infos;
  std::vector<uint32_t> input_formats;
  std::vector<uint32_t> output_formats;

  const auto ret = GraphExecutor::GetInputOutputDescInfo(model_id, input_desc_infos, output_desc_infos, input_formats,
                                                         output_formats, new_model_desc);
  if (ret != domi::SUCCESS) {
    REPORT_CALL_ERROR("E19999", "get input output desc info failed, ret = %u, model id:%u", ret, model_id);
    GELOGE(ret, "[Get][InputOutputDescInfo] failed. ret = %u, model id:%u", ret, model_id);
    return ACL_ERROR_GE_GET_TENSOR_INFO;
  }

  if (input_formats.size() != input_desc_infos.size()) {
    REPORT_INNER_ERROR("E19999", "input_formats size %zu is not equal to input_desc_infos size %zu, model id:%u.",
                       input_formats.size(), input_desc_infos.size(), model_id);
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Check][Param] input_formats size %zu is not equal to input_desc_infos size %zu, model id:%u.",
           input_formats.size(), input_desc_infos.size(), model_id);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  if (output_formats.size() != output_desc_infos.size()) {
    REPORT_INNER_ERROR("E19999", "output_formats size %zu is not equal to output_desc_infos size %zu, model id:%u.",
                       output_formats.size(), output_desc_infos.size(), model_id);
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Check][Param] output_formats size %zu is not equal to output_desc_infos size %zu, model id:%u.",
           output_formats.size(), output_desc_infos.size(), model_id);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  // Transfer data to TensorDesc
  GetGeTensorDescFromDomiInfo(input_desc, input_desc_infos, input_formats);
  GetGeTensorDescFromDomiInfo(output_desc, output_desc_infos, output_formats);

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
Status GeExecutor::GetDynamicBatchInfo(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                       int32_t &dynamic_type) {
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const auto ret = GraphExecutor::GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get Dynamic BatchInfo failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][DynamicBatchInfo] failed, model id:%u.", model_id);
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
Status GeExecutor::GetCombinedDynamicDims(const uint32_t model_id,
                                          std::vector<std::vector<int64_t>> &batch_info) {
  GELOGI("Begin to get combined dynamic dims info.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const auto ret = GraphExecutor::GetCombinedDynamicDims(model_id, batch_info);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get Combined DynamicDims failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][CombinedDynamicDims] failed, model id:%u.", model_id);
    return ret;
  }

  GELOGI("Get combined dynamic dims succ.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get user designeate shape order
/// @param [in] model_id
/// @param [out] user_designate_shape_order
/// @return execute result
///
Status GeExecutor::GetUserDesignateShapeOrder(const uint32_t model_id,
                                              std::vector<std::string> &user_designate_shape_order) {
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const auto ret = GraphExecutor::GetUserDesignateShapeOrder(model_id, user_designate_shape_order);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "GetUserDesignateShapeOrder failed, model id:%u.", model_id);
    GELOGE(ret, "[Call][GetUserDesignateShapeOrder] failed, model id:%u.", model_id);
    return ret;
  }

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get AIPP input format
/// @param [in] model_id
/// @param [in] index
/// @param [out] input_format
/// @return execute result
///
Status GeExecutor::GetAIPPInfo(const uint32_t model_id, const uint32_t index, AippConfigInfo &aipp_info) {
  GELOGI("Begin to GetAIPPInfo.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor not inited yet!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  const auto ret = GraphExecutor::GetAippInfo(model_id, index, aipp_info);
  if (ret != SUCCESS) {
    GELOGW("GetAIPPInfo is not success.");
    return ret;
  }
  GELOGI("GetAIPPInfo succ.");
  return SUCCESS;
}

Status GeExecutor::GetAippType(const uint32_t model_id, const uint32_t index, InputAippType &type,
                               size_t &aipp_index) {
  GELOGI("Begin to get aipp type.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "GeExecutor has not inited yet!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  const auto ret = GraphExecutor::GetAippType(model_id, index, type, aipp_index);
  if (ret != SUCCESS) {
    GELOGW("Get aipp type is not success.");
    return ret;
  }
  GELOGI("Get aipp type success.");
  return SUCCESS;
}

Status GeExecutor::GetOpAttr(const uint32_t model_id, const std::string &op_name, const std::string &attr_name,
                             std::string &attr_value) {
  GELOGI("Begin to get op attr.");
  if (!is_inited_) {
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Init][GeExecutor]Ge executor not inited yet!");
    REPORT_INNER_ERROR("E19999", "Ge executor not inited yet!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  const auto ret = GraphExecutor::GetNodeAttr(model_id, op_name, attr_name, attr_value);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][OpAttr]Get op:%s attr:%s failed, model id:%u.",
           op_name.c_str(), attr_name.c_str(), model_id);
    REPORT_CALL_ERROR("E19999", "Get op:%s attr:%s failed, model id:%u",
                      op_name.c_str(), attr_name.c_str(), model_id);
    return ret;
  }
  return SUCCESS;
}

Status GeExecutor::GetModelAttr(const uint32_t model_id, std::vector<std::string> &dynamic_output_shape_info) {
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not inited yet!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not inited yet!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  const auto ret = GraphExecutor::GetOutputShapeInfo(model_id, dynamic_output_shape_info);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get Model Attr failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][ModelAttr] failed, model id:%u.", model_id);
    return ret;
  }
  return SUCCESS;
}

Status GeExecutor::CommandHandle(const Command &command) {
  const Status ret = GraphLoader::CommandHandle(command);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "call CommandHandle failed, ret:%u", ret);
    GELOGE(ACL_ERROR_GE_COMMAND_HANDLE, "[Call][CommandHandle] failed, ret:%u", ret);
    return ACL_ERROR_GE_COMMAND_HANDLE;
  }
  return SUCCESS;
}

Status GeExecutor::GetMaxUsedMemory(const uint32_t model_id, uint32_t &max_size) {
  GELOGI("Get max used memory begin.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  uint64_t max_mem_size = 0U;
  const auto ret = ModelManager::GetInstance().GetMaxUsedMemory(model_id, max_mem_size);
  max_size = static_cast<uint32_t>(max_mem_size);
  return ret;
}

/**
 * @ingroup ge
 * @brief Load data from model file to memory
 * @param [in] const std::string &path: Offline model file path
 * @param [out] domi::ModelData &model_data: Offline model memory data
 * @return SUCCESS handle successfully / others handle failed
 */
Status GeExecutor::LoadDataFromFile(const std::string &path, ModelData &model_data) {
  GELOGI("Load data from file begin.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const std::string filePath = RealPath(path.c_str());
  if (filePath.empty()) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID,
           "[Call][RealPath] File path is invalid. please check your text file '%s'.", path.c_str());
    return ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID;
  }
  GELOGI("load modelData from file: %s.", path.c_str());
  const int32_t priority = 0;
  const auto ret = GraphLoader::LoadDataFromFile(path, priority, model_data);
  if (ret != SUCCESS) {
    if (model_data.model_data != nullptr) {
      delete[] static_cast<char_t *>(model_data.model_data);
      model_data.model_data = nullptr;
    }
  }
  return ret;
}

/**
* @ingroup ge
* @brief Load model from offline model memory data
* @param [in] domi::ModelData &model_data: Offline model data
              void *dev_ptr: Input/Output memory start address
              size_t memsize: Input/Output memory length
              void *weight_ptr: Weight memory start address
              size_t weightsize: Weight memory length
* @param [out] uint32_t &model_id: identification after model loading
* @return SUCCESS handle successfully / others handle failed
*/
Status GeExecutor::LoadModelFromData(uint32_t &model_id, const ModelData &model_data, void *const dev_ptr,
                                     const size_t mem_size, void *const weight_ptr, const size_t weight_size) {
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not inited yet!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  return GraphLoader::LoadModelFromData(model_id, model_data, PtrToValue(dev_ptr), mem_size, PtrToValue(weight_ptr),
                                        weight_size);
}

/**
 * @ingroup ge
 * @brief Load task list from ModelData with queue.
 * @param [out] model_id: model id allocate from manager.
 * @param [in] ge_model_data: Model data load from offline model.
 * @param [in] input_queue_ids: input queue ids create from user.
 * @param [in] output_queue_ids: input queue ids create from user.
 * @return: 0 for success / others for fail
 */
Status GeExecutor::LoadModelWithQ(uint32_t &model_id, const ModelData &model_data,
                                  const std::vector<uint32_t> &input_queue_ids,
                                  const std::vector<uint32_t> &output_queue_ids) {
  GELOGI("Load model with queue begin.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  return GraphLoader::LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids);
}

Status GeExecutor::LoadModelWithQ(uint32_t &model_id,
                                  const shared_ptr<GeRootModel> &root_model,
                                  const vector<uint32_t> &input_queue_ids,
                                  const vector<uint32_t> &output_queue_ids) {
  return GraphLoader::LoadModelWithQ(model_id, root_model, input_queue_ids, output_queue_ids, false);
}

/**
 * @ingroup ge
 * @brief Load task list from root_model without queue.
 * @param [out] model_id: model id allocate from manager.
 * @param [in] root_model:model of root
 * @return: 0 for success / others for fail
 */
Status GeExecutor::LoadModelWithoutQ(uint32_t &model_id, const shared_ptr<GeRootModel> &root_model) const {
  return GraphLoader::LoadModelWithoutQ(model_id, root_model);
}

/**
* @ingroup ge
* @brief Synchronous execution of offline model(Do not create thread)
* @param [in] uint32_t model_id: Model ID to execute
              void* stream: stream to execute
              const domi::InputData *input_data: Model input data
              bool async_mode: is asynchronize mode.
* @param [out] domi::OutputData *output_data: Model output data
* @return SUCCESS handle successfully / others handle failed
*/
Status GeExecutor::ExecModel(const uint32_t model_id, void *const stream, const RunModelData &input_data,
                             RunModelData &output_data, const bool async_mode) {
  std::vector<GeTensorDesc> output_desc;
  return ExecModel(model_id, stream, input_data, {}, output_data, output_desc, async_mode);
}

/**
* @ingroup ge
* @brief Synchronous execution of offline model(Do not create thread)
* @param [in] uint32_t model_id: Model ID to execute
              void* stream: stream to execute
              const domi::InputData *input_data: Model input data
              const std::vector<GeTensorDesc> &input_desc: Description of model input data
              bool async_mode: is asynchronize mode
* @param [out] domi::OutputData *output_data: Model output data
* @param [out] std::vector<GeTensorDesc> &output_desc: Description of model output data
* @return SUCCESS handle successfully / others handle failed
*/
Status GeExecutor::ExecModel(const uint32_t model_id, void *const stream, const RunModelData &run_input_data,
                             const std::vector<GeTensorDesc> &input_desc, RunModelData &run_output_data,
                             std::vector<GeTensorDesc> &output_desc, const bool async_mode) {
  PROFILING_SCOPE(-1, profiling::kModelExecute);
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  InputData input_data;
  OutputData output_data;
  GetDomiInputData(run_input_data, input_data);
  GetDomiOutputData(run_output_data, output_data);

  if ((run_input_data.dynamic_batch_size != 0U) || (run_input_data.dynamic_image_width != 0U) ||
      (run_input_data.dynamic_image_height != 0U) || (run_input_data.dynamic_dims.size() != 0U)) {
    std::vector<std::vector<int64_t>> batch_info;
    int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::FIXED);
    const auto ret = GraphExecutor::GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "get dynamic batch info failed, model id:%u.", model_id);
      GELOGE(ret, "[Get][DynamicBatchInfo] failed, model id:%u.", model_id);
      return ret;
    }
    if (!batch_info.empty()) {
      SetDynamicInputDataFlag(run_input_data, batch_info, input_data);
    }
  }

  GE_CHK_STATUS_RET_NOLOG(GraphLoader::ExecuteModel(model_id,
                                                    stream,
                                                    async_mode,
                                                    input_data,
                                                    input_desc,
                                                    output_data,
                                                    output_desc));
  UpdateOutputBuffer(async_mode, output_data.blobs, run_output_data.blobs);
  return SUCCESS;
}

/**
* @ingroup ge
* @brief Get weight memory size from model file
* @param [in] const std::string &path: Offline model file path
* @param [out] size_t &mem_size Execution memory size
               size_t &weight_size Weight memory space size
* @return SUCCESS handle successfully / others handle failed
*/
Status GeExecutor::GetMemAndWeightSize(const std::string &path, size_t &mem_size, size_t &weight_size) {
  GELOGI("Get memory and weight size from file begin.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  ModelData model;
  Status ret = GraphLoader::LoadDataFromFile(path, 0, model);
  if ((ret != SUCCESS) || (model.model_data == nullptr)) {
    REPORT_CALL_ERROR("E19999", "load data from file failed, ret = %d", ret);
    GELOGE(ret, "[Load][Data] from file failed. ret = %d", ret);
    return ret;
  }

  ret = ModelManager::GetModelMemAndWeightSize(model, mem_size, weight_size);

  delete[] static_cast<char_t *>(model.model_data);
  model.model_data = nullptr;

  return ret;
}

/**
* @ingroup ge
* @brief Get weight memory size from model file
* @param [in] const void *model_data Offline model buffer
              size_t model_size Offline model buffer length
* @param [out] size_t &mem_size Execution memory size
               size_t &weight_size Weight memory space size
* @return SUCCESS handle successfully / others handle failed
*/
Status GeExecutor::GetMemAndWeightSize(const void *const model_data, const size_t model_size, size_t &mem_size,
                                       size_t &weight_size) {
  GELOGI("Get memory and weight size from data begin.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  if (model_data == nullptr) {
    REPORT_INNER_ERROR("E19999", "param model_data is nullptr, check invalid!");
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID, "[Check][Param] invalid model data!");
    return ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID;
  }

  ModelData model;
  model.model_data = ValueToPtr(PtrToValue(model_data));
  model.model_len = static_cast<uint32_t>(model_size);

  return ModelManager::GetModelMemAndWeightSize(model, mem_size, weight_size);
}

Status GeExecutor::LoadSingleOp(const std::string &model_name, const ModelData &model_data, void *const stream,
                                SingleOp **const single_op) {
  return LoadSingleOpV2(model_name, model_data, stream, single_op, 0U);
}

Status GeExecutor::LoadSingleOpV2(const std::string &model_name, const ModelData &model_data, void *const stream,
                                  SingleOp **const single_op, const uint64_t model_id) {
  return SingleOpManager::GetInstance().GetOpFromModel(model_name, model_data, stream, single_op, model_id);
}

Status GeExecutor::LoadDynamicSingleOp(const std::string &model_name, const ModelData &model_data, void *const stream,
                                       DynamicSingleOp **const single_op) {
  return LoadDynamicSingleOpV2(model_name, model_data, stream, single_op, 0U);
}

Status GeExecutor::LoadDynamicSingleOpV2(const std::string &model_name, const ModelData &model_data, void *const stream,
                                         DynamicSingleOp **const single_op, const uint64_t model_id) {
  return SingleOpManager::GetInstance().GetDynamicOpFromModel(model_name, model_data, stream, single_op, model_id);
}

Status GeExecutor::UnloadSingleOp(const uint64_t op_id) {
  return SingleOpManager::GetInstance().DeleteSingleOp(op_id);
}

Status GeExecutor::UnloadDynamicSingleOp(const uint64_t op_id) {
  return SingleOpManager::GetInstance().DeleteDynamicSingleOp(op_id);
}

Status GeExecutor::ExecuteAsync(SingleOp *const executor, const std::vector<DataBuffer> &inputs,
                                std::vector<DataBuffer> &outputs) {
  if (executor == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param executor is nullptr, check invalid");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] param executor is nullptr");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }
  PROFILING_SCOPE(executor->GetProfilingNodeIndex(), profiling::kOpExecute);
  RECORD_SHELL_PROFILING_START(kOpExecute);
  const auto ret = executor->ExecuteAsync(inputs, outputs);
  RECORD_SHELL_PROFILING_END(profiling::kOpExecute, executor->GetProfilingNodeIndex(), kOpExecute);
  return ret;
}

Status GeExecutor::ExecuteAsync(DynamicSingleOp *const executor, const std::vector<GeTensorDesc> &input_desc,
                                const std::vector<DataBuffer> &inputs, std::vector<GeTensorDesc> &output_desc,
                                std::vector<DataBuffer> &outputs) {
  GE_CHECK_NOTNULL(executor);
  PROFILING_SCOPE(executor->GetProfilingNodeIndex(), profiling::kOpExecute);
  RECORD_SHELL_PROFILING_START(kOpExecute);
  const auto ret = executor->ExecuteAsync(input_desc, inputs, output_desc, outputs);
  RECORD_SHELL_PROFILING_END(profiling::kOpExecute, executor->GetProfilingNodeIndex(), kOpExecute);
  return ret;
}

Status GeExecutor::ReleaseSingleOpResource(void *const stream) {
  (void)ModelManager::GetInstance().ClearAicpuSo();
  return SingleOpManager::GetInstance().ReleaseResource(stream);
}

Status GeExecutor::GetDeviceIdByModelId(const uint32_t model_id, uint32_t &device_id) {
  const auto davinci_model = ModelManager::GetInstance().GetModel(model_id);
  if (davinci_model == nullptr) {
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
           "[Get][Model] failed, Model id:%u is invaild or model is not loaded.", model_id);
    return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
  }

  device_id = davinci_model->GetDeviceId();
  return SUCCESS;
}

Status GeExecutor::GetBatchInfoSize(const uint32_t model_id, size_t &shape_count) {
  std::vector<std::vector<int64_t>> batch_info;
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::FIXED);
  const auto ret = GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][DynamicBatchInfo] failed. ret = %d, model id:%u", ret, model_id);
    return ret;
  }
  if (batch_info.empty()) {
    shape_count = kStaticBatchInfoSize;
  } else {
    shape_count = batch_info.size();
  }
  return SUCCESS;
}

Status GeExecutor::GetOrigInputInfo(const uint32_t model_id, const uint32_t index, OriginInputInfo &orig_input_info) {
  GELOGI("Begin to GetOrigInputInfo.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const auto ret = GraphExecutor::GetOrigInputInfo(model_id, index, orig_input_info);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get Orig Input Info failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][OrigInputInfo] failed, model id:%u.", model_id);
    return ret;
  }

  GELOGI("GetOrigInputInfo succ.");
  return SUCCESS;
}

Status GeExecutor::GetAllAippInputOutputDims(const uint32_t model_id, const uint32_t index,
                                             std::vector<InputOutputDims> &input_dims,
                                             std::vector<InputOutputDims> &output_dims) {
  GELOGI("Begin to GetAllAippInputOutputDims.");
  if (!is_inited_) {
    REPORT_INNER_ERROR("E19999", "GeExecutor has not been initialized!");
    GELOGE(ACL_ERROR_GE_EXEC_NOT_INIT, "[Check][Param] GeExecutor has not been initialized!");
    return ACL_ERROR_GE_EXEC_NOT_INIT;
  }

  const auto ret = GraphExecutor::GetAllAippInputOutputDims(model_id, index, input_dims, output_dims);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get All Aipp Input Output Dims failed, model id:%u.", model_id);
    GELOGE(ret, "[Get][AllAippInputOutputDims] failed, model id:%u.", model_id);
    return ret;
  }

  GELOGI("GetAllAippInputOutputDims succ.");
  return SUCCESS;
}

Status GeExecutor::GetOpDescInfo(const uint32_t device_id, const uint32_t stream_id, const uint32_t task_id,
                                 OpDescInfo &op_desc_info) {
  GELOGI("Begin to GetOpDescInfo.");
  const auto ret = GraphExecutor::GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "get opdesc info failed, device_id:%u, stream_id:%u, task_id:%u.",
                      device_id, stream_id, task_id);
    GELOGE(ret, "[Get][OpDescInfo] failed, device_id:%u, stream_id:%u, task_id:%u.",
           device_id, stream_id, task_id);
    return ret;
  }
  GELOGI("GetOpDescInfo succ.");
  return SUCCESS;
}

Status GeExecutor::SetDump(const DumpConfig &dump_config) {
  GELOGI("Start to set dump config");
  const auto ret = DumpManager::GetInstance().SetDumpConf(dump_config);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][DumpConf] failed, ret:%d", ret);
    return ret;
  }
  GELOGI("Set dump config successfully");
  return SUCCESS;
}
}  // namespace ge
