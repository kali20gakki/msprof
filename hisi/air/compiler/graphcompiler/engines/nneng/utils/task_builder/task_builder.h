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

#ifndef FUSION_ENGINE_UTILS_TASK_BUILDER_TASK_BUILDER_H_
#define FUSION_ENGINE_UTILS_TASK_BUILDER_TASK_BUILDER_H_
#include <map>
#include <memory>
#include <vector>
#include "proto/task.pb.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "ffts/ffts_task_builder_adapter.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "fftseng/inc/ffts_type.h"
#include "graph/debug/ge_attr_define.h"
#include "common/comm_log.h"

namespace fe {
Status CreateHandle(ccHandle_t *handle);
Status DestroyHandle(ccHandle_t *handle);
using FftsPlusCtxDefPtr = std::shared_ptr<domi::FftsPlusCtxDef>;
using FftsTaskBuilderAdapterPtr = std::shared_ptr<FftsTaskBuilderAdapter>;

class TaskBuilder {
 public:
  TaskBuilder();
  virtual ~TaskBuilder();

  /*
   * @ingroup fe
   * @brief   Generate tasks
   * @param   [in] node Node of compute graph
   * @param   [in] context Context for generate tasks
   * @param   [out] task_defs Save the generated tasks.
   * @return  SUCCESS or FAILED
   */
  Status GenerateKernelTask(const ge::Node &node, const ge::RunContext &context, std::vector<domi::TaskDef> &task_defs);

  Status GenerateFFTSPlusCtx(const ge::Node &node, const ge::RunContext &context);

 private:
  TaskBuilder(const TaskBuilder &builder) = delete;
  TaskBuilder &operator=(const TaskBuilder &builder) = delete;

  // follow function for traditional kernel task
  Status DoGenerate(const ge::Node &node, const int32_t &stream_id, std::vector<domi::TaskDef> &task_defs);
  /*
   * @ingroup fe
   * @brief   Create TaskBuilderAdapter
   * @param   [in] node Node of compute graph
   * @return  SUCCESS or FAILED
   */
  Status CreateAdapter(const ge::Node &node);
  /*
   * @ingroup fe
   * @brief   Run TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  Status RunAdapter(domi::TaskDef &task_def);
  Status FillTaskDefAfterGenTask(const ge::OpDescPtr &op_desc, domi::TaskDef &task_def);
  void StartKernelFusion(const ge::OpDescPtr &op_desc_ptr, const int32_t &stream_id,
                         std::vector<domi::TaskDef> &task_defs) const;
  void EndKernelFusion(const ge::OpDescPtr &op_desc_ptr, const int32_t &stream_id,
                       std::vector<domi::TaskDef> &task_defs) const;
  void GenerateCMOTask(const ge::Node &node, std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                       const bool pre_cmo_task);

  // follow functions for ffts+ kernel task
  Status GenCtxParamAndCtxType(const ge::Node &node, ffts::TaskBuilderType &ctx_type);
  Status GenManualAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  Status GenAutoAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  Status GenManualMixAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  Status GenAutoMixAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  Status GenDynamicAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  Status GenMixL2CtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx);
  void SetAutoThreadIOAddrForDataCtx(const ge::OpDescPtr &op_desc);
  void SetManualThreadIOAddrForDataCtx(const ge::OpDescPtr &op_desc);
  bool IsNeedGenerateTask(const ge::Node &node, const int32_t stream_id, std::vector<domi::TaskDef> &task_defs) const;

  template <typename T>
  Status GenCommonAutoAICAIVCtxDef(const ge::OpDescPtr &op_desc, T *ctx) {
    // cache managemet will do at GenerateDataTaskDef()
    ctx->set_prefetch_once_bitmap(0);
    ctx->set_prefetch_enable_bitmap(0);

    // set ffts+ mode to auto, which value is 1;
    ctx->set_aten(1);
    ctx->set_atm(1);

    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = op_desc->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
    ctx->set_thread_dim(slice_info_ptr->slice_instance_num);

    vector<int32_t> block_dims;
    (void)ge::AttrUtils::GetListInt(op_desc, ge::TVM_ATTR_NAME_THREAD_BLOCKDIM, block_dims);
    if (block_dims.size() > 1) {
      ctx->set_non_tail_block_dim(static_cast<uint32_t>(block_dims[0]));
      ctx->set_tail_block_dim(static_cast<uint32_t>(block_dims[1]));
      CM_LOGD("block_dims[0]:%u, block_dims[1]:%u.", static_cast<uint32_t>(block_dims[0]),
              static_cast<uint32_t>(block_dims[1]));
    }

    // generate _register_stub_func
    vector<string> unique_ids;
    string session_graph_id = "";
    if (ge::AttrUtils::GetStr(op_desc, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) && !session_graph_id.empty()) {
      unique_ids.push_back(session_graph_id + "_" + op_desc->GetName() + "_0");
      unique_ids.push_back(session_graph_id + "_" + op_desc->GetName() + "_1");
    } else {
      unique_ids.push_back(op_desc->GetName() + "_0");
      unique_ids.push_back(op_desc->GetName() + "_1");
    }
    (void)ge::AttrUtils::SetListStr(op_desc, "_register_stub_func", unique_ids);

    uint32_t input_output_num = auto_thread_param_offset_.first_thread_input_addrs.size() +
                                auto_thread_param_offset_.first_thread_output_addrs.size();
    ctx->set_input_output_count(input_output_num);
    for (auto input_addr : auto_thread_param_offset_.first_thread_input_addrs) {
      ctx->add_task_addr(reinterpret_cast<uint64_t>(input_addr));
    }
    for (auto output_addr : auto_thread_param_offset_.first_thread_output_addrs) {
      ctx->add_task_addr(reinterpret_cast<uint64_t>(output_addr));
    }
    for (auto workspace_addr : auto_thread_param_offset_.thread_workspace_addrs[0]) {
      ctx->add_task_addr(reinterpret_cast<uint64_t>(workspace_addr));
    }
    for (auto workspace_addr : auto_thread_param_offset_.thread_workspace_addrs[1]) {
      ctx->add_task_addr(reinterpret_cast<uint64_t>(workspace_addr));
    }
    for (auto addr_offset : auto_thread_param_offset_.thread_addr_offset) {
      ctx->add_task_addr_offset(reinterpret_cast<uint64_t>(addr_offset));
    }
    return SUCCESS;
  }

 private:
  TaskBuilderContext context_;
  TaskBuilderAdapterPtr adapter_{nullptr};
  std::vector<uint32_t> orig_op_indexes_;
  ThreadParamOffset auto_thread_param_offset_;
  TaskArgs manual_thread_param_;
  using GenCtxFunc = Status (TaskBuilder::*) (const ge::OpDescPtr &, FftsPlusCtxDefPtr);
  std::map<ffts::TaskBuilderType, GenCtxFunc> gen_ctx_func_map_ = {
          {ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV, &TaskBuilder::GenManualAICAIVCtxDef},
          {ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV_AUTO, &TaskBuilder::GenAutoAICAIVCtxDef},
          {ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV, &TaskBuilder::GenManualMixAICAIVCtxDef},
          {ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV_DYNAMIC, &TaskBuilder::GenDynamicAICAIVCtxDef},
          {ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV_AUTO, &TaskBuilder::GenAutoMixAICAIVCtxDef},
          {ffts::TaskBuilderType::EN_TASK_TYPE_MIX_L2_AIC_AIV, &TaskBuilder::GenMixL2CtxDef}
  };
};

}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_TASK_BUILDER_TASK_BUILDER_H_
