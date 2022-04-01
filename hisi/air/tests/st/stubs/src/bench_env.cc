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
#include "utils/bench_env.h"
#include <iostream>
#include <vector>
#include "runtime/rt.h"
#include "framework/executor/ge_executor.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "single_op/single_op.h"
#include "single_op/single_op_manager.h"
#include "utils/model_data_builder.h"
#include "single_op/task/build_task_utils.h"
#include "utils/tensor_descs.h"
#include "utils/data_buffers.h"
#include "ge_running_env/fake_ns.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_running_env/fake_op.h"
#include "external/ge/ge_api.h"
#include "register/op_tiling_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "hybrid/node_executor/ge_local/ge_local_node_executor.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/operator_factory_impl.h"


using namespace ge;
using namespace std;
using namespace optiling;
using namespace optiling::utils;
using namespace hybrid;

FAKE_NS_BEGIN

void BenchEnv::Init() {
  OpTilingFuncV2 tilingfun =  [](const ge::Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &) ->bool {
    return true;
  };

  OpTilingFuncV2 tilingfun2 =  [](const ge::Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &run_info) ->bool {
    run_info.SetWorkspaces({16, 16, 16, 16, 16, 16, 16, 16});
    std::string temp("xx");
    run_info.GetAllTilingData() << temp;
    return true;
  };

  auto infer_fun = [](Operator &op) -> graphStatus {
    const char_t *name = "__output0";
    op.UpdateOutputDesc(name, op.GetInputDesc(0));
    return GRAPH_SUCCESS;
  };

  OpTilingRegistryInterf_V2(DATA, tilingfun);
  OpTilingRegistryInterf_V2(ADD, tilingfun);
  OpTilingRegistryInterf_V2(TRANSDATA, tilingfun);
  OpTilingRegistryInterf_V2(TRANSLATE, tilingfun);
  OpTilingRegistryInterf_V2(MATMUL, tilingfun);
  OpTilingRegistryInterf_V2("DynamicAtomicAddrClean", tilingfun);
  OpTilingRegistryInterf_V2("UniqueV2", tilingfun);
  OpTilingRegistryInterf_V2("MyTransdata", tilingfun2);

  REGISTER_OP_TILING_UNIQ_V2(Data, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(TransData, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(Translate, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(MatMul, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(DynamicAtomicAddrClean, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(Add, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(UniqueV2, tilingfun, 1);
  REGISTER_OP_TILING_UNIQ_V2(MyTransdata, tilingfun2, 1);

  OperatorFactoryImpl::RegisterInferShapeFunc(TRANSDATA, infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc(ADD, infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc(MATMUL, infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc("Reshape", infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc(RELU, infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc("DynamicAtomicAddrClean", infer_fun);
  OperatorFactoryImpl::RegisterInferShapeFunc("MyTransdata", infer_fun);

  REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::AICORE, AiCoreNodeExecutor);
  REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::GE_LOCAL, GeLocalNodeExecutor);
  MemManager::Instance().Initialize({RT_MEMORY_HBM});
}
FAKE_NS_END
