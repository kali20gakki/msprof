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

#include "single_op/single_op_model.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "framework/common/debug/ge_log.h"
#include "framework/generator/ge_generator.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "runtime/rt.h"
#include "single_op/task/aicpu_task_builder.h"
#include "single_op/task/aicpu_kernel_task_builder.h"
#include "single_op/task/rts_kernel_task_builder.h"
#include "single_op/task/tbe_task_builder.h"
#include "hybrid/executor/hybrid_model_executor.h"
#include "hybrid/node_executor/node_executor.h"
#include "common/profiling_definitions.h"
#include "common/ge_inner_attrs.h"
#include "common/profiling/profiling_manager.h"
#include "common/utils/executor_utils.h"

namespace {
std::atomic<std::uint64_t> aicpu_kernel_id(0U);
}

namespace ge {
namespace {
const size_t kDataOutputNum = 1U;
const uint32_t kInputIndexOfData = 0U;
const uint32_t kOutputIndexOfData = 0U;
const size_t kNumTaskWithMemCpyTask = 2U;
const int64_t kMemTypeHost = 1;
const int64_t kMemTypeHostCompileIndependent = 2;

const std::set<std::string> kGeLocalTaskWithoutHybrid = {ge::DATA, ge::NETOUTPUT, ge::ANN_DATA, ge::AIPPDATA,
                                                         ge::CONSTANT, ge::CONSTANTOP, ge::VARIABLE, ge::VARIABLEV2};

Status CheckHostMem(const std::vector<std::string> &dependencies, const NodePtr &node, bool &is_host_mem) {
  const auto op_desc = node->GetOpDesc();
  for (const auto &input_name : dependencies) {
    const int32_t input_index = op_desc->GetInputIndexByName(input_name);
    if (input_index < 0) {
      GELOGE(INTERNAL_ERROR, "[Get][InputIndex]failed, node:[%s] inputname: %s.",
             node->GetName().c_str(), input_name.c_str());
      REPORT_CALL_ERROR("E19999", "GetInputIndexByName failed, node:[%s] inputname: %s.",
                        node->GetName().c_str(), input_name.c_str());
      return INTERNAL_ERROR;
    }

    const auto &src_node = NodeUtils::GetInDataNodeByIndex(*node, input_index);
    GE_CHECK_NOTNULL(src_node);
    const auto src_op_desc = src_node->GetOpDesc();
    GE_CHECK_NOTNULL(src_op_desc);
    if (src_op_desc->GetType() == DATA) {
      const auto tensor = src_op_desc->MutableInputDesc(kInputIndexOfData);
      GE_CHECK_NOTNULL(tensor);
      int64_t mem_type = 0;
      if (AttrUtils::GetInt(tensor, ATTR_NAME_PLACEMENT, mem_type) &&
          ((mem_type == kMemTypeHost) || (mem_type == kMemTypeHostCompileIndependent))) {
        GELOGD("Get hostmem from node %s, inputname: %s, mem_type = %ld.",
               src_node->GetName().c_str(), input_name.c_str(), mem_type);
        continue;
      }
    }
    is_host_mem = false;
    return SUCCESS;
  }
  is_host_mem = true;
  return SUCCESS;
}

Status CheckInferDepend(const GeModelPtr &ge_model, bool &is_infer_depend, bool &is_host_mem) {
  const auto comp_graph = GraphUtils::GetComputeGraph(ge_model->GetGraph());
  GE_CHECK_NOTNULL(comp_graph);
  for (const auto &node : comp_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    const auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    const auto &depends = op_desc->GetOpInferDepends();
    bool support_dynamic_shape = false;
    (void)AttrUtils::GetBool(op_desc, kAttrSupportDynamicShape, support_dynamic_shape);
    if ((!depends.empty()) && support_dynamic_shape) {
      is_infer_depend = true;
      const auto ret = CheckHostMem(depends, node, is_host_mem);
      return ret;
    }
  }
  return SUCCESS;
}

Status CheckGeLocalNeedHybrid(const GeModelPtr &ge_model, bool &is_ge_local_need_hybrid) {
  const auto comp_graph = GraphUtils::GetComputeGraph(ge_model->GetGraph());
  GE_CHECK_NOTNULL(comp_graph);
  for (const auto &node : comp_graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    const auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    const auto &lib_name = op_desc->GetOpKernelLibName();
    const auto &op_type = op_desc->GetType();
    GELOGD("op name is %s, op kernel name is %s, op type is %s", op_desc->GetName().c_str(),
        lib_name.c_str(), op_type.c_str());
    if ((lib_name == kEngineNameGeLocal) &&
        (kGeLocalTaskWithoutHybrid.find(op_type) == kGeLocalTaskWithoutHybrid.end())) {
      GELOGD("op name is %s, use GE local task with hybrid execute", op_desc->GetName().c_str());
      is_ge_local_need_hybrid = true;
      return SUCCESS;
    }
  }
  return SUCCESS;
}

Status GetAicoreTask(const std::vector<domi::TaskDef> &task_defs, std::vector<domi::TaskDef> &aicore_task_defs) {
  for (size_t i = 0UL; i < task_defs.size(); ++i) {
    const domi::TaskDef &task_def = task_defs[i];
    const auto task_type = static_cast<rtModelTaskType_t>(task_def.type());
    if ((task_type == RT_MODEL_TASK_KERNEL) || (task_type == RT_MODEL_TASK_ALL_KERNEL)) {
      const auto &context = (task_type == RT_MODEL_TASK_KERNEL) ? task_def.kernel().context() :
                                                                  task_def.kernel_with_handle().context();
      const auto kernel_type = static_cast<ccKernelType>(context.kernel_type());
      if (kernel_type == ccKernelType::TE) {
        aicore_task_defs.emplace_back(task_def);
      }
    }
  }
  if (aicore_task_defs.empty()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Size]Node size must larger then 0, but get %zu.",
           aicore_task_defs.size());
    REPORT_INNER_ERROR("E19999", "[Check][Size]task_defs size must larger then 0, but get %zu.",
                       aicore_task_defs.size());
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  return SUCCESS;
}
}  // namespace

