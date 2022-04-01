/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "adapter/tbe_adapter/kernel_launch/tbe_kernel_launch.h"
#include "common/aicore_util_attr_define.h"
#include "common/aicore_util_constants.h"
#include "common/comm_error_codes.h"
#include "common/comm_log.h"
#include "runtime/rt_error_codes.h"
#include "runtime/mem.h"
#include "common/configuration.h"

namespace fe {
static const uint32_t MIN_ARG_SIZE = 4;
static const uint32_t ARG_ENTRY_SIZE = 2624;              // 1536 + 1024(tiling) + 64(host mem)
static const uint32_t MULTI_GRAPH_ARG_ENTRY_SIZE = 6208;  // 5120 + 1024(tiling) + 64(host mem)
static const uint32_t ARG_ENTRY_INCREMENT_SIZE = 15360;   // 15k
uint32_t g_args_count = 1;
uint16_t g_args_offset[ARG_ENTRY_SIZE / MIN_ARG_SIZE];

Status TbeKernelLaunch::DealKernelLaunch(const ge::Node &node, const void *args,
                                         const uint32_t &args_size, const std::string &stub_func,
                                         const uint32_t &core_dim, domi::TaskDef &task_def) {
  string op_name = node.GetName();
  string op_type = node.GetType();
  auto op_desc = node.GetOpDesc();
  // 1. malloc  args_size + append_args_size
  void *all_args_buff = nullptr;
  size_t append_args_size = GetAppendArgsSizeOf() * GetAppendArgsNum();
  uint32_t total_args_size = args_size + append_args_size;
  if (rtMallocHost(&all_args_buff, total_args_size) != ACL_RT_SUCCESS) {
    return TASK_BUILDER_STATUS_INTERNAL_ERROR;
  }
  CM_CHECK_NOTNULL(all_args_buff);

  // 2. memcpy for args
  if (rtMemcpy(all_args_buff, args_size, args, args_size, RT_MEMCPY_HOST_TO_HOST) != SUCCESS) {
    if (rtFreeHost(all_args_buff) != ACL_RT_SUCCESS) {
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }
    all_args_buff = nullptr;
    return TASK_BUILDER_STATUS_INTERNAL_ERROR;
  }

  // 4. memcpy form append_args
  if (AddAppendArgs(node, all_args_buff, args_size) != SUCCESS) {
    if (rtFreeHost(all_args_buff) != ACL_RT_SUCCESS) {
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }
    return TASK_BUILDER_STATUS_INTERNAL_ERROR;
  }

  // 5. call KernelLaunch
  CM_LOGD("Op[name=%s,type=%s]: args_size:%u bytes, append_args_size:%zu bytes, total_args_size:%u bytes.",
          op_name.c_str(), op_type.c_str(), args_size, append_args_size, total_args_size);
  if (append_args_size > 0) {
    PrintAllArgs(op_name, op_type, all_args_buff, args_size);
  }

  bool ret = false;
  std::string first_kernel_name;
  if (ge::AttrUtils::GetStr(op_desc, ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name)) {
    CM_LOGD("Node name is[%s], first kernel name is[%s].", op_name.c_str(), first_kernel_name.c_str());
    ret = KernelLaunchWithHandle(core_dim, all_args_buff, total_args_size, nullptr, task_def);
  } else {
    ret = KernelLaunch(stub_func, core_dim, all_args_buff, total_args_size, nullptr, task_def);
  }

  if (!ret) {
    if (rtFreeHost(all_args_buff) != ACL_RT_SUCCESS) {
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }
    return TASK_BUILDER_STATUS_RUNTIME_ERROR;
  }

  // 6.free all_args_buff
  if (rtFreeHost(all_args_buff) != ACL_RT_SUCCESS) {
    return TASK_BUILDER_STATUS_INTERNAL_ERROR;
  }
  all_args_buff = nullptr;
  return SUCCESS;
}

void TbeKernelLaunch::PrintAllArgs(const string &op_name, const string &op_type, const void *all_args_buff,
                                   uint32_t args_size) {
  for (size_t i = 0; i != args_size / sizeof(uint64_t); ++i) {
    uint64_t value = *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(all_args_buff) + i * sizeof(uint64_t)));
    CM_LOGD("Op[name=%s,type=%s]: args[%zu]=[%lu].", op_name.c_str(), op_type.c_str(), i, value);
  }

  for (size_t i = 0; i != GetAppendArgsNum(); ++i) {
    uint64_t value = *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(all_args_buff) + args_size +
                                                    i * GetAppendArgsSizeOf()));
    CM_LOGD("Op[name=%s,type=%s]: append_args[%zu]=[%lu].", op_name.c_str(), op_type.c_str(), i, value);
  }
}

