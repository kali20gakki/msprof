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

#ifndef GE_GRAPH_LOAD_MODEL_MANAGER_TBE_KERNEL_HANDLE_H
#define GE_GRAPH_LOAD_MODEL_MANAGER_TBE_KERNEL_HANDLE_H

#include <set>
#include <string>
#include <thread>

#include "graph/op_desc.h"
#include "common/tbe_kernel_store.h"
#include "runtime/rt.h"

namespace ge {
bool IsTbeTask(const OpDescPtr &op_desc);
bool IsAllKernelTask(const OpDescPtr &op_desc);

class TBEKernelHandle {
 public:
  TBEKernelHandle() = default;
  ~TBEKernelHandle() = default;

  ///
  /// @ingroup ge
  /// @brief Register TBE kernel to RTS for static or dynamic shape
  /// @param [in] op_desc : current op.
  /// @param [in] prefix: kernel name attr name prefix.
  /// @return SUCCESS / other error code.
  ///
  Status Register(const OpDescPtr &op_desc, const std::string &prefix = "");

  ///
  /// @ingroup ge
  /// @brief Register TBE kernel to RTS for FFTS Task.
  /// @param [in] op_desc : current op.
  /// @return SUCCESS / other error code.
  ///
  Status RegisterHandle(const OpDescPtr &op_desc);  // Ref: InitTbeHandleInAutoMode

  ///
  /// @ingroup ge
  /// @brief Register TBE kernel to RTS for common Task for static shape.
  /// @param [in] op_desc : current op.
  /// @param [in] tbe_kernel: kernel handle for current op.
  /// @param [in] prefix: kernel name attr name prefix.
  /// @param [in] tbe_kernel_store: store to find kernel.
  /// @return SUCCESS / other error code.
  ///
  Status RegisterHandle(const OpDescPtr &op_desc, const TBEKernelPtr &tbe_kernel, const std::string &prefix,
                        const TBEKernelStore &tbe_kernel_store = TBEKernelStore());

  void SetModelId(const uint32_t model_id) { model_id_ = model_id; }

  ///
  /// @ingroup ge
  /// @brief Clean all registered kernel.
  /// @return None
  ///
  void CleanTbeHandle();

  ///
  /// @ingroup ge
  /// @brief get unique identification for op when load two or more models
  /// @param [in] op_desc : current op.
  /// @param [in] std::string identification: unique identification for current op.
  /// @return None
  ///
  void GetUniqueId(const OpDescPtr &op_desc, std::string &unique_identification) const;

  ///
  /// @ingroup ge
  /// @brief For TVM Op, avoid Addr Reuse.
  /// @return void*
  ///
  const char_t *GetRegisterStub(const std::string &binfile, const std::string &session_graph_id = "");

  Status GetAddrAndPrefCnt(const std::string &kernel_name, void *&addr, uint32_t &pref_cnt) const;

  ///
  /// @ingroup ge
  /// @brief Register all kernels for dynamic shape.
  /// @return SUCCESS / other error code.
  ///
  Status RegisterKernelHandle(const OpDescPtr &op_desc, const TBEKernelStore &tbe_kernel_store = TBEKernelStore(),
                              const std::string &prefix = "") const;

  ///
  /// @ingroup ge
  /// @brief Get runtime task start pc and prefetch cnt.
  /// @return SUCCESS / other error code.
  ///
  Status GetAddrAndPrefCnt(const OpDesc &op_desc, const uint64_t tiling_key, uintptr_t &start_pc,
                           uint32_t &prefetch_cnt, const std::string &prefix = "") const;

 private:
  Status FunctionRegister(const OpDescPtr &op_desc, const std::string &bin_file, const OpKernelBinPtr &tbe_kernel,
                          const std::string &prefix, const uint32_t thread_index = UINT32_MAX);
  Status KernelRegister(const OpDescPtr &op_desc, const uint32_t thread_index, const char_t *const bin_file_key,
                        const std::string &attr_prefix, const OpKernelBinPtr &tbe_kernel);
  Status InitBinaryMagic(const OpDescPtr &op_desc, const uint32_t thread_index, rtDevBinary_t &binary,
                         const std::string &prefix = "") const;
  Status InitMetaData(const OpDescPtr &op_desc, const uint32_t thread_index, void *bin_handle,
                      const std::string &prefix = "") const;
  Status InitKernelName(const OpDescPtr &op_desc, const uint32_t thread_index, std::string &kernel_name,
                        const std::string &prefix = "") const;

  ///
  /// @ingroup ge
  /// @brief Init kernel name for thread slice.
  /// @return SUCCESS / other error code.
  ///
  Status ThreadKernelName(const OpDescPtr &op_desc, const uint32_t thread_index,
                          std::string &kernel_name) const;

  void StoreTbeHandle(const std::string &handle_key);

  uint32_t model_id_{0U};

  static std::mutex tvm_bin_mutex_;
  std::set<std::string> tvm_bin_kernel_;
  std::map<std::string, uint32_t> used_tbe_handle_map_;

  std::map<std::string, std::pair<void *, uint32_t>> addr_and_pref_cnt_;
};
} // namespace ge
#endif  // GE_GRAPH_LOAD_MODEL_MANAGER_TBE_KERNEL_HANDLE_H
