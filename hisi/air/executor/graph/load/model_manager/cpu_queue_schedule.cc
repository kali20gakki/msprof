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

#include "graph/load/model_manager/cpu_queue_schedule.h"
#include "framework/common/debug/log.h"

namespace {
const uint32_t kCoreDim = 1U;  // for rtCpuKernelLaunch
const std::string kCpuTaskModelEnqueue = "modelEnqueue";
const std::string kCpuTaskWaitEndGraph = "modelWaitEndGraph";
const std::string kCpuTaskPrepareOutput = "bufferPrepareOutput";
const std::string kCpuTaskStaticOutputPostProcess = "modelPrepareOutput";
const std::string kCpuTaskDynOutputPostProcess = "dynOutputPostProcess";
const std::string kCpuTaskModelDequeue = "modelDequeue";
const std::string kCpuTaskModelRepeat = "modelRepeat";
const std::string kCpuTaskZeroCopy = "zeroCpy";
const std::string kCpuTaskZeroCopyV2 = "zeroCpyV2";
}  // namespace

namespace ge {
CpuTaskInfo::CpuTaskInfo(const rtStream_t stream) : TaskInfo() {
  stream_ = stream;
}

CpuTaskInfo::~CpuTaskInfo() {
  GE_FREE_RT_LOG(args_);
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, bind input queue to task.
/// @param [in] queue_id: input queue id from user.
/// @param [out] in_mbuf: input mbuf addr for input data.
/// @return: 0 for success / others for failed
///
Status CpuTaskModelDequeue::Init(const uint32_t queue_id, uintptr_t &in_mbuf) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  args_size_ = sizeof(MbufQueueInfo) + sizeof(uintptr_t);  // sizeof(uintptr_t) for save in_mbuf.
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  in_mbuf = PtrToValue(args_) + sizeof(MbufQueueInfo);
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);

  MbufQueueInfo queue_info;
  queue_info.queue_id = queue_id;
  queue_info.in_mbuf = in_mbuf;  // Placeholder, input mbuf addr will save to this place.
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), &queue_info, sizeof(MbufQueueInfo),
                         RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskModelDequeue::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_:%u is 0 or stream_ is nullptr,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, kCpuTaskModelDequeue.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch model dequeue task success.");
  return SUCCESS;
}

