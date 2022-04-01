/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include <cstring>
#include "utils/model_data_builder.h"
#include "runtime/rt.h"
#include "graph/model.h"
#include "external/graph/graph.h"
#include "framework/executor/ge_executor.h"
#include "proto/task.pb.h"
#include "hybrid/node_executor/aicpu/aicpu_node_executor.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "base/metadef/inc/graph/utils/graph_utils.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"

using namespace ge;
using namespace std;

namespace {
struct AicpuTaskStruct {
  aicpu::AicpuParamHead head;
  uint64_t io_addrp[6];
}__attribute__((packed));
}  // namespace

ModelDataBuilder::ModelDataBuilder(ModelData &model)
    : model(model),
      model_data((uint8_t *)model.model_data),
      model_header(new (model_data) ModelFileHeader),
      partition_table(reinterpret_cast<ModelPartitionTable *>(model_data + sizeof(ModelFileHeader))),
      graph_info_(partition_table->partition[0]),
      task_info_(partition_table->partition[2]),
      op_index_(0) {
  Init();
}

void ModelDataBuilder::Init(){
    mem_offset = sizeof(ModelFileHeader) + sizeof(ModelPartitionTable) + sizeof(ModelPartitionMemInfo) * PARTITION_SIZE;
    memset(partition_table->partition, 0x00, sizeof(ModelPartitionMemInfo) * PARTITION_SIZE);
    partition_table->num = PARTITION_SIZE;
    partition_table->partition[0].type = ModelPartitionType::MODEL_DEF;
    partition_table->partition[1].type = ModelPartitionType::WEIGHTS_DATA;
    partition_table->partition[2].type = ModelPartitionType::TASK_INFO;
    partition_table->partition[3].type = ModelPartitionType::TBE_KERNELS;
    partition_table->partition[4].type = ModelPartitionType::CUST_AICPU_KERNELS;
}

ModelDataBuilder &ModelDataBuilder::AddGraph(const Graph &graph) {
  graph_ = graph;
  Model model;
  model.SetGraph(graph);
  model.SetVersion(123);
  Buffer buffer;
  model.Save(buffer);
  memcpy(model_data + mem_offset, buffer.GetData(), buffer.GetSize());
  graph_info_.mem_size = buffer.GetSize();
  graph_info_.mem_offset = mem_offset;
  mem_offset += buffer.GetSize();
  return *this;
}

ModelDataBuilder &ModelDataBuilder::AddTask() {
  auto model_task_def = std::make_shared<domi::ModelTaskDef>();
  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  kernel_def->set_block_dim(32);
  kernel_def->set_args_size(64);
  string args(64, '1');
  kernel_def->set_args(args.data(), 64);
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(2);    // ccKernelType::TE
  context->set_op_index(op_index_++);
  uint16_t args_offset[9] = {0};
  context->set_args_offset(args_offset, 9 * sizeof(uint16_t));

  auto task_mem_size = static_cast<int>(model_task_def->ByteSizeLong());
  task_info_.mem_offset = (task_info_.mem_offset == 0) ? mem_offset : task_info_.mem_offset;
  task_info_.mem_size += task_mem_size;
  model_task_def->SerializePartialToArray(model_data + mem_offset, task_mem_size);
  mem_offset += task_mem_size;
  return *this;
}

ModelDataBuilder &ModelDataBuilder::AddTask(int kernel_type, int op_index, size_t arg_size) {
  auto model_task_def = std::make_shared<domi::ModelTaskDef>();
  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  kernel_def->set_block_dim(32);
  kernel_def->set_args_size(arg_size);
  string args(arg_size, '1');
  kernel_def->set_args(args.data(), arg_size);
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(kernel_type);    // 2 means ccKernelType::TE
  context->set_op_index(op_index);
  uint16_t args_offset[9] = {0};
  context->set_args_offset(args_offset, 9 * sizeof(uint16_t));

  auto task_mem_size = static_cast<int>(model_task_def->ByteSizeLong());
  task_info_.mem_offset = (task_info_.mem_offset == 0) ? mem_offset : task_info_.mem_offset;
  task_info_.mem_size += task_mem_size;
  model_task_def->SerializePartialToArray(model_data + mem_offset, task_mem_size);
  mem_offset += task_mem_size;
  return *this;
}

