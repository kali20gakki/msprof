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

#include "single_op/task/tbe_task_builder.h"

#include <mutex>
#include <vector>

#include "common/math_util.h"
#include "common/plugin/ge_util.h"
#include "common/tbe_handle_store/tbe_handle_store.h"
#include "graph/def_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "runtime/rt.h"
#include "single_op/task/build_task_utils.h"

namespace ge {
namespace {
const std::string kSupportDynamicShape = "support_dynamicshape";
const std::string kOpParamSize = "op_para_size";
const std::string kAtomicOpParamSize = "atomic_op_para_size";
std::mutex g_reg_mutex;
}  // namespace

TbeTaskBuilder::TbeTaskBuilder(const std::string &model_name, const NodePtr &node, const domi::TaskDef &task_def)
    : node_(node),
      op_desc_(node->GetOpDesc()),
      task_def_(task_def),
      kernel_def_(task_def.kernel()),
      kernel_def_with_handle_(task_def.kernel_with_handle()),
      model_name_(model_name) {}

TBEKernelPtr TbeTaskBuilder::GetTbeKernel(const OpDescPtr &op_desc) const {
  return op_desc->TryGetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, TBEKernelPtr());
}

void TbeTaskBuilder::GetKernelName(const OpDescPtr &op_desc, std::string &kernel_name) const {
  (void)AttrUtils::GetStr(op_desc, op_desc->GetName() + "_kernelname", kernel_name);
}

Status TbeTaskBuilder::DoRegisterBinary(const OpKernelBin &kernel_bin, void **const bin_handle,
                                        const SingleOpModelParam &param) const {
  rtDevBinary_t binary;
  binary.version = 0U;
  binary.data = kernel_bin.GetBinData();
  binary.length = kernel_bin.GetBinDataSize();
  GE_CHK_STATUS_RET_NOLOG(GetMagic(binary.magic));
  rtError_t ret = RT_ERROR_NONE;
  if (task_def_.type() == RT_MODEL_TASK_ALL_KERNEL) {
    ret = rtRegisterAllKernel(&binary, bin_handle);
  } else {
    ret = rtDevBinaryRegister(&binary, bin_handle);
  }
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[DoRegister][Binary] failed, bin key = %s, core_type = %ld, rt ret = %d", stub_name_.c_str(),
        param.core_type, static_cast<int32_t>(ret));
    REPORT_CALL_ERROR("E19999", "DoRegisterBinary failed, bin key = %s, core_type = %ld, rt ret = %d",
        stub_name_.c_str(), param.core_type, static_cast<int32_t>(ret));
    return FAILED;
  }

  return SUCCESS;
}

Status TbeTaskBuilder::DoRegisterMeta(void *const bin_handle) const {
  std::string meta_data;
  (void)AttrUtils::GetStr(op_desc_, GetKeyForTvmMetaData(), meta_data);
  GELOGI("TBE: meta data: %s", meta_data.empty() ? "null" : meta_data.c_str());
  if (!meta_data.empty()) {
    const auto rt_ret = rtMetadataRegister(bin_handle, meta_data.c_str());
    if (rt_ret != RT_ERROR_NONE) {
      GELOGE(FAILED, "[Invoke][rtMetadataRegister] failed. bin key = %s, meta_data = %s, rt ret = %d",
          stub_name_.c_str(), meta_data.c_str(), rt_ret);
      REPORT_CALL_ERROR("E19999", "rtMetadataRegister failed, bin key = %s, meta_data = %s, rt ret = %d",
          stub_name_.c_str(), meta_data.c_str(), rt_ret);
      return FAILED;
    }
  }

  return SUCCESS;
}

Status TbeTaskBuilder::DoRegisterFunction(void *const bin_handle, const char_t *const stub_name,
                                          const char_t *const kernel_name) {
  const auto rt_ret = rtFunctionRegister(bin_handle, stub_name, stub_name, kernel_name, FUNC_MODE_NORMAL);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[Invoke][rtFunctionRegister] failed. bin key = %s, kernel name = %s, rt ret = %d",
        stub_name, kernel_name, static_cast<int32_t>(rt_ret));
    REPORT_CALL_ERROR("E19999", "rtFunctionRegister failed. bin key = %s, kernel name = %s, rt ret = %d",
        stub_name, kernel_name, static_cast<int32_t>(rt_ret));
    return static_cast<uint32_t>(rt_ret);
  }

  return SUCCESS;
}

