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

#include "ops_kernel_builder/aicore_ops_kernel_builder.h"
#include "common/comm_log.h"
#include "common/constants_define.h"
#include "common/op_tensor_utils.h"
#include "graph/utils/attr_utils.h"
#include "common/aicore_util_attr_define.h"
#include "graph/debug/ge_attr_define.h"
#include "param_calculate/tensorsize_calculator.h"
#include "task_builder/task_builder.h"
#include "register/ops_kernel_builder_registry.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
namespace fe {
REGISTER_OPS_KERNEL_BUILDER(AI_CORE_NAME, AICoreOpsKernelBuilder);
REGISTER_OPS_KERNEL_BUILDER(VECTOR_CORE_NAME, AICoreOpsKernelBuilder);

AICoreOpsKernelBuilder::AICoreOpsKernelBuilder() {}

AICoreOpsKernelBuilder::~AICoreOpsKernelBuilder() {}

Status AICoreOpsKernelBuilder::Initialize(const std::map<std::string, std::string> &options) { return SUCCESS; }

Status AICoreOpsKernelBuilder::Finalize() { return SUCCESS; }

Status AICoreOpsKernelBuilder::CalcOpRunningParam(ge::Node &node) {
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  CM_LOGD("Begin to calculate input and output size of op [%s].", op_desc_ptr->GetName().c_str());

  if (TensorSizeCalculator::CalculateOpTensorSize(*(op_desc_ptr.get())) != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][CalcOpRunningParam][Node %s, type %s] Fail to calculate running parameters.",
                    op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    return FAILED;
  }

  if (node.GetOpDesc()->HasAttr(ge::ATTR_NAME_UNREGST_OPPATH)) {
    if (!ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, EN_IMPL_HW_TBE)) {
      REPORT_CM_ERROR("[GenTask][CalcOpRunningParam][Node %s, type %s] Fail to set fe_impl_type!",
                      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
      return FAILED;
    }
  }
  if (op_desc_ptr->GetType() == OP_TYPE_ROIPOOLING || op_desc_ptr->GetType() == OP_TYPE_SSD_DETECTION_OUTPUT) {
    bool has_reuse_mem_attr = true;
    (void)ge::AttrUtils::SetBool(op_desc_ptr, ATTR_NAME_NO_REUSE_MEM_FLAG, has_reuse_mem_attr);
    CM_LOGD("op:%s set no_reuse_mem_flag true.", op_desc_ptr->GetName().c_str());
  }

  return SUCCESS;
}

Status AICoreOpsKernelBuilder::GenerateTask(const ge::Node &node, ge::RunContext &context,
                                            std::vector<domi::TaskDef> &tasks) {
  CM_LOGD("Begin to generate task for node[%s, %s].", node.GetName().c_str(), node.GetType().c_str());
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  if (op_desc_ptr->HasAttr(kTypeFFTSPlus) || op_desc_ptr->HasAttr(ATTR_NAME_ALIAS_ENGINE_NAME)) {
    TaskBuilder task_builder;
    Status status = task_builder.GenerateFFTSPlusCtx(node, context);
    return status;
  }
  auto owner_graph = node.GetOwnerComputeGraph();
  CM_CHECK_NOTNULL(owner_graph);
  bool is_unknown_shape = owner_graph->GetGraphUnknownFlag();
  CM_LOGD("Graph[name:%s] is_unknown_shape flag is %d.", owner_graph->GetName().c_str(),
          is_unknown_shape);
  if (is_unknown_shape) {
    ge::NodePtr atomic_clean_node = nullptr;
    atomic_clean_node = op_desc_ptr->TryGetExtAttr(ATTR_NAME_ATOMIC_CLEAN_NODE_PTR, atomic_clean_node);
    if (atomic_clean_node != nullptr) {
      int64_t op_desc_id = op_desc_ptr->GetId();
      CM_LOGD("Op:%s op id is %ld", op_desc_ptr->GetName().c_str(), op_desc_id);
      atomic_clean_node->GetOpDesc()->SetId(op_desc_id);
      (void)ge::AttrUtils::SetInt(atomic_clean_node->GetOpDesc(), ATTR_NAME_IS_UNKNOWN_GRAPH, IS_UNKNOWN_SHAPE_VALUE);
      TaskBuilder atomic_task_builder;
      auto atomic_clean_context = context;
      if (atomic_task_builder.GenerateKernelTask(*atomic_clean_node, atomic_clean_context, tasks) == FAILED) {
        REPORT_CM_ERROR("[GenTask][AtomicGen][Node %s, type %s] generate task failed!",
                        atomic_clean_node->GetName().c_str(), atomic_clean_node->GetType().c_str());
        CM_LOGE("op:%s is atomic clean op, generate task failed", atomic_clean_node->GetName().c_str());
        return FAILED;
      }
    }
    (void)ge::AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_IS_UNKNOWN_GRAPH, IS_UNKNOWN_SHAPE_VALUE);
  }

  Status status;
  TaskBuilder task_builder;
  status = task_builder.GenerateKernelTask(node, context, tasks);

  return status;
}
}  // namespace fe
