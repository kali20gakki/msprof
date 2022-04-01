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

#include "task_builder/task_builder.h"

#include <securec.h>
#include <string>
#include <common/sgt_slice_type.h>
#include "common/common_utils.h"
#include "common/comm_error_codes.h"
#include "common/fe_error_code.h"
#include "common/op_tensor_utils.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_attr_define.h"
#include "cmo_task_builder/cmo_task_builder.h"
#include "adapter/factory/task_builder_adapter_factory.h"
#include "graph/utils/node_utils.h"
#include "util/error_manager/error_manager.h"
#include "runtime/rt_error_codes.h"
#include "runtime/rt_model.h"
#include "runtime/mem.h"
#include "runtime/stream.h"

namespace fe {
static const uint32_t kKernelTypeTe = 2;
static const vector<std::string> kMixPrefixs = { "_mix_aic", "_mix_aiv" };
static const uint32_t kManualMode = 0;
static const uint32_t kAutoMode = 1;
static const std::string kAttrAICoreCtxDef = "_aicore_ctx_def";
static const std::string kAttrAICoreCtxType = "_aicore_ctx_type";

TaskBuilder::TaskBuilder() {}

TaskBuilder::~TaskBuilder() {}

Status CreateHandle(ccHandle_t *handle) {
  if (handle == nullptr) {
    REPORT_CM_ERROR("[GenTask][CreatHandle] handle is nullptr");
    return TASK_BUILDER_STATUS_BAD_PARAM;
  }
  void *ccContext = nullptr;
  // alloc ccContext_t
  rtError_t ret = rtMallocHost(&ccContext, sizeof(ccContext_t));
  if ((ret != ACL_RT_SUCCESS) || (ccContext == nullptr)) {
    REPORT_CM_ERROR("[GenTask][CreatHandle] alloc handle entity failed");
    return FAILED;
  }
  (void)memset_s(ccContext, sizeof(ccContext_t), 0, sizeof(ccContext_t));
  *handle = static_cast<ccHandle_t>(ccContext);
  return SUCCESS;
}

Status SetStream(ccHandle_t handle, rtStream_t streamId) {
  CM_CHECK_NOTNULL(handle);
  handle->streamId = streamId;
  return SUCCESS;
}

Status DestroyHandle(ccHandle_t *handle) {
  if (handle == nullptr || *handle == nullptr) {
    REPORT_CM_ERROR("[GenTask][DestroyHandle] handle is nullptr");
    return TASK_BUILDER_STATUS_BAD_PARAM;
  }
  rtError_t ret = rtFreeHost(*handle);
  if (ret != ACL_RT_SUCCESS) {
    REPORT_CM_ERROR("[GenTask][DestroyHandle] free handler failed");
    return FAILED;
  }
  *handle = nullptr;
  return SUCCESS;
}

bool TaskBuilder::IsNeedGenerateTask(const ge::Node &node, const int32_t stream_id,
                                     std::vector<domi::TaskDef> &task_defs) const  {
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  bool is_support_unknown_shape = true;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, is_support_unknown_shape);
  if (!is_support_unknown_shape && OpTensorUtils::IsFeSupportedDynamicOp(*op_desc_ptr, false)) {
    CM_LOGD("No need to generate task for node[%s, %s].", node.GetName().c_str(), node.GetType().c_str());
    StartKernelFusion(op_desc_ptr, stream_id, task_defs);
    EndKernelFusion(op_desc_ptr, stream_id, task_defs);
    return false;
  }

  if (ge::AttrUtils::HasAttr(op_desc_ptr, kOpShapeOrRangeUnsupport)) {
    CM_LOGD("No need to generate task for node[%s, %s], shape not support.",
            node.GetName().c_str(), node.GetType().c_str());
    return false;
  }
  return true;
}