Status TbeTaskBuilder::DoRegisterKernel(const ge::OpKernelBin &tbe_kernel, const char_t *const bin_file_key,
                                        void **const bin_handle, const SingleOpModelParam &param) {
  void *handle = nullptr;
  auto ret = DoRegisterBinary(tbe_kernel, &handle, param);
  if (ret != SUCCESS) {
    return ret;
  }
  if (task_def_.type() == RT_MODEL_TASK_ALL_KERNEL) {
    *bin_handle = handle;
    return SUCCESS;
  }

  ret = DoRegisterMeta(handle);
  if (ret != SUCCESS) {
    GE_CHK_RT(rtDevBinaryUnRegister(handle));
    return ret;
  }

  std::string kernel_name;
  GetKernelName(op_desc_, kernel_name);
  ret = DoRegisterFunction(handle, bin_file_key, kernel_name.c_str());
  if (ret != SUCCESS) {
    GE_CHK_RT(rtDevBinaryUnRegister(handle));
    return ret;
  }

  GELOGI("Register function succeeded: kernel_name = %s", kernel_name.c_str());
  *bin_handle = handle;
  return SUCCESS;
}

Status TbeTaskBuilder::RegisterKernel(TbeOpTask &task, const SingleOpModelParam &param) {
  KernelBinRegistry &registry = KernelBinRegistry::GetInstance();
  // check if already registered
  const char_t *stub_func = registry.GetStubFunc(stub_name_);
  if (stub_func != nullptr) {
    task.SetStubFunc(stub_name_, stub_func);
    return SUCCESS;
  }

  // to avoid repeat register
  const std::lock_guard<std::mutex> lock(g_reg_mutex);
  // check again
  stub_func = registry.GetStubFunc(stub_name_);
  if (stub_func == nullptr) {
    stub_func = registry.GetUnique(stub_name_);
    GELOGI("RegisterKernel begin, stub_func = %s", stub_func);
    const auto tbe_kernel = GetTbeKernel(op_desc_);
    if (tbe_kernel == nullptr) {
      GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Get][TbeKernel] fail for OP EXT ATTR NAME TBE_KERNEL not found. op = %s",
          op_desc_->GetName().c_str());
      REPORT_CALL_ERROR("E19999", "GetTbeKernel fail for OP EXT ATTR NAME TBE_KERNEL not found. op = %s",
          op_desc_->GetName().c_str());
      return ACL_ERROR_GE_INTERNAL_ERROR;
    }

    auto holder = MakeUnique<KernelHolder>(stub_func, tbe_kernel);
    if (holder == nullptr) {
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][KernelHodler] failed.");
      REPORT_INNER_ERROR("E19999", "Create KernelHodler failed.");
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }

    void *bin_handle = nullptr;
    const auto ret = DoRegisterKernel(*tbe_kernel, stub_func, &bin_handle, param);
    if (ret != SUCCESS) {
      GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Register][Kernel] failed. stub name = %s", stub_name_.c_str());
      REPORT_CALL_ERROR("E19999", "DoRegisterKernel failed, stub name = %s", stub_name_.c_str());
      return ACL_ERROR_GE_INTERNAL_ERROR;
    }
    holder->SetBinHandle(bin_handle);
    if (!registry.AddKernel(stub_name_, std::move(holder))) {
      // should not happen. only one thread can reach here
      GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Add][Kernel] failed. stub name = %s", stub_name_.c_str());
      REPORT_CALL_ERROR("E19999", "AddKernel failed. stub name = %s", stub_name_.c_str());
      return ACL_ERROR_GE_INTERNAL_ERROR;
    }
  }

  task.SetStubFunc(stub_name_, stub_func);
  return SUCCESS;
}