SingleOpModel::SingleOpModel(const std::string &model_name, const void *const model_data, const uint32_t model_size)
    : model_name_(model_name), ori_model_data_(model_data), ori_model_size_(model_size) {}

Status SingleOpModel::Init() {
  GE_CHK_STATUS_RET_NOLOG(InitModel());
  return LoadAllNodes();
}

Status SingleOpModel::InitModel() {
  ge::ModelData model;
  model.model_len = ori_model_size_;
  model.model_data = ValueToPtr(PtrToValue(ori_model_data_));

  const auto ret = model_helper_.LoadModel(model);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] failed.");
    REPORT_CALL_ERROR("E19999", "InitModel fail for ModelHelper LoadModel failed.");
    return ret;
  }

  return SUCCESS;
}

void SingleOpModel::ParseOpModelParams(ModelHelper &model_helper, SingleOpModelParam &param) {
  int64_t value = 0;
  bool ret = false;
  const std::shared_ptr<GeModel> model = model_helper.GetGeModel();
  GE_CHECK_NOTNULL_JUST_RETURN(model);
  ret = AttrUtils::GetInt(model, ATTR_MODEL_MEMORY_SIZE, value);
  param.memory_size = ret ? static_cast<uint64_t>(value) : 0U;
  ret = AttrUtils::GetInt(model, ATTR_MODEL_ZERO_COPY_MEMORY_SIZE, value);
  param.zero_copy_mem_size = ret ? static_cast<uint64_t>(value) : 0U;
  ret = AttrUtils::GetInt(model, ATTR_MODEL_WEIGHT_SIZE, value);
  param.weight_size = ret ? static_cast<uint64_t>(value) : 0U;
  ret = AttrUtils::GetInt(model, MODEL_ATTR_TASK_GEN_BASE_ADDR, value);
  param.base_addr = ret ? static_cast<uint64_t>(value) : 0U;
  ret = AttrUtils::GetInt(model, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, value);
  param.weight_addr = ret ? static_cast<uint64_t>(value) : 0U;
  ret = AttrUtils::GetInt(model, ATTR_MODEL_CORE_TYPE, value);
  param.core_type = ret ? value : 0;

  GELOGI("ParseOpModelParams(), total_memory_size:%lu, zero_copy_size:%lu, weight_size:%lu, core_type = %ld",
         param.memory_size, param.zero_copy_mem_size, param.weight_size, param.core_type);
}

Status SingleOpModel::InitOverflowAddr(StreamResource &resource) {
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  const Graph graph = ge_model->GetGraph();
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHECK_NOTNULL(compute_graph);

  int64_t global_workpace_size = 0;
  (void)AttrUtils::GetInt(compute_graph, "globleworkspace_status_bytes", global_workpace_size);
  if (global_workpace_size > 0) {
    if (resource.GetOverflowAddr() == nullptr) {
      GE_CHK_STATUS_RET(resource.MallocOverflowMemory("overflow_memory", global_workpace_size),
                        "[Malloc][OverflowMemor]failed.");
    } else if (global_workpace_size != resource.GetOverflowSize()) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID, "To Malloc attr_memory size inconsistency, size = %zu, previous_size = %zu.",
             global_workpace_size, resource.GetOverflowSize());
      REPORT_CALL_ERROR("E19999", "attr_memory size inconsistency, size = %zu, previous_size = %zu.",
          global_workpace_size, resource.GetOverflowSize());
      return ACL_ERROR_GE_PARAM_INVALID;
    } else {
      // no opeartion
    }
  }
  return SUCCESS;
}

Status SingleOpModel::InitModelMem(StreamResource &resource) {
  GE_CHK_STATUS_RET(InitOverflowAddr(resource), "[Init][OverflowAddr] failed.");
  ParseOpModelParams(model_helper_, model_params_);

  if (model_params_.memory_size > model_params_.zero_copy_mem_size) {
    const std::string purpose("malloc feature map memory on model execute.");
    GELOGI("total memory: %lu, zero_copy_mem: %lu", model_params_.memory_size, model_params_.zero_copy_mem_size);
    model_params_.mem_base =
        resource.MallocMemory(purpose, model_params_.memory_size - model_params_.zero_copy_mem_size, false);
    if (model_params_.mem_base == nullptr) {
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }
  }

  if ((model_params_.weight_size > 0U) && has_weight_) {
    const std::string purpose("malloc weights memory on model execute.");
    model_params_.weight_base = resource.MallocWeight(purpose, model_params_.weight_size);
    if (model_params_.weight_base == nullptr) {
      // no need to free memory, for that was handled by StreamResources
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }

    auto weight_buffer = model_helper_.GetGeModel()->GetWeight();
    GELOGI("To copy weight to device. weight size = %zu", weight_buffer.GetSize());
    GE_CHK_RT_RET(rtMemcpy(model_params_.weight_base,
                           model_params_.weight_size,
                           weight_buffer.GetData(),
                           weight_buffer.GetSize(),
                           RT_MEMCPY_HOST_TO_DEVICE));
  }

  return SUCCESS;
}

