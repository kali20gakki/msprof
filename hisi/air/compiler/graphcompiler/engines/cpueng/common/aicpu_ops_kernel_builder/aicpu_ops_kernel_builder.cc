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
#include "aicpu_ops_kernel_builder.h"

#include "common/util/error_manager/error_manager.h"
#include "config/config_file.h"
#include "engine/base_engine.h"
#include "error_code/error_code.h"
#include "ge/ge_api_types.h"
#include "kernel_builder.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"

using namespace std;
using namespace ge;

namespace aicpu {

Status AicpuOpsKernelBuilder::Initialize(const map<string, string> &options) {
  AICPUE_LOGI("Begin to initialize aicpu ops kernel builder.")
  // check options
  auto iter = options.find(ge::SOC_VERSION);

  AICPU_IF_BOOL_EXEC(iter == options.end(),
      AICPU_REPORT_INNER_ERROR(
          "cannot find [%s] in param of aicpu op builder initialize function.",
          ge::SOC_VERSION.c_str());
      return INPUT_PARAM_VALID)

  AICPU_CHECK_RES(Finalize())

  string kernel_builder_str;
  string kernel_builder_config = Stringcat(engine_name_, "KernelBuilder");
  if (!ConfigFile::GetInstance().GetValue(kernel_builder_config,
                                          kernel_builder_str)) {
    AICPU_REPORT_INNER_ERROR("[%s] not exist.", kernel_builder_config.c_str());
    return LOAD_KERNEL_BUILDER_FAILED;
  }

  vector<string> kernel_builders;
  ConfigFile::GetInstance().SplitValue(kernel_builder_str, kernel_builders);
  if (kernel_builders.empty()) {
    AICPU_REPORT_INNER_ERROR("[%s] is empty.", kernel_builder_str.c_str());
    return NONE_KERNEL_BUILDER;
  }
  for (auto kernel_builder : kernel_builders) {
    FACTORY_KERNEL_BUILDER::FactoryType kernel_builder_ptr =
        FACTORY_KERNEL_BUILDER::Produce(kernel_builder);
    if (kernel_builder_ptr == nullptr) {
      AICPU_REPORT_INNER_ERROR("create kernel builder[%s] failed",
          kernel_builder.c_str());
      return KERNEL_BUILDER_INSTANCE_FAILED;
    }
    kernel_builder_map_[kernel_builder] = kernel_builder_ptr;
    AICPU_CHECK_RES(kernel_builder_ptr->Initialize());
  }
  return SUCCESS;
}

Status AicpuOpsKernelBuilder::Finalize() {
  kernel_builder_map_.clear();
  return SUCCESS;
}

Status AicpuOpsKernelBuilder::CalcOpRunningParam(Node &node) {
  OpDescPtr op_desc_ptr = node.GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL)
  string op_type = op_desc_ptr->GetType();
  if (op_type == kFrameworkOp) {
    string original_type;
    AICPU_IF_BOOL_EXEC(
        (!AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type)),
        AICPU_REPORT_CALL_ERROR(
            "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
            kOriginalType.c_str(), node.GetName().c_str());
        return ErrorCode::GET_ORIGINAL_TYPE_FAILED)
    if (original_type.empty()) {
      AICPU_REPORT_INNER_ERROR("Attr[%s] is empty, op[%s].",
          kOriginalType.c_str(), node.GetName().c_str());
      return STR_IS_EMPTY;
    }
    op_desc_ptr->SetType(original_type);
    op_type = original_type;
  }
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));
  string kernel_name = all_op_info[op_type].opKernelLib;
  const KernelBuilderPtr &kernel_builder = GetKernelBuilderByName(kernel_name);

  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER)
  return kernel_builder->CalcOpRunningParam(node);
}

Status AicpuOpsKernelBuilder::GenerateTask(const Node &node,
                                           RunContext &context,
                                           vector<domi::TaskDef> &tasks) {
  OpDescPtr op_desc_ptr = node.GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL);

  // get original type
  string op_type = op_desc_ptr->GetType();
  if (op_type == kFrameworkOp) {
    string original_type;
    AICPU_IF_BOOL_EXEC(
        (!AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type)),
        AICPU_REPORT_CALL_ERROR(
            "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
            kOriginalType.c_str(), node.GetName().c_str());
        return ErrorCode::GET_ORIGINAL_TYPE_FAILED)
    op_type = original_type;
  }
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));
  string kernel_name = all_op_info[op_type].opKernelLib;
  const KernelBuilderPtr &kernel_builder = GetKernelBuilderByName(kernel_name);
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenerateTask(node, context, tasks);
}

Status AicpuOpsKernelBuilder::GenSingleOpRunTask(const NodePtr &node,
                                                 STR_FWK_OP_KERNEL &task,
                                                 string &task_info) {
  AICPU_CHECK_NOTNULL_ERRCODE(node, ErrorCode::INPUT_PARAM_NULL);
  OpDescPtr op_desc_ptr = node->GetOpDesc();
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, ErrorCode::INPUT_PARAM_NULL);
  string op_type = op_desc_ptr->GetType();
  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpsInfo(all_op_info));
  string kernel_name = all_op_info[op_type].opKernelLib;
  const KernelBuilderPtr &kernel_builder = GetKernelBuilderByName(kernel_name);
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenSingleOpRunTask(node, task, task_info);
}

Status AicpuOpsKernelBuilder::GenMemCopyTask(uint64_t count,
                                             STR_FWK_OP_KERNEL &task,
                                             string &task_info) {
  string kernel_name = "TFKernel";
  const KernelBuilderPtr &kernel_builder = GetKernelBuilderByName(kernel_name);
  AICPU_CHECK_NOTNULL_ERRCODE(kernel_builder, ErrorCode::NONE_KERNEL_BUILDER);
  return kernel_builder->GenMemCopyTask(count, task, task_info);
}

Status AicpuOpsKernelBuilder::GetOpsInfo(
    map<string, OpFullInfo> &all_op_info) const {
  FACTORY_ENGINE::FactoryType engine_ptr = FACTORY_ENGINE::Produce(engine_name_);
  AICPU_CHECK_NOTNULL(engine_ptr)
  AicpuOpsKernelInfoStorePtr aicpu_ops_kernel_info_store_ptr =
      engine_ptr->GetAicpuOpsKernelInfoStore();
  AICPU_CHECK_NOTNULL(aicpu_ops_kernel_info_store_ptr)
  aicpu_ops_kernel_info_store_ptr->GetAllOpsFullKernelInfo(all_op_info);
  return SUCCESS;
}

KernelBuilderPtr AicpuOpsKernelBuilder::GetKernelBuilderByName(
    const string &kernel_name) {
  unsigned int index = kernel_name.find("Kernel");
  if (index == kernel_name.npos) {
    AICPU_REPORT_INNER_ERROR("Wrong kernel_name[%s], should contain 'Kernel'.",
        kernel_name.c_str());
  }
  string kernel_builder_name = kernel_name.substr(0, index) + "Builder";
  if (kernel_builder_name == "CUSTAICPUBuilder") {
    kernel_builder_name = "AICPUBuilder";
  }
  if (kernel_builder_map_.find(kernel_builder_name) != kernel_builder_map_.end()) {
    return kernel_builder_map_[kernel_builder_name];
  }
  AICPU_REPORT_INNER_ERROR("kernel builder[%s] is not exist.",
      kernel_builder_name.c_str());
  return nullptr;
}
}  // namespace aicpu
