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
#ifndef INC_1CB1E40F5C1F4C78AC6CB75D6A8B4179
#define INC_1CB1E40F5C1F4C78AC6CB75D6A8B4179

#include "stdint.h"
#include "ge_running_env/fake_ns.h"
#include <string>
#include "base/metadef/inc/external/graph/graph.h"

FAKE_NS_BEGIN

struct ModelPartitionTable;
struct ModelFileHeader;
struct ModelData;
struct ModelPartitionMemInfo;
struct Graph;

struct ModelDataBuilder {
  ModelDataBuilder(ModelData &);
  ModelDataBuilder& AddGraph(const Graph &graph);
  ModelDataBuilder& AddTask();
  ModelDataBuilder& AddTask(int kernel_type, int op_index, size_t arg_size = 64U);
  ModelDataBuilder& AddTask(int kernel_type, const char *node_name);
  ModelDataBuilder& AddAicpuTask(int op_index);
  void Build();

 private:
  void Init();

 private:
  ModelData &model;
  uint8_t *model_data;
  ModelFileHeader *model_header;
  ModelPartitionTable *partition_table;
  ModelPartitionMemInfo &graph_info_;
  ModelPartitionMemInfo &task_info_;
  uint32_t op_index_;
  uint32_t mem_offset;
  ge::Graph graph_;
};

FAKE_NS_END
#endif
