/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
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

#include "dsa_task_builder.h"

#include <securec.h>
#include <string>
#include <functional>
#include "common/comm_log.h"
#include "common/common_utils.h"
#include "common/comm_error_codes.h"
#include "common/fe_error_code.h"
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_attr_define.h"
#include "common/configuration.h"
#include "ops_store/sub_op_info_store.h"
#include "adapter/factory/task_builder_adapter_factory.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "util/error_manager/error_manager.h"
#include "runtime/rt_error_codes.h"
#include "runtime/rt_model.h"
#include "runtime/mem.h"

namespace fe {
static const std::string kConstantOp = "Constant";
static const std::string kConstant = "Const";

DsaTaskBuilder::DsaTaskBuilder() {}

DsaTaskBuilder::~DsaTaskBuilder() {}

uint32_t DsaTaskBuilder::DSAFlags::CalAddrOrValueFlag() const {
  uint32_t addr_or_value_flag = 0U;
  switch (distribution_type) {
    case DistributionType::DIS_BITMASK:
      addr_or_value_flag = static_cast<uint32_t>(input1_type);
      break;
    case DistributionType::DIS_UNIFORM:
      addr_or_value_flag = static_cast<uint32_t>(input1_type) << 1;
      addr_or_value_flag |= static_cast<uint32_t>(input2_type) << 2;
      break;
    case DistributionType::DIS_NORMAL:
      addr_or_value_flag = static_cast<uint32_t>(input1_type) << 3;
      addr_or_value_flag |= static_cast<uint32_t>(input1_type) << 4;
      break;
    case DistributionType::DIS_TRUNCATED_NORMAL:
      addr_or_value_flag = static_cast<uint32_t>(input1_type) << 3;
      addr_or_value_flag |= static_cast<uint32_t>(input1_type) << 4;
      break;
    default:
      break;
  }

  addr_or_value_flag |= static_cast<uint32_t>(seed_type) << 5;
  addr_or_value_flag |= static_cast<uint32_t>(rand_count_type) << 6;
  return addr_or_value_flag;
}

Status DsaTaskBuilder::GenerateTask(const ge::Node &node, const ge::RunContext &context,
                                    std::vector<domi::TaskDef> &task_defs) {
  CM_LOGD("DsaTaskBuilder::GenerateTask begin, node name:%s, node type:%s.", node.GetName().c_str(),
          node.GetType().c_str());
  auto opDesc = node.GetOpDesc();
  int64_t begin_timestamps = GetMicroSecondsTime();

  CM_CHECK_NOTNULL(opDesc);
  CM_CHECK_NOTNULL(context.dataMemBase);

  context_.dataMemBase = context.dataMemBase;

  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_DSA_TASK);
  auto dsa_task_def = task_def.mutable_dsa_task();
  CM_CHECK_NOTNULL(dsa_task_def);
  dsa_task_def->set_op_index(opDesc->GetId());
  dsa_task_def->set_start(dsa_start_);
  dsa_task_def->set_sqe_type(dsa_type_);

  DSAFlags dsa_flags;
  CM_CHECK(GetDsaValueOrAddrFlags(node, dsa_flags) != SUCCESS, CM_LOGD("GetDsaValueOrAddrFlags failed"), return FAILED);
  dsa_task_def->set_distribution_type(static_cast<uint32_t>(dsa_flags.distribution_type));
  dsa_task_def->set_input_vld(static_cast<uint32_t>(dsa_flags.input_vld));
  dsa_task_def->set_input1_value_or_ptr(static_cast<uint32_t>(dsa_flags.input1_type));
  dsa_task_def->set_input2_value_or_ptr(static_cast<uint32_t>(dsa_flags.input2_type));
  dsa_task_def->set_seed_value_or_ptr(static_cast<uint32_t>(dsa_flags.seed_type));
  dsa_task_def->set_random_count_value_or_ptr(static_cast<uint32_t>(dsa_flags.rand_count_type));
  dsa_task_def->set_input_value_addr_flag(dsa_flags.CalAddrOrValueFlag());

