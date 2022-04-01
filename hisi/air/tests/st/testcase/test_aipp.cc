/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include <unistd.h>
#include <cstdlib>

#include "compiler/offline/main_impl.h"
#include "ge_running_env/path_utils.h"
#include "ge_running_env/atc_utils.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_running_env/fake_op.h"

#include "utils/model_factory.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "utils/graph_utils.h"
#include "types.h"
#include "init_ge.h"

namespace ge {
class AippSTest : public AtcTest {
  void SetUp() override {
    auto aipp_stub_infer_func = [](Operator &op) -> graphStatus {
      auto images_desc = op.GetInputDesc("images");
      auto images_shape = images_desc.GetShape().GetDims();
      int64_t size = 1;
      for (auto dim : images_shape) {
        size *= dim;  // RGB888_U8 size is n*3*h*w (c is 3 in GenerateModel)
      }
      size = abs(size);
      images_desc.SetSize(size);
      (void)op.UpdateOutputDesc("features", images_desc);
      images_desc.SetDataType(DT_UINT8);
      (void)op.UpdateInputDesc("images", images_desc);
      return 0;
    };
    // aipp infershape need register
    GeRunningEnvFaker ge_env;
    ge_env.InstallDefault().Install(FakeOp(AIPP).InfoStoreAndBuilder("AicoreLib").InferShape(aipp_stub_infer_func));
  }
};

TEST_F(AippSTest, StaticAipp_StaticShape) {
  auto path = ModelFactory::GenerateModel_2(false);
  std::string model_arg = "--model=" + path;
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "static_aipp_static_shape");
  std::string output_arg = "--output=" + om_path;

  std::string conf_path = GetAirPath() + "/tests/st/config_file/aipp_conf/aipp_static.cfg";
  char real_path[200];
  realpath(conf_path.c_str(), real_path);
  std::string insert_conf_arg = "--insert_op_conf=" + std::string(real_path);
  char *argv[] = {
      "atc",
      const_cast<char *>(model_arg.c_str()),
      const_cast<char *>(output_arg.c_str()),
      const_cast<char *>(insert_conf_arg.c_str()),
      "--framework=1",  // FrameworkType
      "--mode=0",       // Aipp only support mode 0
      "--out_nodes=relu:0",
      "--soc_version=Ascend310",
      "--input_format=NCHW",
      "--output_type=FP32",
  };
  DUMP_GRAPH_WHEN("PrepareAfterInsertAipp")
  auto ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe();  // the main_impl will call GEFinalize, so re-init after call it

  CHECK_GRAPH(PrepareAfterInsertAipp) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 7 + 2);  // TODO: other check
  };
}

TEST_F(AippSTest, DynamicAipp_Case) {
  auto path = ModelFactory::GenerateModel_1();
  std::string model_arg = "--model=" + path;
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "dynamic_aipp");
  std::string output_arg = "--output=" + om_path;

  std::string conf_path = GetAirPath() + "/tests/st/config_file/aipp_conf/aipp_dynamic.cfg";
  char real_path[200];
  realpath(conf_path.c_str(), real_path);
  std::string insert_conf_arg = "--insert_op_conf=" + std::string(real_path);
  char *argv[] = {
      "atc",
      const_cast<char *>(model_arg.c_str()),
      const_cast<char *>(output_arg.c_str()),
      const_cast<char *>(insert_conf_arg.c_str()),
      "--framework=1",  // FrameworkType
      "--mode=0",       // Aipp only support mode 0
      "--out_nodes=relu:0",
      "--soc_version=Ascend310",
      "--input_format=NCHW",
      "--output_type=FP32",
      "--input_shape=data1:-1,3,16,16",
      "--dynamic_batch_size=1,2",
  };
  DUMP_GRAPH_WHEN("PrepareAfterInsertAipp")
  auto ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe();  // the main_impl will call GEFinalize, so re-init after call it

  CHECK_GRAPH(PrepareAfterInsertAipp) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 9);
    EXPECT_EQ(graph->GetAllNodesSize(), 9 + 5 * 2);
    EXPECT_EQ(graph->GetAllSubgraphs().size(), 2);
    // TODO: other check
  };
}

TEST_F(AippSTest, StaticAipp_Case) {
  auto path = ModelFactory::GenerateModel_2();
  std::string model_arg = "--model=" + path;
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "aipp_static");
  std::string output_arg = "--output=" + om_path;

  std::string conf_path = GetAirPath() + "/tests/st/config_file/aipp_conf/aipp_static.cfg";
  char real_path[200];
  realpath(conf_path.c_str(), real_path);
  std::string insert_conf_arg = "--insert_op_conf=" + std::string(real_path);
  char *argv[] = {
      "atc",
      const_cast<char *>(model_arg.c_str()),
      const_cast<char *>(output_arg.c_str()),
      const_cast<char *>(insert_conf_arg.c_str()),
      "--framework=1",  // FrameworkType
      "--mode=0",       // Aipp only support mode 0
      "--out_nodes=relu:0",
      "--soc_version=Ascend310",
      "--input_format=NCHW",
      "--output_type=FP32",
      "--input_shape=data1:-1,3,16,16",
      "--dynamic_batch_size=1,2",
  };
  DUMP_GRAPH_WHEN("PrepareAfterInsertAipp")
  auto ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe();  // the main_impl will call GEFinalize, so re-init after call it

  CHECK_GRAPH(PrepareAfterInsertAipp) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 8);
    // TODO: other check
  };
}
}  // namespace ge