Status SingleOpModel::ParseInputNode(const OpDescPtr &op_desc) {
  const std::vector<int64_t> offsets = op_desc->GetOutputOffset();
  if (offsets.size() != kDataOutputNum) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Parse][InputNode]Data op should have only one output, but got %zu, op_name:%s, op_type:%s.",
           op_desc->GetOutputOffset().size(), op_desc->GetName().c_str(), op_desc->GetType().c_str());
    REPORT_INNER_ERROR("E19999", "ParseInputNode fail for Data op should have only one output, but got %zu,"
                       "op_name:%s, op_type:%s.", op_desc->GetOutputOffset().size(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  const auto output_desc = op_desc->GetOutputDescPtr(0U);
  GE_CHECK_NOTNULL(output_desc);
  int64_t tensor_size = 0;
  (void)TensorUtils::GetSize(*output_desc, tensor_size);
  input_offset_list_.emplace_back(offsets[0U]);
  input_sizes_.emplace_back(tensor_size);
  GELOGI("[%s] parse input node: %s, size = %ld, offset = %ld", model_name_.c_str(), op_desc->GetName().c_str(),
         tensor_size, offsets[0U]);
  return SUCCESS;
}

void SingleOpModel::ParseOutputNode(const OpDescPtr &op_desc) {
  const std::vector<int64_t> offsets = op_desc->GetInputOffset();
  for (uint32_t k = 0U; k < static_cast<uint32_t>(offsets.size()); ++k) {
    const auto input_desc = op_desc->GetInputDescPtr(k);
    if (input_desc == nullptr) {
      continue;
    }
    int64_t tensor_size = 0;
    (void)TensorUtils::GetSize(*input_desc, tensor_size);
    output_offset_list_.emplace_back(offsets[static_cast<size_t>(k)]);
    output_sizes_.emplace_back(tensor_size);
    GELOGI("[%s] parse output node: %s, size = %ld, offset = %u", model_name_.c_str(), op_desc->GetName().c_str(),
           tensor_size, static_cast<uint32_t>(offsets[static_cast<size_t>(k)]));
  }
}

Status SingleOpModel::LoadAtomicWorkspace(const OpDescPtr &op_desc) const {
  GeAttrValue::NAMED_ATTRS workspaces;
  const bool ret = AttrUtils::GetNamedAttrs(op_desc, EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspaces);
  if (!ret) {
    return SUCCESS;
  }
  std::vector<int64_t> value;
  const std::string &op_name = op_desc->GetName();
  (void)AttrUtils::GetListInt(workspaces, op_name, value);
  if (value.empty()) {
    return SUCCESS;
  }
  std::map<std::string, std::map<int64_t, int64_t>> workspace_info = { {op_name, std::map<int64_t, int64_t>()} };
  if (value.size() >= 1U) {
    std::map<int64_t, int64_t> &index_offset = workspace_info[op_name];
    for (size_t i = 0U; i < (value.size() - 1U); i += 2U) { // two sets of vector, parsing the key value of the map
      index_offset[value[i]] = value[i + 1U];
    }
  }
  if (!op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info)) {
    GELOGE(INTERNAL_ERROR, "[Set][Attr:%s]fail for node:%s.",
           EXT_ATTR_ATOMIC_WORKSPACE_INFO.c_str(), op_desc->GetName().c_str());
    REPORT_INNER_ERROR("E19999", "Set Attr:%s fail for node:%s.",
                       EXT_ATTR_ATOMIC_WORKSPACE_INFO.c_str(), op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status SingleOpModel::LoadAllNodes() {
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  const Graph graph = ge_model->GetGraph();
  model_id_ = ge_model->GetModelId();
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  if (compute_graph == nullptr) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Get][ComputeGraph] fail, model_name:%s.", model_name_.c_str());
    REPORT_CALL_ERROR("E19999", "LoadAllNodes fail for GetComputeGraph return nullptr, model_name:%s.",
        model_name_.c_str());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  const auto nodes = compute_graph->GetDirectNode();
  GELOGI("[%s] node size = %zu", model_name_.c_str(), nodes.size());

  for (const auto &node : nodes) {
    auto op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    op_list_[static_cast<size_t>(op_desc->GetId())] = node;
    const auto op_type = op_desc->GetType();
    GELOGI("[%s] node = %s, type = %s", model_name_.c_str(), node->GetName().c_str(), op_type.c_str());

    if ((op_type == DATA_TYPE) || (op_type == AIPP_DATA_TYPE)) {
      data_ops_.emplace_back(op_desc);
      const auto tensor = op_desc->MutableInputDesc(0U);
      int64_t mem_type = 0;
      if (AttrUtils::GetInt(tensor, ATTR_NAME_PLACEMENT, mem_type) &&
          ((mem_type == kMemTypeHost) || (mem_type == kMemTypeHostCompileIndependent))) {
        int32_t index = 0;
        (void)AttrUtils::GetInt(op_desc, ATTR_NAME_INDEX, index);
        GELOGD("Node %s, index %d, has host mem, mem_type = %ld.", node->GetName().c_str(), index, mem_type);
        op_with_hostmem_[index] = node;
      }
      continue;
    }

    if ((op_type == CONSTANT) || (op_type == CONSTANTOP)) {
      has_weight_ = true;
      continue;
    }

    if (op_type == NETOUTPUT) {
      netoutput_op_ = op_desc;
      continue;
    }

    ge_model->GetTBEKernelStore().LoadTBEKernelBinToOpDesc(op_desc);
    ge_model->GetCustAICPUKernelStore().LoadCustAICPUKernelBinToOpDesc(op_desc);
    GE_CHK_STATUS_RET(LoadAtomicWorkspace(op_desc),
                      "[LoadAtomicWorkSpace]failed for [%s(%s)].",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
  }

  return SUCCESS;
}

Status SingleOpModel::ParseInputsAndOutputs() {
  for (auto &op_desc : data_ops_) {
    GE_CHK_STATUS_RET_NOLOG(ParseInputNode(op_desc));
  }
  if (netoutput_op_ != nullptr) {
    ParseOutputNode(netoutput_op_);
  }
  return SUCCESS;
}

Status SingleOpModel::SetInputsAndOutputs(SingleOp &single_op) {
  size_t arg_index = 0U;
  for (size_t i = 0UL; i < input_offset_list_.size(); ++i) {
    uint8_t *addr = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(model_params_.mem_base) +
                                                       static_cast<uint64_t>(input_offset_list_[i])));
    (void)model_params_.addr_mapping_.emplace(PtrToValue(addr), arg_index++);
    single_op.input_sizes_.emplace_back(input_sizes_[i]);
    single_op.input_addr_list_.emplace_back(addr);
  }

  for (size_t i = 0UL; i < output_offset_list_.size(); ++i) {
    uint8_t *addr = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(model_params_.mem_base) +
                                                       static_cast<uint64_t>(output_offset_list_[i])));
    (void)model_params_.addr_mapping_.emplace(PtrToValue(addr), arg_index++);
    single_op.output_sizes_.emplace_back(output_sizes_[i]);
    single_op.output_addr_list_.emplace_back(addr);
  }

  single_op.args_.resize(arg_index);
  return SUCCESS;
}

