/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "adapter/adapter_itf/task_builder_adapter.h"
#include "common/aicore_util_attr_define.h"
#include "common/comm_log.h"
#include "common/common_utils.h"
#include "common/fe_type_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/op_desc.h"
#include "graph/utils/anchor_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {

TaskBuilderAdapter::TaskBuilderAdapter(const ge::Node &node, TaskBuilderContext &context)
    : node_(node), op_desc_(node_.GetOpDesc()), context_(context) {}

TaskBuilderAdapter::~TaskBuilderAdapter() {}

void TaskBuilderAdapter::GetTaskArgs(TaskArgs &args_info) const {
  args_info.input_addrs = input_addrs_;
  args_info.output_addrs = output_addrs_;
  args_info.workspace_addrs = workspace_addrs_;
}

Status TaskBuilderAdapter::Init() {
  CM_CHECK_NOTNULL(op_desc_);

  // Init input
  Status status = InitInput();
  if (status != SUCCESS) {
    CM_LOGE("[GenTask][Init][InitInput][%s, type %s] InitInput failed.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return status;
  }

  // Init output
  status = InitOutput();
  if (status != SUCCESS) {
    CM_LOGE("[GenTask][Init][InitOutput][%s, type %s] InitOutput failed.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return status;
  }

  // Init workspace
  status = InitWorkspace();
  if (status != SUCCESS) {
    CM_LOGE("[GenTask][Init][InitWorkspace][%s, type %s] InitWorkspace failed.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return status;
  }

  // Init L1 input
  status = InitInputL1Addr();
  if (status != SUCCESS) {
    CM_LOGE("[GenTask][Init][InitInputL1Addr][%s, type %s] InitInputL1Addr failed.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return status;
  }

  int64_t is_unknown_graph_value = 0;
  (void)ge::AttrUtils::GetInt(op_desc_, ATTR_NAME_IS_UNKNOWN_GRAPH, is_unknown_graph_value);
  CM_LOGD("Op[name=%s,type=%s]: is_unknown_graph_value flag is %ld.", op_desc_->GetName().c_str(),
          op_desc_->GetType().c_str(), is_unknown_graph_value);
  if (is_unknown_graph_value == 0) {
    // Verify weight offset.
    status = VerifyWeights();
    if (status != SUCCESS) {
      CM_LOGE("[GenTask][Init][VerifyWeights][%s, type %s] VerifyWeights failed.",
              op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
      return status;
    }
  }

  return SUCCESS;
}

Status TaskBuilderAdapter::Run(domi::TaskDef &task_def) {
  (void)task_def;
  return SUCCESS;
}

Status TaskBuilderAdapter::GetHandleStream(ccHandle_t handle, rtStream_t *streamId) {
  CM_CHECK_NOTNULL(handle);
  *streamId = handle->streamId;
  CM_CHECK_NOTNULL(streamId);
  return SUCCESS;
}

Status TaskBuilderAdapter::InitOutput() {
  // Verify output number.
  size_t output_num = op_desc_->GetOutputsSize();
  vector<int64_t> output_offsets = op_desc_->GetOutputOffset();
  // if output_offsets is empty, set 0 to vector
  if (output_offsets.empty()) {
    vector<int64_t> output_offset_zero(output_num, 0);
    output_offsets.swap(output_offset_zero);
    CM_LOGD("Node[type=%s,name=%s]: output_offset_zero=%zu", op_desc_->GetType().c_str(), op_desc_->GetName().c_str(),
            output_offsets.size());
  }

  if (output_num != output_offsets.size()) {
    REPORT_CM_ERROR(
        "[GenTask][InitOutput][Node %s, type %s]: output size != offset_size, output size:%zu, offset_size:%zu.",
        op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), output_num, output_offsets.size());
    return FAILED;
  }
  vector<bool> output_is_addr_var;
  (void)ge::AttrUtils::GetListBool(op_desc_, ATTR_NAME_OUTPUT_IS_VAR, output_is_addr_var);
  for (size_t i = 0; i < output_num; ++i) {
    auto output_desc = op_desc_->GetOutputDesc(i);
    if (IsMemoryEmpty(output_desc)) {
      CM_LOGI("Node[type=%s,name=%s]: the output %s is memory empty.", op_desc_->GetType().c_str(),
              op_desc_->GetName().c_str(), op_desc_->GetOutputNameByIndex(i).c_str());
      continue;
    }

    int64_t output_offset = output_offsets[i];
    if (output_is_addr_var.size() > i && output_is_addr_var[i]) {
      output_addrs_.push_back(reinterpret_cast<void *>(reinterpret_cast<intptr_t>(output_offset)));
    } else {
      output_addrs_.push_back(context_.dataMemBase + output_offset);
    }
  }

  CM_LOGD("Node[type=%s,name=%s]: init output.", op_desc_->GetType().c_str(), op_desc_->GetName().c_str());
  return SUCCESS;
}

Status TaskBuilderAdapter::InitWorkspace() {
  vector<int64_t> workspace_sizes = op_desc_->GetWorkspaceBytes();
  vector<int64_t> workspace_offsets = op_desc_->GetWorkspace();

  int64_t is_unknown_graph_value = 0;
  (void)ge::AttrUtils::GetInt(op_desc_, ATTR_NAME_IS_UNKNOWN_GRAPH, is_unknown_graph_value);
  if (is_unknown_graph_value == 0) {
    if (workspace_offsets.size() != workspace_sizes.size()) {
      REPORT_CM_ERROR(
          "[GenTask][InitWorkSpace][%s, type %s]: workspaceOffsets.size()[%zu] != workspace_sizes.size()[%zu]",
          op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), workspace_offsets.size(),
          workspace_sizes.size());
      return FAILED;
    }
  }

  size_t workspace_num = workspace_sizes.size();
  for (size_t i = 0; i < workspace_num; i++) {
    auto workspace_size = static_cast<uint64_t>(workspace_sizes[i]);
    uint64_t workspace_offset = 0;
    if (i < workspace_offsets.size()) {
      workspace_offset = static_cast<uint64_t>(workspace_offsets[i]);
    }
    if (is_unknown_graph_value == 0) {
      CM_LOGD("Op[name=%s,type=%s]: is_unknown_graph_value flag is false.", op_desc_->GetName().c_str(),
              op_desc_->GetType().c_str());
      Status status = CheckOffsetAndSize(workspace_offset, workspace_size, context_.dataMemSize);
      if (status != SUCCESS) {
        REPORT_CM_ERROR(
            "[GenTask][InitWorkSpace][%s, type %s]: Check offset and size of workspace index: %zu) failed!",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), i);
        return status;
      }
    }

    workspace_sizes_.push_back(static_cast<uint32_t>(workspace_size));
    workspace_addrs_.push_back(workspace_size == 0 ? nullptr : context_.dataMemBase + workspace_offset);
  }

  return SUCCESS;
}

Status TaskBuilderAdapter::InitInputL1Addr() {
  vector<int64_t> input_l1_flag;
  if (!ge::AttrUtils::GetListInt(op_desc_, ge::ATTR_NAME_OP_INPUT_L1_FLAG, input_l1_flag)) {
    CM_LOGD("Op[%s, %s] does not have OP_INPUT_L1_FLAG attribute.", op_desc_->GetName().c_str(),
            op_desc_->GetType().c_str());
    return SUCCESS;
  }
  vector<int64_t> input_l1_addrs;
  if (!ge::AttrUtils::GetListInt(op_desc_, ge::ATTR_NAME_OP_INPUT_L1_ADDR, input_l1_addrs)) {
    CM_LOGD("Op[%s, %s] does not have OP_INPUT_L1_ADDR attribute.", op_desc_->GetName().c_str(),
            op_desc_->GetType().c_str());
    return SUCCESS;
  }

  if (input_l1_flag.empty() || input_l1_addrs.empty()) {
    CM_LOGD("The vector of op_input_l1_flag or op_input_l1_addrs is empty.");
    return SUCCESS;
  }
  if (input_l1_flag.size() != input_l1_addrs.size()) {
    CM_LOGD("Size of op_input_l1_flag and op_input_l1_addrs is not equal.");
    return SUCCESS;
  }

  CM_LOGD("The value of OpInputL1Flag and OpInputL1Addrs of op[%s, %s] is [%s] and [%s].", op_desc_->GetName().c_str(),
          op_desc_->GetType().c_str(), IntegerVecToString(input_l1_flag).c_str(),
          IntegerVecToString(input_l1_addrs).c_str());

  for (size_t i = 0; i < input_l1_flag.size(); i++) {
    if (input_l1_flag[i] >= 0) {
      input_l1_addrs_.push_back(reinterpret_cast<void *>(reinterpret_cast<intptr_t>(input_l1_addrs[i])));
    }
  }

  return SUCCESS;
}

Status TaskBuilderAdapter::VerifyWeights() {
  // Verify weight offset.
  vector<ge::ConstGeTensorPtr> weights = ge::OpDescUtils::GetWeights(node_);
  for (const ge::ConstGeTensorPtr& weight : weights) {
    int64_t weight_offset = 0;
    if (ge::TensorUtils::GetDataOffset(weight->GetTensorDesc(), weight_offset) != ge::GRAPH_SUCCESS) {
      REPORT_CM_ERROR("[GenTask][VerifyWeights][%s, type %s]: Get weight offset failed.",
                      op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
      return FAILED;
    }

    uint64_t weight_size = ge::TensorUtils::GetWeightSize(weight);
    Status status = CheckOffsetAndSize(weight_offset, weight_size, context_.weightMemSize);
    if (status != SUCCESS) {
      REPORT_CM_ERROR("[GenTask][VerifyWeights][%s, type %s]: Check offset and size of weight failed.",
                      op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
      return status;
    }
  }

  return SUCCESS;
}

Status TaskBuilderAdapter::CheckOffsetAndSize(int64_t offset, uint64_t space_size, uint64_t total_size) {
  if (offset < 0) {
    REPORT_CM_ERROR("[GenTask][Init][Check] offset should not be less than 0. offset[%ld]", offset);
    return FAILED;
  }
  CM_UINT64_ADDCHECK(static_cast<uint64_t>(offset), space_size);
  if (static_cast<uint64_t>(offset) + space_size > total_size) {
    REPORT_CM_ERROR(
        "[GenTask][Init][Check] offset[%ld] + size[%lu] should not be greater than total_size[%lu]",
        offset, space_size, total_size);
    return FAILED;
  }

  return SUCCESS;
}

}  // namespace fe
