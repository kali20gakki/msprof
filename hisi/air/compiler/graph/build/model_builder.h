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

#ifndef GE_GRAPH_BUILD_MODEL_BUILDER_H_
#define GE_GRAPH_BUILD_MODEL_BUILDER_H_

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "framework/common/op/ge_op_utils.h"
#include "common/tbe_kernel_store.h"
#include "common/cust_aicpu_kernel_store.h"
#include "framework/common/types.h"
#include "framework/common/util.h"
#include "graph/compute_graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/model.h"
#include "graph/node.h"
#include "common/model/ge_model.h"
#include "framework/omg/omg_inner_types.h"

namespace ge {
class ModelBuilder {
 public:
  ModelBuilder(uint64_t session_id, ge::ComputeGraphPtr compute_graph, const Graph2SubGraphInfoList &subgraphs,
               const std::map<std::string, int32_t> &stream_max_parallel_num, bool hcom_parallel,
               int32_t mode = static_cast<int32_t>(domi::BuildMode::GEN_TASK_WITHOUT_FUSION));

  ModelBuilder(const ModelBuilder &) = delete;

  ModelBuilder &operator=(const ModelBuilder &op) = delete;

  ~ModelBuilder();

  Status SaveDataToModel(ge::Model &model, ge::GeModel &ge_model);
  Status PreBuildModel();
  Status BuildModelForGetTask(ge::Model &model);
  ge::Status BuildModelForGetDynShapeTask(ge::Model &model_def);

  ge::Buffer GetWeightBuffer() const;

  Status MergeWeights();

 protected:
  void AddNodeInputProperty() const;

  void ClearOriginalFormat() const;

 private:
  bool SetInputConst(const OpDescPtr &op_desc, const NodePtr &src_node,
                     size_t index, std::vector<bool> &is_input_const) const;

  void SetInputIsConst(const ge::NodePtr &n) const;

  void SetModelVersion(ge::Model &model) const;

  Status CalcOutputSize(const ge::NodePtr &n) const;

  Status AdjustConstWeightSize(const ge::NodePtr &node, size_t &mem_offset) const;

  Status SetInputOutputDesc();

  Status AdjustInputTensorFlag() const;

  Status BuildModelDef(ge::Model &model);

  void InitL1FusionOption();

  Status CompileSingleOp() const;

  void CollectCheckAicpuAttr(const OpDescPtr &op_desc, std::set<std::string> &aicpu_op_types,
                               std::set<std::string> &aicpu_tf_op_types) const;

  void SetModelCheckAicpuAttr(ge::Model &model, std::set<std::string> &aicpu_op_types,
                                std::set<std::string> &aicpu_tf_op_types) const;

  Status SaveAtomicTBEKernel(const OpDescPtr &op_desc);

  Status SavaAtomicWorkspace(const OpDescPtr &op_desc) const;
  Status SaveNormalTBEKernel(const OpDescPtr &op_desc);
  Status SaveCustAiCpuKernel(const OpDescPtr &op_desc, std::set<std::string> &aicpu_name_set);
  Status SaveMixAicAivTBEKernel(const OpDescPtr &op_desc);
  TBEKernelPtr CreateOpTBEKernel(const OpDescPtr &op_desc, const std::string &prefix_kernel_name) const;
  // In order to optimize the size of om,
  // delete the attributes saved in tbekernelstore and nodes on the graph at the same time.
  void DelNodeRepeatSaveAttr();
  uint64_t session_id_;

  std::map<uint64_t, size_t> mem_type_to_mem_offset_;

  size_t weight_offset_;

  ge::ComputeGraphPtr compute_graph_;

  const Graph2SubGraphInfoList &subgraphs_;

  int64_t stream_num_;
  int64_t event_num_;
  std::vector<int64_t> huge_streams_;

  uint32_t label_num_;

  ge::Buffer weight_buffer_;

  std::map<std::string, int32_t> stream_max_parallel_num_;
  bool hcom_parallel_;

  int32_t build_mode_;
  size_t max_mem_offset_;
  size_t host_max_mem_offset_;
  size_t p2p_mem_offset_;
  size_t zero_copy_mem_size_;

  TBEKernelStore tbe_kernel_store_;
  CustAICPUKernelStore cust_aicpu_kernel_store_;

  uint8_t platform_type_;
  bool is_loop_graph_;
  bool is_l1_fusion_enable_;
};
}  // namespace ge
#endif  // GE_GRAPH_BUILD_MODEL_BUILDER_H_