Status SingleOpModel::BuildTaskList(StreamResource *const stream_resource, SingleOp &single_op) {
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  single_op.arg_table_.resize(single_op.input_sizes_.size() + single_op.output_sizes_.size());
  const auto &tasks = ge_model->GetModelTaskDefPtr()->task();
  for (int32_t i = 0; i < tasks.size(); ++i) {
    const domi::TaskDef &task_def = tasks[i];
    GELOGI("[%s] Task[%d], type = %u, DebugString = %s", model_name_.c_str(), i, task_def.type(),
           task_def.DebugString().c_str());
    const auto task_type = static_cast<rtModelTaskType_t>(task_def.type());
    if ((task_type == RT_MODEL_TASK_KERNEL) || (task_type == RT_MODEL_TASK_ALL_KERNEL)) {
      const auto &context = (task_type == RT_MODEL_TASK_KERNEL) ? task_def.kernel().context() :
                                                                  task_def.kernel_with_handle().context();
      const auto kernel_type = static_cast<ccKernelType>(context.kernel_type());
      if (kernel_type == ccKernelType::TE) {
        GELOGD("Building TBE task");
        TbeOpTask *tbe_task = nullptr;
        const auto ret = BuildKernelTask(task_def, &tbe_task, stream_resource->GetOverflowAddr());
        if (ret != SUCCESS) {
          return ret;
        }

        ParseArgTable(tbe_task, single_op);
        tbe_task->SetModelArgs(model_name_, model_id_);
        tbe_task->stream_resource_ = tbe_task->need_tiling_ ? stream_resource : nullptr;
        single_op.tasks_.emplace_back(tbe_task);
      } else if ((kernel_type == ccKernelType::AI_CPU) || (kernel_type == ccKernelType::CUST_AI_CPU)) {
        GELOGD("Building AICPU_CC task");
        AiCpuCCTask *task = nullptr;
        const uint64_t singleop_kernel_id = aicpu_kernel_id++;
        GELOGI("Build singleOp CCTask, kernel_id = %lu", singleop_kernel_id);
        GE_CHK_STATUS_RET_NOLOG(BuildCpuKernelTask(task_def.kernel(), &task, singleop_kernel_id));
        task->SetModelArgs(model_name_, model_id_);
        ParseArgTable(task, single_op);
        single_op.tasks_.emplace_back(task);
      } else {
        GELOGE(ACL_ERROR_GE_OP_KERNEL_TYPE_INVALID,
            "[Check][KernelType]Only TBE, AI_CPU, CUST_AI_CPU kernel are supported, but got %u",
            context.kernel_type());
        REPORT_INNER_ERROR("E19999",
            "BuildTaskList fail for %u not supported, Only TBE, AI_CPU, CUST_AI_CPU kernel are supported.",
            context.kernel_type());
        return ACL_ERROR_GE_OP_KERNEL_TYPE_INVALID;
      }
    } else if (task_type == RT_MODEL_TASK_KERNEL_EX) {
      GELOGD("Building AICPU_TF task");
      AiCpuTask *aicpu_task = nullptr;
      const uint64_t singleop_kernel_id = aicpu_kernel_id++;
      GELOGI("Build singleOp TfTask, kernel_id = %lu", singleop_kernel_id);
      GE_CHK_STATUS_RET_NOLOG(
          BuildKernelExTask(task_def.kernel_ex(), &aicpu_task, singleop_kernel_id));
      aicpu_task->SetModelArgs(model_name_, model_id_);
      ParseArgTable(aicpu_task, single_op);
      single_op.tasks_.emplace_back(aicpu_task);
    } else if ((task_type == RT_MODEL_TASK_MEMCPY_ASYNC) || (task_type == RT_MODEL_TASK_MEMCPY_ADDR_ASYNC)) {
      const auto kernel_def = task_def.memcpy_async();
      const auto node = op_list_[kernel_def.op_index()];
      GE_CHECK_NOTNULL(node);
      const auto op_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      std::unique_ptr<MemcpyAsyncTask> task;
      GE_CHK_STATUS_RET_NOLOG(RtsKernelTaskBuilder::BuildMemcpyAsyncTask(op_desc, kernel_def, model_params_, task));
      task->SetModelArgs(model_name_, model_id_);
      ParseArgTable(task.get(), single_op);
      single_op.tasks_.emplace_back(task.release());
    } else {
      // skip
      GELOGD("Skip task type: %d", static_cast<int32_t>(task_type));
    }
  }
  return SUCCESS;
}

