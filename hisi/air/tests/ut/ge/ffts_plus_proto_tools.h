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

#ifndef __INC_TESTS_FFTS_PLUS_PROTO_TOOLS_H
#define __INC_TESTS_FFTS_PLUS_PROTO_TOOLS_H

#include "proto/task.pb.h"
#include "graph/compute_graph.h"


#define ADD_FFTS_PLUS_CTX(type, func_proto, func_init, ctx_node, args...)    \
  do {                                                                       \
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx(); \
    ctx_def->set_op_index(ctx_node->GetOpDesc()->GetId());                   \
    ctx_def->set_context_type(static_cast<uint32_t>(type));                  \
    auto inner_ctx_def = ctx_def->func_proto();                              \
    func_init(inner_ctx_def, ##args);                                        \
} while (false)

#define ADD_FFTS_PLUS_CTX_MANUAL(type, func_proto, func_init, ctx_node, args...) \
  do {                                                                           \
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();     \
    ctx_def->set_op_index(ctx_node->GetOpDesc()->GetId());                       \
    ctx_def->set_context_type(static_cast<uint32_t>(type));                      \
    auto inner_ctx_def = ctx_def->func_proto();                                  \
    func_init(inner_ctx_def, ##args);                                            \
    inner_ctx_def->set_atm(0);                                                   \
  } while (false)

#define ADD_FFTS_PLUS_MIX_CTX(type, func_proto, func_init, ctx_node, args...) \
  {                                                                           \
    domi::FftsPlusCtxDef *ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();  \
    ctx_def->set_op_index(ctx_node->GetOpDesc()->GetId());                    \
    ctx_def->set_context_type(static_cast<uint32_t>(type));                   \
    auto inner_ctx_def = ctx_def->func_proto();                               \
    func_init(inner_ctx_def, ##args);                                         \
  }

namespace ge {
void SetPartitionedCall(const NodePtr &func_node, const ComputeGraphPtr &subgraph);

void SetKnownOpKernel(const ComputeGraphPtr &graph, uint32_t &mem_offset);

OpDescPtr CreateOpDesc(std::string name = "", std::string type = "", int in_num = 0, int out_num = 0);

NodePtr CreateNode(ComputeGraph &graph, const std::string &name, const std::string &type, int in_num, int out_num);

void SetNodeAnchorStatus(const NodePtr &node);
void InitFftsThreadSliceMap(const OpDescPtr &op_desc);

void InitTaskSQEInfo(domi::FftsPlusTaskDef *task_def);

void InitTaskAdditionalDataInfo(domi::FftsPlusTaskDef *task_def);

void InitCachePersistentCtx(domi::FftsPlusCachePersistCtxDef *ctx_def);

void InitAicAivCtx(domi::FftsPlusAicAivCtxDef *ctx_def, bool is_known = true);

void InitMixAicAivCtx(domi::FftsPlusMixAicAivCtxDef *ctx_def, bool is_auto = false, bool is_known = true);

void InitSdmaCtx(domi::FftsPlusSdmaCtxDef *ctx_def);

void InitNotifyCtx(domi::FftsPlusNotifyCtxDef *ctx_def);

void InitWriteValueCtx(domi::FftsPlusWriteValueCtxDef *ctx_def);

void InitAicpuCtxCtx(domi::FftsPlusAicpuCtxDef *ctx_def, bool is_known = true);

void InitAicpuFwkCtxCtx(domi::FftsPlusAicpuCtxDef *ctx_def);

void InitDataCtx(domi::FftsPlusDataCtxDef *ctx_def);

void InitAicpuFwkCtxAndExtInfo(domi::FftsPlusAicpuCtxDef *ctx_def);

void InitAicpuCtxAndExtInfo(domi::FftsPlusAicpuCtxDef *ctx_def);

void InitCustomAicpuCtxAndExtInfo(domi::FftsPlusAicpuCtxDef *ctx_def);

void InitAtStartCtx(domi::FftsPlusAtStartCtxDef *ctx_def);

void InitAtEndCtx(domi::FftsPlusAtEndCtxDef *ctx_def);

void InitLabelCtx(domi::FftsPlusLabelCtxDef *ctx_def);

void InitCaseSwitchCtx(domi::FftsPlusCaseSwitchCtxDef *ctx_def);

void InitCaseDefaultCtx(domi::FftsPlusCaseDefaultCtxDef *ctx_def);

void InitCondSwitchCtx(domi::FftsPlusCondSwitchCtxDef *ctx_def);
} // namespace ge
#endif // __INC_TESTS_FFTS_PLUS_PROTO_TOOLS_H
