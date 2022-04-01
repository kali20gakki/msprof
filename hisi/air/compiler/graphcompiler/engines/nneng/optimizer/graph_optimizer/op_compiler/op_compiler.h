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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_H_

#include <map>
#include <functional>
#include <memory>
#include <string>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/graph_comm.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/op_compiler/tbe_json_parse.h"
#include "graph_optimizer/ffts/tbe_sgt_json_parse.h"
#include "graph_tuner/graph_tuner_errorcode.h"

namespace fe {
using ScopeJsonMap = std::map<int64_t, std::string>;
using TbeJsonFileParsePtr = std::shared_ptr<TbeJsonFileParse>;
using TbeSgtJsonFileParsePtr = std::shared_ptr<TbeSgtJsonFileParse>;
using BufferFusionFunc = std::function<Status(ge::ComputeGraph&, std::shared_ptr<GraphComm>, LxFusionOptimizeResult&)>;

class OpCompiler {
 public:
  using ConstComputeGraph = const ge::ComputeGraph;
  template <class T>
  using Vistor = RangeVistor<T, std::shared_ptr<ConstComputeGraph>>;
  
  OpCompiler(const std::string& compiler_name, const std::string& engine_name,
             OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  virtual ~OpCompiler();

  /**
   * @ingroup fe
   * @brief prohibit copy and assign construct
   */
  OpCompiler(const OpCompiler&) = delete;
  OpCompiler& operator=(const OpCompiler&) = delete;

  /*
   *  @ingroup fe
   *  @brief   initialize op compiler
   *  @return  SUCCESS or FAILED
   */
  Status Initialize();

  /*
   *  @ingroup fe
   *  @brief   finalize op compiler
   *  @return  SUCCESS or FAILED
   */
  Status Finalize();

  /*
   *  @ingroup fe
   *  @brief   precompile tbe op
   *  @param   [in|out] graph  compute graph
   *  @return  SUCCESS or FAILED
   */
  Status PreCompileOp(ge::ComputeGraph& graph);

/*
 *  @ingroup fe
 *  @brief   precompile tbe thread op
 *  @param   [in|out] graph  compute graph
 *  @return  SUCCESS or FAILED
 */
  Status PreCompileThreadOpHelper(const ge::NodePtr& node, const ge::OpDescPtr& op_desc_ptr,
                                  const ge::OpDescPtr &old_op_desc, const string& session_graph_id,
                                  const ge::ComputeGraph& graph);

  Status PreCompileThreadOp(ge::ComputeGraph& graph, bool& sgt_flag);

  /* For op-tune scenario, the first compiling we need to ignore the compile strategy.
   * Because we need the compile result and roll back those which is failed in compiling as
   * fusion nodes to single nodes. And the second compiling is standard without any special
   * operations.
   * So we add input parameter ignore_compile_strategy to differentiate those two kinds of op-tune compile. */
  Status CompileOpOnly(const ge::ComputeGraph& graph, CompileInfoParam &compile_info) const;
  /*
   *  @ingroup fe
   *  @brief   compile tbe op
   *  @param   [in|out] graph  compute graph
   *  @return  SUCCESS or FAILED
   */
  Status CompileOp(const ge::ComputeGraph& graph, std::vector<ge::NodePtr>& buff_fus_compile_failed_nodes,
                   const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                   const std::vector<ge::NodePtr>& buff_fus_to_del_nodes);

  virtual Status GetFusionScope(ge::ComputeGraph& graph, ScopeNodeIdMap &fusion_nodes_map,
                                std::vector<ge::NodePtr> &nodes_be_compiled,
                                std::vector<ge::NodePtr> &all_nodes_after_lx_fusion);

  Status ReCompileOpAfterLxFusion(ge::ComputeGraph& graph, CompileInfoParam &compile_info,
                                  const LxFusionOptimizeResult &opt_ret);

  void GetNodesNeedRePrcmpl(const Vistor<ge::NodePtr> &all_nodes,
                            std::unordered_set<int64_t> &need_re_compile_scope_id,
                            std::vector<ge::NodePtr> &nodes_be_compiled,
                            std::vector<ge::NodePtr>& all_nodes_after_lx_fusion);

  virtual Status RunCompileProcess(ge::ComputeGraph& graph,
                                   const std::shared_ptr<GraphComm>& graph_comm_ptr,
                                   std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                   bool &need_post_process) {return SUCCESS;};

  virtual Status GetFusionScope(const ge::ComputeGraph& graph, const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                                ScopeNodeIdMap &fusion_nodes_map, std::vector<ge::NodePtr> &nodes_be_compiled);

  string GetCompilerName();
 protected:
  /*
   *  @ingroup fe
   *  @brief   add node to fusion_node_map according to scope id
   *  @param   [in]  node           node pointer
   *  @param   [in] scope_id        scope id
   *  @param   [out] fusion_node_map  scope id and node map
   *  @return  SUCCESS or FAILED
   */
  Status AddNodeToFusionMap(ge::Node& node, const int64_t scope_id, ScopeNodeIdMap& fusion_node_map);

  /*
   *  @ingroup fe
   *  @brief   get scope node map
   *  @param   [in]  graph        node pointer
   *  @param   [out] fusion_map    scope id and node map
   *  @return  SUCCESS or FAILED
   */
  Status GetScopeNodeMap(const ge::ComputeGraph& graph, ScopeNodeIdMap& fusion_node_map);

  bool IsTbe(const OpImplType& impl_type) const;

  /* We add the parameter nodemal_node_id is because, in the second round compilation of
   * lx-fusion, the scope id will start from the minimum negative one. */
  Status GetScopeNodeMap(const ge::ComputeGraph& graph, const std::vector<ge::NodePtr>& scope_nodes,
                         ScopeNodeIdMap& fusion_node_map);

  /*
   *  insert Compress op between conv and weight
   *  @ingroup fe
   *  @brief   insert Compress op between conv and weight
   *  @param   [in]  node        node pointer
   *  @return  SUCCESS or FAILED
   */
  Status UpdateCompressOp(ge::NodePtr node) const;

  /**
   * Check whether do compressing weight on conv or FC
   * @param node node pointer
   * @return SUCCESS or FAILED
   */
  Status SetCompressWeightAttr(ge::NodePtr node);

  Status ParseJsonAndUpdateOp(const ge::NodePtr &node, const ge::OpDescPtr op_desc_ptr,
                              const ScopeJsonMap::const_iterator& json_iter);
  /*
   *  @ingroup fe
   *  @brief   after compile tbe op, parse json file and update compress op
   *  @param   [in] graph                 compute graph
   *  @param   [in] scope_json_map          scope_id and json file
   *  @param   [in] buff_fus_rollback_nodes  nodes need to roll back
   *  @param   [in] node_size              node_size in graph
   *  @return  SUCCESS or FAILED
   */
  Status ParseJsonAndCompressOp(const ge::ComputeGraph& graph, map<int64_t, std::string>& scope_json_map,
                                const std::vector<ge::NodePtr>& nodes_be_compiled);

  Status VerifyScopeIdAttr(const int64_t &scope_id, const bool &is_l1_fusion,
                           std::map<int64_t, bool> &fusion_scope_type_map);

  Status AddNormalTbeNodeIntoMap(ge::NodePtr node, ge::OpDescPtr op_desc_ptr,
                                 ScopeNodeIdMap& fusion_node_map,
                                 std::map<int64_t, bool> &fusion_scope_type_map);

  Status SetPreCompParameter(const ge::NodePtr &node, const FEOpsStoreInfo &op_store_info,
                             const string &session_graph_id, OpImplType imply_type,
                             std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> &pre_comp_map);

  bool StopCompileOpInTuningAndAfterBuilderMode();

  Status SetMemoryTypeForOutput(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr) const;

  Status ParseTvmJsonToSetAttr(const ge::NodePtr& node, const ge::OpDescPtr op_desc_ptr,
                               const std::string &json_file_path) const;

  Status ParseSgtTvmJsonToSetAttr(const ge::NodePtr& node, const ge::OpDescPtr op_desc_ptr,
                                  const std::string &json_file_path) const;

  Status CheckCompiledFusionGraph(const ge::ComputeGraph& graph) const;

  Status GetMixComFusionScope(const ge::ComputeGraph& graph, const std::vector<ge::NodePtr>& buff_fus_rollback_nodes,
                              ScopeNodeIdMap &fusion_nodes_map, std::vector<ge::NodePtr> &nodes_be_compiled);

  bool init_flag_;
  std::string engine_name_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  string compiler_name_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_H_