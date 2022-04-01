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

#include "graph/load/model_manager/zero_copy_offset.h"

#include "graph/load/model_manager/model_utils.h"

namespace ge {
namespace {
const uint32_t kInputDataOutputIndex = 0U;
}  // namespace

ZeroCopyOffset::ZeroCopyOffset() {}

ZeroCopyOffset::~ZeroCopyOffset() {}

Status ZeroCopyOffset::InitInputDataInfo(const int64_t output_size, void *virtual_addr, const OpDescPtr &op_desc,
                                         bool &fusion_flag) {
  GELOGI("[ZCPY] Start to InitInputDataInfo of %s, total_data_size is %ld, virtual_addr is %p",
         op_desc->GetName().c_str(), output_size, virtual_addr);
  basic_addr_ = virtual_addr;
  op_name_ = op_desc->GetName();
  (void)ge::AttrUtils::GetListInt(op_desc, ATTR_ZERO_COPY_BASIC_OFFSET, zero_copy_basic_offset_);
  (void)ge::AttrUtils::GetListInt(op_desc, ATTR_ZERO_COPY_RELATIVE_OFFSET, zero_copy_relative_offset_);
  GE_CHK_BOOL_EXEC(zero_copy_basic_offset_.size() == zero_copy_relative_offset_.size(),
                   REPORT_INNER_ERROR("E19999", "basic_offset_size:%zu not equal to relative_offset_size:%zu, "
                                      "check invalid", zero_copy_basic_offset_.size(),
                                      zero_copy_relative_offset_.size());
                   return PARAM_INVALID,
                   "[Check][Param] basic_offset_size:%zu should be equal to relative_offset_size:%zu",
                   zero_copy_basic_offset_.size(), zero_copy_relative_offset_.size());
  GELOGD("[ZCPY] zero_copy_basic_offset size is %zu", zero_copy_basic_offset_.size());

  const int64_t virtual_addr_offset = op_desc->GetOutputOffset().at(kInputDataOutputIndex);
  IsL2Fusion(zero_copy_basic_offset_, virtual_addr_offset, fusion_flag);

  size_t out_count = 0U;
  data_size_ = output_size;
  if (!fusion_flag) {
    out_count++;
    data_info_.emplace_back(output_size, virtual_addr);
    relative_offset_.emplace_back(0);
    GELOGD("[ZCPY] %s size is %ld, virtual_addr is %p.", op_desc->GetName().c_str(), output_size, virtual_addr);
  } else {
    GELOGI("[ZCPY] set l2_fusion for %s.", op_desc->GetName().c_str());
    for (size_t index = 0U; index < zero_copy_basic_offset_.size(); ++index) {
      if (zero_copy_basic_offset_.at(index) == virtual_addr_offset) {
        out_count++;
        const uint64_t out_offset = PtrToValue(virtual_addr) +
            static_cast<uint64_t>(zero_copy_relative_offset_.at(index));
        data_info_.emplace_back(output_size, ValueToPtr(out_offset));
        relative_offset_.emplace_back(zero_copy_relative_offset_.at(index));
        GELOGI("[ZCPY] virtual_addr: %p has been l2-fusion to %lu, need copy data_size is %ld.", basic_addr_,
               out_offset, output_size);
      }
    }
  }
  data_count_ = out_count;
  return SUCCESS;
}

Status ZeroCopyOffset::InitOutputDataInfo(const std::vector<int64_t> &input_size_list,
                                          const std::vector<void *> &virtual_addr_list, const OpDescPtr &op_desc,
                                          const size_t &idx, bool &fusion_flag) {
  int64_t size = input_size_list[idx];
  const auto tensor_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(idx));
  GE_CHECK_NOTNULL(tensor_desc);
  if (TensorUtils::GetTensorSizeInBytes(*tensor_desc, size) != GRAPH_SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Get input TensorSize in op:%s(%s) failed, input_index:%zu",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), idx);
    GELOGE(FAILED, "[Get][InputTensorSize] in op:%s(%s) failed, input_index:%zu",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), idx);
    return FAILED;
  }

  GELOGD("Tensor data size: GetSize=%ld, GetTensorSizeInBytes=%ld", input_size_list[idx], size);

  basic_addr_ = virtual_addr_list[idx];
  op_name_ = op_desc->GetName();
  (void)AttrUtils::GetListInt(op_desc, ATTR_ZERO_COPY_BASIC_OFFSET, zero_copy_basic_offset_);
  (void)AttrUtils::GetListInt(op_desc, ATTR_ZERO_COPY_RELATIVE_OFFSET, zero_copy_relative_offset_);
  if (zero_copy_basic_offset_.size() != zero_copy_relative_offset_.size()) {
    REPORT_INNER_ERROR("E19999", "basic_offset_size:%zu not equal to relative_offset_size:%zu, check invalid",
                       zero_copy_basic_offset_.size(), zero_copy_relative_offset_.size());
    GELOGE(PARAM_INVALID, "[Check][Param] basic_offset_size:%zu should be equal to relative_offset_size:%zu",
           zero_copy_basic_offset_.size(), zero_copy_relative_offset_.size());
    return PARAM_INVALID;
  }
  const int64_t virtual_addr_offset = op_desc->GetInputOffset().at(idx);
  IsL2Fusion(zero_copy_basic_offset_, virtual_addr_offset, fusion_flag);

  size_t in_count = 0U;
  data_size_ = size;
  if (!fusion_flag) {
    in_count++;
    data_info_.emplace_back(size, virtual_addr_list[idx]);
    // op_desc not set l2fusion when fusion_flag is false
    relative_offset_.emplace_back(0);
    GELOGI("[ZCPY] %s size is %ld, virtual_addr is %p.", op_desc->GetName().c_str(), size, virtual_addr_list[idx]);
  } else {
    GELOGI("[ZCPY] set l2-fusion for %s.", op_desc->GetName().c_str());
    for (size_t index = 0U; index < zero_copy_basic_offset_.size(); ++index) {
      if (zero_copy_basic_offset_.at(index) == virtual_addr_offset) {
        in_count++;
        const uint64_t in_offset = PtrToValue(virtual_addr_list[idx]) +
            static_cast<uint64_t>(zero_copy_relative_offset_.at(index));
        const int64_t real_data_size = ModelUtils::GetInputSize(op_desc).at(idx);
        data_info_.emplace_back(real_data_size, ValueToPtr(in_offset));
        relative_offset_.emplace_back(zero_copy_relative_offset_.at(index));
        GELOGI("[ZCPY] virtual_addr: %p has been l2-fusion from %lu, need copy data_size is %ld.", basic_addr_,
               in_offset, real_data_size);
      }
    }
  }
  data_count_ = in_count;
  return SUCCESS;
}

