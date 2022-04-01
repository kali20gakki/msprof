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

#ifndef FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_OP_STORE_ADAPTER_H_
#define FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_OP_STORE_ADAPTER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "adapter/tbe_adapter/tbe_info/tbe_info_assembler.h"
#include "adapter/tbe_adapter/tbe_info/tbe_single_op_info_assembler.h"
#include "common/plugin_manager.h"
#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "tensor_engine/fusion_api.h"

namespace fe {
class TbeOpStoreAdapter;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using PluginManagerPtr = std::shared_ptr<fe::PluginManager>;
using TbeInfoAssemblerPtr = std::shared_ptr<fe::TbeInfoAssembler>;
using TbeSingleOpInfoAssemblerPtr = std::shared_ptr<fe::TbeSingleOpInfoAssembler>;

class TbeOpStoreAdapter : public OpStoreAdapter {
 public:
  /* There are two versions of CompileOp, this one does not care about the
   * compile strategy. */
  Status CompileOp(ScopeNodeIdMap &fusion_nodes_map, std::map<int64_t, std::string> &json_path_map,
                   std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                   const std::vector<ge::NodePtr> &buff_fus_to_del_nodes) override;
  /*
   *  @ingroup fe
   *  @brief   compile fused op and single op, and generate .o and json files
   *  @param   [in]  fusion_nodes_map  op id and fused sub-graph
   *  @param   [out] json_file_map_    keep path of .o and json of each op
   *  @return  SUCCESS or FAILED
   */
  Status CompileOp(CompileInfoParam &compile_info) override;

  /*
   *  @ingroup fe
   *  @brief   pre-compile and return pattern of op
   *  @return  SUCCESS or FAILED
   */
  Status PreCompileOp(vector<PreCompileNodePara> &compile_para_vec) override;
  /*
   *  @ingroup fe
   *  @brief   initial resources needed by TbeCompilerAdapter, such as dlopen so
   * files
   *           and load function symbols etc.
   *  @return  SUCCESS or FAILED
   */
  Status Initialize(const std::map<std::string, std::string> &options, const std::string &engine_name) override;
  Status InitializeInner(const std::map<std::string, std::string> &options, const std::string &engine_name);
  Status InitializeInnerHelp();

  /*
   *  @ingroup fe
   *  @brief   finalize resources initialized in Initialize function,
   *           such as dclose so files etc.
   *  @return  SUCCESS or FAILED
   */
  Status Finalize() override;

  bool CheckSupport(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                    const bool &is_dynamic_impl, std::string &reason) override;

  Status GetLXOpCoreType(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                         const bool &is_dynamic_impl, std::string &lx_op_core_type_str) override;

  Status SelectOpFormat(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                        const bool &is_dynamic_impl, const HeavyFormatInfo &heavy_format_info,
                        std::string &op_format_dtype_str) override;

  Status OpBuilder(ge::NodePtr node_ptr) override;

  Status SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) override;

  Status GeneralizeNode(const ge::NodePtr &node, const te::TbeOpInfo &op_info,
    te::TE_GENERALIZE_TYPE generalize_type) override;

  bool GetSpecificInfo(const te::TbeOpInfo &tbe_op_info, std::string &opSpecificInfo);

  Status GetRangeLimitType(const ge::NodePtr &node_ptr, const te::TbeOpInfo &tbe_op_info, bool &is_limited) override;

  Status LimitedNodesCheck(bool &is_support, const te::TbeOpInfo &tbe_op_info,
      std::vector<size_t> &upper_limited_input_indexs, std::vector<size_t> &lower_limited_input_indexs) override;

  Status IsGeneralizeFuncRegistered(bool &is_registered, const te::TbeOpInfo &op_info) override;
 private:
  PluginManagerPtr plugin_manager_ptr{nullptr};