  uint32_t data_type = 0;
  CM_CHECK(GetDataType(node, data_type) != SUCCESS, CM_LOGD("GetDataType failed"), return FAILED);
  dsa_task_def->set_data_type(data_type);
  dsa_task_def->set_alg_type(dsa_philox_type_);

  DsaWorkspace workspace;
  CM_CHECK(GetWorkspaceInfo(node, workspace) != SUCCESS, CM_LOGD("GetWorkspaceInfo failed"), return FAILED);
  auto args_ptr = dsa_task_def->mutable_args();
  CM_CHECK_NOTNULL(args_ptr);
  args_ptr->set_workspace_input_addr(workspace.input_addr);
  args_ptr->set_workspace_philox_count_addr(workspace.philox_count_addr);

  uint64_t output_addr = 0;
  CM_CHECK(GetOutputAddr(node, output_addr) != SUCCESS, CM_LOGD("GetOutputAddr failed"), return FAILED);
  args_ptr->set_output_addr(output_addr);

  DsaInput input;
  CM_CHECK(GetInputs(node, dsa_flags, input) != SUCCESS, CM_LOGD("GetInputs failed"), return FAILED);
  args_ptr->set_random_count_value_or_addr(input.random_count);
  args_ptr->set_input1_value_or_addr(input.input1);
  args_ptr->set_input2_value_or_addr(input.input2);
  args_ptr->set_seed_value_or_addr(input.seed);

  task_defs.push_back(task_def);
  CM_LOGD("DsaTaskBuilder::GenerateTask end, node name:%s, node type:%s, dsa_task_def:%s.", node.GetName().c_str(),
          node.GetType().c_str(), task_def.DebugString().c_str());

  int64_t end_timestamps = GetMicroSecondsTime();
  CM_LOGV("[FE_PERFORMANCE]The time cost of DsaTaskBuilder::GenerateTask is [%ld] micro second.",
          (end_timestamps - begin_timestamps));
  return SUCCESS;
}

bool DsaTaskBuilder::IsConstInput(const ge::Node &node, uint32_t input_idx) {
  bool ret = false;
  const auto &in_data_anchors = node.GetAllInDataAnchors();
  if (input_idx < static_cast<size_t>(in_data_anchors.size())) {
    for (const auto &anchor : in_data_anchors) {
      if (anchor->GetIdx() != static_cast<int>(input_idx)) {
        continue;
      }
      auto peer_anchor = anchor->GetPeerOutAnchor();
      if (!peer_anchor) {
        break;
      }
      auto owner_node = peer_anchor->GetOwnerNode();
      if (!owner_node) {
        break;
      }

      auto op_desc = owner_node->GetOpDesc();
      if (!op_desc) {
        break;
      }

      const auto type = op_desc->GetType();
      ret = (type == kConstant || type == kConstantOp);
    }
  }
  return ret;
}

bool DsaTaskBuilder::GetConstInputValue(const ge::Node &node, uint32_t input_idx, std::string &value) {
  auto op = ge::OpDescUtils::CreateOperatorFromNode(node.shared_from_this());
  auto const_tensor = ge::OpDescUtils::GetInputConstData(op, input_idx);
  if (const_tensor == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetConstInputValue][node_name %s] get const data failed.", node.GetName().c_str());
    return false;
  }

  value.assign(reinterpret_cast<const char *>(const_tensor->GetData().GetData()), const_tensor->GetData().GetSize());
  return true;
}

bool DsaTaskBuilder::GetInputDataType(const ge::Node &node, uint32_t input_idx, ge::DataType &data_type) {
  auto desc_ptr = node.GetOpDesc();
  if (desc_ptr == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetInputDataType][node_name %s] get desc failed.", node.GetName().c_str());
    return false;
  }

  auto input_desc_ptr = desc_ptr->MutableInputDesc(input_idx);
  if (input_desc_ptr == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetInputDataType][node_name %s] get output desc failed.", node.GetName().c_str());
    return false;
  }

  data_type = input_desc_ptr->GetDataType();
  return true;
}

