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

#include "hybrid/node_executor/rts/rts_node_task.h"
#include "hybrid/node_executor/rts/rts_task_factory.h"

#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/node_utils.h"
#include "common/plugin/ge_util.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/types.h"

namespace {
constexpr int32_t kSwitchPredIndex = 0;
constexpr int32_t kSwitchCompIndex = 1;

const static std::map<rtCondition_t, std::function<bool(const int64_t, const int64_t)>> kCompHandle = {
    {RT_EQUAL, [](const int64_t pred_value, const int64_t comp_value) { return pred_value == comp_value; }},
    {RT_NOT_EQUAL, [](const int64_t pred_value, const int64_t comp_value) { return pred_value != comp_value; }},
    {RT_GREATER, [](const int64_t pred_value, const int64_t comp_value) { return pred_value > comp_value; }},
    {RT_GREATER_OR_EQUAL, [](const int64_t pred_value, const int64_t comp_value) { return pred_value >= comp_value; }},
    {RT_LESS, [](const int64_t pred_value, const int64_t comp_value) { return pred_value < comp_value; }},
    {RT_LESS_OR_EQUAL, [](const int64_t pred_value, const int64_t comp_value) { return pred_value <= comp_value; }},
};
}

namespace ge {
namespace hybrid {
REGISTER_RTS_TASK_CREATOR(STREAMACTIVE, StreamActiveNodeTask);
REGISTER_RTS_TASK_CREATOR(STREAMSWITCH, StreamSwitchNodeTask);
REGISTER_RTS_TASK_CREATOR(STREAMMERGE, StreamMergeNodeTask);

REGISTER_RTS_TASK_CREATOR(ENTER, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(REFENTER, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(LOOPCOND, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(NEXTITERATION, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(REFNEXTITERATION, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(EXIT, PassThroughNodeTask);
REGISTER_RTS_TASK_CREATOR(REFEXIT, PassThroughNodeTask);

REGISTER_RTS_TASK_CREATOR(LABELSET, LabelSetNodeTask);
REGISTER_RTS_TASK_CREATOR(LABELGOTOEX, LabelGotoNodeTask);
REGISTER_RTS_TASK_CREATOR(LABELSWITCHBYINDEX, LabelSwitchNodeTask);

Status RtsNodeTask::GetScalarIndexValue(const TaskContext &task_ctx, const int32_t idx, int64_t &val) {
  const auto tensor_val = task_ctx.GetInput(idx);
  GE_CHECK_NOTNULL(tensor_val);
  const auto tensor_desc_in = task_ctx.MutableInputDesc(idx);
  GE_CHECK_NOTNULL(tensor_desc_in);

  const auto data_type_in = tensor_desc_in->GetDataType();
  int32_t data_val{};
  Status ret = SUCCESS;
  switch (data_type_in) {
    // Just accept index data type.
    case (DT_INT32):
      GE_CHK_STATUS_RET(tensor_val->CopyScalarValueToHost(data_val));
      val = static_cast<int64_t>(data_val);
      break;
    case (DT_INT64):
      GE_CHK_STATUS_RET(tensor_val->CopyScalarValueToHost(data_val));
      val = static_cast<int64_t>(data_val);
      break;
    default:
      GELOGE(UNSUPPORTED, "[Check][Param] Data type %s not index type.",
             TypeUtils::DataTypeToSerialString(data_type_in).c_str());
      ret = UNSUPPORTED;
      break;
  }

  return ret;
}

Status StreamActiveNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  const auto node_state = context.GetNodeState();
  node_state->RunStreamActive();
  GE_CHK_STATUS_RET(context.TryExecuteCallback(done_callback));

  GELOGI("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status StreamSwitchNodeTask::Init(const HybridModel &model, const NodePtr &node) {
  (void)model;
  uint32_t val = 0U;
  if (!AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_STREAM_SWITCH_COND, val)) {
    GELOGE(INTERNAL_ERROR, "[Get][Attr] %s of op:%s(%s) failed.", ATTR_NAME_STREAM_SWITCH_COND.c_str(),
           node->GetName().c_str(), node->GetType().c_str());
    return INTERNAL_ERROR;
  }
  const rtCondition_t cond = static_cast<rtCondition_t>(val);
  const auto it = kCompHandle.find(cond);
  if (it == kCompHandle.end()) {
    GELOGE(INTERNAL_ERROR, "[Check][Param] node:%s(%s) Get Condition: %u handle failed.",
           node->GetName().c_str(), node->GetType().c_str(), val);
    return INTERNAL_ERROR;
  }

  comp_func_ = it->second;
  GELOGD("[%s] Done initialization successfully, condition is %u.", node->GetName().c_str(), val);
  return SUCCESS;
}

Status StreamSwitchNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  GE_CHECK_NOTNULL(comp_func_);

  int64_t pred_value = 0;
  GE_CHK_STATUS_RET(GetScalarIndexValue(context, kSwitchPredIndex, pred_value));
  int64_t comp_value = 0;
  GE_CHK_STATUS_RET(GetScalarIndexValue(context, kSwitchCompIndex, comp_value));

  const bool switch_idx = comp_func_(pred_value, comp_value);
  const auto node_state = context.GetNodeState();
  node_state->SetSwitchIndex(static_cast<int32_t>(switch_idx));

  GE_CHK_STATUS_RET(context.TryExecuteCallback(done_callback));

  GELOGI("[%s] Done executing successfully, pred value: %ld, comp value: %ld, switch index: %d.",
         context.GetNodeName(), pred_value, comp_value, static_cast<int32_t>(switch_idx));
  return SUCCESS;
}

Status StreamMergeNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  const int32_t idx = context.GetNodeState()->GetMergeIndex();
  GELOGD("[%s] Start to execute, merge index: %d.", context.GetNodeName(), idx);
  if ((idx < 0) || (idx >= context.NumInputs())) {
    GELOGE(INTERNAL_ERROR, "[Check][Param] [%s(%s)] Invalid merge param, inputs num:%d, merge index:%d.",
           context.GetNodeName(), context.GetNodeItem().NodeType().c_str(), context.NumInputs(), idx);
    return INTERNAL_ERROR;
  }

  const auto in_x = context.MutableInput(idx); // x
  GE_CHECK_NOTNULL(in_x);
  GE_CHK_STATUS_RET_NOLOG(context.SetOutput(MERGE_DATA_OUTPUT, *in_x)); // y

  const auto out_y = context.MutableOutput(MERGE_INDEX_OUTPUT);  // value_index
  GE_CHECK_NOTNULL(out_y);
  if (out_y->GetSize() > 0UL) {
    GE_CHK_RT_RET(rtMemcpyAsync(out_y->MutableData(), out_y->GetSize(), &idx, sizeof(idx),
                                RT_MEMCPY_HOST_TO_DEVICE_EX, context.GetStream()));
  }

  GE_CHK_STATUS_RET(context.TryExecuteCallback(done_callback));

  context.GetNodeState()->SetMergeIndex(-1); // Invalidate for loop.
  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status PassThroughNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  const auto in_x = context.GetInput(0); // x
  GE_CHECK_NOTNULL(in_x);
  GE_CHK_STATUS_RET_NOLOG(context.SetOutput(0, *in_x)); // y

  const auto &node_state = context.GetNodeState();
  if (kNextIterationOpTypes.count(node_state->GetType()) > 0UL) {
    node_state->RunNextIteration();
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  } else {
    GE_CHK_STATUS_RET(context.TryExecuteCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status LabelSetNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return UNSUPPORTED;
}

Status LabelGotoNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return UNSUPPORTED;
}

Status LabelSwitchNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start to execute.", context.GetNodeName());

  if (done_callback) {
    GE_CHK_STATUS_RET(context.RegisterCallback(done_callback));
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return UNSUPPORTED;
}
}  // namespace hybrid
}  // namespace ge