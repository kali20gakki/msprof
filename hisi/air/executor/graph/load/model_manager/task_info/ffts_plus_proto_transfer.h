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

#ifndef GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_PROTO_TRANSFER_H_
#define GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_PROTO_TRANSFER_H_

#include <vector>
#include <functional>
#include <string>

#include "ge/ge_api_error_codes.h"
#include "graph/op_desc.h"
#include "runtime/rt.h"
#include "proto/task.pb.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"

namespace ge {
void CleanRtFftsPlusTask(rtFftsPlusTaskInfo_t &ffts_plus_task_info);
using FftsRunAddrHandle = std::function<Status(const uintptr_t logic_addr, uint8_t *&addr)>;
using FftsAddrPrefHandle = std::function<Status(const std::string &kernel_name, void *&addr, uint32_t &pref_cnt)>;
using FftsFindNodeHandle = std::function<OpDescPtr(const uint32_t op_index)>;
using FftsSaveCtxArgsHandle = std::function<void(const OpDescPtr &op_desc, const size_t args_offset)>;
using FftsCreateAicpuSession = std::function<Status(STR_FWK_OP_KERNEL &fwk_op_kernel)>;
using FftsLoadCustAiCpuSo = std::function<Status(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def)>;
using FftsSaveAicpuCtxHandle = std::function<Status(const OpDescPtr &op_desc, const domi::aicpuKernelDef &kernel_def)>;

class FftsPlusProtoTransfer {
 public:
  FftsPlusProtoTransfer(const uintptr_t args_base, std::vector<uintptr_t> &io_addrs, const RuntimeParam &rts_param,
                        std::vector<void *> &ext_args, std::set<size_t> &mode_addr_idx)
    : args_base_(args_base), io_addrs_(io_addrs), ext_info_addrs_(ext_args), mode_addr_idx_(mode_addr_idx),
      runtime_param_(rts_param) {
  }
  ~FftsPlusProtoTransfer() = default;

  Status Transfer(const OpDescPtr &op_desc, const domi::FftsPlusTaskDef &ffts_plus_task_def,
                  rtFftsPlusTaskInfo_t &ffts_plus_task_info);

  void SetRunAddrHandle(const FftsRunAddrHandle &handle) { run_addr_handle_ = handle; }
  void SetAddrPrefHandle(const FftsAddrPrefHandle &handle) { addr_pref_handle_ = handle; }
  void SetFindNodeHandle(const FftsFindNodeHandle &handle) { find_node_handle_ = handle; }
  void SetSaveCtxArgsHandle(const FftsSaveCtxArgsHandle &handle) { save_ctx_args_handle_ = handle; }
  void SetGetSessionId(const std::function<uint64_t(void)> &handle) { aicpu_get_session_id_ = handle; }
  void SetCreateAicpuSession(const FftsCreateAicpuSession &handle) { create_aicpu_session_ = handle; }
  void SetLoadCustAicpuSo(const FftsLoadCustAiCpuSo &handle) { load_cust_aicpu_so_ = handle; }
  void SetSaveAicpuCtxHandle(const FftsSaveAicpuCtxHandle &handle) { save_aicpu_ctx_handle_ = handle; }

  const std::vector<FusionOpInfo> &GetAllFusionOpInfo() const { return fusion_op_info_; }

 private:
  void InitFftsPlusSqe(const domi::FftsPlusSqeDef &sqe_def, rtFftsPlusSqe_t *const sqe) const;
  void InitFftsPlusSqeHeader(const domi::StarsSqeHeaderDef &sqe_header_def, rtStarsSqeHeader_t &sqe_header) const;
  Status InitFftsPlusCtx(const domi::FftsPlusTaskDef &task_def, uint8_t *const ctx, const int32_t num);