Status TaskBuilder::GenerateKernelTask(const ge::Node &node, const ge::RunContext &context,
                                       std::vector<domi::TaskDef> &task_defs) {
  CM_LOGD("TaskBuilder::GenerateKernelTask begin, node name:%s, type:%s.", node.GetName().c_str(),
          node.GetType().c_str());
  int64_t start_usec_gentask = GetMicroSecondsTime();

  CM_CHECK_NOTNULL(context.model);
  CM_CHECK_NOTNULL(context.stream);
  CM_CHECK_NOTNULL(context.dataMemBase);
  CM_CHECK_NOTNULL(context.weightMemBase);

  int32_t stream_id = -1;
  if (rtGetStreamId(context.stream, &stream_id) != ACL_RT_SUCCESS) {
    CM_LOGE("Can not get stream id by conetxt stream.");
    return FAILED;
  }

  CM_LOGD("Node %s, stream id %d.", node.GetName().c_str(), stream_id);
  if (!IsNeedGenerateTask(node, stream_id, task_defs)) {
    return SUCCESS;
  }
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  ccHandle_t handle = nullptr;
  Status ret = CreateHandle(&handle);
  if (ret != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][CreateHandle][Node %s type %s] CreateHandle failed. ret:0x%X",
                    node.GetName().c_str(), node.GetType().c_str(), ret);
    return FAILED;
  }

  ret = SetStream(handle, context.stream);
  if (ret != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][SetStream][Node %s type %s] SetStream failed. ret:0x%X",
                    node.GetName().c_str(), node.GetType().c_str(), ret);
    (void)DestroyHandle(&handle);
    return FAILED;
  }

  context_.dataMemSize = context.dataMemSize;
  context_.dataMemBase = context.dataMemBase;
  context_.weightMemSize = context.weightMemSize;
  context_.weightMemBase = context.weightMemBase;
  context_.weightBufferHost = context.weightsBuffer;
  context_.model = context.model;
  context_.stream = context.stream;
  context_.handle = handle;

  // Save current TaskBuilder for runtime callback
  StartKernelFusion(op_desc_ptr, stream_id, task_defs);
  GenerateCMOTask(node, task_defs, stream_id, true);

  Status status = DoGenerate(node, stream_id, task_defs);

  GenerateCMOTask(node, task_defs, stream_id, false);
  EndKernelFusion(op_desc_ptr, stream_id, task_defs);

  (void)DestroyHandle(&handle);

  CM_LOGD("TaskBuilder::GenerateTask end, node name:%s, type:%s.", node.GetName().c_str(), node.GetType().c_str());
  int64_t end_usec_gentask = GetMicroSecondsTime();
  CM_LOGV("[FE_PERFORMANCE] The time cost of TaskBuilder::GenerateTask is [%ld] micro second.",
          (end_usec_gentask - start_usec_gentask));
  return status;
}

void TaskBuilder::StartKernelFusion(const ge::OpDescPtr &op_desc_ptr, const int32_t &stream_id,
                                    std::vector<domi::TaskDef> &task_defs) const {
  bool is_first_node = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_IS_FIRST_NODE, is_first_node);
  if (!is_first_node) {
    return;
  }

  CM_LOGD("Start kernel fusion from node %s, type %s.", op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
  domi::TaskDef task_def = {};
  task_def.set_type(RT_MODEL_TASK_FUSION_START);
  task_def.set_stream_id(stream_id);
  task_defs.push_back(task_def);
}

void TaskBuilder::EndKernelFusion(const ge::OpDescPtr &op_desc_ptr, const int32_t &stream_id,
                                  std::vector<domi::TaskDef> &task_defs) const {
  bool is_last_node = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_IS_LAST_NODE, is_last_node);
  if (!is_last_node) {
    return;
  }

  CM_LOGD("Finish kernel fusion of node %s, type %s.", op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
  domi::TaskDef task_def = {};
  task_def.set_type(RT_MODEL_TASK_FUSION_END);
  task_def.set_stream_id(stream_id);
  task_defs.push_back(task_def);
}