void SingleOpModel::ParseArgTable(OpTask *const task, SingleOp &op) {
  if (task == nullptr) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Parse][ArgTable] fail for input OpTask is nullptr.");
    REPORT_INNER_ERROR("E19999", "ParseArgTable fail for input OpTask is nullptr.");
    return;
  }

  // args: addr1, addr2, addr3 ...
  uintptr_t *arg_base = nullptr;
  size_t arg_num = 0U;
  task->GetIoAddr(arg_base, arg_num);
  const auto ptr_size = sizeof(uintptr_t);
  for (size_t i = 0U; i < arg_num; ++i) {
    uintptr_t *ptr_to_addr = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(arg_base) +
                                                                  (ptr_size * i)));
    const uintptr_t addr = *ptr_to_addr;
    const std::map<uintptr_t, int32_t>::const_iterator iter = model_params_.addr_mapping_.find(addr);
    if (iter != model_params_.addr_mapping_.cend()) {
      const int32_t arg_index = iter->second;
      GELOGI("%s args[%zu] mapped to user designated args[%d]", task->GetOpdesc()->GetName().c_str(), i, arg_index);
      op.arg_table_[static_cast<size_t>(iter->second)].emplace_back(ptr_to_addr);
    }
  }
}

Status SingleOpModel::BuildKernelTask(const domi::TaskDef &task_def, TbeOpTask **const task,
                                      void *const overflow_addr) {
  GE_CHECK_NOTNULL(task);
  const auto task_type = static_cast<rtModelTaskType_t>(task_def.type());
  const auto &context = (task_type == RT_MODEL_TASK_KERNEL) ? task_def.kernel().context() :
                                                              task_def.kernel_with_handle().context();
  const std::map<uint32_t, NodePtr>::const_iterator iter = op_list_.find(context.op_index());
  if (iter == op_list_.cend()) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Check][Param:TaskDef]op desc not found. op index = %u", context.op_index());
    REPORT_INNER_ERROR("E19999", "BuildKernelTask fail for op desc not found. op index = %u", context.op_index());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }

  std::unique_ptr<TbeOpTask> tbe_task = MakeUnique<TbeOpTask>(iter->second);
  if (tbe_task == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][TbeOpTask]failed.");
    REPORT_INNER_ERROR("E19999", "BuildKernelTask fail for new TbeOpTask.");
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  SetHostMemInputFlagToTask(iter->second->GetOpDesc(), *tbe_task);
  tbe_task->SetOverflowAddr(overflow_addr);
  auto builder = TbeTaskBuilder(model_name_, iter->second, task_def);
  const auto ret = builder.BuildTask(*tbe_task, model_params_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][TbeOpTask]failed.");
    REPORT_INNER_ERROR("E19999", "[Build][TbeOpTask]failed.");
    return ret;
  }

  *task = tbe_task.release();
  return SUCCESS;
}

Status SingleOpModel::BuildAtomicTask(const domi::TaskDef &task_def, AtomicAddrCleanOpTask **const task) {
  GE_CHECK_NOTNULL(task);
  const auto &context = task_def.kernel().context();
  const std::map<uint32_t, NodePtr>::const_iterator iter = op_list_.find(context.op_index());
  if (iter == op_list_.cend()) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Check][Param:TaskDef]op desc not found. op index = %u", context.op_index());
    REPORT_INNER_ERROR("E19999", "BuildKernelTask fail for op desc not found. op index = %u", context.op_index());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }

  std::unique_ptr<AtomicAddrCleanOpTask> atomic_task = MakeUnique<AtomicAddrCleanOpTask>();
  if (atomic_task == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][AtomicAddrCleanOpTask]failed.");
    REPORT_INNER_ERROR("E19999", "BuildKernelTask fail for new AtomicAddrCleanOpTask.");
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  auto builder = AtomicAddrCleanTaskBuilder(model_name_, iter->second, task_def);
  const auto ret = builder.BuildTask(*atomic_task, model_params_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][AtomicAddrCleanOpTask]failed.");
    REPORT_INNER_ERROR("E19999", "[Build][AtomicAddrCleanOpTask]failed.");
    return ret;
  }

  *task = atomic_task.release();
  return SUCCESS;
}

Status SingleOpModel::BuildKernelExTask(const domi::KernelExDef &kernel_def, AiCpuTask **const task,
                                        const uint64_t kernel_id) {
  const std::map<uint32_t, NodePtr>::const_iterator iter = op_list_.find(kernel_def.op_index());
  if (iter == op_list_.cend()) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR,
        "[Check][Param:KernelExDef]op not found. op index = %u", kernel_def.op_index());
    REPORT_INNER_ERROR("E19999",
        "BuildKernelExTask fail for param kernel_def, because op of kernel_def not found, op index:%u.",
        kernel_def.op_index());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }

  std::unique_ptr<AiCpuTask> aicpu_task = MakeUnique<AiCpuTask>();
  if (aicpu_task == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][AiCpuTask] failed.");
    REPORT_INNER_ERROR("E19999", "BuildKernelExTask fail for new AiCpuTask, model_name:%s.", model_name_.c_str());
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }
  SetHostMemInputFlagToTask(iter->second->GetOpDesc(), *aicpu_task);
  const auto builder = AiCpuTaskBuilder(iter->second->GetOpDesc(), kernel_def);
  const auto ret = builder.BuildTask(*aicpu_task, model_params_, kernel_id);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][Task] failed, kernel_id:%lu.", kernel_id);
    return ret;
  }

  *task = aicpu_task.release();
  return SUCCESS;
}

