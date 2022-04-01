/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FFTSPLUS_GRAPH_OPTIMIZER_H_
#define FFTS_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FFTSPLUS_GRAPH_OPTIMIZER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "inc/ffts_utils.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_type.h"
#include "inc/ffts_error_codes.h"
#include "graph_tuner/graph_tuner_errorcode.h"
#include "graph/compute_graph.h"
#include "common/optimizer/graph_optimizer.h"
#include "common/optimizer/graph_optimizer_types.h"
#include "inc/common/util/error_manager/error_manager.h"
#include "common/plugin_manager.h"
#include "nneng/inc/common/graph_comm.h"


namespace ffts {

using FFTSOptimizerFunc = std::function<tune::Status(ge::ComputeGraph &, bool)>;
using SubGraphNodeMap = std::map<uint32_t, std::vector<ge::NodePtr>>;
using PluginManagerPtr = std::shared_ptr<PluginManager>;

class FFTSPlusGraphOptimizer : public ge::GraphOptimizer {
 public:
  FFTSPlusGraphOptimizer();
  ~FFTSPlusGraphOptimizer() override;

  /**
   * @ingroup ffts
   * @brief prohibit copy and assign construct
   */
  FFTSPlusGraphOptimizer(const FFTSPlusGraphOptimizer&) = delete;
  FFTSPlusGraphOptimizer& operator=(const FFTSPlusGraphOptimizer&) = delete;

  /*
   *  @ingroup ffts
   *  @brief   initialize graph optimizer
   *  @param   [in] options
   *  @return  SUCCESS or FAILED
   */
  Status Initialize(const std::map<std::string, std::string>& options,
                    ge::OptimizeUtility *const optimize_utility) override;

  /*
   *  @ingroup ffts
   *  @brief   close graph optimizer
   *  @return  SUCCESS or FAILED
   */
  Status Finalize() override;

  /*
   *  @ingroup ffts
   *  @brief   optimize original graph
   *  @param   [in|out] graph  compute graph
   *  @return  SUCCESS or FAILED
   */
  Status OptimizeOriginalGraph(ge::ComputeGraph& graph) override;

  /*
   *  @ingroup ffts
   *  @brief   optimize fused graph
   *  @param   [in|out] graph   compute graph
   *  @return  SUCCESS or FAILED
   */
  Status OptimizeFusedGraph(ge::ComputeGraph& graph) override;

  /*
   *  @ingroup ffts
   *  @brief   get attribute of graph optimizer
   *  @param   [in|out] attrs
   *  @return  SUCCESS or FAILED
   */
  Status GetAttributes(ge::GraphOptimizerAttribute& attrs) const override;

  Status OptimizeWholeGraph(ge::ComputeGraph& graph) override;

    /*
   *  @ingroup ffts
   *  @brief   optimize complete graph
   *  @param   [in|out] graph   compute graph
   *  @return  SUCCESS or FAILED
   */
  Status OptimizeGraphBeforeBuild(ge::ComputeGraph& graph) override;

 private:
  Status TransSubGraphToFunctionOp(ge::ComputeGraph& graph);
  Status GetSubGraphNodes(const ge::ComputeGraph &graph, SubGraphNodeMap &node_map) const;
  ge::OpDescPtr CreateFunctionOp(vector<ge::NodePtr> &node_vec) const ;
  Status AddFunctionNodeInputDesc(ge::OpDescPtr fus_op, vector<fe::FusionDataFlow> &fus_input_edge_list);
  Status AddFunctionNodeOutputDesc(ge::OpDescPtr fus_op, vector<fe::FusionDataFlow> &fus_output_edge_list);
  Status CreateFunctionOpSubGraph(ge::NodePtr &function_node,
                                  std::vector<ge::NodePtr> &node_vec,
                                  vector<fe::FusionDataFlow> &input_edge_list,
                                  vector<fe::FusionDataFlow> &output_edge_list);
  Status TransSingleSubGraph(ge::ComputeGraph &graph, std::vector<ge::NodePtr> &node_vec);
  Status PostSubGraph(ge::ComputeGraph& graph);
  Status CacheOptionJudge(ge::ComputeGraph& graph) const;
  void GetSliceInfo(ge::ComputeGraph &graph) const;
  void JudgeThreadTensorAlignedAndAlarm(ge::NodePtr &node, vector<vector<vector<DimRange>>> &tensor_slice,
                                        const bool &is_input) const;
  ThreadSliceMapPtr GetSliceInfoFromJson(const ge::OpDescPtr &op_desc_ptr) const;
  Status InitLibPath();

 private:
  PluginManagerPtr lx_fusion_plugin_ffts_plus_;
  FFTSOptimizerFunc FFTSOptimizer_{nullptr};
  ge::GraphOptimizerAttribute graph_optimizer_attr_;
  std::shared_ptr<fe::GraphComm> graph_comm_ptr_;
  size_t sgt_graph_index_{0};
  std::string lib_path_;
  bool init_flag_;
};
}  // namespace ffts
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FFTS_GRAPH_OPTIMIZER_H_