template <typename T>
static std::string ToValueString(const std::string &value) {
  const T *value_ptr = reinterpret_cast<const T *>(value.data());
  return std::to_string(*value_ptr);
}

std::string DsaTaskBuilder::GetValueDebugString(const ge::Node &node, uint32_t input_idx) {
  std::string value;
  if (!GetConstInputValue(node, input_idx, value)) {
    return "";
  }

  ge::DataType data_type = ge::DT_FLOAT;
  if (!GetInputDataType(node, input_idx, data_type)) {
    return value;
  }

  using namespace std::placeholders;
  static const std::map<ge::DataType, std::function<std::string(const std::string &)>> type_funcs = {
      {ge::DT_INT64, std::bind(ToValueString<int64_t>, _1)},
      {ge::DT_UINT64, std::bind(ToValueString<uint64_t>, _1)},
      {ge::DT_FLOAT, std::bind(ToValueString<float>, _1)},
      {ge::DT_FLOAT16, std::bind(ToValueString<float>, _1)},
  };

  auto found = type_funcs.find(data_type);
  if (found != type_funcs.end()) {
    return found->second(value);
  }

  return value;
}

Status DsaTaskBuilder::GetDsaValueOrAddrFlags(const ge::Node &node, DSAFlags &flags) const {
  const auto op_type = node.GetType();
  auto iter = dsa_opname_values_.find(op_type);
  if (iter == dsa_opname_values_.end()) {
    REPORT_CM_ERROR("[GenTask][GetDsaValueOrAddrFlags][type %s] not supported.", op_type.c_str());
    return FAILED;
  }

  auto flags_indexes = dsa_op_flags_idx_.find(op_type);
  if (flags_indexes == dsa_op_flags_idx_.end() || flags_indexes->second.empty()) {
    REPORT_CM_ERROR("[GenTask][GetDsaValueOrAddrFlags][type %s] not supported.", op_type.c_str());
    return FAILED;
  }

  flags = iter->second;

  vector<DsaValueType *> input_dsa_value_types = {&(flags.rand_count_type), &(flags.seed_type), &(flags.input1_type),
                                                  &(flags.input2_type)};

  uint32_t input_idx = 0;
  for (auto flag_index : flags_indexes->second) {
    if (flag_index >= input_dsa_value_types.size()) {
      REPORT_CM_ERROR("[GenTask][GetDsaValueOrAddrFlags][type %s] not supported.", op_type.c_str());
      return FAILED;
    }
    DsaValueType &value = *input_dsa_value_types[flag_index];

    if (IsConstInput(node, input_idx)) {
      value = DsaValueType::DSA_DATA_VALUE;
      CM_LOGD("DsaTaskBuilder::GenerateTask input%u is value.", input_idx);
    } else {
      value = DsaValueType::DSA_DATA_ADDR;
      CM_LOGD("DsaTaskBuilder::GenerateTask input%u is addr.", input_idx);
    }
    input_idx++;
  }

  return SUCCESS;
}

Status DsaTaskBuilder::GetDataType(const ge::Node &node, uint32_t &type) const {
  const uint32_t input_value_idx = 2;
  ge::DataType data_type = ge::DT_FLOAT;
  if (!GetInputDataType(node, input_value_idx, data_type)) {
    return FAILED;
  }

  CM_LOGD("DsaTaskBuilder::GenerateTask input data type is %s.",
          ge::TypeUtils::DataTypeToSerialString(data_type).c_str());
  auto iter = dsa_datatype_values_.find(data_type);
  if (iter == dsa_datatype_values_.end()) {
    REPORT_CM_ERROR("[GenTask][GetDataType][node_name %s] dsa unsupported data_type[%s].", node.GetName().c_str(),
                    ge::TypeUtils::DataTypeToSerialString(data_type).c_str());
    return FAILED;
  }

  type = iter->second;
  return SUCCESS;
}

