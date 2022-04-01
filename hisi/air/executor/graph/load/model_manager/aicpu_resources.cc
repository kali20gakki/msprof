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
#include "aicpu_resources.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "framework/common/debug/log.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/node_utils.h"
#include "runtime/rt.h"

namespace ge {
namespace {
constexpr size_t kNameMaxLength = 128U;
const std::string kResourceTypeQueue = "RES_QUEUE";
const std::string kResourceTypeChannel = "RES_CHANNEL";
const std::string kResourceTypeVdecChannel = "RES_VDEC_CHANNEL";
const std::string kKernelNameCreateChannel = "CreateChannel";
const std::string kKernelNameDestroyChannel = "DestroyChannel";
const std::string kKernelNameCreateVdecChannel = "CreateVdecChannel";
const std::string kKernelNameDestroyVdecChannel = "DestroyVdecChannel";
const std::string kKernelNameCreateQueue = "CreateQueue";
const std::string kKernelNameDestroyQueue = "DestroyQueue";
const std::string kAttrFieldQueueName = "queue_name";
const std::string kAttrFieldQueueDepth = "queue_depth";
const std::string kAttrFieldQueueIdIdx = "queue_id_idx";
const std::string kSoNameBuiltin = "libbuiltin_kernels.so";
const int32_t kDefaultPriority = 0;
const uint32_t kKernelBlockDim = 1U;
const uint32_t kAiCpuQueueDepth = 8U;
}

AiCpuResources::~AiCpuResources() {
  if ((!aicpu_queues_.empty()) || (!aicpu_channels_.empty()) || (!aicpu_vdec_channels_.empty())) {
    ReleaseResources();
  }
}

Status AiCpuResources::CreateQueue(const std::string &name, const uint32_t depth, uint32_t &queue_id) {
  GELOGD("Start to create queue, name = %s, depth = %u", name.c_str(), depth);
  std::vector<uint8_t> task_args;
  void *queue_id_dev = nullptr;
  GE_CHK_RT_RET(rtMalloc(&queue_id_dev, sizeof(queue_id), RT_MEMORY_HBM));
  GE_MAKE_GUARD(queue_id_dev, [&queue_id_dev] {
    GE_CHK_RT(rtFree(queue_id_dev));
  });
  GE_CHK_STATUS_RET_NOLOG(BuildCreateQueueTask(PtrToValue(queue_id_dev), name, depth, task_args));
  GE_CHK_STATUS_RET(ExecuteKernel(kKernelNameCreateQueue, task_args));
  GE_CHK_RT_RET(rtMemcpy(&queue_id, sizeof(queue_id), queue_id_dev, sizeof(queue_id), RT_MEMCPY_DEVICE_TO_HOST));
  GELOGD("Queue created successfully, name = %s, queue id = %u", name.c_str(), queue_id);
  return SUCCESS;
}

Status AiCpuResources::BuildCreateQueueTask(const uintptr_t queue_id_dev,
                                            const std::string &name,
                                            const uint32_t depth,
                                            std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(uintptr_t) + kNameMaxLength + sizeof(uint32_t);
  task_args.resize(args_size);

  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 1U;  // single input: queue_id
  size_t args_pos = sizeof(aicpu::AicpuParamHead);

  // assign queue id
  *(static_cast<uintptr_t *>(static_cast<void *>(&task_args[args_pos]))) = queue_id_dev;
  args_pos += sizeof(uintptr_t);

  // assign queue name
  if (strcpy_s(static_cast<char_t *>(static_cast<void *>(&task_args[args_pos])), kNameMaxLength, name.c_str()) != EOK) {
    GELOGE(INTERNAL_ERROR, "Failed to copy queue name");
    return INTERNAL_ERROR;
  }
  args_pos += kNameMaxLength;

  // assign queue depth
  *(static_cast<uint32_t *>(static_cast<void *>(&task_args[args_pos]))) = depth;

  GELOGD("%s task args constructed, size = %zu", kKernelNameCreateQueue.c_str(), args_size);
  return SUCCESS;
}

Status AiCpuResources::CreateChannel(const int32_t rt_stream_id) {
  GELOGD("Start to create channel, rt stream id = %d", rt_stream_id);
  std::vector<uint8_t> task_args;
  GE_CHK_STATUS_RET_NOLOG(BuildCreateChannelTask(rt_stream_id, task_args));
  GE_CHK_STATUS_RET(ExecuteKernel(kSoNameBuiltin.c_str(), kKernelNameCreateChannel, task_args));
  GELOGD("Channel created successfully, rt stream id = %d", rt_stream_id);
  return SUCCESS;
}

Status AiCpuResources::BuildCreateChannelTask(const int32_t rt_stream_id,
                                              std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(int32_t);
  task_args.resize(args_size);

  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 0U;
  task_args[sizeof(aicpu::AicpuParamHead)] = static_cast<uint8_t>(rt_stream_id);

  // assign rt stream id
  GELOGD("%s task args constructed, size = %zu", kKernelNameCreateChannel.c_str(), args_size);
  return SUCCESS;
}

Status AiCpuResources::CreateVdecChannel(const int32_t rt_stream_id) {
  GELOGD("Start to create Vdec channel, rt stream id = %d", rt_stream_id);
  std::vector<uint8_t> task_args;
  GE_CHK_STATUS_RET_NOLOG(BuildCreateVdecChannelTask(rt_stream_id, task_args));
  GE_CHK_STATUS_RET(ExecuteKernel(kSoNameBuiltin.c_str(), kKernelNameCreateVdecChannel, task_args));
  GELOGD("Vdec channel created successfully, rt stream id = %d", rt_stream_id);
  return SUCCESS;
}

Status AiCpuResources::BuildCreateVdecChannelTask(const int32_t rt_stream_id,
                                                  std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(int32_t);
  task_args.resize(args_size);

  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 0U;
  task_args[sizeof(aicpu::AicpuParamHead)] = static_cast<uint8_t>(rt_stream_id);

  // assign rt stream id
  GELOGD("%s task args constructed, size = %zu", kKernelNameCreateVdecChannel.c_str(), args_size);
  return SUCCESS;
}

Status AiCpuResources::ExecuteKernel(const char_t *const so_name,
                                     const std::string &kernel_name,
                                     const std::vector<uint8_t> &task_args) {
  rtStream_t stream = nullptr;
  GE_CHK_RT_RET(rtStreamCreate(&stream, kDefaultPriority));
  GE_MAKE_GUARD_RTSTREAM(stream);
  GE_CHK_RT_RET(rtCpuKernelLaunch(so_name,
                                  kernel_name.c_str(),
                                  kKernelBlockDim,
                                  task_args.data(),
                                  static_cast<uint32_t>(task_args.size()),
                                  nullptr,
                                  stream));
  GELOGD("Launch kernel successfully, kernel name = %s", kernel_name.c_str());
  GE_CHK_RT_RET(rtStreamSynchronize(stream));
  GELOGD("Sync stream successfully, kernel name = %s", kernel_name.c_str());
  return SUCCESS;
}

Status AiCpuResources::ExecuteKernel(const std::string &kernel_name,
                                     const std::vector<uint8_t> &task_args) {
  return ExecuteKernel(nullptr, kernel_name, task_args);
}

Status AiCpuResources::DestroyQueue(const uint32_t queue_id) {
  GELOGD("Start to destroy queue, id = %u", queue_id);
  std::vector<uint8_t> task_args;
  GE_CHK_STATUS_RET(BuildDestroyQueueTask(queue_id, task_args), "Failed to init task args");
  GE_CHK_STATUS_RET(ExecuteKernel(kKernelNameDestroyQueue, task_args), "Failed to launch kernel");
  GELOGD("Queue destroyed successfully, queue id = %u", queue_id);
  return SUCCESS;
}

Status AiCpuResources::BuildDestroyQueueTask(const uint32_t queue_id, std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(queue_id);
  task_args.resize(args_size);
  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 0U;  // no input

  // assign queue id
  *(static_cast<uint32_t *>(static_cast<void *>(&task_args[sizeof(aicpu::AicpuParamHead)]))) = queue_id;
  GELOGD("%s task args constructed, size = %zu", kKernelNameDestroyQueue.c_str(), args_size);
  return SUCCESS;
}

Status AiCpuResources::DestroyChannel(const int32_t rt_stream_id) {
  GELOGD("Start to destroy channel, rt stream id = %d", rt_stream_id);
  std::vector<uint8_t> task_args;
  GE_CHK_STATUS_RET(BuildDestroyChannelTask(rt_stream_id, task_args), "Failed to init task args");
  GE_CHK_STATUS_RET(ExecuteKernel(kSoNameBuiltin.c_str(), kKernelNameDestroyChannel, task_args),
                    "Failed to launch kernel");
  GELOGD("Channel destroyed successfully, rt stream id = %d", rt_stream_id);
  return SUCCESS;
}

Status AiCpuResources::BuildDestroyChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(rt_stream_id);
  task_args.resize(args_size);
  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 0U;  // no input