Status SingleOpModel::BuildCpuKernelTask(const domi::KernelDef &kernel_def, AiCpuCCTask **const task,
                                         const uint64_t kernel_id) {
  const auto &context = kernel_def.context();
  const std::map<uint32_t, NodePtr>::const_iterator iter = op_list_.find(context.op_index());
  if (iter == op_list_.cend()) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR,
        "[Check][Param:KernelDef] op desc not found. op index = %u", context.op_index());
    REPORT_INNER_ERROR("E19999",
        "BuildCpuKernelTask fail for kernel_def is invalid, because op of kernel_def not found, op index:%u.",
        context.op_index());
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  std::unique_ptr<AiCpuCCTask> aicpucc_task = MakeUnique<AiCpuCCTask>();
  if (aicpucc_task == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Create][AiCpuCCTask] failed");
    REPORT_INNER_ERROR("E19999", "BuildCpuKernelTask fail for new AiCpuCCTask, model_name:%s.", model_name_.c_str());
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  SetHostMemInputFlagToTask(iter->second->GetOpDesc(), *aicpucc_task);
  const auto builder = AiCpuCCTaskBuilder(iter->second->GetOpDesc(), kernel_def);
  const auto ret = builder.BuildTask(*aicpucc_task, kernel_id, model_params_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Build][AiCpuCCTask]failed, kernel_id:%lu.", kernel_id);
    REPORT_CALL_ERROR("E19999", "BuildCpuKernelTask fail for build AiCpuTask, kernel_id:%lu.", kernel_id);
    return ret;
  }
  *task = aicpucc_task.release();
  return SUCCESS;
}

Status SingleOpModel::InitHybridModelExecutor(const StreamResource &resource, const GeModelPtr &ge_model,
                                              SingleOp &single_op) {
  for (const auto &op_desc : data_ops_) {
    const auto output_tensor_desc = op_desc->GetOutputDesc(kOutputIndexOfData);
    GeTensorDesc tensor_desc(output_tensor_desc);
    single_op.inputs_desc_.emplace_back(tensor_desc);
    GELOGD("Init inputs desc from %s.", op_desc->GetName().c_str());
  }
  GE_CHK_STATUS(SetHostMemNode(single_op.node_with_hostmem_), "[Init][HostMem]Failed.");
  GE_CHK_STATUS_RET_NOLOG(hybrid::NodeExecutorManager::GetInstance().EnsureInitialized());
  const auto root_model = model_helper_.GetGeRootModel();
  GE_CHECK_NOTNULL(root_model);
  root_model->SetRootGraph(GraphUtils::GetComputeGraph(ge_model->GetGraph()));
  root_model->SetSubgraphInstanceNameToModel(root_model->GetRootGraph()->GetName(), ge_model);
  single_op.hybrid_model_ = MakeUnique<hybrid::HybridModel>(root_model);
  GE_CHECK_NOTNULL(single_op.hybrid_model_);
  GE_CHK_STATUS_RET(single_op.hybrid_model_->Init(true), "[Init][HybridModel]Failed.");
  int32_t device_id = 0;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  single_op.hybrid_model_executor_ = MakeUnique<hybrid::HybridModelExecutor>(single_op.hybrid_model_.get(),
                                                                             device_id,
                                                                             resource.GetStream());
  GE_CHECK_NOTNULL(single_op.hybrid_model_executor_);
  GE_CHK_STATUS_RET(single_op.hybrid_model_executor_->Init(), "[Init][HybridModelExecutor]Failed.");
  return SUCCESS;
}

Status SingleOpModel::BuildOp(StreamResource &resource, SingleOp &single_op) {
  GE_CHK_STATUS_RET_NOLOG(ParseInputsAndOutputs());
  GE_CHK_STATUS_RET_NOLOG(InitModelMem(resource));
  single_op.running_param_ = MakeUnique<SingleOpModelParam>(model_params_);
  GE_CHECK_NOTNULL(single_op.running_param_);
  GE_CHK_STATUS_RET_NOLOG(SetInputsAndOutputs(single_op));
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  std::string single_op_type;
  if (AttrUtils::GetStr(ge_model, kAttrNameSingleOpType, single_op_type)) {
    ProfilingManager::Instance().RegisterElement(single_op.profiling_node_type_index_, single_op_type);
  } else {
    single_op.profiling_node_type_index_ = -1;
    GELOGW("Can not find single op type from GeModel");
  }

  bool infer_depend_flag = false;
  bool is_host_mem = false;
  GE_CHK_STATUS_RET(CheckInferDepend(ge_model, infer_depend_flag, is_host_mem), "[Check][InferDepend] failed.");
  if (infer_depend_flag) {
    // construct single_op, do single op with HybridModelExecutor
    GELOGD("Init hybrid model params of single op, and will do execute with hybrid model executor.");
    return InitHybridModelExecutor(resource, ge_model, single_op);
  }
  return BuildTaskList(&resource, single_op);
}

