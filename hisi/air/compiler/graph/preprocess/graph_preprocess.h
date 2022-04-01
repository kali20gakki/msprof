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

#ifndef GE_GRAPH_PREPROCESS_GRAPH_PREPROCESS_H_
#define GE_GRAPH_PREPROCESS_GRAPH_PREPROCESS_H_
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "framework/common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/model_parser/model_parser.h"
#include "common/properties_manager.h"
#include "framework/common/string_util.h"
#include "framework/common/types.h"
#include "framework/common/util.h"
#include "graph/compute_graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/manager/util/graph_rebuild_state_ctrl.h"
#include "graph/model.h"
#include "graph/node.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "framework/omg/omg_inner_types.h"
#include "runtime/context.h"
#include "graph/resource_context_mgr.h"

namespace ge {
class GraphPrepare {
 public:
  GraphPrepare();
  virtual ~GraphPrepare();
  GraphPrepare(const GraphPrepare &in) = delete;
  GraphPrepare &operator=(const GraphPrepare &in) = delete;
  Status PrepareInitAndUpdateInput(const GraphNodePtr &graph_node,
                                   const std::vector<GeTensor> &user_input,
                                   uint64_t session_id = 0,
                                   GraphRebuildStateCtrl *graph_rebuild_state_ctrl = nullptr,
                                   ResourceContextMgr *resource_context_mgr = nullptr);
  Status PrepareDynShape();
  Status RecordAIPPInfo(const ge::ComputeGraphPtr &compute_graph) const;
  Status PrepareRunningFormatRefiner();
  void SetOptions(const GraphManagerOptions &options);
  Status GenerateInfershapeGraph(ConstGraphPtr graph);
  Status SwitchOpOptimize(ComputeGraphPtr &compute_graph) const;
  // some functions required session_id on compute_graph, which set when graph_prepare init.
  // so every func which invoke InferShapeForPreprocess need keep position after graph_prepare init.
  static Status InferShapeForPreprocess(ComputeGraphPtr &compute_graph, GraphRebuildStateCtrl *rebuild_ctrl,
                                        ResourceContextMgr *resource_mgr);

 private:
  Status CheckGraphAndUpdateOriginShape() const;
  Status Init(const ge::Graph &graph, uint64_t session_id = 0, GraphRebuildStateCtrl *graph_rebuild_state_ctrl = nullptr,
              ResourceContextMgr *resource_context_mgr = nullptr);
  Status CheckRefInputNode(const NodePtr &node, const std::string &input_name,
                           const std::set<NodePtr> &ref_nodes) const;
  Status CheckRefOp();
  Status AdjustDataOpOutput(const NodePtr &node) const;
  Status CheckInternalFormat(const NodePtr &input_node, const GeTensorDesc &desc) const;
  Status UpdateDataInputOutputDesc(int64_t index, const OpDescPtr &op, GeTensorDesc &desc) const;
  Status UpdateInput(const std::vector<GeTensor> &user_input, const std::map<std::string, std::string> &graph_option);
  Status CheckAndUpdateInput(const std::vector<GeTensor> &user_input, const std::map<std::string, std::string> &graph_option);
  Status CheckConstOp() const;
  Status VerifyConstOp(const NodePtr &node) const;
  Status CheckUserInput(const std::vector<GeTensor> &user_input);
  Status UpdateDataNetOutputByStorageFormat() const;
  Status PrepareOptimize();
  Status TryDoAipp();
  Status UpdateVariableFormats(const ComputeGraphPtr &graph) const;
  Status FormatAndShapeProcess();
  Status ResourcePairProcess(const std::string &action) const;
  Status SaveOriginalGraphToOmModel() const;
  Status ProcessNetOutput() const;
  Status ProcessBeforeInfershape();
  Status UpdateInputOutputByOptions();
  Status CtrlFlowPreProcess() const;

  bool IsTansDataOpData(const ge::NodePtr &var_node) const;

  Status GraphEquivalentTransformation();
  void TypeConversionOfConstant() const;
  bool IsDynamicDims(const NodePtr &input_node) const;
  Status UpdateUninitializedOriginShape(const NodePtr &input_node) const;

  ge::ComputeGraphPtr compute_graph_;
  GraphManagerOptions options_;
  uint64_t session_id_ = 0;
  GraphRebuildStateCtrl *graph_rebuild_state_ctrl_ = nullptr;
  ResourceContextMgr *resource_context_mgr_ = nullptr;
};
}  // namespace ge
#endif  // GE_GRAPH_PREPROCESS_GRAPH_PREPROCESS_H_
