/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "hybrid/node_executor/host_cpu/host_cpu_node_executor.h"

#include "hybrid/model/hybrid_model.h"
#include "graph/def_types.h"
#include "graph/manager/graph_mem_manager.h"
#include "local_engine/engine/host_cpu_engine.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "mmpa/mmpa_api.h"

namespace ge {
namespace hybrid {
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::HOST_CPU, HostCpuNodeExecutor);

Status HostAicpuNodeTask::UpdateArgs(TaskContext &context) {
  if ((context.NumInputs() == 0) && (context.NumOutputs() == 0)) {
    GELOGD("Node[%s] has no input and output, no need to update args.", node_name_.c_str());
    return SUCCESS;
  }

  std::vector<uint64_t> io_addrs;
  io_addrs.reserve(static_cast<uint64_t>(context.NumInputs() + context.NumOutputs()));
  for (int32_t i = 0; i < context.NumInputs(); ++i) {
    const auto tensor = context.GetInput(i);
    GE_CHECK_NOTNULL(tensor);
    io_addrs.emplace_back(PtrToValue(tensor->GetData()));
  }

  for (int32_t i = 0; i < context.NumOutputs(); ++i) {
    const auto &output_desc = context.GetOutputDesc(i);
    GE_CHECK_NOTNULL(output_desc);
    AllocationAttr attr;
    attr.SetMemType(MemStorageType::HOST_DDR);
    if (context.AllocateOutput(i, *output_desc, nullptr, &attr) != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "node:%s(%s) Failed to allocate output %d",
                        context.GetNodeName(), context.GetNodeItem().NodeType().c_str(), i);
      GELOGE(FAILED, "[Allocate][Output] for node:%s(%s) failed, output idx:%d",
             context.GetNodeName(), context.GetNodeItem().NodeType().c_str(), i);
      return FAILED;
    }
    const auto tensor = context.GetOutput(i);
    GE_CHECK_NOTNULL(tensor);
    io_addrs.emplace_back(PtrToValue(tensor->GetData()));
  }
  const auto io_addr = PtrAdd<uint8_t>(args_.get(), static_cast<size_t>(args_size_), sizeof(aicpu::AicpuParamHead));

