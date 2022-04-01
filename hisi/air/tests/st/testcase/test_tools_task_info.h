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

#ifndef __INC_TEST_TOOLS_TASK_INFO_H
#define __INC_TEST_TOOLS_TASK_INFO_H

#include "proto/task.pb.h"
#include "graph/compute_graph.h"
#include "common/tbe_kernel_store.h"
#include "common/cust_aicpu_kernel_store.h"
#include "runtime/rt.h"
#include <set>

namespace ge {
extern std::set<std::string> actual_info_type;

void AddPartitionedCall(const ComputeGraphPtr &graph, const std::string &func_name, const ComputeGraphPtr &subgraph);

void AddCaseBranch(const ComputeGraphPtr &graph, const std::string &func_name, const ComputeGraphPtr &subgraph);

void AddIfBranchs(const ComputeGraphPtr &graph, const std::string &func_name,
                  const ComputeGraphPtr &then_graph, const ComputeGraphPtr &else_graph);

void SetUnknownOpKernel(const ComputeGraphPtr &graph, uint32_t &mem_offset, bool reset_index = false);
void DelStaticForOffline(const ComputeGraphPtr &graph, uint32_t &mem_offset);

void CleanAippNodeInfo(const ComputeGraphPtr &graph, const std::string &op_name);
void InitAippNodeDynamic(const ComputeGraphPtr &graph, const std::string &op_name);
void InitAippNodeStatic(const ComputeGraphPtr &graph, const std::string &op_name);
void InitAippNodeRelated(const ComputeGraphPtr &graph, const std::string &op_name, const std::string &related_name);

void InitConstantNode(const ComputeGraphPtr &graph, const std::string &op_name, int32_t const_value);
void InitConstantNode(const ComputeGraphPtr &graph, const std::string &op_name, const GeTensorDesc &tensor_desc, const std::string &const_value);

void InitKernelTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitKernelTaskDef_TE(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name, TBEKernelStore &kernel_store);
void InitKernelTaskDef_TE_SM(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitKernelTaskDef_AI_CPU(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);
void InitKernelTaskDef_CPU_AllShape(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);
void InitKernelTaskDef_CPU_Blocking(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);
void InitKernelTaskDef_CUST_CPU(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name, CustAICPUKernelStore &kernel_store);

void InitKernelTaskDef_CUSTOM(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitKernelExTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);
void InitKernelExTaskDef_AllShape(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);
void InitKernelExTaskDef_Blocking(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitStreamActiveDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitStreamSwitchDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitStreamSwitchNDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitStreamMergeDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitLabelSwitchDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitLabelSetDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitLabelGotoDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitMemcpyAsyncDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitMemcpyAddrAsyncDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitEndGraphDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitHcclTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name);

void InitModelExitTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitProfilerTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitEventTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitFftsPlusCaseDefaultDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def,
                                const std::string &op_name);

void InitFftsPlusNotifyDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitWriteValueDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitMixAicAivDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitSdmaDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitDataCtx(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitCondSwitchCtx(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitFusionTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitFftsplusTaskDef(const ComputeGraphPtr &graph, domi::TaskDef &model_def);

void InitFftsPlusCachePersistDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def,
                                 const std::string &op_name);

void InitFftsPlusAicCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitFftsPlusAicpuCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitCustomFftsPlusAicpuCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def,
                                   const std::string &op_name);

void InitFftsPlusAicpuFwkCtxDef(const ComputeGraphPtr &graph, domi::FftsPlusCtxDef &ctx_def, const std::string &op_name);

void InitCmoTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitCmoBarrierTaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def);

void InitDSATaskDef(const ComputeGraphPtr &graph, domi::ModelTaskDef &model_def, const std::string &op_name,
                    const bool set_ptr_or_value);

// init fusion op info for profiling
void InitFusionOpInfo(const ComputeGraphPtr &graph, const std::string &op_name);

// mock function to report profiling data
int32_t ReporterCallback(uint32_t moduleId, uint32_t type, void *data, uint32_t len);
} // namespace ge
#endif  // __INC_TEST_TOOLS_TASK_INFO_H