Status TbeTaskBuilder::RegisterKernelWithHandle(const SingleOpModelParam &param) {
  GELOGD("RegisterKernelWithHandle begin.");
  HandleRegistry &registry = HandleRegistry::GetInstance();
  const auto tbe_kernel = GetTbeKernel(op_desc_);
  if (tbe_kernel == nullptr) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Get][TbeKernel] fail for OP EXT ATTR NAME TBE_KERNEL not found. op = %s",
        op_desc_->GetName().c_str());
    REPORT_CALL_ERROR("E19999", "GetTbeKernel fail for OP EXT ATTR NAME TBE_KERNEL not found. op = %s",
        op_desc_->GetName().c_str());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  void *bin_handle = nullptr;
  const auto ret = DoRegisterKernel(*tbe_kernel, nullptr, &bin_handle, param);
  if (ret != SUCCESS) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Register][Kernel] failed. node name = %s", op_desc_->GetName().c_str());
    REPORT_CALL_ERROR("E19999", "DoRegisterKernel failed, node name = %s", op_desc_->GetName().c_str());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  handle_ = bin_handle;
  auto holder = MakeUnique<HandleHolder>(handle_);
  if (holder == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][HandleHolder] failed.");
    REPORT_INNER_ERROR("E19999", "Create HandleHolder failed.");
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }
  if (!registry.AddHandle(std::move(holder))) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Add][Handle] failed. node name = %s", op_desc_->GetName().c_str());
    REPORT_CALL_ERROR("E19999", "AddHandle failed, node name = %s", op_desc_->GetName().c_str());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status TbeTaskBuilder::InitKernelArgs(void *const args_addr, const size_t arg_size, const SingleOpModelParam &param) {
  // copy args
  std::vector<void *> tensor_device_addr_vec = BuildTaskUtils::GetKernelArgs(op_desc_, param);
  void *const src_addr = reinterpret_cast<void *>(tensor_device_addr_vec.data());
  const uint64_t src_len = sizeof(void *) * tensor_device_addr_vec.size();
  GE_CHK_RT_RET(rtMemcpy(args_addr, arg_size, src_addr, src_len, RT_MEMCPY_HOST_TO_HOST));
  return SUCCESS;
}

Status TbeTaskBuilder::UpdateTilingArgs(TbeOpTask &task, const size_t index, const size_t tiling_arg_index) const {
  if (task.need_tiling_) {
    GE_CHECK_GE(tiling_arg_index, sizeof(void *));
    REQUIRE_COMPAT_UINT16(tiling_arg_index);
    REQUIRE_COMPAT_UINT16(tiling_arg_index - sizeof(void *));
    task.args_ex_.tilingAddrOffset = static_cast<uint16_t>(tiling_arg_index - (index * sizeof(void *)));
    task.args_ex_.tilingDataOffset = static_cast<uint16_t>(tiling_arg_index);
    task.args_ex_.hasTiling = true;
    task.tiling_data_idx_ = tiling_arg_index / sizeof(void *);
  }
  return SUCCESS;
}