void ZeroCopyOffset::IsL2Fusion(const std::vector<int64_t> &fusion_basic_addrs, const int64_t &tensor_addr,
                                bool &fusion_flag) {
  for (size_t fusion_count = 0U; fusion_count < fusion_basic_addrs.size(); ++fusion_count) {
    if (fusion_basic_addrs.at(fusion_count) == tensor_addr) {
      fusion_flag = true;
      break;
    }
  }
}

void ZeroCopyOffset::SetInputOutsideAddrs(const int64_t output_offset, const uintptr_t addr_val, const bool fusion_flag,
                                          std::set<const void *> &real_virtual_addrs) {
  size_t out_count = 0U;
  if (!fusion_flag) {
    out_count++;
    std::map<uintptr_t, std::vector<uintptr_t>> addr_mapping { {addr_val, {}} };
    outside_addrs_.emplace_back(std::move(addr_mapping));
    (void)real_virtual_addrs.insert(ValueToPtr(addr_val));
  } else {
    GELOGI("[ZCPY] set l2-fusion for virtual_addr 0x%lx.", addr_val);
    for (size_t i = 0U; i < zero_copy_basic_offset_.size(); ++i) {
      if (zero_copy_basic_offset_.at(i) == output_offset) {
        out_count++;
        const uintptr_t virtual_addr = addr_val + static_cast<uint64_t>(zero_copy_relative_offset_.at(i));
        std::map<uintptr_t, std::vector<uintptr_t>> addr_mapping { {virtual_addr, {}} };
        outside_addrs_.emplace_back(std::move(addr_mapping));
        (void)real_virtual_addrs.insert(ValueToPtr(virtual_addr));
        GELOGI("[ZCPY] virtual_addr 0x%lx has been fusion to virtual_addr 0x%lx", addr_val, virtual_addr);
      }
    }
  }
  addr_count_ = out_count;
  valid_relative_offset_ = true;
}

