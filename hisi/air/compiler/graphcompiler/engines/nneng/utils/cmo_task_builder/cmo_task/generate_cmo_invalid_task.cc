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

#include "generate_cmo_invalid_task.h"
#include "common/comm_log.h"
#include "graph/ge_tensor.h"

namespace fe {
GenerateCMOInvalidTask::GenerateCMOInvalidTask(const ge::Node &node, TaskBuilderContext &context)
    : GenerateCMOTaskBase(node, context) {}
GenerateCMOInvalidTask::~GenerateCMOInvalidTask() {}

Status GenerateCMOInvalidTask::GenerateTask(std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                                            const std::vector<CmoAttr> &cmo_attrs) {
  for (auto &cmo_attr : cmo_attrs) {
    TaskArgs task_args;
    if (InitCmoNodeAddrs(cmo_attr.node, task_args) != SUCCESS) {
      CM_LOGW("[GenTask][GenInvalidTask] Init node[%s] addrs failed.", cmo_attr.node->GetName().c_str());
      return FAILED;
    }
    domi::TaskDef task_def;
    task_def.set_stream_id(stream_id);
    task_def.set_type(RT_MODEL_TASK_CMO);
    domi::CmoTaskDef *cmo_task_def = task_def.mutable_cmo_task();
    if (cmo_task_def == nullptr) {
      CM_LOGW("Create cmo task def for node[%s] failed.", node_.GetName().c_str());
      return FAILED;
    }
    rtCMOType cmoType = rtCMOType::rtCMOInvalid;
    cmo_task_def->set_cmo_type(static_cast<uint32_t>(cmoType));
    // gen cmo id
    uint32_t cmo_id = static_cast<uint32_t>(CMOIdGenStrategy::Instance().GenerateCMOId(node_));
    if (cmo_id == 0) {
      CM_LOGW("Generate cmo id for mode[%s] failed, not launch cmo task.", node_.GetName().c_str());
      return FAILED;
    }
    CM_LOGD("Generate invalid cmo task id[%d] for node[%s] success.", cmo_id, node_.GetName().c_str());
    cmo_task_def->set_logic_id(cmo_id);
    ge::DataType data_type = ge::DT_UNDEFINED;
    uint32_t length_inner = 0;
    uint64_t source_addr;
    int64_t complex_cmo_id = (static_cast<int64_t>(cmoType) << 32) + cmo_id;
    Status status = ParseTensorInfo(cmo_attr, task_args, data_type, complex_cmo_id, source_addr, length_inner);
    if (status != SUCCESS) {
      CM_LOGW("Generate cmo task def for node[%s] failed.", node_.GetName().c_str());
      return FAILED;
    }
    // low 4bit: cmo type; high 4bit: data type
    // invalid: 0x7
    uint8_t op_code = 0x7;
    if (DATA_TYPE_CODE.count(data_type) == 0) {
      op_code += 0xf0;
    } else {
      op_code += (DATA_TYPE_CODE.at(data_type) << 4);
    }
    cmo_task_def->set_op_code(op_code);
    cmo_task_def->set_qos(0);
    cmo_task_def->set_part_id(0);
    cmo_task_def->set_pmg(0);
    cmo_task_def->set_num_inner(1);
    cmo_task_def->set_num_outer(1);
    cmo_task_def->set_length_inner(length_inner);
    cmo_task_def->set_source_addr(source_addr);
    cmo_task_def->set_strider_outer(0);
    cmo_task_def->set_strider_inner(0);

    task_defs.push_back(task_def);
  }
  return SUCCESS;
}

Status GenerateCMOInvalidTask::ParseTensorInfo(const CmoAttr &cmo_attr, const TaskArgs &task_args,
                                               ge::DataType &data_type, int64_t &complex_cmo_id, uint64_t &source_addr,
                                               uint32_t &length_inner) const {
  int64_t tensor_size = 0;
  if (cmo_attr.object == CmoTypeObject::INPUT || cmo_attr.object == CmoTypeObject::WEIGHT) {
    auto input_size = cmo_attr.node->GetOpDesc()->GetAllInputsSize();
    if (cmo_attr.object_index >= static_cast<int32_t>(input_size)) {
      CM_LOGW("Node[%s] input object_index[%d] is out of range[%zu]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, input_size);
      return FAILED;
    }
    ge::GeTensorDescPtr tensor_desc_ptr = cmo_attr.node->GetOpDesc()->MutableInputDesc(cmo_attr.object_index);
    if (ge::TensorUtils::GetSize(*tensor_desc_ptr, tensor_size) != ge::GRAPH_SUCCESS || tensor_size > UINT32_MAX) {
      CM_LOGW("Node[%s] tensor size is out of range.", cmo_attr.node->GetName().c_str());
      return FAILED;
    }
    length_inner = static_cast<uint32_t>(tensor_size);
    (void)ge::AttrUtils::SetInt(tensor_desc_ptr, "_complex_cmo_id", complex_cmo_id);
    data_type = tensor_desc_ptr->GetDataType();
    source_addr = reinterpret_cast<uint64_t>(task_args.input_addrs.at(cmo_attr.object_index));
  } else if (cmo_attr.object == CmoTypeObject::OUTPUT) {
    auto output_size = cmo_attr.node->GetOpDesc()->GetAllOutputsDescSize();
    if (cmo_attr.object_index >= static_cast<int32_t>(output_size)) {
      CM_LOGW("Node[%s] output object_index[%d] is out of range[%d]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, output_size);
      return FAILED;
    }
    ge::GeTensorDescPtr tensor_desc_ptr = cmo_attr.node->GetOpDesc()->MutableOutputDesc(cmo_attr.object_index);
    if (ge::TensorUtils::GetSize(*tensor_desc_ptr, tensor_size) != ge::GRAPH_SUCCESS || tensor_size > UINT32_MAX) {
      CM_LOGW("Node[%s] tensor size is out of range.", cmo_attr.node->GetName().c_str());
      return FAILED;
    }
    length_inner = static_cast<uint32_t>(tensor_size);
    (void)ge::AttrUtils::SetInt(tensor_desc_ptr, "_complex_cmo_id", complex_cmo_id);
    data_type = tensor_desc_ptr->GetDataType();
    source_addr = reinterpret_cast<uint64_t>(task_args.output_addrs.at(cmo_attr.object_index));
  } else {
    std::vector<int64_t> workspace_bytes = cmo_attr.node->GetOpDesc()->GetWorkspaceBytes();
    if (cmo_attr.object_index >= static_cast<int32_t>(workspace_bytes.size()) ||
        workspace_bytes[cmo_attr.object_index] > UINT32_MAX) {
      CM_LOGW("Node[%s] workspace object_index[%d] is out of range[%zu]",
              cmo_attr.node->GetName().c_str(), cmo_attr.object_index, workspace_bytes.size());
      return FAILED;
    }
    length_inner = static_cast<uint32_t>(workspace_bytes[cmo_attr.object_index]);
    data_type = ge::DT_INT8;
    (void)ge::AttrUtils::SetInt(cmo_attr.node->GetOpDesc(),
                                "_worksapce_" + std::to_string(cmo_attr.object_index) + "_complex_cmo_id",
                                complex_cmo_id);
    source_addr = reinterpret_cast<uint64_t>(task_args.workspace_addrs.at(cmo_attr.object_index));
  }
  return SUCCESS;
}
}