Status CpuTaskZeroCopy::InitAddrs(std::vector<uintptr_t> &mbuf_list,
                                  const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
                                  const std::vector<bool> &is_no_tiling_list,
                                  const ZeroCpyArgs &cpy_args) {
  // init src_addrs/dst_addrs/no_tilings_
  for (const auto &addrs : outside_addrs) {
    const size_t data_idx = addrs.first;
    const auto &addrs_mapping_list = addrs.second.GetOutsideAddrs();
    if (addrs_mapping_list.empty()) {
      GELOGE(FAILED, "[Check][Param] not set outside_addrs");
      return PARAM_INVALID;
    }
    const std::map<uintptr_t, std::vector<uintptr_t>> &virtual_args_addrs = addrs_mapping_list[0U];
    for (const auto &virtual_args_addr : virtual_args_addrs) {
      for (size_t i = 0U; i < virtual_args_addr.second.size(); ++i) {
        const bool is_no_tiling = is_no_tiling_list.at(data_idx);
        GELOGI("Init addr, index:%zu, cpy_type:%d, is notiling:%d", data_idx, static_cast<int32_t>(cpy_args.cpy_type),
               static_cast<int32_t>(is_no_tiling));
        if (cpy_args.cpy_type == ZeroCpyType::kAllStatic) {
          if (!is_no_tiling) {
            addr_num_++;
            src_addrs_.emplace_back(mbuf_list.at(data_idx));
            dst_addrs_.push_back(virtual_args_addr.second.at(i));
            no_tilings_.emplace_back(static_cast<int32_t>(is_no_tiling));
          }
        } else if (cpy_args.cpy_type == ZeroCpyType::kAllDynamic) {
          if (is_no_tiling) {
            addr_num_++;
            src_addrs_.emplace_back(mbuf_list.at(data_idx));
            dst_addrs_.push_back(virtual_args_addr.second.at(i));
            no_tilings_.emplace_back(static_cast<int32_t>(is_no_tiling));
          }
        } else {
          addr_num_++;
          src_addrs_.emplace_back(mbuf_list.at(data_idx));
          dst_addrs_.push_back(virtual_args_addr.second.at(i));
          no_tilings_.emplace_back(static_cast<int32_t>(is_no_tiling));
        }
      }
    }
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, zero copy.
/// @param [in] mbuf_list: input/output mbuf addr list for input/output data.
/// @param [in] outside_addrs: model input/output memory addr
/// @param [in] is_no_tiling_list: model input/output is notiling
/// @param [out] cpy_args: cpy args
/// @return: 0 for success / others for failed
///
Status CpuTaskZeroCopy::Init(std::vector<uintptr_t> &mbuf_list,
                             const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
                             const std::vector<bool> &is_no_tiling_list,
                             ZeroCpyArgs &cpy_args) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  has_no_tiling_ = cpy_args.has_no_tiling;
  cpy_args.need_distribute = true;
  GE_CHK_STATUS_RET_NOLOG(InitAddrs(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args));
  GELOGI("addr_num is %u, outside_addrs size is %zu, is_no_tiling_list size is %zu",
         addr_num_, outside_addrs.size(), is_no_tiling_list.size());
  if (addr_num_ == 0U) {
    cpy_args.need_distribute = false;
    GELOGI("addr_num is 0, no need to distribute task");
    return SUCCESS;
  }

  // malloc mem for src_addrs/dst_addrs, and copy data of src_addrs/dst_addrs
  GE_CHK_RT_RET(rtMalloc(&src_addr_, src_addrs_.size() * sizeof(uint64_t), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(src_addr_, src_addrs_.size() * sizeof(uint64_t), src_addrs_.data(),
                         src_addrs_.size() * sizeof(uint64_t), RT_MEMCPY_HOST_TO_DEVICE));

  GE_CHK_RT_RET(rtMalloc(&dst_addr_, dst_addrs_.size() * sizeof(uint64_t), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(dst_addr_, dst_addrs_.size() * sizeof(uint64_t), dst_addrs_.data(),
                         dst_addrs_.size() * sizeof(uint64_t), RT_MEMCPY_HOST_TO_DEVICE));

  // src_addr_list is init to src_addr, which is the point to src_addrs
  const void *args = nullptr;
  AddrMapInfo addr_map_info;
  AddrMapInfoV2 addr_map_info_v2;
  if (!has_no_tiling_) {
    addr_map_info.addr_num = addr_num_;
    addr_map_info.src_addr_list = PtrToValue(src_addr_);
    addr_map_info.dst_addr_list = PtrToValue(dst_addr_);
    args = &addr_map_info;
    args_size_ = sizeof(AddrMapInfo);
    GELOGI("src_addr_list is %lu, dst_addr_list is %lu", addr_map_info.src_addr_list, addr_map_info.dst_addr_list);
  } else {
    GE_CHK_RT_RET(rtMalloc(&no_tiling_addr_, no_tilings_.size() * sizeof(int32_t), RT_MEMORY_HBM));
    GE_CHK_RT_RET(rtMemcpy(no_tiling_addr_, no_tilings_.size() * sizeof(int32_t), no_tilings_.data(),
                           no_tilings_.size() * sizeof(int32_t), RT_MEMCPY_HOST_TO_DEVICE));
    addr_map_info_v2.addr_num = addr_num_;
    addr_map_info_v2.src_addr_list = PtrToValue(src_addr_);
    addr_map_info_v2.dst_addr_list = PtrToValue(dst_addr_);
    addr_map_info_v2.is_no_tiling_list = PtrToValue(no_tiling_addr_);
    args = &addr_map_info_v2;
    args_size_ = sizeof(AddrMapInfoV2);
    GELOGI("src_addr_list is %lu, dst_addr_list is %lu, is_no_tiling_list is %lu",
           addr_map_info_v2.src_addr_list, addr_map_info_v2.dst_addr_list, addr_map_info_v2.is_no_tiling_list);
  }

  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_),
                         args, static_cast<uint64_t>(args_size_), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskZeroCopy::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_:%u is 0 or stream_ is nullptr,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  const std::string zero_cpy_task = (has_no_tiling_ ? kCpuTaskZeroCopyV2 : kCpuTaskZeroCopy);
  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, zero_cpy_task.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch zero copy task success.");
  return SUCCESS;
}

CpuTaskZeroCopy::~CpuTaskZeroCopy() {
  GE_FREE_RT_LOG(src_addr_);
  GE_FREE_RT_LOG(dst_addr_);
  GE_FREE_RT_LOG(no_tiling_addr_);
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, bind output queue to task.
/// @param [in] addr: NetOutput Op input tensor address.
/// @param [in] size: NetOutput Op input tensor size.
/// @param [in] in_mbuf: input mbuf addr for input data.
/// @param [out] out_mbuf: output mbuf addr for output data.
/// @return: 0 for success / others for failed
///
Status CpuTaskProcessOutput::Init(const uintptr_t addr, const uint32_t size, const uintptr_t in_mbuf,
                                  uintptr_t &out_mbuf) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  args_size_ = sizeof(ProcessOutputInfo) + sizeof(uintptr_t);  // sizeof(uintptr_t) for save out_mbuf.
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  out_mbuf = PtrToValue(args_) + sizeof(ProcessOutputInfo);
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);

  // Get NetOutput Input address and bind to queue.
  ProcessOutputInfo process;
  process.data_size = size;
  process.data_addr = addr;
  process.in_mbuf = in_mbuf;
  process.out_mbuf = out_mbuf;  // Placeholder, output mbuf addr will save to this place.
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), &process, sizeof(ProcessOutputInfo),
                         RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskProcessOutput::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_:%u is 0 or stream_ is nullptr,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  std::string kernel_name;
  if (stage_ == ProcessStage::kPrepare) {
    kernel_name = kCpuTaskPrepareOutput;
  } else if (stage_ == ProcessStage::kPostDynamic) {
    kernel_name = kCpuTaskDynOutputPostProcess;
  } else {
    kernel_name = kCpuTaskStaticOutputPostProcess;
  }
  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, kernel_name.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch %s output task success.", kernel_name.c_str());
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, bind output queue to task.
/// @param [in] queue_id: output queue id from user.
/// @param [in] out_mbuf: mbuf for output data.
/// @return: 0 for success / others for failed
///
Status CpuTaskModelEnqueue::Init(const uint32_t queue_id, const uintptr_t out_mbuf) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  // Get NetOutput Input address and bind to queue.
  args_size_ = sizeof(MbufQueueInfo);
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);

  MbufQueueInfo queue_info;
  queue_info.queue_id = queue_id;
  queue_info.in_mbuf = out_mbuf;
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), &queue_info, static_cast<uint64_t>(args_size_),
                         RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskModelEnqueue::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_ is 0 or stream_ is nullptr, arg_size:%u,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, kCpuTaskModelEnqueue.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch model enqueue task success.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, active entry stream.