Status TaskBuilder::DoGenerate(const ge::Node &node, const int32_t &stream_id, std::vector<domi::TaskDef> &task_defs) {
  domi::TaskDef task_def = {};
  task_def.set_stream_id(stream_id);
  // Create TaskBuilderAdapter
  Status status = CreateAdapter(node);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][DoGenerate][Node %s type %s] Failed to create adapter.",
                    node.GetName().c_str(), node.GetType().c_str());
    return TASK_BUILDER_CREATE_ADAPTER_FAILED;
  }

  status = RunAdapter(task_def);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][DoGenerate][Node %s type %s] Fail to run task adapter.",
                    node.GetName().c_str(), node.GetType().c_str());
    return status;
  }

  status = FillTaskDefAfterGenTask(node.GetOpDesc(), task_def);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][DoGenerate][Node %s type %s] Fail to fill up task def.",
                    node.GetName().c_str(), node.GetType().c_str());
    return status;
  }

  task_defs.push_back(task_def);
  return status;
}

Status TaskBuilder::FillTaskDefAfterGenTask(const ge::OpDescPtr &op_desc, domi::TaskDef &task_def) {
  std::string attr_key_kernel_name = op_desc->GetName() + "_kernelname";
  std::string attr_val_kernel_name;
  (void)ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_val_kernel_name);
  domi::KernelContext *kernel_context = nullptr;
  if (task_def.type() == RT_MODEL_TASK_KERNEL) {
    domi::KernelDef *kernel_def = task_def.mutable_kernel();
    if (kernel_def == nullptr) {
      REPORT_CM_ERROR("[GenTask][InitKernelTask] kernel_def is nullptr.");
      return ACL_ERROR_RT_PARAM_INVALID;
    }

    kernel_def->set_kernel_name(attr_val_kernel_name);
    CM_LOGD("Set kernel_name[%s] for the kernel_def of node[%s, %s].",
            attr_val_kernel_name.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str());

    kernel_context = kernel_def->mutable_context();
  }
  if (task_def.type() == RT_MODEL_TASK_ALL_KERNEL) {
    domi::KernelDefWithHandle *kernel_def_with_handle = task_def.mutable_kernel_with_handle();
    if (kernel_def_with_handle == nullptr) {
      CM_LOGD("[GenTask][InitKernelTask] kernel_def_with_handle is nullptr.");
      return ACL_ERROR_RT_PARAM_INVALID;
    }
    std::string first_kernel_name;
    if (ge::AttrUtils::GetStr(op_desc, ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name))  {
      kernel_def_with_handle->set_node_info(first_kernel_name);
      kernel_def_with_handle->set_original_kernel_key(attr_val_kernel_name);
      CM_LOGD("Set node_info[%s] and original_kernel_key[%s] for the kernel_def_with_handle of node[%s, %s].",
              first_kernel_name.c_str(), attr_val_kernel_name.c_str(),
              op_desc->GetName().c_str(), op_desc->GetType().c_str());
    } else {
      REPORT_CM_ERROR("[GenTask][GenAllKernelTask] No kernel list name in op desc.");
      return ACL_ERROR_RT_PARAM_INVALID;
    }

    kernel_context = kernel_def_with_handle->mutable_context();
  }

  if (kernel_context == nullptr) {
    REPORT_CM_ERROR("[GenTask][FillTaskDefAfterGenTask] kernel_context is nullptr.");
    return ACL_ERROR_RT_PARAM_INVALID;
  }

  if (orig_op_indexes_.empty()) {
    REPORT_CM_ERROR("[GenTask][FillTaskDefAfterGenTask] The value[orig_op_indexes] is empty.");
    return ACL_ERROR_RT_PARAM_INVALID;
  }
  for (const uint32_t &orig_index : orig_op_indexes_) {
    kernel_context->add_origin_op_index(orig_index);
  }
  kernel_context->set_op_index(orig_op_indexes_[0]);
  kernel_context->set_kernel_type(kKernelTypeTe);
  kernel_context->set_is_flowtable(false);
  return SUCCESS;
}

