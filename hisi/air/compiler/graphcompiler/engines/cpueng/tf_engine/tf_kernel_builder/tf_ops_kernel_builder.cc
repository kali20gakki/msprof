/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "util/log.h"
#include "error_code/error_code.h"
#include "common/util/error_manager/error_manager.h"
#include "ge/ge_api_types.h"
#include "tf_kernel_builder/tf_ops_kernel_builder.h"
#include "register/ops_kernel_builder_registry.h"
#include "common/util/util.h"
#include "common/config/config_file.h"

namespace aicpu {
REGISTER_OPS_KERNEL_BUILDER(kTfOpsKernelInfo, AicpuTfOpsKernelBuilder);

ge::Status AicpuTfOpsKernelBuilder::Initialize(const std::map<std::string, std::string> &options) {
  AICPUE_LOGI("Begin to initialize aicpu ops kernel builder.")
  AICPU_CHECK_RES(Finalize())

  std::string kernel_builder_str = "TFBuilder";
  FACTORY_KERNEL_BUILDER::FactoryType kernel_builder_ptr = FACTORY_KERNEL_BUILDER::Produce(kernel_builder_str);
  if (kernel_builder_ptr == nullptr) {
    AICPU_REPORT_INNER_ERROR("[%s] instantiate failed", kernel_builder_str.c_str());
    return KERNEL_BUILDER_INSTANCE_FAILED;
  }
  kernel_builder_map_[kernel_builder_str] = kernel_builder_ptr;
  AICPU_CHECK_RES(kernel_builder_ptr->Initialize());
  return ge::SUCCESS;
}

ge::Status AicpuTfOpsKernelBuilder::Finalize() {
  kernel_builder_map_.clear();
  return ge::SUCCESS;
}

ge::Status AicpuTfOpsKernelBuilder::CalcOpRunningParam(ge::Node &node) {
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL)
  std::string op_type = op_desc_ptr->GetType();
  if (op_type == kFrameworkOp) {
    std::string original_type;
    AICPU_IF_BOOL_EXEC((!ge::AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type)),
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
          kOriginalType.c_str(), node.GetName().c_str())
      return ErrorCode::GET_ORIGINAL_TYPE_FAILED)
    if (original_type.empty()) {
      AICPU_REPORT_INNER_ERROR("Attr[%s] is empty, op[%s].",
          kOriginalType.c_str(), node.GetName().c_str());
      return STR_IS_EMPTY;
    }
    op_desc_ptr->SetType(original_type);
    op_type = original_type;
  }
  const KernelBuilderPtr &kernel_builder = kernel_builder_map_["TFBuilder"];
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER)
  return kernel_builder->CalcOpRunningParam(node);
}

ge::Status AicpuTfOpsKernelBuilder::GenerateTask(const ge::Node &node, ge::RunContext &context,
                                                 vector<domi::TaskDef> &tasks) {
  ge::OpDescPtr op_desc_ptr = node.GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL);
  // get original type
  std::string op_type = op_desc_ptr->GetType();
  if (op_type == kFrameworkOp) {
    std::string original_type;
    AICPU_IF_BOOL_EXEC((!ge::AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type)),
        AICPU_REPORT_CALL_ERROR(
            "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
            kOriginalType.c_str(), op_desc_ptr->GetName().c_str())
        return ErrorCode::GET_ORIGINAL_TYPE_FAILED)
    op_type = original_type;
  }
  const KernelBuilderPtr &kernel_builder = kernel_builder_map_["TFBuilder"];
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenerateTask(node, context, tasks);
}

ge::Status AicpuTfOpsKernelBuilder::GenSingleOpRunTask(const ge::NodePtr &node,
                                                       STR_FWK_OP_KERNEL &task,
                                                       std::string &task_info) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL);
  std::string op_type = op_desc_ptr->GetType();
  const KernelBuilderPtr &kernel_builder = kernel_builder_map_["TFBuilder"];
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenSingleOpRunTask(node, task, task_info);
}

ge::Status AicpuTfOpsKernelBuilder::GenMemCopyTask(uint64_t count,
                                                   STR_FWK_OP_KERNEL &task,
                                                   std::string &task_info) {
  const KernelBuilderPtr &kernel_builder = kernel_builder_map_["TFBuilder"];
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenMemCopyTask(count, task, task_info);
}
} // namespace aicpu