/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_ADAPTER_DSA_ADAPTER_TBE_OP_STORE_ADAPTER_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_ADAPTER_DSA_ADAPTER_TBE_OP_STORE_ADAPTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {
class DSAOpStoreAdapter;
using DSAOpStoreAdapterPtr = std::shared_ptr<DSAOpStoreAdapter>;

class DSAOpStoreAdapter : public OpStoreAdapter {
 public:
  /* There are two versions of CompileOp, this one does not care about the
   * compile strategy.
   */
  Status CompileOp(ScopeNodeIdMap &fusion_nodes_map, std::map<int64_t, std::string> &json_path_map,
                   std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                   const std::vector<ge::NodePtr> &buff_fus_to_del_nodes) override { return SUCCESS; }
  /*
   *  @ingroup fe
   *  @brief   compile fused op and single op, and generate .o and json files
   *  @param   [in]  fusion_nodes_map  op id and fused sub-graph
   *  @param   [out] json_file_map_    keep path of .o and json of each op
   *  @return  SUCCESS or FAILED
   */
  Status CompileOp(CompileInfoParam &compile_info) override { return SUCCESS; }

  /*
   *  @ingroup fe
   *  @brief   pre-compile and return pattern of op
   *  @return  SUCCESS or FAILED
   */
  Status PreCompileOp(vector<PreCompileNodePara> &compile_para_vec) override { return SUCCESS; }
  /*
   *  @ingroup fe
   *  @brief   initial resources needed by TbeCompilerAdapter, such as dlopen so
   *  files and load function symbols etc.
   *  @return  SUCCESS or FAILED
   */
  Status Initialize(const std::map<std::string, std::string> &options, const std::string &engine_name) override;

  /*
   *  @ingroup fe
   *  @brief   finalize resources initialized in Initialize function,
   *           such as dclose so files etc.
   *  @return  SUCCESS or FAILED
   */
  Status Finalize() override;

  bool CheckSupport(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                    const bool &is_dynamic_impl, std::string &reason) override { return SUCCESS; }

  Status SelectOpFormat(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                        const bool &is_dynamic_impl, const HeavyFormatInfo &heavy_format_info,
                        std::string &op_format_dtype_str) override { return SUCCESS; }

  Status GetLXOpCoreType(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                         const bool &is_dynamic_impl, std::string &lx_op_core_type_str) override { return SUCCESS; }

  Status OpBuilder(ge::NodePtr node_ptr) override { return SUCCESS; }

  Status SetTbeOpSliceInfo(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr) override {
    return SUCCESS;
  }

  Status GeneralizeNode(const ge::NodePtr &node, const te::TbeOpInfo &op_info,
                        te::TE_GENERALIZE_TYPE generalize_type) override {
    return SUCCESS;
  }

  Status GetRangeLimitType(const ge::NodePtr &node_ptr, const te::TbeOpInfo &tbe_op_info, bool &is_limited) override {
    return SUCCESS;
  }

  Status LimitedNodesCheck(bool &is_support, const te::TbeOpInfo &tbe_op_info,
                           std::vector<size_t> &upper_limited_input_indexs,
                           std::vector<size_t> &lower_limited_input_indexs) override {
    return SUCCESS;
  }

  Status IsGeneralizeFuncRegistered(bool &is_registered, const te::TbeOpInfo &op_info) override {
    return SUCCESS;
  }
 private:
  std::string engine_name_;
  bool init_flag{false};
};
}  // namespace fe

#endif  // AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_ADAPTER_DSA_ADAPTER_TBE_OP_STORE_ADAPTER_H_