void ZeroCopyOffset::SetOutputOutsideAddrs(const int64_t input_offset, const bool fusion_flag, const uintptr_t addr_val,
                                           std::vector<const void *> &tensor_addrs) {
  GELOGI("[ZCPY] Start to SetOutputOutsideAddrs for virtual_addr 0x%lx.", addr_val);
  size_t out_count = 0U;
  if (!fusion_flag) {
    out_count++;
    std::map<uintptr_t, std::vector<uintptr_t>> addr_mapping { {addr_val, {}} };
    outside_addrs_.emplace_back(std::move(addr_mapping));
    tensor_addrs.emplace_back(ValueToPtr(addr_val));
  } else {
    GELOGI("[ZCPY] set l2-fusion for virtual_addr 0x%lx", addr_val);
    for (size_t i = 0U; i < zero_copy_basic_offset_.size(); ++i) {
      if (zero_copy_basic_offset_.at(i) == input_offset) {
        out_count++;
        const uintptr_t virtual_addr = addr_val + static_cast<uint64_t>(zero_copy_relative_offset_.at(i));
        std::map<uintptr_t, std::vector<uintptr_t>> addr_mapping { {virtual_addr, {}} };
        outside_addrs_.emplace_back(std::move(addr_mapping));
        tensor_addrs.emplace_back(ValueToPtr(virtual_addr));
        GELOGI("[ZCPY] virtual_addr 0x%lx has been fusion to virtual_addr 0x%lx", addr_val, virtual_addr);
      }
    }
  }
  addr_count_ = out_count;
  valid_relative_offset_ = true;
}

void ZeroCopyOffset::SetOutsideAddrsValue(ZeroCopyTask &zero_copy_task, const uintptr_t outside_addr,
                                          const uintptr_t args_base, const size_t offset) {
  if (!valid_relative_offset_) {
    return;
  }

  for (size_t out_count = 0U; out_count < GetAddrCount(); ++out_count) {
    const auto args_addrs = outside_addrs_[out_count].find(outside_addr);
    if (args_addrs != outside_addrs_[out_count].end()) {
      GE_CHK_STATUS(zero_copy_task.SetTaskArgsOffset(outside_addr, offset),
                    "[Set][TaskArgsOffset] failed, Input args invalid, offset:%zu.", offset);
      args_addrs->second.push_back(args_base + offset);
      GELOGD("[ZCPY] set copy input: virtual_addr: 0x%lx, task_addr: 0x%lx, args: 0x%lx, offset: %zu",
             outside_addr, args_base + offset, args_base, offset);
    }
  }
}

void ZeroCopyOffset::SetLogicalOutsideAddrs(const uintptr_t logical_addr, const uintptr_t device_addr) {
  if (!valid_relative_offset_) {
    GELOGD("Skip set logical addr outside device addr for 0x%lx as invalid offset", logical_addr);
    return;
  }
  for (size_t i = 0U; i < GetAddrCount(); ++i) {
    const auto args_addrs = outside_addrs_[i].find(logical_addr);
    if (args_addrs != outside_addrs_[i].end()) {
      GELOGD("Set logical addr 0x%lx outside device addr 0x%lx", logical_addr, device_addr);
      args_addrs->second.push_back(device_addr);
    }
  }
}
}  // namespace ge