Status DsaTaskBuilder::GetWorkspaceInfo(const ge::Node &node, DsaWorkspace &workspace) const {
  auto desc_ptr = node.GetOpDesc();
  if (desc_ptr == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetWorkspaceInfo][node_name %s] get desc failed.", node.GetName().c_str());
    return FAILED;
  }

  auto workspaces = desc_ptr->GetWorkspace();
  const size_t needed_workspace_count = 2;
  if (workspaces.size() != needed_workspace_count) {
    REPORT_CM_ERROR("[GenTask][GetWorkspaceInfo][node_name %s] need %zu workspace, but only %zu given.",
                    node.GetName().c_str(), needed_workspace_count, workspaces.size());
    return FAILED;
  }

  workspace.philox_count_addr = reinterpret_cast<uint64_t>(context_.dataMemBase) + workspaces[0];
  workspace.input_addr = reinterpret_cast<uint64_t>(context_.dataMemBase) + workspaces[1];
  return SUCCESS;
}

Status DsaTaskBuilder::GetOutputAddr(const ge::Node &node, uint64_t &addr) const {
  auto desc_ptr = node.GetOpDesc();
  if (desc_ptr == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetOutputAddr][node_name %s] get desc failed.", node.GetName().c_str());
    return FAILED;
  }

  auto offsets = desc_ptr->GetOutputOffset();
  if (offsets.empty()) {
    REPORT_CM_ERROR("[GenTask][GetOutputAddr][node_name %s] output offset is empty.", node.GetName().c_str());
    return FAILED;
  }

  addr = reinterpret_cast<uint64_t>(context_.dataMemBase) + offsets[0];
  return SUCCESS;
}

Status DsaTaskBuilder::GetInputs(const ge::Node &node, const DSAFlags &flags, DsaInput &inputs) const {
  auto desc_ptr = node.GetOpDesc();
  if (desc_ptr == nullptr) {
    REPORT_CM_ERROR("[GenTask][GetInputs][node_name %s] get desc failed.", node.GetName().c_str());
    return FAILED;
  }

  const vector<std::pair<std::string &, DsaValueType>> input_map = {{inputs.random_count, flags.rand_count_type},
                                                                    {inputs.seed, flags.seed_type},
                                                                    {inputs.input1, flags.input1_type},
                                                                    {inputs.input2, flags.input2_type}};

  auto offsets = desc_ptr->GetInputOffset();
  auto op_type = node.GetType();
  auto flag_indexes = dsa_op_flags_idx_.find(op_type);
  if (flag_indexes == dsa_op_flags_idx_.end() || flag_indexes->second.empty()) {
    REPORT_CM_ERROR("[GenTask][GetInputs][type %s] not supported.", op_type.c_str());
    return FAILED;
  }

  uint32_t input_index = 0;
  for (auto flag_index : flag_indexes->second) {
    if (flag_index > input_map.size()) {
      REPORT_CM_ERROR("[GenTask][GetInputs][type %s] not supported.", op_type.c_str());
      return FAILED;
    }
    auto content = input_map[flag_index];
    if (content.second == DsaValueType::DSA_DATA_VALUE) {
      string input_value;
      if (!GetConstInputValue(node, input_index, input_value)) {
        REPORT_CM_ERROR("[GenTask][GetInputs][name %s] get const value of %u failed.", node.GetName().c_str(),
                        input_index);
        return FAILED;
      }
      content.first = input_value;
      CM_LOGD("DsaTaskBuilder::GenerateTask input%u is value[%s].", input_index,
              GetValueDebugString(node, input_index).c_str());
    } else {
      if (offsets.size() != flag_indexes->second.size()) {
        REPORT_CM_ERROR("[GenTask][GetInputs][node_name %s] need %zu input, but only %zu given.",
                        node.GetName().c_str(), flag_indexes->second.size(), offsets.size());
        return FAILED;
      }
      uint64_t input_addr = reinterpret_cast<uint64_t>(context_.dataMemBase) + offsets[input_index];
      content.first.assign(reinterpret_cast<char *>(&input_addr), sizeof(input_addr));
      CM_LOGD("DsaTaskBuilder::GenerateTask input%u is addr[%lu].", input_index, input_addr);
    }
    input_index++;
  }

  return SUCCESS;
}
}  // namespace fe
