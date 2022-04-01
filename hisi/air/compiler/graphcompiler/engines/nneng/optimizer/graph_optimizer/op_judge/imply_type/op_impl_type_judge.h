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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_IMPLY_TYPE_OP_IMPL_TYPE_JUDGE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_IMPLY_TYPE_OP_IMPL_TYPE_JUDGE_H_

#include "graph_optimizer/op_judge/op_judge_base.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

namespace fe {
extern const std::string LX_CORE_TYPE_WITH_DYNAMIC_UPPER;
extern const std::string LX_CORE_TYPE_WITH_DYNAMIC_LOWER;
using FEOpsKernelInfoStorePtr = std::shared_ptr<FEOpsKernelInfoStore>;
class OpImplTypeJudge : public OpJudgeBase {
 public:
  OpImplTypeJudge(const std::string& engine_name, FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr);
  ~OpImplTypeJudge() override;

  Status GetLXCoreType(ge::NodePtr &node_ptr);

  Status Judge(ge::ComputeGraph& graph) override;
  /**
   * judge imply type for the node
   * @param node_ptr current node
   * @return SUCCESS or FAIL
   */
  Status JudgeByNode(ge::NodePtr node_ptr);

  /**
   * judge the imply type for the node in sub graph
   * delete it after ischecksupported ok
   * supported ok
   * @param graph sub graph
   * @return SUCCESS or FAIL
   */
  Status JudgeInSubGraph(ge::ComputeGraph& graph);

  /**
   * judge the imply type for the node in sub graph, delete it after check
   * supported ok
   * @param node_ptr current node
   * @return SUCCESS or FAIL
   */
  Status JudgeInSubGraphByNode(ge::NodePtr node_ptr);

 private:
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  /**
   * set the op imply type
   * @param op_desc_ptr current op desc
   * @param impl_type the imply type of the op
   * @return SUCCESS or FAIL
   */
  Status SetOpImplType(const ge::NodePtr &node, OpImplType& imply_type);

  /**
   * set the op core type
   * @param op_desc_ptr current op desc
   * @return SUCCESS or FAIL
   */
  Status SetEngineType(ge::OpDescPtr op_desc_ptr);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_IMPLY_TYPE_OP_IMPL_TYPE_JUDGE_H_