ModelDataBuilder &ModelDataBuilder::AddAicpuTask(int op_index) {
  auto model_task_def = std::make_shared<domi::ModelTaskDef>();

  AicpuTaskStruct args;
  args.head.length = sizeof(args);
  args.head.ioAddrNum = 2;
  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  kernel_def->set_args(reinterpret_cast<const char *>(&args), args.head.length);
  kernel_def->set_args_size(args.head.length);
  ge::hybrid::AicpuExtInfo aicpu_ext_info;
  ge::hybrid::AicpuExtInfo aicpu_ext_info1;
  ge::hybrid::AicpuExtInfo aicpu_ext_info2;

  aicpu_ext_info.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_SHAPE_TYPE;
  aicpu_ext_info.infoLen = sizeof(int32_t);
  int32_t type = 0;

  aicpu_ext_info1.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_INPUT_SHAPE;
  aicpu_ext_info1.infoLen = sizeof(hybrid::AicpuShapeAndType);
  hybrid::AicpuShapeAndType types;
  types.type = 1;

  aicpu_ext_info2.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_OUTPUT_SHAPE;
  aicpu_ext_info2.infoLen = sizeof(hybrid::AicpuShapeAndType);
  hybrid::AicpuShapeAndType typess;
  typess.type = 2;

  char *ext_mem = (char*)malloc(3 * sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t) +
                                2 * sizeof(hybrid::AicpuShapeAndType));
  memcpy_s(ext_mem, sizeof(ge::hybrid::AicpuExtInfo), &aicpu_ext_info,
           sizeof(ge::hybrid::AicpuExtInfo));
  memcpy_s(ext_mem + sizeof(ge::hybrid::AicpuExtInfo), sizeof(int32_t), &type,
           sizeof(int32_t));

  memcpy_s(ext_mem + (sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t)),
           sizeof(ge::hybrid::AicpuExtInfo), &aicpu_ext_info1,
           sizeof(ge::hybrid::AicpuExtInfo));
  memcpy_s(ext_mem + 2 * sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t),
           sizeof(hybrid::AicpuShapeAndType), &types,
           sizeof(hybrid::AicpuShapeAndType));
  
  memcpy_s(ext_mem + (2 * sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t)) +
           sizeof(hybrid::AicpuShapeAndType), sizeof(ge::hybrid::AicpuExtInfo), &aicpu_ext_info2,
           sizeof(ge::hybrid::AicpuExtInfo));     
  memcpy_s(ext_mem + 3 * sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t) +
           sizeof(hybrid::AicpuShapeAndType), sizeof(hybrid::AicpuShapeAndType), &typess,
           sizeof(hybrid::AicpuShapeAndType));

  kernel_def->set_kernel_ext_info(ext_mem, 3 * sizeof(ge::hybrid::AicpuExtInfo) +
                                  sizeof(int32_t) + 2 * sizeof(hybrid::AicpuShapeAndType));
  kernel_def->set_kernel_ext_info_size(3 * sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t) +
                                       2 * sizeof(hybrid::AicpuShapeAndType));
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(6);    // ccKernelType::AI_CPU
  context->set_op_index(op_index);
  uint16_t args_offset[9] = {0};
  context->set_args_offset(args_offset, 9 * sizeof(uint16_t));

  auto task_mem_size = static_cast<int>(model_task_def->ByteSizeLong());
  task_info_.mem_offset = (task_info_.mem_offset == 0) ? mem_offset : task_info_.mem_offset;
  task_info_.mem_size += task_mem_size;
  model_task_def->SerializePartialToArray(model_data + mem_offset, task_mem_size);
  mem_offset += task_mem_size;
  return *this;
}

void ModelDataBuilder::Build(){
  model_header->length = mem_offset - sizeof(ModelFileHeader);
  model.model_len = mem_offset;
}
ModelDataBuilder &ModelDataBuilder::AddTask(int kernel_type, const char *node_name) {
  auto graph = GraphUtils::GetComputeGraph(graph_);
  auto node = graph->FindNode(node_name);
  if (node == nullptr) {
    std::cout << "ERROR: Can not find op by name " << node_name << " when AddTask" << std::endl;
    return *this;
  }
  return AddTask(kernel_type, node->GetOpDesc()->GetId());
}
