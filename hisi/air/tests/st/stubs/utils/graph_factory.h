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
#ifndef INC_22AB7AD26BB44415A28DDF92D6FB08D5
#define INC_22AB7AD26BB44415A28DDF92D6FB08D5

#include "ge_running_env/fake_ns.h"
#include "external/graph/graph.h"
FAKE_NS_BEGIN

struct GraphFactory{
  static Graph SingeOpGraph();
  static Graph SingeOpGraph2();
  static Graph SingeOpGraph3();
  static Graph HybridSingeOpGraph();
  static Graph HybridSingeOpGraph2();
  static Graph BuildAicpuSingeOpGraph();
  static Graph BuildVarInitGraph1();
  static Graph BuildCheckpointGraph1();
  static Graph BuildVarTrainGraph1();
  static Graph BuildVarWriteNoOutputRefGraph1();
  static Graph BuildV1LoopGraph1();
  static Graph BuildV1LoopGraph2_CtrlEnterIn();
  static Graph BuildV1LoopGraph3_CtrlEnterIn2();
  static Graph BuildV1LoopGraph4_DataEnterInByPassMerge();
  static Graph BuildGraphForMergeShapeNPass();
  static Graph HybridSingeOpGraphForHostMemInput();
  static Graph HybridSingeOpGraphAicpuForHostMemInput();
};

FAKE_NS_END

#endif