  // assign rt stream id
  *(static_cast<int32_t *>(static_cast<void *>(&task_args[sizeof(aicpu::AicpuParamHead)]))) = rt_stream_id;
  GELOGD("%s task args constructed, size = %zu", kKernelNameDestroyChannel.c_str(), args_size);
  return SUCCESS;
}

Status AiCpuResources::DestroyVdecChannel(const int32_t rt_stream_id) {
  GELOGD("Start to destroy Vdec channel, rt stream id = %d", rt_stream_id);
  std::vector<uint8_t> task_args;
  GE_CHK_STATUS_RET(BuildDestroyVdecChannelTask(rt_stream_id, task_args), "Failed to init task args");
  GE_CHK_STATUS_RET(ExecuteKernel(kSoNameBuiltin.c_str(), kKernelNameDestroyVdecChannel, task_args),
                    "Failed to launch kernel");
  GELOGD("Vdec channel destroyed successfully, rt stream id = %d", rt_stream_id);
  return SUCCESS;
}

Status AiCpuResources::BuildDestroyVdecChannelTask(const int32_t rt_stream_id, std::vector<uint8_t> &task_args) {
  const auto args_size = sizeof(aicpu::AicpuParamHead) + sizeof(rt_stream_id);
  task_args.resize(args_size);
  auto &param_head = *(static_cast<aicpu::AicpuParamHead *>(static_cast<void *>(task_args.data())));
  param_head.length = args_size;
  param_head.ioAddrNum = 0U;  // no input

  // assign rt stream id
  *(static_cast<int32_t *>(static_cast<void *>(&task_args[sizeof(aicpu::AicpuParamHead)]))) = rt_stream_id;
  GELOGD("%s task args constructed, size = %zu", kKernelNameDestroyVdecChannel.c_str(), args_size);
  return SUCCESS;
}

