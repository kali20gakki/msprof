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

#include "graph/load/model_manager/tbe_kernel_handle.h"

#include "common/tbe_handle_store/tbe_handle_store.h"
#include "framework/common/profiling_definitions.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
namespace {
const std::string kStubFuncName = "_register_stub_func";
const std::string kAutoAttrPrefix = "_thread_";
const std::string kAllKernel = "_kernel_list_first_name";
const std::string kMixAicAllKernel = "_mix_aic_kernel_list_first_name";
const std::string kMixAivAllKernel = "_mix_aiv_kernel_list_first_name";

const std::map<std::string, uint32_t> kMixBinaryMagic = {
    {"_mix_aiv", RT_DEV_BINARY_MAGIC_ELF_AIVEC},
    {"_mix_aic", RT_DEV_BINARY_MAGIC_ELF_AICUBE}
};

std::string GetKernelKey(const OpDesc &op_desc, const std::string &prefix) {
  if (prefix.empty()) {
    return op_desc.GetName();
  }
  std::string kernel_name;
  (void)AttrUtils::GetStr(op_desc, prefix + ATTR_NAME_TBE_KERNEL_NAME, kernel_name);
  return kernel_name;
}
}  // namespace

bool IsTbeTask(const OpDescPtr &op_desc) {
  uint32_t run_mode = static_cast<uint32_t>(domi::ImplyType::INVALID);
  if (!AttrUtils::GetInt(op_desc, ATTR_NAME_IMPLY_TYPE, run_mode)) {
    return false;
  }

  if (run_mode != static_cast<uint32_t>(domi::ImplyType::TVM)) {
    return false;
  }

  // Skip no_task operator, such as concat and split.
  bool attr_no_task = false;
  const bool get_attr_no_task_flag = AttrUtils::GetBool(op_desc, ATTR_NAME_NOTASK, attr_no_task);
  if (get_attr_no_task_flag && attr_no_task) {
    GELOGI("Node[name:%s, type:%s] does not generate task, skip initialization.",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return false;
  }

  return true;
}

bool IsAllKernelTask(const OpDescPtr &op_desc) {
  return op_desc->HasAttr(kAllKernel) || op_desc->HasAttr(kMixAicAllKernel) || op_desc->HasAttr(kMixAivAllKernel);
}

std::mutex TBEKernelHandle::tvm_bin_mutex_;

///
/// @ingroup ge
/// @brief get unique identification for op when load two or more models
/// @param [in] const OpDescPtr: current op.
/// @param [in] std::string identification: unique identification for current op.
/// @return SUCCESS handle successfully / others handle failed
///
void TBEKernelHandle::GetUniqueId(const OpDescPtr &op_desc, std::string &unique_identification) const {
  std::string session_graph_id;
  if ((!AttrUtils::GetStr(op_desc, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) || session_graph_id.empty()) {
    return;
  }

  unique_identification = session_graph_id;
  if (session_graph_id.find("-1") != std::string::npos) {
    unique_identification +=  std::string("_" + std::to_string(model_id_));
  }
}

///
/// @ingroup ge
/// @brief For TVM Op, avoid Addr Reuse.
/// @return void*
///
const char_t *TBEKernelHandle::GetRegisterStub(const std::string &binfile, const std::string &session_graph_id) {
  const std::string binfile_key = session_graph_id.empty() ? binfile : (session_graph_id + "_" + binfile);
  std::set<std::string>::const_iterator it = tvm_bin_kernel_.find(binfile_key);
  if (it != tvm_bin_kernel_.cend()) {
    return it->c_str();
  } else {
    it = tvm_bin_kernel_.insert(tvm_bin_kernel_.cend(), binfile_key);
    return it->c_str();
  }
}

Status TBEKernelHandle::GetAddrAndPrefCnt(const std::string &kernel_name, void *&addr, uint32_t &pref_cnt) const {
  const std::map<std::string, std::pair<void *, uint32_t>>::const_iterator iter = addr_and_pref_cnt_.find(kernel_name);
  if (iter == addr_and_pref_cnt_.cend()) {
    REPORT_INNER_ERROR("E19999", "Get addr and pref cnt failed, kernel_name:%s", kernel_name.c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] Get addr and pref cnt failed, kernel_name:%s", kernel_name.c_str());
    return INTERNAL_ERROR;
  }
  addr = iter->second.first;
  pref_cnt = iter->second.second;
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Register TBE kernel to RTS for common Task.
/// @param [in] op_desc : current op.
/// @param [in] tbe_kernel: kernel handle for current op.
/// @param [in] prefix: kernel name attr name prefix.
/// @param [in] tbe_kernel_store: store to find kernel.
/// @return SUCCESS / other error code.
///
Status TBEKernelHandle::RegisterHandle(const OpDescPtr &op_desc, const TBEKernelPtr &tbe_kernel,
                                       const std::string &prefix, const TBEKernelStore &tbe_kernel_store) {
  GE_CHECK_NOTNULL(op_desc);
  if (tbe_kernel != nullptr) {
    return FunctionRegister(op_desc, op_desc->GetName(), tbe_kernel, prefix);
  }
  const std::string ext_kernel_name = prefix + std::string(OP_EXTATTR_NAME_TBE_KERNEL);
  auto tbe_kernel_try_get = op_desc->TryGetExtAttr(ext_kernel_name, TBEKernelPtr());
  if (tbe_kernel_try_get == nullptr) {
    const auto &kernel_key = GetKernelKey(*op_desc, prefix);
    tbe_kernel_try_get = tbe_kernel_store.FindKernel(kernel_key);
    if (tbe_kernel_try_get == nullptr) {
      REPORT_CALL_ERROR("E19999", "TBE: %s can't find tvm bin file with attr name [%s]!", op_desc->GetName().c_str(),
                        ext_kernel_name.c_str());
      GELOGE(INTERNAL_ERROR, "[Invoke][TryGetExtAttr]TBE: %s can't find tvm bin file with attr name [%s]!",
             op_desc->GetName().c_str(), ext_kernel_name.c_str());
      return INTERNAL_ERROR;
    }
  }
  return FunctionRegister(op_desc, op_desc->GetName(), tbe_kernel_try_get, prefix);
}

///
/// @ingroup ge
/// @brief Register TBE kernel to RTS for FFTS Task.
/// @param [in] op_desc : current op.
/// @return SUCCESS / other error code.
///
Status TBEKernelHandle::RegisterHandle(const OpDescPtr &op_desc) {
  const auto tbe_kernel = op_desc->TryGetExtAttr(OP_EXTATTR_NAME_THREAD_TBE_KERNEL, std::vector<OpKernelBinPtr>{});
  if (tbe_kernel.empty()) {
    REPORT_INNER_ERROR("E19999", "[%s] tbe kernel is empty", op_desc->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] [%s] tbe kernel is empty", op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }

  std::vector<std::string> bin_file_keys;
  (void)AttrUtils::GetListStr(op_desc, kStubFuncName, bin_file_keys);
  if (tbe_kernel.size() != bin_file_keys.size()) {
    REPORT_INNER_ERROR("E19999", "[%s] number of bin_file != number of file_name, bin_file_num=%zu, file_name_num=%zu",
                       op_desc->GetName().c_str(), tbe_kernel.size(), bin_file_keys.size());
    GELOGE(INTERNAL_ERROR,
           "[Check][Param] [%s] number of bin_file != number of file_name, bin_file_num=%zu, file_name_num=%zu",
           op_desc->GetName().c_str(), tbe_kernel.size(), bin_file_keys.size());
    return INTERNAL_ERROR;
  }
  const size_t num = tbe_kernel.size();
  GELOGD("Kernel bin num is %zu", num);
  for (size_t i = 0U; i < num; i++) {
    if (tbe_kernel[i] == nullptr) {
      REPORT_INNER_ERROR("E19999", "Tbe kernel for op:%s is nullptr.", op_desc->GetName().c_str());
      GELOGE(INTERNAL_ERROR, "[Check][Param] TBE: tvm bin file of %s is nullptr when ffts.",
             op_desc->GetName().c_str());
      return INTERNAL_ERROR;
    }
    GE_CHK_STATUS_RET(FunctionRegister(op_desc, bin_file_keys[i], tbe_kernel[i], kAutoAttrPrefix,
        static_cast<uint32_t>(i)), "Function register of No. %zu bin file %s failed.", i, bin_file_keys[i].c_str());
  }
  return SUCCESS;
}

Status TBEKernelHandle::FunctionRegister(const OpDescPtr &op_desc, const std::string &bin_file,
                                         const OpKernelBinPtr &tbe_kernel, const std::string &prefix,
                                         const uint32_t thread_index) {
  const char_t *bin_file_key;
  if (thread_index != UINT32_MAX) {
    bin_file_key = GetRegisterStub(bin_file, "");
    GELOGI("Node:%s inherit func name:%s directly.", op_desc->GetName().c_str(), bin_file_key);
  } else {
    std::string session_graph_model_id;
    GetUniqueId(op_desc, session_graph_model_id);
    bin_file_key = GetRegisterStub(prefix + bin_file, session_graph_model_id);  // from set, always valid.
  }

  const std::lock_guard<std::mutex> lock(tvm_bin_mutex_);
  if (rtQueryFunctionRegistered(bin_file_key) != RT_ERROR_NONE) {
    GE_CHK_STATUS_RET(KernelRegister(op_desc, thread_index, bin_file_key, prefix, tbe_kernel),
                      "Kernel register failed, node:%s, thread index:%u, bin file:%s, prefix:%s",
                      op_desc->GetName().c_str(), thread_index, bin_file_key, prefix.c_str());
    return SUCCESS;
  }

  // Kernel registed, Increase used num in store.
  StoreTbeHandle(bin_file_key);
  return SUCCESS;
}

Status TBEKernelHandle::KernelRegister(const OpDescPtr &op_desc, const uint32_t thread_index,
                                       const char_t *const bin_file_key, const std::string &attr_prefix,
                                       const OpKernelBinPtr &tbe_kernel) {
  TBEHandleStore &kernel_store = TBEHandleStore::GetInstance();
  void *bin_handle = nullptr;
  if (!kernel_store.FindTBEHandle(bin_file_key, bin_handle)) {
    GELOGD("TBE: can't find the kernel_name[%s] in HandleMap", bin_file_key);
    rtDevBinary_t binary;
    GE_CHK_STATUS_RET(InitBinaryMagic(op_desc, thread_index, binary, attr_prefix), "Init binary magic of %s failed.",
                      op_desc->GetName().c_str());
    binary.version = 0U;
    binary.data = tbe_kernel->GetBinData();
    binary.length = tbe_kernel->GetBinDataSize();
    GELOGD("TBE: binary.length: %lu", binary.length);
    GE_CHK_RT_RET(rtDevBinaryRegister(&binary, &bin_handle));

    GE_CHK_STATUS_RET(InitMetaData(op_desc, thread_index, bin_handle, attr_prefix), "Init tvm meta data of %s failed.",
                      op_desc->GetName().c_str());
    kernel_store.StoreTBEHandle(bin_file_key, bin_handle, tbe_kernel);
  } else {
    GELOGI("TBE: find the kernel_name[%s] in HandleMap", bin_file_key);
    kernel_store.ReferTBEHandle(bin_file_key);
  }
  std::string kernel_name;
  GE_CHK_STATUS_RET(InitKernelName(op_desc, thread_index, kernel_name, attr_prefix), "Init kernel name of %s failed.",
                    op_desc->GetName().c_str());
  GE_CHK_RT_RET(rtFunctionRegister(bin_handle, bin_file_key, bin_file_key, kernel_name.c_str(), 0U));

  PROFILING_START(-1, profiling::kKnownGetAddrAndPrefCnt);
  void *addr = nullptr;
  uint32_t prefetch_cnt = 0U;
  GE_CHK_RT_RET(rtKernelGetAddrAndPrefCnt(bin_handle, 0U, bin_file_key, RT_STATIC_SHAPE_KERNEL, &addr, &prefetch_cnt));
  GELOGI("Get addr 0x%lx, pref_cnt %u for kernel_name %s", PtrToValue(addr), prefetch_cnt, kernel_name.c_str());
  addr_and_pref_cnt_[kernel_name] = {addr, prefetch_cnt};
  used_tbe_handle_map_[bin_file_key] = 1U;  // Init used num to 1.
  PROFILING_END(-1, profiling::kKnownGetAddrAndPrefCnt);
  return SUCCESS;
}

Status TBEKernelHandle::InitBinaryMagic(const OpDescPtr &op_desc, const uint32_t thread_index,
                                        rtDevBinary_t &binary, const std::string &prefix) const {
  const static std::map<std::string, uint32_t> binary_magics = {
      {"RT_DEV_BINARY_MAGIC_ELF_AICPU", RT_DEV_BINARY_MAGIC_ELF_AICPU},
      {"RT_DEV_BINARY_MAGIC_ELF", RT_DEV_BINARY_MAGIC_ELF},
      {"RT_DEV_BINARY_MAGIC_ELF_AIVEC", RT_DEV_BINARY_MAGIC_ELF_AIVEC},
      {"RT_DEV_BINARY_MAGIC_ELF_AICUBE", RT_DEV_BINARY_MAGIC_ELF_AICUBE}
  };

  std::string json_string;
  const std::string &tvm_magic = prefix + TVM_ATTR_NAME_MAGIC;
  if (thread_index != UINT32_MAX) {
    std::vector<std::string> json_list;
    (void)AttrUtils::GetListStr(op_desc, tvm_magic, json_list);
    if (json_list.size() <= thread_index) {
      GELOGE(INTERNAL_ERROR, "[Check][Param] failed. Attr is %s, thread index is %u, json list size is %zu.",
             tvm_magic.c_str(), thread_index, json_list.size());
      return INTERNAL_ERROR;
    }
    json_string = json_list[static_cast<size_t>(thread_index)];
  } else {
    if (prefix.empty()) {
      (void)AttrUtils::GetStr(op_desc, tvm_magic, json_string);
    } else {
      const std::map<std::string, uint32_t>::const_iterator it = kMixBinaryMagic.find(prefix);
      if (it != kMixBinaryMagic.cend()) {
        binary.magic = it->second;
        return SUCCESS;
      }
    }
  }

  const std::map<std::string, uint32_t>::const_iterator it = binary_magics.find(json_string);
  if (it == binary_magics.cend()) {
    REPORT_INNER_ERROR("E19999", "Attr:%s value:%s in op:%s(%s), model_id:%u, check invalid",
                       tvm_magic.c_str(), json_string.c_str(), op_desc->GetName().c_str(),
                       op_desc->GetType().c_str(), model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] Attr:%s value:%s in op:%s(%s), model_id:%u, check invalid",
           tvm_magic.c_str(), json_string.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return PARAM_INVALID;
  }
  binary.magic = it->second;
  return SUCCESS;
}

Status TBEKernelHandle::InitMetaData(const OpDescPtr &op_desc, const uint32_t thread_index,
                                     void *bin_handle, const std::string &prefix) const {
  std::string meta_data;
  const std::string &tvm_metadata = prefix + TVM_ATTR_NAME_METADATA;
  if (thread_index != UINT32_MAX) {
    std::vector<std::string> meta_data_list;
    (void)AttrUtils::GetListStr(op_desc, tvm_metadata, meta_data_list);
    if (meta_data_list.size() <= thread_index) {
      GELOGE(INTERNAL_ERROR, "[Check][Param] failed, attr is %s, thread index is %u, meta data list size is %zu.",
             tvm_metadata.c_str(), thread_index, meta_data_list.size());
      return INTERNAL_ERROR;
    }
    meta_data = meta_data_list[static_cast<size_t>(thread_index)];
  } else {
    (void)AttrUtils::GetStr(op_desc, tvm_metadata, meta_data);
  }
  GELOGD("TBE: meta data: %s", meta_data.empty() ? "null" : meta_data.c_str());
  if (!meta_data.empty()) {
    GE_CHK_RT_RET(rtMetadataRegister(bin_handle, meta_data.c_str()));
  }
  return SUCCESS;
}

Status TBEKernelHandle::InitKernelName(const OpDescPtr &op_desc, const uint32_t thread_index,
                                       std::string &kernel_name, const std::string &prefix) const {
  if (thread_index != UINT32_MAX) {
    return ThreadKernelName(op_desc, thread_index, kernel_name);
  } else {
    const std::string attr_kernel_name = prefix + op_desc->GetName() + "_kernelname";
    (void)AttrUtils::GetStr(op_desc, attr_kernel_name, kernel_name);
    GELOGD("[%s] attr: %s, kernel: %s", op_desc->GetName().c_str(), attr_kernel_name.c_str(), kernel_name.c_str());
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Init kernel name for thread slice.
/// @return SUCCESS / other error code.
///
Status TBEKernelHandle::ThreadKernelName(const OpDescPtr &op_desc, const uint32_t thread_index,
                                         std::string &kernel_name) const {
  const std::string kernel_name_attr = "_thread_kernelname";
  std::vector<std::string> thread_kernel_names;
  (void)AttrUtils::GetListStr(op_desc, kernel_name_attr, thread_kernel_names);
  if (thread_kernel_names.size() <= thread_index) {
    GELOGE(INTERNAL_ERROR, "[Check][Param] failed, attr is %s, thread index is %u, kernel name list size is %zu.",
           kernel_name_attr.c_str(), thread_index, thread_kernel_names.size());
    return INTERNAL_ERROR;
  }

  kernel_name = thread_kernel_names[static_cast<size_t>(thread_index)];
  GELOGD("[%s] attr: %s, kernel: %s", op_desc->GetName().c_str(), kernel_name_attr.c_str(), kernel_name.c_str());
  return SUCCESS;
}


///
/// @ingroup ge
/// @brief Register all kernels for dynamic shape.
/// @return SUCCESS / other error code.
///
Status TBEKernelHandle::RegisterKernelHandle(const OpDescPtr &op_desc, const TBEKernelStore &tbe_kernel_store,
                                             const std::string &prefix) const {
  const auto &kernel_key = GetKernelKey(*op_desc, prefix);
  void *bin_handle = nullptr;
  TBEHandleStore &handle_store = TBEHandleStore::GetInstance();
  if (handle_store.FindTBEHandle(kernel_key, bin_handle)) {
    GELOGD("[%s]'s kernel with name [%s] already registered.", op_desc->GetName().c_str(), kernel_key.c_str());
    return SUCCESS;
  }
  const std::string ext_kernel_name = prefix + std::string(OP_EXTATTR_NAME_TBE_KERNEL);
  auto tbe_kernel = op_desc->TryGetExtAttr(ext_kernel_name, TBEKernelPtr());
  if (tbe_kernel == nullptr) {
    tbe_kernel = tbe_kernel_store.FindKernel(kernel_key);
    if (tbe_kernel == nullptr) {
      REPORT_CALL_ERROR("E19999", "TBE: %s can't find tvm bin file with attr name %s !", op_desc->GetName().c_str(),
                        ext_kernel_name.c_str());
      GELOGE(INTERNAL_ERROR, "[Invoke][TryGetExtAttr]TBE: %s can't find tvm bin file with attr name %s !",
             op_desc->GetName().c_str(), ext_kernel_name.c_str());
      return INTERNAL_ERROR;
    }
  }

  GELOGD("Start to register kernel for node: [%s], kernel key[%s].", op_desc->GetName().c_str(), kernel_key.c_str());
  rtDevBinary_t binary;
  GE_CHK_STATUS_RET_NOLOG(InitBinaryMagic(op_desc, UINT32_MAX, binary, prefix));
  binary.version = 0U;
  binary.data = tbe_kernel->GetBinData();
  binary.length = tbe_kernel->GetBinDataSize();
  GELOGI("TBE: binary.length: %lu", binary.length);
  GE_CHK_RT_RET(rtRegisterAllKernel(&binary, &bin_handle));

  handle_store.StoreTBEHandle(kernel_key, bin_handle, nullptr);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get runtime task start pc and prefetch cnt.
/// @return SUCCESS / other error code.
///
Status TBEKernelHandle::GetAddrAndPrefCnt(const OpDesc &op_desc, const uint64_t tiling_key, uintptr_t &start_pc,
                                          uint32_t &prefetch_cnt, const std::string &prefix) const {
  void *bin_handle = nullptr;
  TBEHandleStore &kernel_store = TBEHandleStore::GetInstance();
  const auto &kernel_key = GetKernelKey(op_desc, prefix);
  if (!kernel_store.FindTBEHandle(kernel_key, bin_handle)) {
    REPORT_INNER_ERROR("E19999", "[%s] with kernel_key [%s] handle not found", op_desc.GetName().c_str(),
                       kernel_key.c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] [%s] with kernel_key [%s] handle not found", op_desc.GetName().c_str(),
           kernel_key.c_str());
    return INTERNAL_ERROR;
  }

  PROFILING_START(-1, profiling::kKernelGetAddrAndPrefCnt);
  void *addr = nullptr;
  if (rtKernelGetAddrAndPrefCnt(bin_handle, tiling_key, nullptr, RT_DYNAMIC_SHAPE_KERNEL, &addr, &prefetch_cnt) !=
      RT_ERROR_NONE) {
    REPORT_INNER_ERROR("E19999", "Get addr and prefetch cnt failed for op:%s", op_desc.GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] Get addr and prefetch cnt failed for op:%s", op_desc.GetName().c_str());
    return INTERNAL_ERROR;
  }
  start_pc = PtrToValue(addr);
  GELOGI("Get addr 0x%lx, pref_cnt %u for op %s", start_pc, prefetch_cnt, op_desc.GetName().c_str());
  PROFILING_END(-1, profiling::kKernelGetAddrAndPrefCnt);
  return SUCCESS;
}

void TBEKernelHandle::StoreTbeHandle(const std::string &handle_key) {
  // Online mode FE may call rtFunctionRegister.
  TBEHandleStore &kernel_store = TBEHandleStore::GetInstance();
  const auto it = used_tbe_handle_map_.find(handle_key);
  if (it != used_tbe_handle_map_.end()) {
    // GE registered, increase reference.
    kernel_store.ReferTBEHandle(handle_key);
    it->second++;
    return;
  }

  void *bin_handle = nullptr;
  if (kernel_store.FindTBEHandle(handle_key, bin_handle)) {
    // GE registered, increase reference.
    used_tbe_handle_map_[handle_key] = 1U;  // Init used num to 1.
    kernel_store.ReferTBEHandle(handle_key);
  }
}

void TBEKernelHandle::CleanTbeHandle() {
  TBEHandleStore &kernel_store = TBEHandleStore::GetInstance();
  kernel_store.EraseTBEHandle(used_tbe_handle_map_);
  used_tbe_handle_map_.clear();
  tvm_bin_kernel_.clear();
}

Status TBEKernelHandle::Register(const OpDescPtr &op_desc, const std::string &prefix) {
  if (IsAllKernelTask(op_desc)) {  // dynamic shape
    GE_CHK_STATUS_RET_NOLOG(RegisterKernelHandle(op_desc, TBEKernelStore(), prefix));
  } else {  // static shape
    GE_CHK_STATUS_RET_NOLOG(RegisterHandle(op_desc, nullptr, prefix));
  }
  return SUCCESS;
}
} // namespace ge