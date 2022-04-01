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

#include "graph/load/model_manager/task_info/super_kernel/super_kernel.h"

namespace ge {
namespace skt {
Status SuperKernel::Launch(rtStream_t stream, const uint32_t dump_flag, const char_t *const op_name) {
  const void *func_stub = this->GetFuncStub();

  const void *args[] = {this->GetNavTablePtr(),
                        ValueToPtr(this->GetNavTableSize())};

  rtError_t rt_ret = rtMalloc(reinterpret_cast<void **>(&device_args_addr_), sizeof(args), RT_MEMORY_HBM);
  GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                  REPORT_CALL_ERROR("E19999", "Call rtMalloc failed, size:%lu, ret:0x%X", sizeof(args), rt_ret);
                  GELOGE(RT_FAILED, "[Call][RtMalloc] failied, size:%lu, ret:0x%X", sizeof(args), rt_ret);
                  return RT_ERROR_TO_GE_STATUS(rt_ret);)
  rt_ret = rtMemcpy(reinterpret_cast<void *>(device_args_addr_), sizeof(args), reinterpret_cast<void *>(args),
                    sizeof(args), RT_MEMCPY_HOST_TO_DEVICE);
  GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE,
                  (void)rtFree(device_args_addr_);
                  REPORT_CALL_ERROR("E19999", "Call rtMemcpy failed, size:%lu, ret:0x%X", sizeof(args), rt_ret);
                  GELOGE(RT_FAILED, "[Call][RtMemcpy] failied, size:%lu, ret:0x%X", sizeof(args), rt_ret);
                  return RT_ERROR_TO_GE_STATUS(rt_ret);)
  SetTaskTag(op_name);
  rtArgsEx_t args_ex = {};
  args_ex.args = device_args_addr_;
  args_ex.argsSize = sizeof(args);
  rt_ret = rtKernelLaunchWithFlag(func_stub, block_dim_, &args_ex, nullptr, stream, dump_flag);
  GE_IF_BOOL_EXEC(
      rt_ret != RT_ERROR_NONE,
      (void)rtFree(device_args_addr_);
      REPORT_CALL_ERROR("E19999", "Call rtKernelLaunchWithFlag failed, dump_flag:%u, ret:0x%X", dump_flag, rt_ret);
      GELOGE(RT_FAILED, "[Call][rtKernelLaunchWithFlag] failied. error: 0x%X", rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);)
  return SUCCESS;
}

Status SuperKernel::Launch(rtStream_t stream, const uint32_t dump_flag) {
  return Launch(stream, dump_flag, nullptr);
}

void SuperKernel::SetTaskTag(const char_t *const op_name) {
  if (op_name != nullptr) {
    GE_CHK_RT(rtSetTaskTag(op_name));
  }
}
}  // namespace skt
}  // namespace ge