Status TbeTaskBuilder::SetKernelArgs(TbeOpTask &task, const SingleOpModelParam &param, const OpDescPtr &op_desc) {
  bool is_dynamic = false;
  (void)AttrUtils::GetBool(op_desc_, kSupportDynamicShape, is_dynamic);
  if (is_dynamic) {
    GE_CHK_STATUS_RET_NOLOG(InitTilingInfo(task));
  }

  const auto task_type = static_cast<rtModelTaskType_t>(task_def_.type());
  const bool is_task_all_kernel = (task_type == RT_MODEL_TASK_ALL_KERNEL);
  size_t arg_size = 0U;
  size_t kernel_def_arg_size = 0U;
  const size_t default_addr_index = (task.overflow_addr_ != nullptr) ? 2U : 1U; // // distance of tiling_addr to the end
  std::unique_ptr<uint8_t[]> args = nullptr;
  const void *kernel_def_args = nullptr;
  if (is_task_all_kernel) {
    GELOGD("SetKernelArgs of %s in branch of RT_MODEL_TASK_ALL_KERNEL.", op_desc->GetName().c_str());
    kernel_def_arg_size = kernel_def_with_handle_.args_size();
    GE_CHECK_GE(kernel_def_with_handle_.args().size(), kernel_def_arg_size);
    kernel_def_args = kernel_def_with_handle_.args().data();
  } else {
    GELOGD("SetKernelArgs of %s in branch of RT_MODEL_TASK_KERNEL.", op_desc->GetName().c_str());
    kernel_def_arg_size = kernel_def_.args_size();
    GE_CHECK_GE(kernel_def_.args().size(), kernel_def_arg_size);
    kernel_def_args = kernel_def_.args().data();
  }

  arg_size = kernel_def_arg_size + task.max_tiling_size_ +
      (task.extend_args_for_host_input_ ? kMaxHostMemInputLen : 0U) +
      ((task.overflow_addr_ != nullptr) ? sizeof(void *) : 0U);
  REQUIRE_COMPAT_UINT16(arg_size);
  args = MakeUnique<uint8_t[]>(arg_size);
  GE_CHECK_NOTNULL(args);
  GE_CHK_RT_RET(rtMemcpy(args.get(), arg_size, kernel_def_args, kernel_def_arg_size,
                         RT_MEMCPY_HOST_TO_HOST));
  const domi::KernelContext &context = (task_type == RT_MODEL_TASK_ALL_KERNEL) ?
                                       kernel_def_with_handle_.context() : kernel_def_.context();
  const auto *const args_offset_tmp = PtrToPtr<const char_t, const uint16_t>(context.args_offset().data());
  const uint16_t offset = *args_offset_tmp;
  GE_CHK_STATUS_RET_NOLOG(InitKernelArgs(args.get() + static_cast<int32_t>(offset), arg_size - offset, param));

  if (is_task_all_kernel) {
    task.SetKernelWithHandleArgs(std::move(args), arg_size, kernel_def_with_handle_.block_dim(), op_desc,
                                 kernel_def_with_handle_);
  } else {
    task.SetKernelArgs(std::move(args), arg_size, kernel_def_.block_dim(), op_desc);
  }
  
  if (task.overflow_addr_ != nullptr) {
    GE_CHK_RT_RET(rtMemcpy(args.get() + kernel_def_arg_size, sizeof(void *), &(task.overflow_addr_), sizeof(void *),
                           RT_MEMCPY_HOST_TO_HOST));
    kernel_def_arg_size += sizeof(void *);
    GE_CHECK_GE(kernel_def_arg_size, default_addr_index * sizeof(void *));
  }
  if ((!param.graph_is_dynamic) && task.need_tiling_) {
    GELOGD("Need to update run info when graph is static with dynamic node: %s.", op_desc->GetName().c_str());
    uintptr_t *const arg_base = PtrToPtr<uint8_t, uintptr_t>(task.args_.get());
    GE_CHECK_GE(kernel_def_arg_size, sizeof(void *));
    REQUIRE_COMPAT_UINT16(kernel_def_arg_size);
    const uint32_t tiling_data_idx = static_cast<uint32_t>(kernel_def_arg_size / sizeof(void *));
    const uint32_t tiling_arg_idx = tiling_data_idx - default_addr_index;
    task.tiling_data_idx_ = tiling_data_idx;
    arg_base[tiling_arg_idx] = static_cast<uintptr_t>(PtrToValue(arg_base + tiling_data_idx));
    GE_CHK_STATUS_RET_NOLOG(task.UpdateRunInfo());
  }

  task.args_ex_.args = task.args_.get();
  task.args_ex_.argsSize = arg_size;
  GE_CHK_STATUS_RET_NOLOG(UpdateTilingArgs(task, default_addr_index, kernel_def_arg_size));
  if (task.extend_args_for_host_input_) {
    task.args_item_offsets_.host_input_data_offset = kernel_def_arg_size;
  }

  task.run_info_ = MakeUnique<optiling::utils::OpRunInfo>(0, true, 0);
  GE_CHECK_NOTNULL(task.run_info_);
  return SUCCESS;
}

