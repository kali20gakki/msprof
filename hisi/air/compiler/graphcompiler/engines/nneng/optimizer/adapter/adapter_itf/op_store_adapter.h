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

#ifndef FUSION_ENGINE_OPTIMIZER_ADAPTER_ADAPTER_ITF_OP_STORE_ADAPTER_H_
#define FUSION_ENGINE_OPTIMIZER_ADAPTER_ADAPTER_ITF_OP_STORE_ADAPTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "nlohmann/json.hpp"
#include "ops_store/op_kernel_info.h"
#include "common/op_info_common.h"

namespace fe {
class OpStoreAdapter;
using OpStoreAdapterPtr = std::shared_ptr<OpStoreAdapter>;
using ScopeNodeMap = std::map<int64_t, std::vector<ge::NodePtr>>;
using ScopeNodeIdMap = std::map<int64_t, std::vector<ge::Node *>>;
using TbeOpInfoPtr = std::shared_ptr<te::TbeOpInfo>;

struct PreCompileNodePara {
  ge::Node *node;
  OpKernelInfoPtr op_kernel_info_ptr;
  std::string imply_type_str;
  std::string op_dsl_file_path;
  std::string session_graph_id;
  TbeOpInfoPtr tbe_op_info_ptr;
};

enum class CompileStrategy {
  COMPILE_STRATEGY_OP_SPEC,
  COMPILE_STRATEGY_KEEP_OPTUNE,
  COMPILE_STRATEGY_NO_TUNE,
  COMPILE_STRATEGY_ONLINE_FUZZ
};

struct CompileInfoParam {
  std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes;
  std::vector<ge::NodePtr> buff_fus_to_del_nodes;
  std::vector<ge::NodePtr> l1_fusion_failed_nodes;
  ScopeNodeIdMap fusion_nodes_map;
  std::map<int64_t, std::string> scope_json_map;
  CompileStrategy compile_strategy;
  explicit CompileInfoParam(std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes_param)
      : buff_fus_compile_failed_nodes(buff_fus_compile_failed_nodes_param),
        compile_strategy(CompileStrategy::COMPILE_STRATEGY_OP_SPEC) {
  }
};

class OpStoreAdapter {
 public:
  virtual ~OpStoreAdapter() {}

  /*
   *  @ingroup fe
   *  @brief   initialize op adapter
   *  @return  SUCCESS or FAILED
   */
  virtual Status Initialize(const std::map<std::string, std::string> &options, const std::string &engine_name) = 0;

  /*
   *  @ingroup fe
   *  @brief   finalize op adapter
   *  @return  SUCCESS or FAILED
   */
  virtual Status Finalize() = 0;

  /*
   *  @ingroup fe
   *  @brief   check op wether supported in ops store
   *  @param   [in] op_desc   infomation of op in ge
   *  @return  true or false
   */
  virtual bool CheckSupport(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                            const bool &is_dynamic_impl, std::string &reason) = 0;

  virtual Status CompileOp(ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string> &json_path_map,
                           std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                           const std::vector<ge::NodePtr> &buff_fus_to_del_nodes) = 0;

  virtual Status CompileOp(CompileInfoParam &compile_info) = 0;

  virtual Status PreCompileOp(vector<PreCompileNodePara> &compile_para_vec) = 0;

  virtual Status GetLXOpCoreType(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                 const bool &is_dynamic_impl, std::string &lx_op_core_type_str) = 0;

  virtual Status SelectOpFormat(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                                const bool &is_dynamic_impl, const HeavyFormatInfo &heavy_format_info,
                                std::string &op_format_dtype_str) = 0;

  virtual Status OpBuilder(ge::NodePtr node_ptr) = 0;

  virtual Status SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) = 0;

  virtual Status GeneralizeNode(const ge::NodePtr &node, const te::TbeOpInfo &op_info,
    te::TE_GENERALIZE_TYPE generalize_type) = 0;

  virtual Status GetRangeLimitType(const ge::NodePtr &node_ptr, const te::TbeOpInfo &tbe_op_info, bool &is_limited) = 0;

  virtual Status LimitedNodesCheck(bool &is_support, const te::TbeOpInfo &tbe_op_info,
      std::vector<size_t> &upper_limited_input_indexs, std::vector<size_t> &lower_limited_input_indexs) = 0;

  virtual Status IsGeneralizeFuncRegistered(bool &is_registered, const te::TbeOpInfo &op_info) = 0;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_ADAPTER_ADAPTER_ITF_OP_STORE_ADAPTER_H_