const std::string &AiCpuResources::ResourceTypeQueue() {
  return kResourceTypeQueue;
}

const std::string &AiCpuResources::ResourceTypeChannel() {
  return kResourceTypeChannel;
}

const std::string &AiCpuResources::ResourceTypeVdecChannel() {
  return kResourceTypeVdecChannel;
}

Status AiCpuResources::AllocateQueueResource(const OpDescPtr &op_desc,
                                             const NamedAttrs &resource_attr,
                                             int32_t &input_idx,
                                             uint32_t &queue_id) {
  std::string queue_name;
  int64_t input_index = -1;
  if (!AttrUtils::GetStr(resource_attr, kAttrFieldQueueName, queue_name)) {
    GELOGE(PARAM_INVALID, "[%s] Failed to get queue name", op_desc->GetName().c_str());
    return PARAM_INVALID;
  }
  if (!AttrUtils::GetInt(resource_attr, kAttrFieldQueueIdIdx, input_index)) {
    GELOGE(PARAM_INVALID, "[%s] Failed to get input index for queue %s",
           op_desc->GetName().c_str(), queue_name.c_str());
    return PARAM_INVALID;
  }
  uint32_t queue_depth = kAiCpuQueueDepth;
  if (AttrUtils::GetInt(resource_attr, kAttrFieldQueueDepth, queue_depth)) {
    GELOGD("Got queue depth from attribute = %u", queue_depth);
  }
  GE_CHECK_GE(input_index, 0);
  GE_CHECK_LE(input_index, INT32_MAX);
  input_idx = static_cast<int32_t>(input_index);
  GE_CHK_STATUS_RET_NOLOG(GetOrCreateQueue(queue_name, queue_depth, queue_id));
  return SUCCESS;
}