/// @param [in] stream: stream to be active.
/// @return: 0 for success / others for failed
///
Status CpuTaskActiveEntry::Init(const rtStream_t stream) {
  if (stream == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param stream is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] Task active stream not valid");
    return FAILED;
  }

  active_stream_ = stream;
  return SUCCESS;
}

Status CpuTaskActiveEntry::Distribute() {
  if ((active_stream_ == nullptr) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param stream is nullptr or active_stream_ is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  GE_CHK_RT_RET(rtStreamActive(active_stream_, stream_));

  GELOGI("Cpu kernel launch active entry task success.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, wait for end graph.
/// @param [in] model_id: model id for wait end graph.
/// @return: 0 for success / others for failed
///
Status CpuTaskWaitEndGraph::Init(const uint32_t model_id) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  args_size_ = sizeof(model_id);
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);

  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), &model_id, static_cast<uint64_t>(args_size_),
                         RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskWaitEndGraph::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_:%u is 0 or stream_ is nullptr,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, kCpuTaskWaitEndGraph.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch wait end task success.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, repeat run model.
/// @param [in] model_id: model id for repeat run.
/// @return: 0 for success / others for failed
///
Status CpuTaskModelRepeat::Init(const uint32_t model_id) {
  if ((args_ != nullptr) || (args_size_ > 0U)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is not nullptr or args_size_:%u > 0, check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task already initialized, size:%u", args_size_);
    return FAILED;
  }

  args_size_ = sizeof(model_id);
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "args data.", args_size_);

  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), &model_id, static_cast<uint64_t>(args_size_),
                         RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status CpuTaskModelRepeat::Distribute() {
  if ((args_ == nullptr) || (args_size_ == 0U) || (stream_ == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param args_ is nullptr or args_size_:%u is 0 or stream_ is nullptr,"
                       "check invalid", args_size_);
    GELOGE(FAILED, "[Check][Param] Task not initialized, distribute failed, size:%u", args_size_);
    return FAILED;
  }

  GE_CHK_RT_RET(rtCpuKernelLaunch(nullptr, kCpuTaskModelRepeat.data(), kCoreDim, args_, args_size_, nullptr, stream_));

  GELOGI("Cpu kernel launch repeat task success.");
  return SUCCESS;
}
}  // namespace ge
