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

#include "graph/load/model_manager/task_info/super_kernel/super_kernel_factory.h"

#include "common/math/math_util.h"
#include "common/plugin/ge_util.h"

namespace ge {
namespace skt {
const size_t kFusedKernelMinimumSize = 2U;
const size_t kFusedKernelSizeUnit = 2U;

SuperKernelFactory &SuperKernelFactory::GetInstance() {
  static SuperKernelFactory factory;
  return factory;
}

Status SuperKernelFactory::Init() {
  if (!is_init_) {
    const std::string skt_bin = "libcce_aicore.so";
    handle_ = mmDlopen(skt_bin.c_str(), static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                                             static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
    if (handle_ == nullptr) {
      const ge::char_t *error = mmDlerror();
      GE_IF_BOOL_EXEC(error == nullptr, error = "");
      GELOGE(FAILED, "[Open][SktLib] failed, please check LD_LIBRARY_PATH. errmsg:%s", error);
    }
    rtError_t rt_ret = rtGetFunctionByName(this->sk_stub_name_.c_str(), &this->func_stub_);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                    REPORT_CALL_ERROR("E19999", "Call rtGetFunctionByName failed, stub_func:%s, ret:0x%X",
                                      this->sk_stub_name_.c_str(), rt_ret);
                    GELOGE(RT_FAILED, "[Call][RtGetFunctionByName] failed. stub_func:%s, "
                           "please export LD_LIBRARY_PATH for libcce_aicore.so", this->sk_stub_name_.c_str());
                    return RT_ERROR_TO_GE_STATUS(rt_ret);)
    rt_ret = rtGetAddrByFun(this->func_stub_, &this->func_ptr_);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                    REPORT_CALL_ERROR("E19999", "Call rtGetAddrByFun failed, ret:0x%X", rt_ret);
                    GELOGE(RT_FAILED, "[Call][RtGetAddrByFun] failed, ret:0x%X", rt_ret);
                    return RT_ERROR_TO_GE_STATUS(rt_ret);)
    GELOGD("SKT: fuseKernels super_kernel_template subFunc %p, device func address %p",
           this->func_stub_, this->func_ptr_);
  }
  is_init_ = true;

  return SUCCESS;
}

Status SuperKernelFactory::FuseKernels(const std::vector<const void *> &stub_func_list,
                                       const std::vector<void *> &args_addr_list, const uint32_t block_dim,
                                       std::unique_ptr<skt::SuperKernel> &h) const {
  // Iterate through the ops to be fused
  // Each subkernel to be fused contains 2 fields: fn address offset, args
  // address.
  // Generate the nav table contents. The format is as follows:
  // [[fn_ptr_address, args_addr1], [fn_ptr_address2, args_addr2],
  // ...]
  if (this->func_stub_ == nullptr) {
    GELOGW("SKT: func_stub_ is empty. Please make sure init() is run first");
    return FAILED;
  }

  const size_t super_kernel_size = stub_func_list.size();
  if (super_kernel_size != args_addr_list.size()) {
    GELOGW("SKT: The size of stub_func_list doesn't match args_addr_list");
    return FAILED;
  }

  if (super_kernel_size < kFusedKernelMinimumSize) {
    GELOGW("SKT: the number of kernels being fused must be greater than or equal to 2");
    return FAILED;
  }
  GELOGI("SKT: superkernel start fuse, superkernel size %zu.", stub_func_list.size());
  GE_CHK_STATUS_RET(CheckUint64MulOverflow(kFusedKernelSizeUnit, stub_func_list.size()));
  const size_t nav_table_len = kFusedKernelSizeUnit * stub_func_list.size();
  GE_CHK_STATUS_RET(CheckUint64MulOverflow(nav_table_len, sizeof(int64_t)));
  const uint64_t nav_table_size = kFusedKernelSizeUnit * stub_func_list.size() * sizeof(int64_t);
  std::vector<uint64_t> nav_table(nav_table_len);

  rtError_t rt_ret = RT_ERROR_NONE;
  void *hbm_nav_table_addr = nullptr;
  for (size_t i = 0U; i < stub_func_list.size(); i++) {
    void *sub_device_func = nullptr;
    rt_ret = rtGetAddrByFun(stub_func_list[i], &sub_device_func);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                    REPORT_CALL_ERROR("E19999", "Call rtGetAddrByFun failed, ret:0x%X", rt_ret);
                    GELOGE(RT_FAILED, "[Call][RtGetAddrByFun] failed, ret:0x%X", rt_ret);
                    return RT_ERROR_TO_GE_STATUS(rt_ret);)
    GELOGD("SKT: fuseKernels subFunc %p, device func address %p", stub_func_list[i], sub_device_func);
    // store two uint64_t address
    // address divided by 4 because of 32bits encoding, call offset will *4 when calculating
    nav_table[i * kFusedKernelSizeUnit] = PtrToValue(sub_device_func) / 4U;
    GELOGD("SKT: CALL offet %lu", nav_table[i * kFusedKernelSizeUnit]);
    nav_table[(i * kFusedKernelSizeUnit) + 1U] = PtrToValue(args_addr_list[i]);
    GELOGD("SKT: fuseKernels args base address %lu", nav_table[(i * kFusedKernelSizeUnit) + 1U]);
  }
  rt_ret = rtMalloc(reinterpret_cast<void **>(&hbm_nav_table_addr), nav_table_size, RT_MEMORY_HBM);
  GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                  REPORT_CALL_ERROR("E19999", "Call rtMalloc failed, size:%lu, ret:0x%X", nav_table_size, rt_ret);
                  GELOGE(RT_FAILED, "[Call][RtMalloc] failed, size:%lu, ret:0x%X", nav_table_size, rt_ret);
                  return RT_ERROR_TO_GE_STATUS(rt_ret);)
  rt_ret = rtMemcpy(static_cast<void *>(hbm_nav_table_addr), nav_table_size,
                    static_cast<void *>(nav_table.data()), nav_table_size, RT_MEMCPY_HOST_TO_DEVICE);
  GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                  REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, size:%lu, ret:0x%X", nav_table_size, rt_ret);
                  GELOGE(RT_FAILED, "[Call][RtMemcpy] failed, size:%lu, ret:0x%X", nav_table_size, rt_ret);
                  GE_CHK_RT(rtFree(hbm_nav_table_addr)); return RT_ERROR_TO_GE_STATUS(rt_ret);)
  // Create the necessary metadata for the super kernel
  h = MakeUnique<skt::SuperKernel>(this->func_stub_, hbm_nav_table_addr, nav_table_size, block_dim);
  return SUCCESS;
}
}  // namespace skt
}  // namespace ge