Status TaskBuilder::CreateAdapter(const ge::Node &node) {
  ge::OpDescPtr op_desc = node.GetOpDesc();

  int32_t imply_type = -1;
  if (!ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_type)) {
    REPORT_CM_ERROR("[GenTask][CreateAdapter][Node %s type %s] Get op imply_type failed, imply_type[%u]]",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str(), imply_type);
    return FAILED;
  }
  auto op_impl_type = static_cast<OpImplType>(imply_type);

  adapter_ = TaskBuilderAdapterFactory::Instance().CreateTaskBuilderAdapter(op_impl_type, node, context_);
  CM_CHECK_NOTNULL(adapter_);

  CM_LOGD("Create TaskBuilderAdapter success. OpName:%s, OpType:%s",
          op_desc->GetName().c_str(), op_desc->GetType().c_str());

  Status status = adapter_->Init();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][CreateAdapter][Node %s type %s] Init TaskBuilderAdapter failed.",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }

  return SUCCESS;
}

Status TaskBuilder::RunAdapter(domi::TaskDef &task_def) {
  ge::ConstOpDescPtr op_desc = adapter_->GetOpDesc();
  CM_LOGD("Start run TaskBuilderAdapter, OpName:%s, OpType:%s", op_desc->GetName().c_str(), op_desc->GetType().c_str());

  orig_op_indexes_.clear();
  orig_op_indexes_.push_back(static_cast<uint32_t>(op_desc->GetId()));
  CM_LOGD("Push back node(%s)'s op_desc id(%d) into orig_op_indexes success.", op_desc->GetName().c_str(),
          orig_op_indexes_.back());

  if (context_.handle == nullptr) {
    REPORT_CM_ERROR("[GenTask][RunAdapter][Node %s type %s] handle is nullptr!",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }
  context_.handle->opIndex = static_cast<uint32_t>(op_desc->GetId());

  Status status = adapter_->Run(task_def);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][RunAdapter][Node %s type %s] Fail to run TaskBuilderAdapter.",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }

  CM_LOGD("TaskBuilderAdapter run success, OpName:%s, OpType:%s",
          op_desc->GetName().c_str(), op_desc->GetType().c_str());

  return SUCCESS;
}

Status TaskBuilder::GenerateFFTSPlusCtx(const ge::Node &node, const ge::RunContext &context) {
  CM_CHECK_NOTNULL(context.model);
  CM_CHECK_NOTNULL(context.stream);
  CM_CHECK_NOTNULL(context.dataMemBase);
  CM_CHECK_NOTNULL(context.weightMemBase);
  context_.dataMemSize = context.dataMemSize;
  context_.dataMemBase = context.dataMemBase;
  context_.weightMemSize = context.weightMemSize;
  context_.weightMemBase = context.weightMemBase;
  context_.weightBufferHost = context.weightsBuffer;
  auto op_desc = node.GetOpDesc();
  CM_LOGD("Start to generate FFTSPlus Ctx, dataMemSize %ld, dataMemBase %p, weightMemSize %ld, weightMemBase %p.",
          context.dataMemSize, context.dataMemBase, context.weightMemSize, context.weightMemBase);
  ffts::TaskBuilderType ctx_type;
  Status status = GenCtxParamAndCtxType(node, ctx_type);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][GenerateFFTSPlusCtx][Node %s type %s] Fail to run GenCtxParamAndCtxType.",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return status;
  }

  FftsPlusCtxDefPtr ctx = std::make_shared<domi::FftsPlusCtxDef>();
  CM_CHECK_NOTNULL(ctx);

  status = (this->*(gen_ctx_func_map_[ctx_type]))(op_desc, ctx);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][GenerateFFTSPlusCtx][Node %s type %s ctx type %d] Fail to run GenCtxDef.",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str(), static_cast<int32_t>(ctx_type));
    return status;
  }

  (void)ge::AttrUtils::SetInt(op_desc, kAttrAICoreCtxType, static_cast<int64_t>(ctx_type));
  (void)op_desc->SetExtAttr(kAttrAICoreCtxDef, ctx);
  return SUCCESS;
}