size_t TbeKernelLaunch::GetAppendArgsSizeOf() {
  return 0;
}
size_t TbeKernelLaunch::GetAppendArgsNum() {
  return 0;
}
Status TbeKernelLaunch::AddAppendArgs(const ge::Node &node, void *all_args_buff, const uint32_t &args_size) {
  (void)node;
  (void)all_args_buff;
  (void)args_size;
  return SUCCESS;
}

bool TbeKernelLaunch::KernelLaunch(const std::string &stub_func, uint32_t block_dim, const void *args,
                                   uint32_t args_size, const rtSmDesc_t *sm_desc, domi::TaskDef &task_def) {
  task_def.set_type(static_cast<uint32_t>(RT_MODEL_TASK_KERNEL));
  domi::KernelDef *kernel_def = task_def.mutable_kernel();
  if (kernel_def == nullptr) {
    CM_LOGE("[GenTask][KernelLaunch] kernel_def is nullptr.");
    return false;
  }

  CM_LOGD("[GenTask][KernelLaunch] stub_func_name is [%s].", stub_func.c_str());
  kernel_def->set_stub_func(stub_func);
  if (sm_desc != nullptr) {
    uintptr_t sm_desc_data = reinterpret_cast<uintptr_t>(sm_desc);
    uint8_t *sm_desc_ptr = reinterpret_cast<uint8_t *>(sm_desc_data);
    kernel_def->set_sm_desc(sm_desc_ptr, sizeof(rtSmDesc_t));
  }

  kernel_def->set_block_dim(block_dim);

  uint32_t item_size = Configuration::Instance(AI_CORE_NAME).IsLhisiSoc() ? ARG_ENTRY_SIZE : MULTI_GRAPH_ARG_ENTRY_SIZE;
  if (args_size > item_size) {
    item_size += ARG_ENTRY_INCREMENT_SIZE;
  }
  if (item_size <= args_size) {
    args_size = item_size;
  }
  kernel_def->set_args_size(args_size);
  kernel_def->set_args(args, args_size);

  domi::KernelContext *kernel_context = kernel_def->mutable_context();
  if (kernel_context == nullptr) {
    REPORT_CM_ERROR("[GenTask][KernelLaunch] kernel_context is nullptr.");
    return false;
  }
  g_args_offset[0] = 0;
  kernel_context->set_args_count(g_args_count);
  kernel_context->set_args_offset(g_args_offset, g_args_count * sizeof(uint16_t));
  return true;
}

bool TbeKernelLaunch::KernelLaunchWithHandle(uint32_t block_dim, const void *args, uint32_t args_size,
                                             const rtSmDesc_t *sm_desc, domi::TaskDef &task_def) {
  task_def.set_type(static_cast<uint32_t>(RT_MODEL_TASK_ALL_KERNEL));
  domi::KernelDefWithHandle *kernel_def_with_handle = task_def.mutable_kernel_with_handle();
  if (kernel_def_with_handle == nullptr) {
    CM_LOGE("[GenTask][KernelLaunchWithHandle] kernel_def_with_handle is nullptr.");
    return false;
  }

  if (sm_desc != nullptr) {
    uintptr_t sm_desc_data = reinterpret_cast<uintptr_t>(sm_desc);
    uint8_t *sm_desc_ptr = reinterpret_cast<uint8_t *>(sm_desc_data);
    kernel_def_with_handle->set_sm_desc(sm_desc_ptr, sizeof(rtSmDesc_t));
  }
  kernel_def_with_handle->set_block_dim(block_dim);

  uint32_t item_size = Configuration::Instance(AI_CORE_NAME).IsLhisiSoc() ? ARG_ENTRY_SIZE : MULTI_GRAPH_ARG_ENTRY_SIZE;
  if (args_size > item_size) {
    item_size += ARG_ENTRY_INCREMENT_SIZE;
  }
  if (item_size <= args_size) {
    args_size = item_size;
  }
  kernel_def_with_handle->set_args_size(args_size);
  kernel_def_with_handle->set_args(args, args_size);

  domi::KernelContext *kernel_context = kernel_def_with_handle->mutable_context();
  if (kernel_context == nullptr) {
    CM_LOGE("[GenTask][KernelLaunch] kernel_context is nullptr.");
    return false;
  }
  g_args_offset[0] = 0;
  kernel_context->set_args_count(g_args_count);
  kernel_context->set_args_offset(g_args_offset, g_args_count * sizeof(uint16_t));
  return true;
}

}  // namespace fe