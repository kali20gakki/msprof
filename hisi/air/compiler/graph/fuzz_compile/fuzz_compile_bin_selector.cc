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

#include "fuzz_compile_bin_selector.h"

#include "framework/common/debug/log.h"
#include "framework/common/util.h"
#include "common/profiling_definitions.h"
#include "graph/def_types.h"
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "runtime/rt_model.h"
#include "runtime/stream.h"

namespace ge {
namespace hybrid {
namespace {
const uintptr_t kWeightBase = 0x10000000;
const uintptr_t kMemBase = 0x20000000;
const uint64_t kFakeSize = 0x10000000UL;
const std::string kKernelName = "AIcoreEngine";

void BackUpNodeShapes(const ge::NodePtr &node_ptr, NodeInOutShape &in_out_shapes) {
  for (size_t in_index = 0UL; in_index < node_ptr->GetOpDesc()->GetAllInputsSize(); ++in_index) {
    const auto in_tensor_desc = node_ptr->GetOpDesc()->MutableInputDesc(in_index);
    if (in_tensor_desc == nullptr) {
      continue;
    }
    in_out_shapes.in_ori_shapes.emplace(std::make_pair(in_index, in_tensor_desc->GetOriginShape()));
    in_out_shapes.in_shapes.emplace(std::make_pair(in_index, in_tensor_desc->GetShape()));
  }

  for (size_t out_index = 0UL; out_index < node_ptr->GetOpDesc()->GetAllOutputsDescSize(); ++out_index) {
    const auto out_tensor_desc = node_ptr->GetOpDesc()->MutableOutputDesc(out_index);
    if (out_tensor_desc == nullptr) {
      continue;
    }
    in_out_shapes.out_ori_shapes.emplace(std::make_pair(out_index, out_tensor_desc->GetOriginShape()));
    in_out_shapes.out_shapes.emplace(std::make_pair(out_index, out_tensor_desc->GetShape()));
  }
}

void SetNodeShapes(const ge::NodePtr &node_ptr, NodeInOutShape &in_out_shapes) {
  for (size_t in_index = 0UL; in_index < node_ptr->GetOpDesc()->GetAllInputsSize(); ++in_index) {
    const auto in_tensor_desc = node_ptr->GetOpDesc()->MutableInputDesc(in_index);
    if (in_tensor_desc == nullptr) {
      continue;
    }
    in_tensor_desc->SetOriginShape(in_out_shapes.in_ori_shapes[in_index]);
    in_tensor_desc->SetShape(in_out_shapes.in_shapes[in_index]);
  }

  for (size_t out_index = 0UL; out_index < node_ptr->GetOpDesc()->GetAllOutputsDescSize(); ++out_index) {
    const auto out_tensor_desc = node_ptr->GetOpDesc()->MutableOutputDesc(out_index);
    if (out_tensor_desc == nullptr) {
      continue;
    }
    out_tensor_desc->SetOriginShape(in_out_shapes.out_ori_shapes[out_index]);
    out_tensor_desc->SetShape(in_out_shapes.out_shapes[out_index]);
  }
}
}  // namespace

Status FuzzCompileBinSelector::Initialize() {
  const auto ge_lib = GELib::GetInstance();
  GE_CHECK_NOTNULL(ge_lib);
  if (!ge_lib->InitFlag()) {
    GELOGE(GE_CLI_GE_NOT_INITIALIZED, "Ge_lib is uninitialized, failed.");
    return GE_CLI_GE_NOT_INITIALIZED;
  }

  const auto &kernel_manager = ge_lib->OpsKernelManagerObj();
  aicore_kernel_store_ = kernel_manager.GetOpsKernelInfoStore(kKernelName);
  GE_CHECK_NOTNULL(aicore_kernel_store_);
  return SUCCESS;
}

NodeCompileCacheItem *FuzzCompileBinSelector::SelectBin(const NodePtr &node, const GEThreadLocalContext *ge_context,
                                                        std::vector<domi::TaskDef> &task_defs) {
  if (node == nullptr) {
    GELOGE(FAILED, "Node is nullptr.");
    return nullptr;
  }

  if (ge_context != nullptr) {
    GetThreadLocalContext() = *ge_context;
  }

  PROFILING_START(-1, profiling::kFindCompileCache);
  NodeCompileCacheItem *cci = nccm_->FindCompileCache(node);
  PROFILING_END(-1, profiling::kFindCompileCache);
  if (cci != nullptr) {
    GELOGD("Found cci %lu of node %s.", cci->GetCacheItemId(), node->GetName().c_str());
    return cci;
  }

  // save execution shapes and roll back it after fuzz compile
  NodeInOutShape execution_shapes{};
  BackUpNodeShapes(node, execution_shapes);
  auto ret = DoCompileOp(node);

  // save generalize shapes to add cci
  NodeInOutShape generalize_shapes{};
  BackUpNodeShapes(node, generalize_shapes);
  SetNodeShapes(node, execution_shapes);

  if (ret != SUCCESS) {
    return nullptr;
  }

  ret = OpsKernelBuilderManager::Instance().CalcOpRunningParam(*node);
  if (ret != SUCCESS) {
    return nullptr;
  }

  ret = DoGenerateTask(node, task_defs);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "Failed to execute generate task, node = %s", node->GetName().c_str());
    return nullptr;
  }