void TaskBuilder::SetAutoThreadIOAddrForDataCtx(const ge::OpDescPtr &op_desc) {
  vector<int64_t> input_addrs;
  for (auto ele : auto_thread_param_offset_.first_thread_input_addrs) {
    input_addrs.emplace_back(reinterpret_cast<int64_t>(ele));
  }
  vector<int64_t> output_addrs;
  for (auto ele : auto_thread_param_offset_.first_thread_output_addrs) {
    output_addrs.emplace_back(reinterpret_cast<int64_t>(ele));
  }

  (void)ge::AttrUtils::SetListInt(op_desc, "input_addrs", input_addrs);
  (void)ge::AttrUtils::SetListInt(op_desc, "output_addrs", output_addrs);
}

void TaskBuilder::SetManualThreadIOAddrForDataCtx(const ge::OpDescPtr &op_desc) {
  vector<int64_t> input_addrs;
  for (auto ele : manual_thread_param_.input_addrs) {
    input_addrs.emplace_back(reinterpret_cast<int64_t>(ele));
  }
  vector<int64_t> output_addrs;
  for (auto ele : manual_thread_param_.output_addrs) {
    output_addrs.emplace_back(reinterpret_cast<int64_t>(ele));
  }

  (void)ge::AttrUtils::SetListInt(op_desc, "input_addrs", input_addrs);
  (void)ge::AttrUtils::SetListInt(op_desc, "output_addrs", output_addrs);
}

inline void GetCoreTypeByMode(const ge::OpDescPtr &op_desc, std::string &core_type, bool auto_mode, bool is_dynamic)
{
  if (auto_mode && !is_dynamic) {
    vector<string> thread_core_type;
    (void)ge::AttrUtils::GetListStr(op_desc, ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE, thread_core_type);
    core_type = thread_core_type.empty() ? core_type : thread_core_type[0];
  } else {
    (void)ge::AttrUtils::GetStr(op_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  }
  return;
}

Status GetCtxTypeByCoreType(const ge::OpDescPtr &op_desc, std::string &core_type, ffts::TaskBuilderType &ctx_type,
                            bool auto_mode, bool is_dynamic)
{
  if (core_type.empty()) {
    return FAILED;
  }
  bool core_type_aic_aiv = core_type == kCoreTypeAIC || core_type == kCoreTypeAIV;
  bool core_type_mix_aic_aiv = core_type == kCoreTypeMixAIC || core_type == kCoreTypeMixAIV;
  if (core_type_aic_aiv) {
    ctx_type = ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV;
    if (auto_mode) {
      ctx_type = is_dynamic ? ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV_DYNAMIC :
                 ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV_AUTO;
    }
  } else if (core_type_mix_aic_aiv) {
    ctx_type = auto_mode ? ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV_AUTO :
               ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV;
    if (op_desc->HasAttr(ATTR_NAME_ALIAS_ENGINE_NAME)) {
      ctx_type = ffts::TaskBuilderType::EN_TASK_TYPE_MIX_L2_AIC_AIV;
    }
  } else {
    return FAILED;
  }
  return SUCCESS;
}

Status TaskBuilder::GenCtxParamAndCtxType(const ge::Node &node, ffts::TaskBuilderType &ctx_type) {
  auto op_desc = node.GetOpDesc();
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = op_desc->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  bool auto_mode = slice_info_ptr && slice_info_ptr->thread_mode ==
                   static_cast<uint32_t>(ffts::ThreadMode::AUTO_THREAD);
  bool is_unknown = false;
  (void)ge::AttrUtils::GetBool(op_desc, ATTR_NAME_OWNER_GRAPH_IS_UNKNOWN, is_unknown);
  bool is_dynamic = is_unknown || OpTensorUtils::IsUnKnownShapeOp(*(op_desc.get()));
  if (is_dynamic) {
    CM_LOGD("Node[%s] is unknow shape, no need get args.", node.GetName().c_str());
  } else if (auto_mode) {
    FftsTaskBuilderAdapterPtr ffts_task_builder_adapter_ptr = nullptr;
    CM_MAKE_SHARED(ffts_task_builder_adapter_ptr = std::make_shared<FftsTaskBuilderAdapter>(node, context_),
                   return FAILED);
    Status status = ffts_task_builder_adapter_ptr->Init();
    if (status != SUCCESS) {
      REPORT_CM_ERROR("[FFTSPlusTaskBuidler][GenContextArgs][Node %s] Ffts plus Init ffts task builder adapter failed.",
                      node.GetOpDesc()->GetName().c_str());
      return status;
    }
    (void)ffts_task_builder_adapter_ptr->GetThreadParamOffset(auto_thread_param_offset_);
    SetAutoThreadIOAddrForDataCtx(op_desc);
  } else {
    TaskBuilderAdapterPtr task_builder_adapter_ptr = nullptr;
    CM_MAKE_SHARED(task_builder_adapter_ptr = std::make_shared<TbeTaskBuilderAdapter>(node, context_), return FAILED);
    Status status = task_builder_adapter_ptr->Init();
    if (status != SUCCESS) {
      REPORT_CM_ERROR("[FFTSPlusTaskBuidler][GenContextArgs][Node %s] Ffts plus init tbe task builder adapter failed.",
                      node.GetOpDesc()->GetName().c_str());
      return status;
    }
    (void)task_builder_adapter_ptr->GetTaskArgs(manual_thread_param_);
    SetManualThreadIOAddrForDataCtx(op_desc);
  }
  std::string core_type;
  GetCoreTypeByMode(op_desc, core_type, auto_mode, is_dynamic);
  int32_t block_dim = 0;
  (void)ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  uint32_t thread_dim = slice_info_ptr ? slice_info_ptr->slice_instance_num : 1;
  CM_LOGD("Gen ctx type, Node[%s %s]'s core type: %s, block dim: %d, thread dim: %u",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), core_type.c_str(), block_dim, thread_dim);
  return GetCtxTypeByCoreType(op_desc, core_type, ctx_type, auto_mode, is_dynamic);
}