  // function wrt TBE API
  function<bool(const te::TbeOpInfo &, std::string &)> SelectTbeOpFormat{nullptr};
  function<bool(te::TbeOpInfo &, te::CheckSupportedResult &, string &reason)> CheckTbeSupported{nullptr};
  function<bool(te::TbeOpInfo &, uint64_t, uint64_t)> PreBuildTbeOp{nullptr};
  function<te::OpBuildResCode(std::vector<ge::Node *>, ge::OpDescPtr, const std::vector<ge::NodePtr> &, uint64_t,
                              uint64_t, const std::string &)> TeFusion{nullptr};
  function<te::OpBuildResCode(std::vector<ge::Node *>, ge::OpDescPtr, const std::vector<ge::NodePtr> &, uint64_t,
                              uint64_t, uint64_t, const std::string &)> TeFusionV{nullptr};
  function<te::OpBuildResCode(uint64_t, uint64_t, ge::Node &)> FuzzBuildTbeOp{nullptr};
  function<te::LX_QUERY_STATUS(const te::TbeOpInfo &, std::string &)> GetOpInfo{nullptr};
  function<bool(const std::map<std::string, std::string> &, bool *)> TbeInitialize{nullptr};
  function<bool()> TbeFinalize{nullptr};
  function<bool(uint64_t, vector<te::FinComTask> &)> WaitAllFinished{nullptr};
  function<bool(const te::TbeOpInfo &, bool &)> CheckIsTbeGeneralizeFuncRegistered{nullptr};
  function<bool(const te::TbeOpInfo &, const te::TE_GENERALIZE_TYPE &, const ge::NodePtr &)> TeGeneralize{nullptr};
  function<bool(const te::TbeOpInfo &, std::string &)> GetOpSpecificInfo{nullptr};
  function<bool(const te::TbeOpInfo &, bool &, std::vector<size_t> &, std::vector<size_t> &)>
    DynamicShapeRangeCheck{nullptr};

  struct CompileTaskPara {
    uint64_t task_num;
    map<int64_t, std::string> *json_path_map;
    ScopeNodeIdMap *fusion_nodes_map;
    std::unordered_map<uint64_t, int64_t> task_scope_id_map;
    std::unordered_map<int64_t, vector<uint64_t>> scope_task_ids_map;
    std::unordered_map<uint64_t, te::FinComTask> failed_tasks;
    std::map<uint64_t, te::FinComTask> succ_tasks;

    std::unordered_map<uint64_t, ge::Node *> task_node_map;
    std::unordered_map<uint64_t, TbeOpInfoPtr> task_tbe_info_map;
    std::unordered_map<int64_t, bool> failed_task_able_to_delete;
  };
  std::string engine_name_;
  bool init_flag{false};
  bool support_parallel_compile{false};
  bool ConvertCheckSupportResult(const ge::NodePtr &node, const std::string &reason,
                                 te::CheckSupportedResult &is_supported) const;
  bool CheckUnsupportReason(const ge::NodePtr &node, const std::string &reason,
                            te::CheckSupportedResult &is_supported) const;

  Status SetOpJsonPath(const ge::OpDescPtr &compile_op_desc, map<int64_t, std::string> &json_path_map,
                       int scope_idx) const;

  Status SetSgtOpJsonPath(const ge::OpDescPtr &compile_op_desc, map<int64_t, std::string> &json_path_map,
                          const int &scope_idx) const;

  Status ParallelCompileOp(ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string> &json_path_map,
                           std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                           std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                           const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                           const CompileStrategy &compile_strategy);

  Status WaitTaskFinish(CompileTaskPara &task_para) const;

  Status ProcessSuccCompileTask(CompileTaskPara &task_para);

  Status ProcessFailCompileTask(CompileTaskPara &task_para, const CompileStrategy &compile_strategy);

  Status ProcessAllFailedCompileTasks(CompileTaskPara &task_para,
                                      std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                      std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                                      const CompileStrategy &compile_strategy);

  Status SetFailedOpCompileTask(ge::Node* node, CompileTaskPara &task_para, const CompileStrategy &compile_strategy);

  Status RetryCompileFailOp(CompileTaskPara &task_para);

  Status ProcessSuccPreCompTask(CompileTaskPara &task_para) const;

  Status ProcessFailPreCompTask(CompileTaskPara &task_para) const;

  void SetOpDescCustomOp(ge::OpDescPtr op_desc) const;

  Status DoFuzzBuildTbeOp(std::vector<ge::Node *> &node_vec, uint64_t taskId, uint64_t thread_id);

  Status SetTeTask(vector<ge::Node *> &node_vec, uint64_t taskId,
                   const std::vector<ge::NodePtr> &buff_fus_to_del_nodes, const CompileStrategy &compile_strategy);

  Status SgtSetTeTask(vector<ge::Node *> &node_vec, uint64_t taskId,
                      const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                      const CompileStrategy &compile_strategy, uint64_t slice_shape_index = 0xFFFFFFFF);

  void SgtGetCompileStrategy(std::vector<ge::Node *> &node_vec, std::string &op_compile_strategy,
                             const CompileStrategy &compile_strategy) const;

  Status GetTbeOpStoreInfo(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                           FEOpsStoreInfo &op_store_info);

  TbeInfoAssemblerPtr tbe_info_assembler_ptr_;

  TbeSingleOpInfoAssemblerPtr tbe_single_op_info_assembler_ptr_;

