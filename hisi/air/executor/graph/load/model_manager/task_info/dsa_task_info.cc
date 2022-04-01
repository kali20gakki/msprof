/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "graph/load/model_manager/task_info/dsa_task_info.h"

#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"

namespace ge {
namespace {
constexpr size_t kDSASetInputAddr = 0U;
constexpr size_t kDSAOutputAddrSize = 1U;
constexpr size_t kDSAWorkspaceAddrSize = 2U;
constexpr size_t kDSAInputAddrSize = 3U;
constexpr size_t kDSAArgsInputAddrSize = 4U;
constexpr size_t k32Bits = 32U;
constexpr uint32_t kMask32Bits = 0xFFFFFFFFU;  // 32 bits, 1111,1111,1111,1111,1111,1111,1111,1111
}

Status DSATaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("DSATaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model->GetStreamList()));

  const domi::DSATaskDef &dsa_task = task_def.dsa_task();
  const OpDescPtr op_desc = davinci_model->GetOpByIndex(dsa_task.op_index());
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Can't get op_desc from davinci_model by index:%u", dsa_task.op_index());
    GELOGE(INTERNAL_ERROR, "[Get][Op] Task op index:%u out of range!", dsa_task.op_index());
    return INTERNAL_ERROR;
  }

  dsa_sqe_.sqeHeader.type = static_cast<uint8_t>(dsa_task.sqe_type());
  dsa_sqe_.start = dsa_task.start();
  dsa_sqe_.functionType = dsa_task.distribution_type();
  dsa_sqe_.dataType = dsa_task.data_type();
  dsa_sqe_.algoType = dsa_task.alg_type();
  dsa_sqe_.paramVldBitmap = dsa_task.input_vld();
  dsa_sqe_.paramAddrValBitmap = dsa_task.input_value_addr_flag();
  dsa_sqe_.kernelCredit = 100U;

  const RuntimeParam &rts_param = davinci_model->GetRuntimeParam();
  const vector<void *> output_data_addrs = ModelUtils::GetOutputDataAddrs(rts_param, op_desc);
  if (output_data_addrs.size() != kDSAOutputAddrSize) {
    GELOGE(INTERNAL_ERROR, "Node %s output addr size %zu is wrong", op_desc->GetName().c_str(),
           output_data_addrs.size());
    return INTERNAL_ERROR;
  }
  const uint64_t dev_output_addr = PtrToValue(output_data_addrs[0U]);
  dsa_sqe_.dsaCfgResultAddrLow = static_cast<uint32_t>(dev_output_addr & kMask32Bits);
  dsa_sqe_.dsaCfgResultAddrHigh = static_cast<uint32_t>(dev_output_addr >> k32Bits);