Status TbeTaskBuilder::BuildTask(TbeOpTask &task, const SingleOpModelParam &param) {
  GELOGD("Build tbe task begin");
  task.input_num_ = op_desc_->GetInputsSize();
  task.output_num_ = op_desc_->GetOutputsSize();
  task.SetOpDesc(op_desc_);
  auto ret = SetKernelArgs(task, param, op_desc_);
  if (ret != SUCCESS) {
    return ret;
  }

  const auto task_type = static_cast<rtModelTaskType_t>(task_def_.type());
  if (task_type == RT_MODEL_TASK_ALL_KERNEL) {
    stub_name_ = model_name_ + "/" + node_->GetName() + "_tvmbin";
    ret = RegisterKernelWithHandle(param);
  } else {
    const domi::KernelDef &kernel_def = task_def_.kernel();
    stub_name_ = model_name_ + "/" + kernel_def.stub_func() + "_tvmbin";
    ret = RegisterKernel(task, param);
  }

  task.SetHandle(handle_);
  if (ret != SUCCESS) {
    return ret;
  }

  const auto task_info = BuildTaskUtils::GetTaskInfo(op_desc_);
  GELOGI("[TASK_INFO] %s %s", stub_name_.c_str(), task_info.c_str());

  if (task_type != RT_MODEL_TASK_ALL_KERNEL) {
    void *stub_func = nullptr;
    const auto rt_ret = rtGetFunctionByName(stub_name_.c_str(), &stub_func);
    if (rt_ret != RT_ERROR_NONE) {
      GELOGE(FAILED, "[Get][FunctionByName] failed. stub_name:%s.", stub_name_.c_str());
      REPORT_CALL_ERROR("E19999", "rtGetFunctionByName failed, stub_name:%s.", stub_name_.c_str());
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
    task.SetStubFunc(stub_name_, stub_func);
  }
  GE_CHK_STATUS_RET(task.SetArgIndex(), "[Set][ArgTable] failed.");

  return SUCCESS;
}

Status TbeTaskBuilder::InitTilingInfo(TbeOpTask &task) {
  GELOGD("Start alloc tiling data of node %s.", op_desc_->GetName().c_str());
  int64_t max_size = -1;
  (void)AttrUtils::GetInt(op_desc_, GetKeyForOpParamSize(), max_size);
  GELOGD("Got op param size by key: %s, ret = %ld", GetKeyForOpParamSize().c_str(), max_size);
  if (max_size < 0) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Get][Int] %s Invalid op_param_size: %ld.",
        op_desc_->GetName().c_str(), max_size);
    REPORT_CALL_ERROR("E19999", "AttrUtils::GetInt failed, %s Invalid op_param_size: %ld.",
        op_desc_->GetName().c_str(), max_size);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  task.EnableDynamicSupport(node_, static_cast<uint32_t>(max_size));
  return SUCCESS;
}

Status TbeTaskBuilder::GetMagic(uint32_t &magic) const {
  std::string json_string;
  GE_IF_BOOL_EXEC(AttrUtils::GetStr(op_desc_, TVM_ATTR_NAME_MAGIC, json_string),
                  GELOGD("Get original type of session_graph_id."));
  if (json_string == "RT_DEV_BINARY_MAGIC_ELF") {
    magic = RT_DEV_BINARY_MAGIC_ELF;
  } else if (json_string == "RT_DEV_BINARY_MAGIC_ELF_AIVEC") {
    magic = RT_DEV_BINARY_MAGIC_ELF_AIVEC;
  } else if (json_string == "RT_DEV_BINARY_MAGIC_ELF_AICUBE") {
    magic = RT_DEV_BINARY_MAGIC_ELF_AICUBE;
  } else {
    REPORT_INNER_ERROR("E19999", "Attr:%s in op:%s(%s), value:%s check invalid",
                       TVM_ATTR_NAME_MAGIC.c_str(), op_desc_->GetName().c_str(),
                       op_desc_->GetType().c_str(), json_string.c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Attr:%s in op:%s(%s), value:%s check invalid",
           TVM_ATTR_NAME_MAGIC.c_str(), op_desc_->GetName().c_str(),
           op_desc_->GetType().c_str(), json_string.c_str());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

std::string TbeTaskBuilder::GetKeyForOpParamSize() const {
  return kOpParamSize;
}

std::string TbeTaskBuilder::GetKeyForTvmMetaData() const {
  return TVM_ATTR_NAME_METADATA;
}

Status AtomicAddrCleanTaskBuilder::InitKernelArgs(void *const args_addr, const size_t arg_size,
                                                  const SingleOpModelParam &param) {
  (void)args_addr;
  (void)arg_size;
  (void)param;
  return SUCCESS;
}

std::string AtomicAddrCleanTaskBuilder::GetKeyForOpParamSize() const {
  return kAtomicOpParamSize;
}

std::string AtomicAddrCleanTaskBuilder::GetKeyForTvmMetaData() const {
  return ATOMIC_ATTR_TVM_METADATA;
}

void AtomicAddrCleanTaskBuilder::GetKernelName(const OpDescPtr &op_desc, std::string &kernel_name) const {
  (void)AttrUtils::GetStr(op_desc, op_desc->GetName() + "_atomic_kernelname", kernel_name);
}

TBEKernelPtr AtomicAddrCleanTaskBuilder::GetTbeKernel(const OpDescPtr &op_desc)  const {
  return op_desc->TryGetExtAttr(EXT_ATTR_ATOMIC_TBE_KERNEL, TBEKernelPtr());
}

}  // namespace ge