Status TaskBuilder::GenManualAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ctx->mutable_aic_aiv_ctx();
  CM_CHECK_NOTNULL(aic_aiv_ctx_def);

  // cache managemet will do at GenerateDataTaskDef()
  aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  aic_aiv_ctx_def->set_atm(kManualMode);
  aic_aiv_ctx_def->set_thread_dim(1);

  int32_t block_dim = 0;
  (void) ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  for (auto input_addr: manual_thread_param_.input_addrs) {
    uint64_t input_addr_tmp = reinterpret_cast<uint64_t>(input_addr);
    aic_aiv_ctx_def->add_task_addr(input_addr_tmp);
    CM_LOGD("input_addr, %lu", input_addr_tmp);
  }

  for (auto output_addr: manual_thread_param_.output_addrs) {
    uint64_t output_addr_tmp = reinterpret_cast<uint64_t>(output_addr);
    aic_aiv_ctx_def->add_task_addr(output_addr_tmp);
    CM_LOGD("output_addr, %lu", output_addr_tmp);
  }

  for (auto workspace_addr: manual_thread_param_.workspace_addrs) {
    uint64_t workspace_addr_tmp = reinterpret_cast<uint64_t>(workspace_addr);
    aic_aiv_ctx_def->add_task_addr(workspace_addr_tmp);
    CM_LOGD("workspace_addr, %lu", workspace_addr_tmp);
  }
  string attr_key_kernel_name = op_desc->GetName() + kKernelName;
  string attr_kernel_name;
  (void) ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
  aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);

  CM_LOGD("aic_aiv_ctx_def FillContextData SUCCESS. Op:%s, optype:%s, block_dim:%u, size:%u, attr_kernel_name:%s",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), static_cast<uint32_t>(block_dim),
          aic_aiv_ctx_def->task_addr_size(), attr_kernel_name.c_str());
  return SUCCESS;
}