Status SingleOpModel::BuildTaskListForDynamicOp(StreamResource *const stream_resource, DynamicSingleOp &single_op) {
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);

  const auto compute_graph = GraphUtils::GetComputeGraph(ge_model->GetGraph());
  GE_CHECK_NOTNULL(compute_graph);
  single_op.compute_graph_ = compute_graph;

  if (node_tasks_.size() != 1U) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Size]Node size must be 1, but get %zu.", node_tasks_.size());
    REPORT_INNER_ERROR("E19999", "[Check][Size]Node size must be 1, but get %zu.", node_tasks_.size());
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  const auto iter = node_tasks_.cbegin();
  const auto node = iter->first;
  const auto &task_defs = iter->second;
  if (task_defs.size() <= 0U) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Size]Node size must larger then 0, but get %zu.", task_defs.size());
    REPORT_INNER_ERROR("E19999", "[Check][Size]task_defs size must larger then 0, but get %zu.", task_defs.size());
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  GE_CHECK_NOTNULL(node);
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    GE_CHECK_NOTNULL(in_data_anchor);
    const auto out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    if (out_data_anchor == nullptr) {
      continue;
    }
    const auto peer_node = out_data_anchor->GetOwnerNode();
    GE_CHECK_NOTNULL(peer_node);
    single_op.input_node_anchor_map_[in_data_anchor->GetIdx()] =
        {peer_node->GetOpDesc()->GetId(), out_data_anchor->GetIdx()};
  }

  const auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const auto &lib_name = op_desc->GetOpKernelLibName();
  if (lib_name == kEngineNameAiCore) {
    GELOGD("Building TBE task.");
    std::vector<domi::TaskDef> aicore_task_defs;
    GE_CHK_STATUS_RET_NOLOG(GetAicoreTask(task_defs, aicore_task_defs));
    const auto &task_def = aicore_task_defs.back();
    TbeOpTask *tbe_task = nullptr;
    GE_CHK_STATUS_RET_NOLOG(BuildKernelTask(task_def, &tbe_task, stream_resource->GetOverflowAddr()));
    tbe_task->SetModelArgs(model_name_, model_id_);
    if (tbe_task->need_tiling_) {
      GELOGD("tiling buffer is not nullptr.");
      tbe_task->stream_resource_ = stream_resource;
    }
    if (aicore_task_defs.size() == kNumTaskWithAtomicAddrCleanTask) {
      const auto &atomic_task_def = aicore_task_defs.front();
      AtomicAddrCleanOpTask *atomic_task = nullptr;
      GE_CHK_STATUS_RET_NOLOG(BuildAtomicTask(atomic_task_def, &atomic_task));
      GE_CHK_STATUS_RET_NOLOG(atomic_task->InitAtomicAddrCleanIndices());
      tbe_task->SetAtomicAddrCleanTask(atomic_task);
    }
    single_op.op_task_.reset(tbe_task);
  } else if (lib_name == kEngineNameAiCpu) {
    const auto &task_def = task_defs[0U];
    GELOGD("Building AICPU_CC task");
    AiCpuCCTask *task = nullptr;
    const uint64_t dynamic_singleop_kernel_id = aicpu_kernel_id++;
    GELOGI("Build dynamic singleOp CCTask, kernel_id = %lu", dynamic_singleop_kernel_id);
    GE_CHK_STATUS_RET_NOLOG(BuildCpuKernelTask(task_def.kernel(), &task, dynamic_singleop_kernel_id));
    if (task->GetUnknownType() == DEPEND_COMPUTE) {
      if (task_defs.size() < kNumTaskWithMemCpyTask) {
        GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Task]The copy task of the fourth operator was not found.");
        REPORT_INNER_ERROR("E19999", "The copy task of the fourth operator was not found.");
        return ACL_ERROR_GE_PARAM_INVALID;
      }
      const domi::TaskDef &copy_task_def = task_defs[1U];
      GE_CHK_STATUS_RET_NOLOG(task->SetMemCopyTask(copy_task_def.kernel()));
    }
    task->SetModelArgs(model_name_, model_id_);
    single_op.op_task_.reset(task);
  } else if (lib_name == kEngineNameAiCpuTf) {
    const auto &task_def = task_defs[0U];
    GELOGD("Building AICPU_TF task");
    AiCpuTask *aicpu_task = nullptr;
    const uint64_t dynamic_singleop_kernel_id = aicpu_kernel_id++;
    GELOGI("Build dynamic singleOp TfTask, kernel_id = %lu", dynamic_singleop_kernel_id);
    GE_CHK_STATUS_RET_NOLOG(BuildKernelExTask(task_def.kernel_ex(), &aicpu_task, dynamic_singleop_kernel_id));
    if (aicpu_task->GetUnknownType() == DEPEND_COMPUTE) {
      if (task_defs.size() < kNumTaskWithMemCpyTask) {
        GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Task]The copy task of the fourth operator was not found.");
        REPORT_INNER_ERROR("E19999", "The copy task of the fourth operator was not found.");
        return ACL_ERROR_GE_PARAM_INVALID;
      }
      const domi::TaskDef &copy_task_def = task_defs[1U];
      GE_CHK_STATUS_RET_NOLOG(aicpu_task->SetMemCopyTask(copy_task_def.kernel_ex()));
    }
    aicpu_task->SetModelArgs(model_name_, model_id_);
    single_op.op_task_.reset(aicpu_task);
  } else {
    // something
  }
  single_op.InjectRuntimeContext();
  return SUCCESS;
}

Status SingleOpModel::NeedHybridModel(const GeModelPtr &ge_model, bool &need_hybrid_model) const {
  bool is_infer_depend = false;
  bool is_host_mem = false;
  GE_CHK_STATUS_RET(CheckInferDepend(ge_model, is_infer_depend, is_host_mem), "[Check][InferDepend] failed.");
  const bool need_d2h_cpy = is_infer_depend && (!is_host_mem);

  // Check if GE local task with executor
  bool is_ge_local_need_hybrid = false;
  GE_CHK_STATUS_RET(CheckGeLocalNeedHybrid(ge_model, is_ge_local_need_hybrid));

  need_hybrid_model = need_d2h_cpy || (node_tasks_.size() > 1U) || is_ge_local_need_hybrid;
  return SUCCESS;
}

