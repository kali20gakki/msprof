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

#include "generate_cmo_writeback_task.h"
#include "common/comm_log.h"

namespace fe {
GenerateCMOWritebackTask::GenerateCMOWritebackTask(const ge::Node &node, TaskBuilderContext &context)
    : GenerateCMOTaskBase(node, context) {}
GenerateCMOWritebackTask::~GenerateCMOWritebackTask() {}


Status GenerateCMOWritebackTask::GenerateTask(std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                                              const std::vector<CmoAttr> &cmo_attrs) {
  for (auto &cmo_attr : cmo_attrs) {
    TaskArgs task_args;
    if (InitCmoNodeAddrs(cmo_attr.node, task_args) != SUCCESS) {
      CM_LOGW("[GenTask][GenWriteBackTask] Init node[%s] addrs failed.", cmo_attr.node->GetName().c_str());
      return FAILED;
    }
    domi::TaskDef task_def;
    task_def.set_stream_id(stream_id);
    task_def.set_type(RT_MODEL_TASK_CMO);
    domi::CmoTaskDef *cmo_task_def = task_def.mutable_cmo_task();
    cmo_task_def->set_cmo_type(static_cast<uint32_t>(rtCMOType::rtCMOWriteBack));
    if (cmo_task_def == nullptr) {
      CM_LOGW("Create cmo task def for node[%s] failed.", node_.GetName().c_str());
      return FAILED;
    }
    // gen cmo id
    uint32_t cmo_id = static_cast<uint32_t>(CMOIdGenStrategy::Instance().GenerateCMOId(node_));
    if (cmo_id == 0) {
      CM_LOGW("Generate cmo id for mode[%s] failed, not launch cmo task.", node_.GetName().c_str());
      return FAILED;
    }
    CM_LOGD("Generate writeback cmo task id[%d] for node[%s] success.", cmo_id, node_.GetName().c_str());
    cmo_task_def->set_logic_id(cmo_id);
    ge::DataType data_type = ge::DT_UNDEFINED;
    uint32_t length_inner = 0;
    uint64_t source_addr;
    Status status = ParseTensorInfo(cmo_attr, task_args, data_type, source_addr, length_inner);
    if (status != SUCCESS) {
      CM_LOGW("Generate cmo task def for node[%s] failed.", node_.GetName().c_str());
      return FAILED;
    }
    // low 4bit: cmo type; high 4bit: data type
    // writeback: 0x8
    uint8_t op_code = 0x8;
    if (DATA_TYPE_CODE.count(data_type) == 0) {
      op_code += 0xf0;
    } else {
      op_code += (DATA_TYPE_CODE.at(data_type) << 4);
    }
    cmo_task_def->set_op_code(static_cast<uint32_t>(op_code));
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
}