Status TaskBuilder::GenManualMixAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def = ctx->mutable_mix_aic_aiv_ctx();
  CM_CHECK_NOTNULL(mix_aic_aiv_ctx_def);
  uint32_t task_ratio;
  (void)ge::AttrUtils::GetInt(op_desc, kTaskRadio, task_ratio);

  mix_aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  mix_aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  mix_aic_aiv_ctx_def->set_ns(1);
  mix_aic_aiv_ctx_def->set_atm(kManualMode);
  mix_aic_aiv_ctx_def->set_thread_dim(1);

  mix_aic_aiv_ctx_def->set_tail_block_ratio_n(task_ratio);
  mix_aic_aiv_ctx_def->set_non_tail_block_ratio_n(task_ratio);

  int32_t block_dim = 0;
  (void)ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  mix_aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  mix_aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  // modeInArgsFirstField
  uint32_t mode = 0;
  (void)ge::AttrUtils::GetInt(op_desc, kModeInArgsFirstField, mode);
  // mode == 1 indicates we need reserve 8 Bytes for the args beginning
  if (mode == 1) {
    uint64_t modeArgs = 0;
    mix_aic_aiv_ctx_def->add_task_addr(modeArgs);
  }
  for (auto input_addr : manual_thread_param_.input_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uint64_t>(input_addr));
  }

  for (auto output_addr : manual_thread_param_.output_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uint64_t>(output_addr));
  }

  for (auto workspace_addr : manual_thread_param_.workspace_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uint64_t>(workspace_addr));
  }
  for (auto &prefix : kMixPrefixs) {
    string attr_key_kernel_name = prefix + op_desc->GetName() + kKernelName;
    string attr_kernel_name;
    (void)ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
    mix_aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  }
  return SUCCESS;
}

Status TaskBuilder::GenAutoAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ctx->mutable_aic_aiv_ctx();
  CM_CHECK_NOTNULL(aic_aiv_ctx_def);

  if (GenCommonAutoAICAIVCtxDef(op_desc, aic_aiv_ctx_def) != SUCCESS) {
    CM_LOGE("Generate common ctc for auto AIC/AIV task failed.");
    return FAILED;
  }

  string attr_key_kernel_name = kThreadKernelName;
  vector<string> thread_kernel_name;
  (void)ge::AttrUtils::GetListStr(op_desc, attr_key_kernel_name, thread_kernel_name);
  for (const auto &kernel_name : thread_kernel_name) {
    aic_aiv_ctx_def->add_kernel_name(kernel_name);
    CM_LOGD("auto kernel_name: %s.", kernel_name.c_str());
  }
  size_t args_num = auto_thread_param_offset_.first_thread_input_addrs.size() +
                    auto_thread_param_offset_.first_thread_output_addrs.size() +
                    auto_thread_param_offset_.thread_workspace_addrs[0].size();
  aic_aiv_ctx_def->set_task_param_ptr_offset(args_num * sizeof(uint64_t));

  CM_LOGD("aic_aiv_ctx_def FillContextData SUCCESS. Op:%s, optype:%s, def:%s.",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), aic_aiv_ctx_def->DebugString().c_str());
  return SUCCESS;
}

Status TaskBuilder::GenAutoMixAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def = ctx->mutable_mix_aic_aiv_ctx();
  CM_CHECK_NOTNULL(mix_aic_aiv_ctx_def);

  if (GenCommonAutoAICAIVCtxDef(op_desc, mix_aic_aiv_ctx_def) != SUCCESS) {
    CM_LOGE("Generate common ctc for auto AIC/AIV task failed.");
    return FAILED;
  }

  mix_aic_aiv_ctx_def->set_ns(1);

  vector<uint32_t> task_ratio_list;
  (void)ge::AttrUtils::GetListInt(op_desc, kThreadTaskRadio, task_ratio_list);
  if (task_ratio_list.size() > 1) {
    mix_aic_aiv_ctx_def->set_non_tail_block_ratio_n(task_ratio_list[0]);
    mix_aic_aiv_ctx_def->set_tail_block_ratio_n(task_ratio_list[1]);
  }

  // modeInArgsFirstField
  uint32_t mode = 0;
  (void)ge::AttrUtils::GetInt(op_desc, kModeInArgsFirstField, mode);
  // mode == 1 indicates we need reserve 8 Bytes for the args beginning
  if (mode == 1) {
    uint64_t modeArgs = 0;
    mix_aic_aiv_ctx_def->add_task_addr(modeArgs);
  }

  for (auto &prefix : kMixPrefixs) {
    string attr_key_kernel_name = prefix + kThreadKernelName;
    string attr_kernel_name;
    (void)ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
    mix_aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  }
  return SUCCESS;
}