Status AiCpuResources::GetOrCreateQueue(const std::string &queue_name, const uint32_t queue_depth, uint32_t &queue_id) {
  const std::lock_guard<std::mutex> lk(mu_);
  const std::map<std::string, uint32_t>::const_iterator it = aicpu_queues_.find(queue_name);
  if (it != aicpu_queues_.cend()) {
    queue_id = it->second;
    GELOGD("Queue [%s] already created, queue_id = %u", queue_name.c_str(), queue_id);
    return SUCCESS;
  }
  GE_CHK_STATUS_RET(CreateQueue(queue_name, queue_depth, queue_id),
                    "Failed to create queue, name = %s",
                    queue_name.c_str());
  (void)aicpu_queues_.emplace(queue_name, queue_id);
  return SUCCESS;
}

Status AiCpuResources::AllocateChannelResource(const OpDescPtr &op_desc,
                                               const int32_t rt_stream_id) {
  const std::lock_guard<std::mutex> lk(mu_);
  const std::set<int32_t>::const_iterator it = aicpu_channels_.find(rt_stream_id);
  if (it != aicpu_channels_.cend()) {
    GELOGD("[%s] Channel already created, rt_stream_id = %d", op_desc->GetName().c_str(), rt_stream_id);
    return SUCCESS;
  }
  GE_CHK_STATUS_RET(CreateChannel(rt_stream_id), "Failed to create channel, id = %d", rt_stream_id);
  (void)aicpu_channels_.emplace(rt_stream_id);
  return SUCCESS;
}

Status AiCpuResources::AllocateVdecChannelResource(const OpDescPtr &op_desc,
                                                   const int32_t rt_stream_id) {
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = aicpu_vdec_channels_.find(rt_stream_id);
  if (it != aicpu_vdec_channels_.end()) {
    GELOGD("[%s] Channel already created, rt_stream_id = %d", op_desc->GetName().c_str(), rt_stream_id);
    return SUCCESS;
  }
  GE_CHK_STATUS_RET(CreateVdecChannel(rt_stream_id), "Failed to create vdec channel, id = %d", rt_stream_id);
  (void)aicpu_vdec_channels_.emplace(rt_stream_id);
  return SUCCESS;
}

void AiCpuResources::ReleaseResources() {
  const std::lock_guard<std::mutex> lk(mu_);
  GELOGD("Release queue resource started, size = %zu", aicpu_queues_.size());
  for (const auto &it : aicpu_queues_) {
    GE_CHK_STATUS(DestroyQueue(it.second),
                  "Failed to destroy queue, name = %s, queue id = %u", it.first.c_str(), it.second);
  }
  aicpu_queues_.clear();

  GELOGD("Release channel resource started, size = %zu", aicpu_queues_.size());
  for (const auto it : aicpu_channels_) {
    GE_CHK_STATUS(DestroyChannel(it), "Failed to destroy channel, rt stream id = %d", it);
  }
  aicpu_channels_.clear();

  GELOGD("Release vdec channel resource started, size = %zu", aicpu_queues_.size());
  for (const auto it : aicpu_vdec_channels_) {
    GE_CHK_STATUS(DestroyVdecChannel(it), "Failed to destroy vdec channel, rt stream id = %d", it);
  }
  aicpu_vdec_channels_.clear();

  GELOGD("Release ended");
}
}  // namespace ge