Status SingleOpModel::ParseTasks() {
  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);

  const auto &tasks = ge_model->GetModelTaskDefPtr()->task();
  for (int32_t i = 0; i < tasks.size(); ++i) {
    const domi::TaskDef &task_def = tasks[i];
    GELOGI("[%s] Task[%d], type = [%u], DebugString = [%s]", model_name_.c_str(), i, task_def.type(),
           task_def.DebugString().c_str());
    const auto task_type = static_cast<rtModelTaskType_t>(task_def.type());
    uint32_t op_index = 0U;
    if (task_type == RT_MODEL_TASK_KERNEL) {
      op_index = task_def.kernel().context().op_index();
    } else if (task_type == RT_MODEL_TASK_KERNEL_EX) {
      op_index = task_def.kernel_ex().op_index();
    } else if (task_type == RT_MODEL_TASK_ALL_KERNEL) {
      op_index = task_def.kernel_with_handle().context().op_index();
    } else {
      GELOGD("Skip task type: %d", static_cast<int32_t>(task_type));
      continue;
    }
    GELOGD("op_index = %u, task_type = %d", op_index, task_type);

    const auto iter = op_list_.find(op_index);
    if (iter == op_list_.end()) {
      GELOGE(INTERNAL_ERROR, "[Find][Node]Failed to get node by op_index = %u", op_index);
      REPORT_INNER_ERROR("E19999", "Failed to get node by op_index = %u.", op_index);
      return INTERNAL_ERROR;
    }
    auto &node = iter->second;
    node_tasks_[node].emplace_back(task_def);
  }
  return SUCCESS;
}

Status SingleOpModel::BuildDynamicOp(StreamResource &resource, DynamicSingleOp &single_op) {
  single_op.num_inputs_ = data_ops_.size();
  single_op.num_outputs_ = netoutput_op_->GetAllInputsSize();
  GE_CHK_STATUS_RET_NOLOG(InitModelMem(resource));
  model_params_.memory_size = UINT64_MAX;
  model_params_.graph_is_dynamic = true;
  GE_CHK_STATUS_RET(ParseTasks(), "[Parse][Tasks] failed.");

  const auto ge_model = model_helper_.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  std::string single_op_type;
  if (AttrUtils::GetStr(ge_model, kAttrNameSingleOpType, single_op_type)) {
    ProfilingManager::Instance().RegisterElement(single_op.profiling_node_type_index_, single_op_type);
  } else {
    single_op.profiling_node_type_index_ = -1;
    GELOGW("Can not find single op type from GeModel");
  }

  bool need_hybrid_model = false;
  GE_CHK_STATUS_RET(NeedHybridModel(ge_model, need_hybrid_model), "[Check][NeedHybridModel] failed.");
  if (need_hybrid_model) {
    GELOGD("Build single op HybridModel.");
    GE_CHK_STATUS_RET_NOLOG(hybrid::NodeExecutorManager::GetInstance().EnsureInitialized());
    SetHostMemTensorAndNode(single_op);
    GE_CHK_STATUS(SetHostMemNode(single_op.node_with_hostmem_), "[Init][HostMem]Failed.");
    const auto root_model = model_helper_.GetGeRootModel();
    GE_CHECK_NOTNULL(root_model);
    root_model->SetRootGraph(GraphUtils::GetComputeGraph(ge_model->GetGraph()));
    root_model->SetSubgraphInstanceNameToModel(root_model->GetRootGraph()->GetName(), ge_model);
    single_op.hybrid_model_ = MakeUnique<hybrid::HybridModel>(root_model);
    GE_CHECK_NOTNULL(single_op.hybrid_model_);
    GE_CHK_STATUS_RET(single_op.hybrid_model_->SetOverflowAddr(resource.GetOverflowAddr(),
                                                               static_cast<uint64_t>(resource.GetOverflowSize())),
                      "[Set][OverflowAddr]failed.");
    GE_CHK_STATUS_RET(single_op.hybrid_model_->Init(true), "[Init][HybridModel]Failed.");
    int32_t device_id = 0;
    GE_CHK_RT_RET(rtGetDevice(&device_id));
    ThreadPool *thread_pool = nullptr;
    GE_CHK_STATUS_RET_NOLOG(resource.GetThreadPool(&thread_pool));
    single_op.hybrid_model_executor_ = MakeUnique<hybrid::HybridModelExecutor>(single_op.hybrid_model_.get(),
                                                                               device_id,
                                                                               resource.GetStream(), thread_pool);
    GE_CHECK_NOTNULL(single_op.hybrid_model_executor_);
    hybrid::CallbackManager *callback_manager = nullptr;
    GE_CHK_STATUS_RET_NOLOG(resource.GetCallbackManager(&callback_manager));
    GE_CHK_STATUS_RET(single_op.hybrid_model_executor_->Init(callback_manager), "[Init][HybridModelExecutor]Failed.");
    return SUCCESS;
  }
  return BuildTaskListForDynamicOp(&resource, single_op);
}

void SingleOpModel::SetHostMemInputFlagToTask(const OpDescPtr &op_desc, OpTask &task) const {
  if (ExecutorUtils::HasHostMemInput(op_desc)) {
    task.SetHostMemInputFlag(true);
    GELOGD("node[%s] has host mem", op_desc->GetName().c_str());
  }
}

void SingleOpModel::SetHostMemTensorAndNode(DynamicSingleOp &single_op) const {
  for (const auto &node_map : op_with_hostmem_) {
    const auto node = node_map.second;
    single_op.hostmem_node_id_map_[node_map.first] = node->GetOpDesc()->GetId();
  }
}

Status SingleOpModel::SetHostMemNode(std::vector<NodePtr> &node_with_hostmem) {
  for (const auto &node_map : op_with_hostmem_) {
    const NodePtr node = node_map.second;
    const int32_t idx = node_map.first;
    const auto out_anchor = node->GetOutDataAnchor(0);
    GE_CHECK_NOTNULL(out_anchor);
    const auto in_anchors = out_anchor->GetPeerInDataAnchors();

    for (const auto &anchor : in_anchors) {
      GE_CHECK_NOTNULL(anchor);
      auto output_node = anchor->GetOwnerNode();
      GE_CHECK_NOTNULL(output_node);

      node_with_hostmem.emplace_back(output_node);
      GELOGD("Get %d th input tensor desc of %s by %d data node: %s.", anchor->GetIdx(),
             output_node->GetName().c_str(), idx, node->GetName().c_str());
    }
  }
  return SUCCESS;
}
}  // namespace ge
