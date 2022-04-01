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

#include "framework/memory/memory_api.h"

#include <memory>

#include "common/math/math_util.h"
#include "common/plugin/plugin_manager.h"
#include "graph/def_types.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/manager/host_mem_manager.h"
#include "graph/manager/rdma_pool_allocator.h"
#include "graph/utils/type_utils.h"
#include "common/plugin/ge_util.h"
#include "hccl/base.h"
#include "hccl/hccl_types.h"

namespace ge {
Status InitRdmaPool(size_t size, rtMemType_t mem_type) {
  GELOGD("InitRdmaPool in");
  return MemManager::Instance().RdmaPoolInstance(mem_type).InitMemory(size);
}

Status RdmaRemoteRegister(const std::vector<HostVarInfo> &var_info, rtMemType_t mem_type) {
  GELOGD("Start to register rdma memory with host var size %zu", var_info.size());
  uint64_t device_base = 0U;
  uint64_t device_size = 0U;
  GE_CHK_STATUS_RET(MemManager::Instance().RdmaPoolInstance(mem_type).GetBaseAddr(device_base, device_size));
  const auto table_len = var_info.size() + 1U;
  const std::unique_ptr<MemRegisterAddr[]> reg_addrs = MakeUnique<MemRegisterAddr[]>(table_len);
  GE_CHECK_NOTNULL(reg_addrs);
  for (size_t i = 0U; i < var_info.size(); ++i) {
    reg_addrs[i] = {var_info[i].base_addr, var_info[i].var_size};
  }
  reg_addrs[table_len - 1U] = {device_base, device_size};

  const std::string file_name = "libhccl.so";
  std::string path = GetModelPath();
  (void)path.append(file_name);
  const std::string canonical_path = RealPath(path.c_str());
  if (canonical_path.empty()) {
    REPORT_INNER_ERROR("E19999", "canonical_path:%s is empty, check invalid", canonical_path.c_str());
    GELOGE(FAILED, "[Call][RealPath] Failed to get realpath of %s", path.c_str());
    return FAILED;
  }
  GELOGI("FileName:%s, Path:%s.", file_name.c_str(), canonical_path.c_str());
  const auto handle = mmDlopen(canonical_path.c_str(), static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                                                            static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  GE_CHECK_NOTNULL(handle);
  GE_MAKE_GUARD(not_used_var, [&handle] {
    if (mmDlclose(handle) != 0) {
      GELOGW("Failed to close handle %s", mmDlerror());
    }
  });

  const auto hcom_remote_mem_register =
      reinterpret_cast<HcclResult(*)(const MemRegisterAddr *, uint32_t)>(mmDlsym(handle, "HcomRegRemoteAccessMem"));
  if (hcom_remote_mem_register == nullptr) {
    REPORT_CALL_ERROR("E19999", "Symbol HcomRegRemoteAccessMem can't find in %s, check invalid",
                      canonical_path.c_str());
    GELOGE(FAILED, "[Check][Param] Symbol HcomRegRemoteAccessMem can't find in %s", canonical_path.c_str());
    return FAILED;
  }

  const HcclResult hccl_ret = hcom_remote_mem_register(reg_addrs.get(), static_cast<uint32_t>(table_len));
  if (hccl_ret != HCCL_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call hcom_remote_mem_register failed, ret:%d,", hccl_ret);
    GELOGE(HCCL_E_INTERNAL, "[Call][HcomRemoteMemRegister] Rdma mem register failed, ret:0x%X", hccl_ret);
    return HCCL_E_INTERNAL;
  }
  return SUCCESS;
}

Status MallocSharedMemory(const TensorInfo &tensor_info, uint64_t &dev_addr, uint64_t &memory_size) {
  GELOGD("MallocSharedMemory in");
  uint32_t type_size = 0U;
  const bool result = TypeUtils::GetDataTypeLength(tensor_info.data_type, type_size);
  if (!result) {
    GELOGE(GRAPH_FAILED, "[Get][DataTypeLength] failed, data_type=(%s).",
           TypeUtils::DataTypeToSerialString(tensor_info.data_type).c_str());
    return GRAPH_FAILED;
  }
  memory_size = type_size;
  for (const auto dim : tensor_info.dims) {
    if (dim <= 0) {
      GELOGE(GRAPH_FAILED, "[Check][Param] Tensor dims should be positive");
      return GRAPH_FAILED;
    }
    FMK_UINT64_MULCHECK(memory_size, static_cast<uint64_t>(dim));
    memory_size *= static_cast<uint64_t>(dim);
  }
  SharedMemInfo mem_info(tensor_info.var_name, memory_size);
  const Status ret = HostMemManager::Instance().MallocHostSharedMemory(mem_info);
  if (ret != SUCCESS) {
    GELOGE(GRAPH_FAILED, "[Malloc][SharedMemory] failed, op name [%s]", tensor_info.var_name.c_str());
    return GRAPH_FAILED;
  }
  dev_addr = PtrToValue(mem_info.device_address);
  GELOGD("MallocSharedMemory Succeeded");
  return SUCCESS;
}

Status GetVarBaseAddrAndSize(const std::string &var_name, uint64_t &base_addr, uint64_t &var_size) {
  GELOGD("GetVarBaseAddrAndSize in, var name:[%s]", var_name.c_str());
  SharedMemInfo mem_info;
  if (!HostMemManager::Instance().QueryVarMemInfo(var_name, mem_info)) {
    GELOGE(FAILED, "Get addr and size failed, name:[%s]", var_name.c_str());
    return FAILED;
  }
  base_addr = PtrToValue(mem_info.host_aligned_ptr->Get());
  var_size = mem_info.mem_size;
  return SUCCESS;
}
}  // namespace ge