  // if has input and output, need copy to ioaddr
  const int32_t cpy_ret = memcpy_s(io_addr, static_cast<size_t>(args_size_ - sizeof(aicpu::AicpuParamHead)),
      &io_addrs[0UL], sizeof(uint64_t) * io_addrs.size());
  if (cpy_ret != EOK) {
    REPORT_INNER_ERROR("E19999", "Node[%s(%s)] memcpy io addr to AicpuParamHead failed,"
                       "ret=%d, args_size=%u, io nums=%zu.",
                       node_name_.c_str(), node_type_.c_str(), cpy_ret, args_size_, io_addrs.size());
    GELOGE(INTERNAL_ERROR, "[Update][IoAddr]Node[%s(%s)] memcpy io addr to AicpuParamHead failed,"
           "ret=%d, args_size=%u, io nums=%zu.",
           node_name_.c_str(), node_type_.c_str(), cpy_ret, args_size_, io_addrs.size());
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status HostAicpuNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start execute.", context.GetNodeName());
  GE_CHK_STATUS_RET(Execute(), "[Invoke][Execute] failed for node:%s(%s).",
                    node_name_.c_str(), node_type_.c_str());
  if (done_callback) {
    GELOGD("[%s] Start invoke callback.", context.GetNodeName());
    done_callback();
  }
  GELOGD("[%s] Done execute successfully.", context.GetNodeName());
  return SUCCESS;
}

Status HostAicpuNodeTask::Execute(void) const {
  GELOGD("Node[%s] launch task start.", node_name_.c_str());
  if (run_cpu_kernel_) {
    GE_CHK_STATUS_RET(run_cpu_kernel_(args_.get()), "[Run][CpuKernel] failed for node:%s(%s).",
                      node_name_.c_str(), node_type_.c_str());
  } else {
    REPORT_CALL_ERROR("E19999", "Run cpu kernel failed node:%s(%s), cpu kernel is not initialized.",
                      node_name_.c_str(), node_type_.c_str());
    GELOGE(INTERNAL_ERROR,
           "[Run][Kernel]Run cpu kernel failed node:%s(%s), cpu kernel is not initialized.",
           node_name_.c_str(), node_type_.c_str());
    return INTERNAL_ERROR;
  }

  GELOGD("Node[%s] launch task successfully.", node_name_.c_str());
  return SUCCESS;
}

Status HostAicpuNodeTask::SetHostExtInfo() const {
  if (aicpu_ext_handle_.GetExtInfoLen() == 0UL) {
    GELOGD("Node[%s] don't have ext info, no need update.", node_name_.c_str());
    return SUCCESS;
  }

  const auto aicpu_param_head = PtrToPtr<uint8_t, aicpu::AicpuParamHead>(args_.get());
  GE_CHECK_NOTNULL(aicpu_param_head);
  aicpu_param_head->extInfoLength = aicpu_ext_handle_.GetExtInfoLen();
  aicpu_param_head->extInfoAddr = PtrToValue(static_cast<void *>(aicpu_ext_handle_.GetExtInfo()));
  return SUCCESS;
}

Status HostCpuNodeExecutor::PrepareTask(NodeTask &task, TaskContext &context) const {
  return task.UpdateArgs(context);
}

Status HostCpuNodeExecutor::ValidateTaskDef(const domi::TaskDef &task_def) {
  const auto task_type = static_cast<rtModelTaskType_t>(task_def.type());
  if (task_type != RT_MODEL_TASK_KERNEL) {
    REPORT_CALL_ERROR("E19999", "[Check][TaskType]Invalid task type (%d) in host cpu excutor.",
                      static_cast<int32_t>(task_type));
    GELOGE(INTERNAL_ERROR,
           "[Check][TaskType]Invalid task type (%d) in host cpu excutor.", static_cast<int32_t>(task_type));
    return INTERNAL_ERROR;
  }
  const auto kernel_type = static_cast<ccKernelType>(task_def.kernel().context().kernel_type());
  if (kernel_type != ccKernelType::HOST_CPU) {
    REPORT_INNER_ERROR("E19999", "Invalid kernel type(%d) in host cpu excutor.",
                       static_cast<int32_t>(kernel_type));
    GELOGE(INTERNAL_ERROR,
           "[Check][TaskType]Invalid kernel type(%d) in host cpu excutor.", static_cast<int32_t>(kernel_type));
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status HostCpuNodeExecutor::LoadTask(const HybridModel &model, const NodePtr &node,
                                     std::shared_ptr<NodeTask> &task) const {
  GE_CHECK_NOTNULL(node);
  auto node_item = model.GetNodeItem(node);
  GE_CHECK_NOTNULL(node_item);
  const auto task_defs = model.GetTaskDefs(node);
  GE_CHECK_NOTNULL(task_defs);

  if ((*task_defs).size() != 1UL) {
    REPORT_CALL_ERROR("E19999", "Node[%s(%s)] task_def num[%zu] != 1",
                      node->GetName().c_str(), node->GetType().c_str(), (*task_defs).size());
    GELOGE(PARAM_INVALID, "[Check][Size] Node[%s(%s)] task_def num[%zu] != 1",
           node->GetName().c_str(), node->GetType().c_str(), (*task_defs).size());
    return PARAM_INVALID;
  }
  const auto &task_def = (*task_defs)[0UL];
  GE_CHK_STATUS_RET(ValidateTaskDef(task_def),
                    "[Validate][TaskDef] failed for Node[%s(%s)].",
                    node->GetName().c_str(), node->GetType().c_str());
  auto host_aicpu_task = MakeShared<HostAicpuNodeTask>(node_item, task_def);
  GE_CHK_BOOL_RET_STATUS(host_aicpu_task != nullptr, MEMALLOC_FAILED,
                         "[Create][HostAicpuNodeTask] Load task for node %s(%s) failed.",
                         node->GetName().c_str(), node->GetType().c_str());
  GE_CHK_STATUS_RET(host_aicpu_task->Init(model),
                    "[Init][HostAicpuNodeTask] failed for Node[%s(%s)].",
                    node->GetName().c_str(), node->GetType().c_str());
  GE_CHK_STATUS_RET(host_aicpu_task->SetHostExtInfo(),
                    "[Set][HostExtInfo] failed for Node[%s(%s)].", node->GetName().c_str(), node->GetType().c_str());

  const auto handle = HostCpuEngine::GetInstance().GetConstantFoldingHandle();
  if (handle == nullptr) {
    REPORT_CALL_ERROR("E19999", "Get constant folding handle failed.");
    GELOGE(INTERNAL_ERROR, "[Get][Handle]Get constant folding handle failed.");
    return INTERNAL_ERROR;
  }
  const auto run_cpu_kernel = reinterpret_cast<uint32_t (*)(void *)>(mmDlsym(handle, "RunHostCpuKernel"));
  if (run_cpu_kernel != nullptr) {
    host_aicpu_task->SetRunKernel(run_cpu_kernel);
  } else {
    REPORT_CALL_ERROR("E19999", "Get run cpu kernel failed.");
    GELOGE(INTERNAL_ERROR, "[Get][Kernel]Get run cpu kernel failed.");
    return INTERNAL_ERROR;
  }

  task = std::move(host_aicpu_task);
  GELOGD("Node[%s] load task end.", node->GetName().c_str());

  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
