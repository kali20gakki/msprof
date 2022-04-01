/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "ge_default_running_env.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_running_env/fake_op.h"

FAKE_NS_BEGIN
namespace {
std::vector<FakeEngine> default_engines = {
    FakeEngine("AIcoreEngine").KernelInfoStore("AiCoreLib").GraphOptimizer("AIcoreEngine"),
    FakeEngine("VectorEngine").KernelInfoStore("VectorLib").GraphOptimizer("VectorEngine"),
    FakeEngine("DNN_VM_AICPU").KernelInfoStore("AicpuLib").GraphOptimizer("aicpu_tf_optimizer"),
    FakeEngine("DNN_VM_AICPU_ASCEND").KernelInfoStore("AicpuAscendLib").GraphOptimizer("aicpu_ascend_optimizer"),
    FakeEngine("DNN_HCCL").KernelInfoStore("HcclLib").GraphOptimizer("hccl_graph_optimizer").GraphOptimizer("hvd_graph_optimizer"),
    FakeEngine("DNN_VM_RTS").KernelInfoStore("RTSLib").GraphOptimizer("DNN_VM_RTS_GRAPH_OPTIMIZER_STORE"),
    FakeEngine("DNN_VM_GE_LOCAL").KernelInfoStore("DNN_VM_GE_LOCAL_OP_STORE").GraphOptimizer("DNN_VM_HOST_CPU_OPTIMIZER")
};

std::vector<FakeOp> fake_ops = {
    FakeOp(ENTER).InfoStoreAndBuilder("RTSLib"),
    FakeOp(MERGE).InfoStoreAndBuilder("RTSLib"),
    FakeOp(SWITCH).InfoStoreAndBuilder("RTSLib"),
    FakeOp(SWITCHN).InfoStoreAndBuilder("RTSLib"),
    FakeOp(LOOPCOND).InfoStoreAndBuilder("RTSLib"),
    FakeOp(STREAMMERGE).InfoStoreAndBuilder("RTSLib"),
    FakeOp(STREAMSWITCH).InfoStoreAndBuilder("RTSLib"),
    FakeOp(STREAMSWITCHN).InfoStoreAndBuilder("RTSLib"),
    FakeOp(STREAMACTIVE).InfoStoreAndBuilder("RTSLib"),
    FakeOp(EXIT).InfoStoreAndBuilder("RTSLib"),
    FakeOp(SEND).InfoStoreAndBuilder("RTSLib"),
    FakeOp(RECV).InfoStoreAndBuilder("RTSLib"),
    FakeOp(IDENTITY).InfoStoreAndBuilder("RTSLib"),
    FakeOp(IDENTITYN).InfoStoreAndBuilder("RTSLib"),
    FakeOp(MEMCPYASYNC).InfoStoreAndBuilder("RTSLib"),
    FakeOp(MEMCPYADDRASYNC).InfoStoreAndBuilder("RTSLib"),
    FakeOp(STARTOFSEQUENCE).InfoStoreAndBuilder("RTSLib"),

    FakeOp(NEG).InfoStoreAndBuilder("AicpuLib"),
    FakeOp(FRAMEWORKOP).InfoStoreAndBuilder("AicpuLib"),
    FakeOp(CONSTANTOP).InfoStoreAndBuilder("AicpuLib"),
    FakeOp(MODELEXIT).InfoStoreAndBuilder("AicpuLib"),
    FakeOp(GETDYNAMICDIMS).InfoStoreAndBuilder("AicpuLib"),

    FakeOp(DROPOUTGENMASK).InfoStoreAndBuilder("HcclLib"),
    FakeOp(DROPOUTDOMASK).InfoStoreAndBuilder("HcclLib"),
    FakeOp(HCOMALLREDUCE).InfoStoreAndBuilder("HcclLib"),

    FakeOp(RESOURCEAPPLYMOMENTUM).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(ASSIGNVARIABLEOP).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(GATHERV2).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("MatMulV2").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(LESS).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(NEXTITERATION).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CAST).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(TRANSDATA).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(NOOP).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(VARIABLE).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CONSTANT).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(ASSIGN).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(ASSIGNADD).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(ADD).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(SUB).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("Abs").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(MUL).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(DATA).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(AIPP).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(AIPPDATA).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(NETOUTPUT).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(PARTITIONEDCALL).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CONV2D).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CONCAT).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CONCATV2).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(RELU).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("SplitD").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(SLICE).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(PACK).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(RELU6GRAD).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(HCOMALLREDUCE).InfoStoreAndBuilder("HcclLib"),
    FakeOp(HCOMALLGATHER).InfoStoreAndBuilder("HcclLib"),
    FakeOp(HCOMBROADCAST).InfoStoreAndBuilder("HcclLib"),
    FakeOp(HVDCALLBACKBROADCAST).InfoStoreAndBuilder("HcclLib"),
    FakeOp(REALDIV).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(FILECONSTANT).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(RESHAPE).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("GetShape").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("MapIndex").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp(CASE).InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("UpdateTensorDesc").InfoStoreAndBuilder("AiCoreLib"),
    FakeOp("LabelSet").InfoStoreAndBuilder("RTSLib"),
    FakeOp("LabelSwitchByIndex").InfoStoreAndBuilder("RTSLib"),
    FakeOp("LabelGotoEx").InfoStoreAndBuilder("RTSLib"),

    FakeOp(REFIDENTITY).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(STACK).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(STACKPUSH).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(STACKPOP).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(STACKCLOSE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(CONSTANTOP).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(FOR).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(WHILE).InfoStoreAndBuilder("DNN_VM_GE_LOCAL_OP_STORE"),
    FakeOp(ATOMICADDRCLEAN).InfoStoreAndBuilder("AiCoreLib"),
};
}  // namespace

void GeDefaultRunningEnv::InstallTo(GeRunningEnvFaker& ge_env) {
  for (auto& fake_engine : default_engines) {
    ge_env.Install(fake_engine);
  }

  for (auto& fake_op : fake_ops) {
    ge_env.Install(fake_op);
  }
}

FAKE_NS_END