  const vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);
  if (workspace_data_addrs.size() != kDSAWorkspaceAddrSize) {
    GELOGE(INTERNAL_ERROR, "Node %s workspace addr size %zu is wrong", op_desc->GetName().c_str(),
           workspace_data_addrs.size());
    return INTERNAL_ERROR;
  }
  const uint64_t workspace_philox_count_addr = PtrToValue(workspace_data_addrs[0U]);
  dsa_sqe_.dsaCfgStateAddrLow = static_cast<uint32_t>(workspace_philox_count_addr & kMask32Bits);
  dsa_sqe_.dsaCfgStateAddrHigh = static_cast<uint32_t>(workspace_philox_count_addr >> k32Bits);

  const uint64_t workspace_input_addr = PtrToValue(workspace_data_addrs[1U]);
  dsa_sqe_.dsaCfgParamAddrLow = static_cast<uint32_t>(workspace_input_addr & kMask32Bits);
  dsa_sqe_.dsaCfgParamAddrHigh = static_cast<uint32_t>(workspace_input_addr >> k32Bits);

  const vector<void *> input_data_addrs = ModelUtils::GetInputDataAddrs(rts_param, op_desc);
  if ((input_data_addrs.size() != kDSAInputAddrSize) && (input_data_addrs.size() != kDSAArgsInputAddrSize)) {
    GELOGE(INTERNAL_ERROR, "Node %s input addr size %zu is wrong", op_desc->GetName().c_str(),
           input_data_addrs.size());
    return INTERNAL_ERROR;
  }

  if (dsa_task.input1_value_or_ptr() == kDSASetInputAddr) {
    vector<uint64_t> input_addr{ PtrToValue(input_data_addrs[2U]) };
    if (input_data_addrs.size() == kDSAArgsInputAddrSize) {
      input_addr.push_back(PtrToValue(input_data_addrs[3U]));
    }
    const auto workspace_size = ModelUtils::GetWorkspaceSize(op_desc);
    GE_CHECK_GE(workspace_size.size(), kDSAWorkspaceAddrSize);
    GE_CHK_RT_RET(rtMemcpy(workspace_data_addrs[1U], static_cast<uint64_t>(workspace_size[1U]), input_addr.data(),
                           sizeof(uint64_t) * input_addr.size(), RT_MEMCPY_HOST_TO_DEVICE));
  } else {
    uint64_t input_data[2] = {0U, 0U};
    const std::string &input1 = dsa_task.args().input1_value_or_addr();
    auto mem_ret = memcpy_s(&input_data[0], sizeof(uint64_t), input1.c_str(), input1.size());
    if (mem_ret != EOK) {
      GELOGE(INTERNAL_ERROR, "dsa input data memcpy failed.");
      return INTERNAL_ERROR;
    }
    if (input_data_addrs.size() == kDSAArgsInputAddrSize) {
      const std::string &input2 = dsa_task.args().input2_value_or_addr();
      mem_ret = memcpy_s(&input_data[1], sizeof(uint64_t), input2.c_str(), input2.size());
      if (mem_ret != EOK) {
        GELOGE(INTERNAL_ERROR, "dsa input data memcpy failed.");
        return INTERNAL_ERROR;
      }
    }
    const auto workspace_size = ModelUtils::GetWorkspaceSize(op_desc);
    GE_CHECK_GE(workspace_size.size(), kDSAWorkspaceAddrSize);
    GE_CHK_RT_RET(rtMemcpy(workspace_data_addrs[1U], static_cast<uint64_t>(workspace_size[1U]), input_data,
                           sizeof(input_data), RT_MEMCPY_HOST_TO_DEVICE));
  }

  const uint64_t seed_value_or_addr = (dsa_task.seed_value_or_ptr() == kDSASetInputAddr) ?
                                      PtrToValue(input_data_addrs[1U]) :
                                      *(PtrToPtr<char_t, uint64_t>(dsa_task.args().seed_value_or_addr().c_str()));
  dsa_sqe_.dsaCfgSeedLow = static_cast<uint32_t>(seed_value_or_addr & kMask32Bits);
  dsa_sqe_.dsaCfgSeedHigh = static_cast<uint32_t>(seed_value_or_addr >> k32Bits);

  const uint64_t random_count_value_or_addr = (dsa_task.random_count_value_or_ptr() == kDSASetInputAddr) ?
      PtrToValue(input_data_addrs[0U]) :
      *(PtrToPtr<char_t, uint64_t>(dsa_task.args().random_count_value_or_addr().c_str()));
  dsa_sqe_.dsaCfgNumberLow = static_cast<uint32_t>(random_count_value_or_addr & kMask32Bits);
  dsa_sqe_.dsaCfgNumberHigh = static_cast<uint32_t>(random_count_value_or_addr >> k32Bits);
  GELOGI("DSATaskInfo Init Success");
  return SUCCESS;
}

Status DSATaskInfo::Distribute() {
  GELOGI("DSATaskInfo Distribute Start.");
  GE_CHK_RT_RET(rtStarsTaskLaunch(&dsa_sqe_, sizeof(dsa_sqe_), stream_));
  GELOGI("DSATaskInfo Distribute Success.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_DSA_TASK, DSATaskInfo);
}  // namespace ge
