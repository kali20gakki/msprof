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

#include "generate_cmo_barrier_task.h"
#include "common/comm_log.h"

namespace fe {
const uint8_t kMaxBarrierTaskNum = 6;
GenerateCMOBarrierTask::GenerateCMOBarrierTask(const ge::Node &node, TaskBuilderContext &context)
    : GenerateCMOTaskBase(node, context) {}
GenerateCMOBarrierTask::~GenerateCMOBarrierTask() {}

Status GenerateCMOBarrierTask::GenerateTask(std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                                            const std::vector<CmoAttr> &cmo_attrs) {
  domi::TaskDef task_def;
  task_def.set_stream_id(stream_id);
  task_def.set_type(RT_MODEL_TASK_BARRIER);
  domi::CmoBarrierTaskDef *cmo_task_def = task_def.mutable_cmo_barrier_task();
  if (cmo_task_def == nullptr) {
    CM_LOGW("Create cmo task def for node[%s] failed.", node_.GetName().c_str());
    return FAILED;
  }
  // get cmo id
  uint8_t cmoNum = 0;
  std::vector<uint32_t> cmo_ids;
  for (auto &cmo_attr : cmo_attrs) {
    int64_t complex_cmo_id = 0;
    if (GenerateCMOId(cmo_attr, complex_cmo_id) != SUCCESS) {
      CM_LOGW("Generate barrier cmo task of node[%s] failed.", node_.GetName().c_str());
      return FAILED;
    }
    uint32_t cmo_id = static_cast<uint32_t>(complex_cmo_id & 0x00000000ffffffff);
    uint16_t cmo_type = static_cast<uint16_t>((complex_cmo_id & 0xffffffff00000000) >> 32);
    CM_LOGD("Generate barrier cmo task[id:%d, type:%d] before node[%s].", cmo_id, cmo_type, node_.GetName().c_str());
    domi::CmoBarrierInfoDef *barrier_info_ptr = cmo_task_def->add_barrier_info();
    CM_CHECK_NOTNULL(barrier_info_ptr);
    barrier_info_ptr->set_cmo_type(cmo_type);
    barrier_info_ptr->set_logic_id(cmo_id);
    cmo_ids.emplace_back(cmo_id);
    cmoNum++;
    if (cmoNum >= kMaxBarrierTaskNum) {
      break;
    }
  }
  for (auto cmo_id : cmo_ids) {
    CMOIdGenStrategy::Instance().UpdateReuseMap(node_.GetOpDesc()->GetId(), cmo_id);
  }
  cmo_task_def->set_logic_id_num(cmoNum);
  task_defs.push_back(task_def);
  return SUCCESS;
}

Status GenerateCMOBarrierTask::GenerateCMOId(const CmoAttr &cmo_attr, int64_t &complex_cmo_id) const {
  ge::NodePtr pre_node = cmo_attr.node;
  if (cmo_attr.object == CmoTypeObject::INPUT || cmo_attr.object == CmoTypeObject::WEIGHT) {
    auto input_size = pre_node->GetOpDesc()->GetAllInputsSize();
    if (cmo_attr.object_index >= static_cast<int32_t>(input_size)) {
      CM_LOGW("Node[%s] input object_index[%d] is out of range[%zu]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, input_size);
      return FAILED;
    }
    ge::GeTensorDescPtr tensor_desc_ptr = pre_node->GetOpDesc()->MutableInputDesc(cmo_attr.object_index);
    (void)ge::AttrUtils::GetInt(tensor_desc_ptr, "_complex_cmo_id", complex_cmo_id);
    CM_LOGD("Get complex cmo id[%ld] from node[%s] success.", complex_cmo_id, pre_node->GetName().c_str());
  } else if (cmo_attr.object == CmoTypeObject::OUTPUT) {
    auto output_size = pre_node->GetOpDesc()->GetAllOutputsDescSize();
    if (cmo_attr.object_index >= static_cast<int32_t>(output_size)) {
      CM_LOGW("Node[%s] output object_index[%d] is out of range[%d]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, output_size);
      return FAILED;
    }
    ge::GeTensorDescPtr tensor_desc_ptr = pre_node->GetOpDesc()->MutableOutputDesc(cmo_attr.object_index);
    (void)ge::AttrUtils::GetInt(tensor_desc_ptr, "_complex_cmo_id", complex_cmo_id);
    CM_LOGD("Get complex cmo id[%ld] from node[%s] success.", complex_cmo_id, pre_node->GetName().c_str());
  } else {
    auto workspace_bytes = pre_node->GetOpDesc()->GetWorkspaceBytes();
    if (cmo_attr.object_index >= static_cast<int32_t>(workspace_bytes.size())) {
      CM_LOGW("Node[%s] workspace object_index[%d] is out of range[%zu]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, workspace_bytes.size());
      return FAILED;
    }
    (void)ge::AttrUtils::GetInt(pre_node->GetOpDesc(),
                                "_worksapce_" + std::to_string(cmo_attr.object_index) + "_complex_cmo_id",
                                complex_cmo_id);
    CM_LOGD("Get complex cmo id[%ld] from node[%s] success.", complex_cmo_id, pre_node->GetName().c_str());
  }
  return SUCCESS;
}
}