  GELOGD("Successfully generate task, node is %s", node->GetName().c_str());

  const auto op_desc = *node->GetOpDesc();
  const auto task_def = task_defs.back();

  void *handle = nullptr;
  KernelLaunchBinType bin_type = kBinTypeEnd;
  // fuzz compile success, register bin handle
  ret = DoRegisterBin(op_desc, task_def, bin_type, handle);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "Register bin failed.");
    return nullptr;
  }

  NodeCompileCacheItem node_cci;
  if (NodeCompileCacheItem::Build(bin_type, node, handle, node_cci) != SUCCESS) {
    GELOGI("Fail to build compile cache item of node %s.", node->GetName().c_str());
    return nullptr;
  }

  SetNodeShapes(node, generalize_shapes);
  PROFILING_START(-1, profiling::kAddCompileCache);
  const auto ret_cci = nccm_->AddCompileCache(node, node_cci);
  PROFILING_END(-1, profiling::kAddCompileCache);
  SetNodeShapes(node, execution_shapes);
  return ret_cci;
}

Status FuzzCompileBinSelector::DoCompileOp(const NodePtr &node_ptr) const {
  std::vector<NodePtr> node_vec{node_ptr};
  PROFILING_START(-1, profiling::kFuzzCompileOp);
  const auto ret = aicore_kernel_store_->FuzzCompileOp(node_vec);
  PROFILING_END(-1, profiling::kFuzzCompileOp);
  if (ret != SUCCESS) {
    GELOGI("Fuzz compile node %s failed. Execute the origin graph", node_ptr->GetName().c_str());
  }
  return ret;
}

Status FuzzCompileBinSelector::DoRegisterBin(const OpDesc &op_desc, const domi::TaskDef &task_def,
                                             KernelLaunchBinType &bin_type, void *&handle) const {
  PROFILING_SCOPE(-1, profiling::kRegisterBin);
  Status ret;
  AttrNameOfBinOnOp attr_names_of_bin = {OP_EXTATTR_NAME_TBE_KERNEL, TVM_ATTR_NAME_METADATA, "_kernelname"};

  if (task_def.type() != RT_MODEL_TASK_ALL_KERNEL) {
    bin_type = kStubFunc;
    ret = BinRegisterUtils::RegisterBin(op_desc, task_def.kernel().stub_func(), attr_names_of_bin, handle);
  } else {
    bin_type = kWithHandle;
    ret = BinRegisterUtils::RegisterBinWithHandle(op_desc, attr_names_of_bin, handle);
  }
  return ret;
}

Status FuzzCompileBinSelector::DoGenerateTask(const NodePtr &node_ptr, std::vector<domi::TaskDef> &task_defs) {
  PROFILING_SCOPE(-1, profiling::kGenTask);
  auto op_desc_ptr = node_ptr->GetOpDesc();
  std::vector<int64_t> input_offsets(op_desc_ptr->GetInputsSize(), kMemBase);
  std::vector<int64_t> output_offsets(op_desc_ptr->GetOutputsSize(), kMemBase);
  op_desc_ptr->SetInputOffset(input_offsets);
  op_desc_ptr->SetOutputOffset(output_offsets);
  std::vector<int64_t> workspaces(op_desc_ptr->GetWorkspaceBytes().size(), kMemBase);
  op_desc_ptr->SetWorkspace(std::move(workspaces));

  rtModel_t rt_model = nullptr;
  GE_CHK_RT_RET(rtModelCreate(&rt_model, 0));
  GE_MAKE_GUARD(rt_model, [&rt_model] {
    if (rt_model) {
      GE_CHK_RT(rtModelDestroy(rt_model));
    }
  });

  rtStream_t stream = nullptr;
  auto ret = rtStreamCreate(&stream, 0);
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "Call rt api failed, ret: 0x%X", ret);
    GE_CHK_RT(rtModelDestroy(rt_model));
    return RT_FAILED;
  }
  GE_MAKE_GUARD_RTSTREAM(stream);

  ret = rtModelBindStream(rt_model, stream, 0);
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "Call rt api failed, ret: 0x%X", ret);
    GE_CHK_RT(rtModelDestroy(rt_model));
    return RT_FAILED;
  }

  RunContext run_ctx;
  run_ctx.stream = stream;
  run_ctx.model = rt_model;
  run_ctx.graphStreamList.emplace_back(stream);
  run_ctx.weightMemBase = PtrToPtr<void, uint8_t>(ValueToPtr(kWeightBase));
  run_ctx.dataMemBase = PtrToPtr<void, uint8_t>(ValueToPtr(kWeightBase));
  run_ctx.weightMemSize = kFakeSize;
  run_ctx.dataMemSize = kFakeSize;

  {
    const std::lock_guard<std::mutex> lk(gen_task_mutex_);
    ret = OpsKernelBuilderManager::Instance().GenerateTask(*node_ptr, run_ctx, task_defs);
  }

  GE_CHK_RT(rtModelUnbindStream(rt_model, stream));
  return ret;
}
}  // namespace hybrid
}  // namespace ge