  Status InitPersistentCacheCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitAicAivCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitManualAicAivCtx(const domi::FftsPlusAicAivCtxDef &ctx_def, rtFftsPlusAicAivCtx_t &ctx) const;
  Status InitAutoAicAivCtx(const domi::FftsPlusAicAivCtxDef &ctx_def, rtFftsPlusAicAivCtx_t &ctx) const;

  Status InitNotifyCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitWriteValueCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitMixAicAivCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitManualMixAicAivCtx(const domi::FftsPlusMixAicAivCtxDef &ctx_def,
                                rtFftsPlusMixAicAivCtx_t &ctx, const int32_t start_idx = 0) const;
  Status InitAutoMixAicAivCtx(const domi::FftsPlusMixAicAivCtxDef &ctx_def,
                              rtFftsPlusMixAicAivCtx_t &ctx, const int32_t start_idx = 0) const;

  Status InitSdmaCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitDataCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitAicpuCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitAicpuCtxUserData(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def,
                              rtFftsPlusAiCpuCtx_t &ctx) const;

  Status InitAicpuFwkAddrInfo(const OpDescPtr &op_desc, uint8_t *const ori_args_addr, const size_t args_size) const;
  Status InitAicpuInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def, void *&addr) const;
  Status InitAicpuFwkExtInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def, void *&addr) const;
  Status InitAicpuExtInfo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def, void *&addr) const;
  Status InitAicpuTaskExtInfo(const OpDescPtr &op_desc, const std::string &ext_info,
                              std::shared_ptr<hybrid::AicpuExtInfoHandler> &ext_handle) const;
  Status CopyTaskInfoToWorkspace(const OpDescPtr &op_desc, const void *const task_info_addr,
                                 const size_t task_info_addr_size) const;
  Status InitAicpuIoAddrs(const OpDescPtr &op_desc, const uintptr_t &io_addr) const;

  Status InitCondSwitchCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitCaseSwitchCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitCaseDefaultCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;
  Status InitCaseCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitAtStartCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitAtEndCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  Status InitLabelCtx(const domi::FftsPlusCtxDef &task_def, rtFftsPlusComCtx_t *const com_ctx) const;

  void InitAdditionalData(const domi::FftsPlusTaskDef &task_def);

  template<typename T>
  void InitThrdIoAddrs(const T &ctx_def, const uint16_t thread_id, const int32_t addr_count,
                       const int32_t start_idx = 0) const;

  uintptr_t args_base_{0U};     // runtime args memory
  std::vector<uintptr_t> &io_addrs_;
  std::vector<void *> &ext_info_addrs_;
  std::set<size_t> &mode_addr_idx_;
  const RuntimeParam &runtime_param_;
  uint32_t logic_stream_id_{0xFFFFFFFFU};
  std::vector<FusionOpInfo> fusion_op_info_;
  std::map<uint32_t, std::set<uint32_t>> ctx_additional_data_;
  FftsFindNodeHandle find_node_handle_{nullptr};
  FftsRunAddrHandle run_addr_handle_{nullptr};
  FftsAddrPrefHandle addr_pref_handle_{nullptr};
  FftsSaveCtxArgsHandle save_ctx_args_handle_{nullptr};

  std::function<uint64_t(void)> aicpu_get_session_id_ = []() { return 0U; };
  FftsCreateAicpuSession create_aicpu_session_ = [](STR_FWK_OP_KERNEL &) { return 0U; };
  FftsLoadCustAiCpuSo load_cust_aicpu_so_ = [](const OpDescPtr &, const domi::FftsPlusAicpuCtxDef &) { return 0U; };
  FftsSaveAicpuCtxHandle save_aicpu_ctx_handle_ = [](const OpDescPtr &, const domi::aicpuKernelDef &) { return 0U; };

  using CtxHandle = std::function<Status(FftsPlusProtoTransfer *, const domi::FftsPlusCtxDef &,
                                         rtFftsPlusComCtx_t *const)>;
  static std::map<rtFftsPlusContextType_t, CtxHandle> init_ctx_fun_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_PROTO_TRANSFER_H_