void TaskBuilder::GenerateCMOTask(const ge::Node &node, std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                                  const bool pre_cmo_task) {
  CMOTaskBuilderPtr cmo_task_builder_ptr = nullptr;
  CM_MAKE_SHARED(cmo_task_builder_ptr = std::make_shared<CMOTaskBuilder>(), return);
  (void)cmo_task_builder_ptr->GenerateCMOTask(node, task_defs, stream_id, context_, pre_cmo_task);
}

Status TaskBuilder::GenMixL2CtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  CM_LOGD("Node[%s] fill Mixl2 context date.", op_desc->GetName().c_str());
  domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def = ctx->mutable_mix_aic_aiv_ctx();
  CM_CHECK_NOTNULL(mix_aic_aiv_ctx_def);

  uint32_t task_ratio;
  (void)ge::AttrUtils::GetInt(op_desc, kTaskRadio, task_ratio);
  mix_aic_aiv_ctx_def->set_tail_block_ratio_n(task_ratio);
  mix_aic_aiv_ctx_def->set_non_tail_block_ratio_n(task_ratio);
  mix_aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  mix_aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  mix_aic_aiv_ctx_def->set_ns(1);
  mix_aic_aiv_ctx_def->set_atm(kManualMode);
  mix_aic_aiv_ctx_def->set_thread_dim(1);
  vector<std::string> prefixs = { "_mix_aic", "_mix_aiv" };
  for (auto &prefix : prefixs) {
    string attr_key_kernel_name = prefix + op_desc->GetName() + kKernelName;
    string attr_kernel_name;
    (void)ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
    mix_aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  }
  int32_t block_dim = 0;
  (void)ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  mix_aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  mix_aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  if (OpTensorUtils::IsUnKnownShapeOp(*(op_desc.get()))) {
    CM_LOGD("Node[%s] is unknow shape.", op_desc->GetName().c_str());
    return SUCCESS;
  }

  for (auto input_addr : manual_thread_param_.input_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uintptr_t>(input_addr));
  }
  for (auto output_addr : manual_thread_param_.output_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uintptr_t>(output_addr));
  }
  for (auto workspace_addr : manual_thread_param_.workspace_addrs) {
    mix_aic_aiv_ctx_def->add_task_addr(reinterpret_cast<uintptr_t>(workspace_addr));
  }
  return SUCCESS;
}

Status TaskBuilder::GenDynamicAICAIVCtxDef(const ge::OpDescPtr &op_desc, FftsPlusCtxDefPtr ctx) {
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ctx->mutable_aic_aiv_ctx();
  CM_CHECK_NOTNULL(aic_aiv_ctx_def);
  aic_aiv_ctx_def->set_ns(1);
  aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  aic_aiv_ctx_def->set_aten(kAutoMode);
  aic_aiv_ctx_def->set_atm(kAutoMode);

  // generate _register_stub_func
  vector<string> unique_ids;
  string session_graph_id;
  if (ge::AttrUtils::GetStr(op_desc, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) && !session_graph_id.empty()) {
    unique_ids.push_back(session_graph_id + "_" + op_desc->GetName() + "_0");
  } else {
    unique_ids.push_back(op_desc->GetName() + "_0");
  }
  (void)ge::AttrUtils::SetListStr(op_desc, "_register_stub_func", unique_ids);
  CM_LOGD("FillContextData SUCCESS. Op:%s, type:%s.", op_desc->GetName().c_str(), op_desc->GetType().c_str());
  return SUCCESS;
}
}  // namespace fe
