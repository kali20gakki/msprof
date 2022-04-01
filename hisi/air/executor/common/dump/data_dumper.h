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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_DUMPER_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_DUMPER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "framework/common/ge_inner_error_codes.h"
#include "common/properties_manager.h"
#include "graph/node.h"
#include "graph/compute_graph.h"
#include "proto/ge_ir.pb.h"
#include "proto/op_mapping.pb.h"
#include "runtime/mem.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "framework/common/ge_types.h"
#include "runtime/base.h"

namespace ge {
class DataDumper {
 public:
  explicit DataDumper(RuntimeParam *const rsh) : runtime_param_(rsh) {}

  ~DataDumper();

  void SetModelName(const std::string &model_name) { model_name_ = model_name; }

  void SetModelId(const uint32_t model_id) { model_id_ = model_id; }

  void SetDeviceId(const uint32_t device_id) { device_id_ = device_id; }

  void SetComputeGraph(const ComputeGraphPtr &compute_graph) { compute_graph_ = compute_graph; };

  void SetRefInfo(const std::map<OpDescPtr, void *> &ref_info) { ref_info_ = ref_info; };

  void SetL1FusionAddr(const uintptr_t addr) { l1_fusion_addr_ = addr; };

  void SetLoopAddr(const uintptr_t global_step, const uintptr_t loop_per_iter, const uintptr_t loop_cond);

  void SaveDumpInput(const std::shared_ptr<Node> &node);

  // args is device memory stored first output addr
  void SaveDumpTask(const uint32_t task_id, const uint32_t stream_id, const std::shared_ptr<OpDesc> &op_desc,
                    const uintptr_t args);
  void SaveEndGraphId(const uint32_t task_id, const uint32_t stream_id);

  void SetOmName(const std::string &om_name) { om_name_ = om_name; }
  void SaveOpDebugId(const uint32_t task_id, const uint32_t stream_id, const void *const op_debug_addr,
                     const bool is_op_debug);

  Status LoadDumpInfo();

  void UnloadDumpInfo();

  void DumpShrink();

  void SetDumpProperties(const DumpProperties &dump_properties) { dump_properties_ = dump_properties; }
  const DumpProperties &GetDumpProperties() const { return dump_properties_; }

 private:
  void PrintCheckLog(std::string &dump_list_key);

  std::string model_name_;

  // for inference data dump
  std::string om_name_;

  uint32_t model_id_{0U};
  RuntimeParam *runtime_param_{nullptr};
  void *dev_mem_load_{nullptr};
  void *dev_mem_unload_{nullptr};

  struct InnerDumpInfo;
  struct InnerInputMapping;

  std::vector<OpDescInfo> op_desc_info_;
  std::vector<InnerDumpInfo> op_list_;  // release after DavinciModel::Init
  uint32_t end_graph_task_id_{0U};
  uint32_t end_graph_stream_id_{0U};
  bool is_end_graph_ = false;
  std::multimap<std::string, InnerInputMapping> input_map_;  // release after DavinciModel::Init
  bool load_flag_{false};
  uint32_t device_id_{0U};
  uintptr_t global_step_{0U};
  uintptr_t loop_per_iter_{0U};
  uintptr_t loop_cond_{0U};
  ComputeGraphPtr compute_graph_;  // release after DavinciModel::Init
  std::map<OpDescPtr, void *> ref_info_;     // release after DavinciModel::Init
  uintptr_t l1_fusion_addr_{0U};

  uint32_t op_debug_task_id_{0U};
  uint32_t op_debug_stream_id_{0U};
  const void *op_debug_addr_{nullptr};
  bool is_op_debug_ = false;
  bool need_generate_op_buffer_ = false;
  DumpProperties dump_properties_;

  // Build task info of op mapping info
  Status BuildTaskInfo(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info);
  Status DumpOutput(const InnerDumpInfo &inner_dump_info, toolkit::aicpu::dump::Task &task);
  Status DumpRefOutput(const DataDumper::InnerDumpInfo &inner_dump_info, toolkit::aicpu::dump::Output &output,
                       const size_t i, const std::string &node_name_index);
  Status DumpOutputWithTask(const InnerDumpInfo &inner_dump_info, toolkit::aicpu::dump::Task &task);
  Status DumpInput(const InnerDumpInfo &inner_dump_info, toolkit::aicpu::dump::Task &task);
  Status DumpRefInput(const DataDumper::InnerDumpInfo &inner_dump_info, toolkit::aicpu::dump::Input &input,
                      const size_t i, const std::string &node_name_index);
  Status ExecuteLoadDumpInfo(const toolkit::aicpu::dump::OpMappingInfo &op_mapping_info);
  void SetEndGraphIdToAicpu(toolkit::aicpu::dump::OpMappingInfo &op_mapping_info);
  void SetOpDebugIdToAicpu(const uint32_t task_id, const uint32_t stream_id, const void *const op_debug_addr,
                           toolkit::aicpu::dump::OpMappingInfo &op_mapping_info) const;
  Status ExecuteUnLoadDumpInfo(const toolkit::aicpu::dump::OpMappingInfo &op_mapping_info);
  static Status GenerateInput(toolkit::aicpu::dump::Input &input, const OpDesc::Vistor<GeTensorDesc> &tensor_descs,
                              const uintptr_t &addr, const size_t index);
  static Status GenerateOutput(toolkit::aicpu::dump::Output &output, const OpDesc::Vistor<GeTensorDesc> &tensor_descs,
                               const uintptr_t &addr, const size_t index);
  void GenerateOpBuffer(const uint64_t size, toolkit::aicpu::dump::Task &task);
};
struct DataDumper::InnerDumpInfo {
  uint32_t task_id;
  uint32_t stream_id;
  std::shared_ptr<OpDesc> op;
  uintptr_t args;
  bool is_task;
  int32_t input_anchor_index;
  int32_t output_anchor_index;
  std::vector<int64_t> dims;
  int64_t data_size;
};

struct DataDumper::InnerInputMapping {
  std::shared_ptr<OpDesc> data_op;
  int32_t input_anchor_index;
  int32_t output_anchor_index;
};
}  // namespace ge

#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DATA_DUMPER_H_
