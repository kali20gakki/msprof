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

#include "compiler/offline/main_impl.h"
#include "ge_running_env/path_utils.h"
#include "ge_running_env/atc_utils.h"

#include "utils/model_factory.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "utils/graph_utils.h"
#include "types.h"
#include "init_ge.h"

namespace ge {
class MultiDimsSTest : public AtcTest {};

TEST_F(MultiDimsSTest, MultiBatch) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "ms_1");
  auto path = ModelFactory::GenerateModel_1();
  std::string model_arg = "--model="+path;
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=1", // FrameworkType
                  "--out_nodes=relu:0",
                  "--soc_version=Ascend310",
                  "--input_format=NCHW",
                  "--output_type=FP32",
                  "--input_shape=data1:-1,3,16,16",
                  "--dynamic_batch_size=1,2,4,8",
                  };
  DUMP_GRAPH_WHEN("PreRunAfterPrepare")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
  /*
   *
   * root graph:
   *                      netoutput
   *                         |
   *         -------------  case
   *        /     /             \
   *      /     /              mapindex
   *    /     /                 /    \
   * data1 const1   data_multibatch const_multibatch
   *
   *
   *
   * subgraphs:
   *
   *     netoutput
   *        |
   *       relu
   *        |
   *      conv2d
   *      /   \
   *    data0 data1
   */
  CHECK_GRAPH(PreRunAfterPrepare) {
    // mmSetEnv("DUMP_GE_GRAPH", "1", 1);
    // GraphUtils::DumpGEGraphToOnnx(*graph, "SnDebugPrepare");
    // GraphUtils::DumpGEGraphToOnnx(*(graph->GetAllSubgraphs()[0]), "SnDebugPrepareSub0");

    EXPECT_EQ(graph->GetDirectNodesSize(), 7);
    EXPECT_EQ(graph->GetAllNodesSize(), 7 + 5 * 4);
    EXPECT_EQ(graph->GetAllSubgraphs().size(), 4);

    // todo check topo
    // todo check shape in every subgraphs
    // todo check the max shape
  };
}
}