  Status SetPreCompilePattern(ge::OpDescPtr op_desc, te::TbeOpInfo &op_info,
                              const string &op_pattern_before_buff_fus) const;

  TbeOpInfoPtr PreCompSetTbeOpInfo(PreCompileNodePara &comp_para);

  Status ParallelPreCompileOp(vector<PreCompileNodePara> &compile_para_vec);

  Status SerialPreCompileOp(vector<PreCompileNodePara> &compile_para_vec);

  void ChangeBufferOptimize(const std::map<std::string, std::string> &options,
                            std::map<std::string, std::string> &new_options);

  Status SetOpCompileInfo(std::vector<ge::Node *> &nodes, const ge::OpDescPtr &op_desc_ptr) const;

  Status SetSupportDynamicShape(std::vector<ge::Node *> &nodes) const;

  bool StopCompileOpInTuningAndAfterUBMatchMode() const;

  bool StopWaitTaskFinishInTuningAndAfterBuilderMode(const CompileStrategy &compile_strategy) const;

  void SetFusionFailedId(const vector<ge::Node *> &fusion_nodes, const int64_t &fusion_failed_id) const;

  void SetCustomFlag(ScopeNodeIdMap &fusion_nodes_map) const;

  void GetAutoMode(ScopeNodeIdMap &fusion_nodes_map, bool &auto_mode) const;

  // initialize required tbe api for tbe adapter
  Status InitTbeFunctions(const PluginManagerPtr &plugin_manager_ptr);

  Status FillInTaskParam(ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string> &json_path_map,
                         const std::vector<ge::NodePtr> &buff_fus_to_del_nodes, CompileTaskPara &task_para,
                         const CompileStrategy &compile_strategy);

  void RollBackAttributes(std::vector<ge::Node *> &failed_nodes) const;
  Status SetTaskToTeFusion(CompileTaskPara &task_para, const std::vector<ge::NodePtr> &buff_fus_to_del_nodes,
                           const CompileStrategy &compile_strategy);

  Status CompileMultiKernelSliceOp(ScopeNodeIdMap &fusion_nodes_map,
                           map<int64_t, std::string> &json_path_map,
                           std::vector<ge::NodePtr> &compile_failed_nodes,
                           const std::vector<ge::NodePtr> &to_del_nodes);

  Status ProcessLxFusionFailCompileTasks(CompileTaskPara &task_para,
                                         std::vector<ge::NodePtr> &l1_fusion_failed_nodes,
                                         std::vector<ge::NodePtr> &buff_fus_failed_nodes) const;

  bool IsBuffFusOptimizedNodes(const std::vector<ge::Node *> &scope_op) const;

  bool IsL1FusionOptimizedNodes(const std::vector<ge::Node *> &nodes) const;

  void SaveMsTuneErrorMsg(CompileTaskPara &task_para) const;
  Status GetSgtSliceTaskRollbackNode(CompileTaskPara &task_para,
                                     std::vector<ge::NodePtr> &need_rollback_nodes) const;

  Status SetSgtTensorSliceInfoToNodes(std::vector<ge::Node*> &compile_nodes, int32_t thread_idx) const;

  Status SetTaskForOneScope(std::vector<ge::Node *> &nodes,
                            const int64_t scope_id,
                            const std::vector<ge::NodePtr> &to_del_nodes,
                            CompileTaskPara &task_para,
                            const CompileStrategy &compile_strategy);
  Status SetSgtSliceTaskToTeFusion(CompileTaskPara &task_para,
                                   const std::vector<ge::NodePtr> &to_del_nodes);
  Status ProcessSuccSgtSliceTask(CompileTaskPara &task_para);

  void ClearTaskPara(CompileTaskPara &task_para) const;

  Status UpdateTensorByMixPrecisionMode(const ge::OpDescPtr &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
      std::pair<std::vector<size_t>, std::vector<size_t>> &in_out_changed_idx_vec) const;

  void UpdateDtypeByAllowFp32ToFp16(const ge::OpDescPtr &op_desc, size_t input_or_output_index,
                                    std::pair<std::vector<size_t>, std::vector<size_t>> &in_out_changed_idx_vec,
                                    const bool &isinput) const;

  bool UpdateInputOrOutputDtype(const ge::OpDescPtr &op_desc, const ge::GeTensorDescPtr &tensor_desc,
                                const size_t input_or_output_index) const;
  bool AssembleTbeByMixPrecisionMode(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                     const bool &is_dynamic_impl, te::TbeOpInfo &op_info) const;

  inline bool IsFuzzCompileStrategy(const CompileStrategy &compile_strategy) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_OP_STORE_ADAPTER_H_
