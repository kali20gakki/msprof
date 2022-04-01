/**
 * Copyright 2022-2022 Huawei Technologies Co., Ltd
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
#include "common/utils/executor_utils.h"

#include <string>
#include <ostream>
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/util.h"
#include "graph/ge_context.h"

namespace {
constexpr int64_t kMemtypeHostCompileDependent = 1;
constexpr int64_t kMemtypeHostCompileIndependent = 2;

std::string GetDataBufferInfo(const std::vector<ge::DataBuffer> &inputs) {
  std::ostringstream buf;
  buf << "[";
  for (auto &in : inputs) {
    buf << " (len: " << in.length << ", place: " << in.placement << " )";
  }
  buf << " ]";
  return buf.str();
}

Status GetAlignedValue(const size_t input, const size_t align_bytes, size_t &output) {
  if (align_bytes == 0U) {
    GELOGE(ge::PARAM_INVALID, "align_bytes is zero");
    return ge::PARAM_INVALID;
  }
  if (ge::CheckUint32AddOverflow(static_cast<uint32_t>(input),
                                 static_cast<uint32_t>((align_bytes - 1U))) != ge::SUCCESS) {
    GELOGE(ge::PARAM_INVALID, "Padding size is beyond the UINT32_MAX, input = %zu, align_bytes = %zu.",
           input, align_bytes);
    return ge::PARAM_INVALID;
  }
  GE_CHECK_GE(input + align_bytes, 1U);
  output = (((input + align_bytes) - 1U) / align_bytes) * align_bytes;
  return ge::SUCCESS;
}
} // namespace

namespace ge {
bool ExecutorUtils::HasHostMemInput(const OpDescPtr &op_desc) {
  GE_RT_FALSE_CHECK_NOTNULL(op_desc);
  for (size_t i = 0U; i < op_desc->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr &input_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (input_desc == nullptr) {
      continue;
    }
    int64_t mem_type = 0;
    (void)ge::AttrUtils::GetInt(*input_desc, ge::ATTR_NAME_PLACEMENT, mem_type);
    if ((mem_type == kMemtypeHostCompileIndependent) || (mem_type == kMemtypeHostCompileDependent)) {
      GELOGD("node[%s] input[%zu] has host mem", op_desc->GetName().c_str(), i);
      return true;
    }
  }
  return false;
}

// for non hybrid (dynamic) single op
Status ExecutorUtils::UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs,
                                             const OpTask &op_task,
                                             void *const io_base,
                                             const size_t io_size,
                                             std::vector<rtHostInputInfo_t> &host_inputs) {
  GE_CHECK_NOTNULL(op_task.GetOpdesc());
  GE_CHECK_NOTNULL(io_base);
  const auto &op_desc = op_task.GetOpdesc();
  size_t host_mem_data_offset = op_task.GetHostMemInputDataOffsetInIoAddr();
  if ((host_mem_data_offset + kMaxHostMemInputLen) > io_size) {
    GELOGE(PARAM_INVALID, "[Check] memory reserved for host memory input is not enough, offset = %zu,"
                          " io_addrs_size = %zu", host_mem_data_offset, io_size);
    return PARAM_INVALID;
  }
  const size_t align_bytes = op_task.GetInputAddrAlignBytes();
  size_t dst_len_left = kMaxHostMemInputLen;
  const auto &input_is_const = op_task.GetOpdesc()->GetIsInputConst();
  size_t input_index = 0UL;
  size_t io_index = 0UL;
  for (size_t i = 0UL; i < op_task.GetOpdesc()->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr &tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      continue;
    }
    if ((i < input_is_const.size()) && input_is_const[i]) {
      io_index++;
      continue;
    }
    if (input_index >= inputs.size()) {
      GELOGE(PARAM_INVALID, "input index(%zu) >= inputs size(%zu)", input_index, inputs.size());
      return PARAM_INVALID;
    }
    if (inputs[input_index].placement == kHostMemType) {
      size_t aligned_len = 0U;
      GE_CHK_STATUS_RET(GetAlignedValue(inputs[input_index].length, align_bytes, aligned_len),
                        "get align value failed.");
      GE_CHECK_LE(io_index, io_size / sizeof(void *));
      GE_CHECK_LE(host_mem_data_offset, io_size);
      uintptr_t *const host_mem_input_addr =
          PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(io_base) + (sizeof(void *) * io_index)));
      void *const data_addr = ValueToPtr(PtrToValue(io_base) + host_mem_data_offset);
      *host_mem_input_addr = PtrToValue(data_addr);
      if (memcpy_s(data_addr, dst_len_left, inputs[input_index].data, inputs[input_index].length) != EOK) {
        GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Update][HostMemInputArgs]failed, dst length is %zu,"
                                                   " src length is %zu.", dst_len_left, inputs[input_index].length);
        REPORT_INNER_ERROR("E19999", "update kernel args failed");
        return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
      }
      rtHostInputInfo_t host_in = {};
      GE_CHECK_LE(host_mem_data_offset, static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
      GE_CHECK_LE(io_index, static_cast<uint32_t>(std::numeric_limits<uint16_t>::max() / sizeof(void *)));
      host_in.addrOffset = static_cast<uint16_t>(sizeof(void *) * io_index);
      host_in.dataOffset = static_cast<uint16_t>(host_mem_data_offset);
      host_mem_data_offset += aligned_len; // No integer overflow
      GE_CHECK_GE(dst_len_left, aligned_len);
      dst_len_left -= aligned_len;
      host_inputs.emplace_back(std::move(host_in));
      GELOGD("Finish to copy host mem input[%zu]. size = %zu, task arg index = %zu", input_index,
             inputs[input_index].length, io_index);
    }
    input_index++;
    io_index++;
  }
  if (host_inputs.empty()) {
    GELOGE(GRAPH_FAILED, "[%s(%s)] host memory input(s) should be copied to io_base, but it(they) did not!!!"
           "inputs size:%zu, io_size:%zu, input_is_const:%s, inputs:%s, input_index:%zu, io_index:%zu,"
           "op_desc_input_num:%zu, align_bytes=%zu, io_base=%p",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), inputs.size(), io_size,
           ToString(input_is_const).c_str(), GetDataBufferInfo(inputs).c_str(), input_index, io_index,
           op_task.GetOpdesc()->GetAllInputsSize(), align_bytes, io_base);
    return GRAPH_FAILED;
  }
  return SUCCESS;
}

// for hybrid
Status ExecutorUtils::UpdateHostMemInputArgs(const hybrid::TaskContext &context,
                                             void *const io_addrs,
                                             const size_t io_addrs_size,
                                             const size_t host_mem_input_data_offset_in_args,
                                             std::vector<rtHostInputInfo_t> &host_inputs,
                                             bool need_64b_aligned) {
  GE_CHECK_NOTNULL(io_addrs);
  if ((host_mem_input_data_offset_in_args + kMaxHostMemInputLen) > io_addrs_size) {
    GELOGE(PARAM_INVALID, "[Check] memory reserved for host memory input is not enough, offset = %zu,"
           " io_addrs_size = %zu", host_mem_input_data_offset_in_args, io_addrs_size);
    return PARAM_INVALID;
  }
  size_t host_mem_input_data_offset = host_mem_input_data_offset_in_args;
  size_t dst_length_left = kMaxHostMemInputLen;
  size_t aligned_bytes = need_64b_aligned ? kAlignBytes64 : kAlignBytes4;
  for (int32_t i = 0; i < context.NumInputs(); ++i) {
    const auto input_data = context.GetInput(i);
    GE_CHECK_NOTNULL(input_data);
    if ((input_data->GetMemType() == MemStorageType::HOST_DDR) && (input_data->GetData() != nullptr)) {
      size_t aligned_size = 0U;
      GE_CHK_STATUS_RET(GetAlignedValue(input_data->GetSize(), aligned_bytes, aligned_size),
                        "get align value failed.");
      GE_CHECK_LE(host_mem_input_data_offset, io_addrs_size);
      GE_CHECK_LE(static_cast<size_t>(i), io_addrs_size / sizeof(void *));
      uint64_t *const host_mem_input_index = PtrAdd(PtrToPtr<void, uint64_t>(io_addrs),
                                                    io_addrs_size / sizeof(uint64_t), static_cast<size_t>(i));
      *host_mem_input_index = PtrToValue(io_addrs) + host_mem_input_data_offset;
      void *const host_mem_input_data_ptr = ValueToPtr(PtrToValue(io_addrs) + host_mem_input_data_offset);
      if (memcpy_s(host_mem_input_data_ptr, dst_length_left, input_data->GetData(), input_data->GetSize()) != EOK) {
        GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Update][Args]failed, dst length is %zu, src length is %zu.",
               dst_length_left, input_data->GetSize());
        REPORT_INNER_ERROR("E19999", "update kernel args failed of %s.", context.GetNodeName());
        return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
      }
      rtHostInputInfo_t host_input = {};
      GE_CHECK_LE(host_mem_input_data_offset, static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
      GE_CHECK_LE(static_cast<size_t>(i),
                  static_cast<uint32_t>(std::numeric_limits<uint16_t>::max() / sizeof(uint64_t)));
      host_input.dataOffset = static_cast<uint16_t>(host_mem_input_data_offset);
      host_input.addrOffset = static_cast<uint16_t>(static_cast<size_t>(i) * sizeof(uint64_t));
      host_inputs.emplace_back(std::move(host_input));
      host_mem_input_data_offset += aligned_size; // No integer overflow
      GE_CHECK_GE(dst_length_left, aligned_size);
      dst_length_left -= aligned_size;
      GELOGD("Finish to copy host mem input[%d]. size = %zu", i, input_data->GetSize());
    }
  }
  if (host_inputs.empty()) {
    GELOGE(GRAPH_FAILED, "host memory input(s) should be copied to io_base, but it(they) did not!!!");
    return GRAPH_FAILED;
  }
  return SUCCESS;
}

bool ExecutorUtils::IsReuseZeroCopyMemory() {
  const static std::string kEnabled = "1";
  std::string reuse_zero_copy_memory;
  (void)ge::GetContext().GetOption(OPTION_EXEC_REUSE_ZERO_COPY_MEMORY, reuse_zero_copy_memory);
  return (reuse_zero_copy_memory == kEnabled);
}

bool ExecutorUtils::IsGeUseExtendSizeStaticMemory() {
  const static std::string kExtendSizeType = "2";
  std::string ge_use_static_memory;
  (void)ge::GetContext().GetOption(kOptionExecGeUseStaticMemory, ge_use_static_memory);
  return (ge_use_static_memory == kExtendSizeType);
}

}