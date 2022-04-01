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

#ifndef COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_FUZZY_GENERALIZE_H_
#define COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_FUZZY_GENERALIZE_H_

#include <vector>
#include <map>
#include <unordered_set>
#include <utility>

#include "common/optimizer/optimize_utility.h"
#include "common/unknown_shape_util.h"
#include "ops_store/op_kernel_info.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_kernel_store/sub_ops_store.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/fuzzy_compiler/input_node_generalize.h"

namespace fe {
const uint32_t kDowngradesTimeMax = 30;
const uint32_t MAX_DECENT_TIMES = 5;
const uint32_t MAX_UNSET_DYNAMIC_TIMES = 5;
const uint32_t TOTAL_DECENT_TIMES = 4;
const int64_t MAX_RANGE_UPPER = -1;
const std::string ORIGIN_SHAPE_RANGE = "origin_shape_range";
const std::string SHAPE_RANGE = "shape_range";
const std::string STR_RANGE_LIMIT = "limited";
const std::string STR_RANGE_UNLIMIT = "unlimited";
const std::string STR_RANGE_UNKNOWN = "dynamic";
const std::map<std::string, bool> RANGE_LIMIT_BOOL_MAP{{STR_RANGE_LIMIT, true}, {STR_RANGE_UNLIMIT, false}};

using FEOpsKernelInfoStorePtr = std::shared_ptr<fe::FEOpsKernelInfoStore>;
using OpStoreAdapterManagerPtr = std::shared_ptr<fe::OpStoreAdapterManager>;

Status GetRangeLimit(const OpStoreAdapterPtr &op_store_adapter, const NodeGeneralInfoPtr &node_info_ptr,
                     const ge::NodePtr &node_ptr);
class FuzzyGeneralize {
 public:
  FuzzyGeneralize(ge::OptimizeUtility *optimize_utility,
                  const FEOpsKernelInfoStorePtr &ops_kernel_info_store_ptr,
                  const OpStoreAdapterManagerPtr &op_store_adapter_manager_ptr);
  ~FuzzyGeneralize();

  FuzzyGeneralize(const FuzzyGeneralize &) = delete;
  FuzzyGeneralize &operator=(const FuzzyGeneralize &) = delete;

  Status GeneralizeGraph(ge::ComputeGraph &graph);

 private:
  Status FeedInputsRootSet(const ge::NodePtr &node_ptr, const NodeGeneralInfoPtr &node_info_ptr) const;

  void CheckIsSubGraphNode(const ge::NodePtr &node_ptr, const NodeGeneralInfoPtr &node_info_ptr) const;

  bool CheckIsExternalNode(const ge::NodePtr &node) const;

  bool CheckIsFirstNode(const ge::NodePtr &node) const;

  Status CheckAndUpdateLimitedNodes(const OpStoreAdapterPtr &op_store_adapter,
                                    const std::vector<ge::NodePtr> &limited_nodes, bool &generalize_flag);

  Status LimitNode(const ge::NodePtr &limited_node) const;

  Status InputNodeDowngrades(const ge::NodePtr &cur_node, const std::vector<size_t> &upper_limited_input_indexs,
                             const std::vector<size_t> &lower_limited_input_indexs, bool &is_range_out);

  Status UpdateDynamicShapeToNewInputNode(const std::unordered_set<ge::NodePtr> &external_input_nodes,
                                          const std::map<std::string, ge::NodePtr> &new_input_nodes) const;

  Status GetReshapeTypeByOpStore(const ge::NodePtr &node, const std::string &input_name,
                                 std::string &reshape_type) const;

  Status GetReshapeType(const ge::Format &origin_format, ge::GeShape &ori_shape, const ge::NodePtr &first_node,
                        const std::string &input_name, std::string &reshape_type) const;

  Status CorrectCAxisByOriginalFormat(const ge::Format &origin_format, const ge::NodePtr &input_node,
                                      const ge::NodePtr &first_node, const std::string &input_name) const;

  Status CorrectInputNodeCAxisByFirstNode(const ge::NodePtr &input_node) const;

  Status CAxisCorrection() const;

  Status UpdateDynamicShapeToFirstNode(const ge::NodePtr &ori_input_node) const;

  Status UpdateDynamicShapeToOriginalGraph(const ge::ComputeGraph &graph) const;

  Status UpdateDynamicShapeToNewBakGraph(const ge::ComputeGraph &graph) const;

  Status GraphDynamicShapeInfer(const OpStoreAdapterPtr &op_store_adapter, ge::ComputeGraphPtr &graph_bak,
                                const ge::ComputeGraphPtr &ori_graph);

  Status GraphPreprocessing(const ge::ComputeGraph &graph, const OpStoreAdapterPtr &op_store_adapter);

  Status RangeDecent(const ge::NodePtr &external_node, uint32_t &decent_times);

  Status GetCurNodeInfo(const ge::NodePtr &node, NodeGeneralInfoPtr &node_info_ptr);

  Status GetRangeLimitValue(const OpStoreAdapterPtr &op_store_adapter,
                            const NodeGeneralInfoPtr &node_info_ptr, const ge::NodePtr &node);

  Status CalDecentSteps(const ge::NodePtr &external_node, const ge::OpDescPtr &opdesc);

  Status SingleOpDowngrades(const ge::NodePtr &external_node, const bool &is_upper_limited, bool &is_range_out);

  Status Downgrades(const ge::NodePtr &cur_node, const bool &is_upper_limited,
      const std::vector<size_t> &limited_input_indexs, bool &is_range_out);

  Status InitOriginalGraphInfos(const ge::ComputeGraph &graph);

  ge::OptimizeUtility *optimize_utility_;
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  std::map<ge::NodePtr, NodeGeneralInfoPtr> node_info_map_;
  std::unordered_set<ge::NodePtr> external_input_nodes_;
  std::map<std::string, ge::NodePtr> original_input_nodes_;
  std::vector<ge::NodePtr> limited_range_nodes_;
  std::map<std::string, uint32_t> decent_times_count_;
  std::map<std::string, std::vector<double>> decent_steps_;
  bool is_range_limited_graph_ = false;
  bool is_need_generalize_graph_ = false;
};
} // namespace fe
#endif // COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_FUZZY_GENERALIZE_H_
