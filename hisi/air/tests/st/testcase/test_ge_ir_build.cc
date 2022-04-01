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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define private public
#define protected public
#undef private
#undef protected

#include "runtime/rt.h"
#include "framework/executor/ge_executor.h"
#include "framework/generator/ge_generator.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "single_op/single_op.h"
#include "single_op/single_op_manager.h"
#include "utils/model_data_builder.h"
#include "single_op/task/build_task_utils.h"
#include "single_op/task/tbe_task_builder.h"
#include "utils/tensor_descs.h"
#include "utils/data_buffers.h"
#include "register/op_tiling_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "hybrid/node_executor/ge_local/ge_local_node_executor.h"
#include "graph/manager/graph_mem_manager.h"
#include "utils/bench_env.h"
#include "utils/graph_factory.h"
#include "hybrid/model/hybrid_model_builder.h"
#include "ge_running_env/fake_ops_kernel_builder.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"
#include "ge/ge_ir_build.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/tuning_utils.h"

namespace ge {
class GeIrBuildTest : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
    GeRunningEnvFaker env;
    env.InstallDefault();
  }
};

TEST_F(GeIrBuildTest, TestBuildModel) {
  BenchEnv::Init();
  std::map<AscendString, AscendString> init_options;
  init_options.emplace(ge::OPTION_EXEC_HCCL_FLAG, "0");
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  setenv("ASCEND_OPP_PATH", "./", 0);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<AscendString, AscendString> build_options;
  build_options.emplace(ge::ir_option::INPUT_FORMAT, "NCHW");
  ModelBufferData model_buffer_data{};
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  std::string output_file = "saved_model.om";
  EXPECT_EQ(aclgrphSaveModel(output_file, model_buffer_data), SUCCESS);
  EXPECT_EQ(aclgrphSaveModel(output_file.c_str(), model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

//TEST_F(GeIrBuildTest, TestInferShapePrepare) {
//  auto graph = GraphFactory::SingeOpGraph2();
//  auto compute_graph = GraphUtils::GetComputeGraph(graph);
//  EXPECT_TRUE(compute_graph != nullptr);
//}

TEST_F(GeIrBuildTest, TestGenerateForOp) {
  BenchEnv::Init();
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  Graph graph;
  Shape shape({1, 16});
  TensorDesc input_tensor_desc(shape);
  input_tensor_desc.SetConstData(MakeUnique<uint8_t[]>(64), 64);
  TensorDesc output_tensor_desc(shape);
  EXPECT_EQ(aclgrphGenerateForOp(NEG, {input_tensor_desc}, {output_tensor_desc},  graph), SUCCESS);
}

TEST_F(GeIrBuildTest, TestBuildOptions) {
  std::map<std::string, std::string> init_options;

  init_options[ge::ir_option::SPARSITY] = "1";
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  init_options[ge::ir_option::ENABLE_COMPRESS_WEIGHT] = "true";
  init_options[ge::ir_option::COMPRESS_WEIGHT_CONF] = "./";
  EXPECT_EQ(aclgrphBuildInitialize(init_options), GRAPH_PARAM_INVALID);

  init_options[ge::ir_option::ENABLE_COMPRESS_WEIGHT] = "yes";
  init_options[ge::ir_option::COMPRESS_WEIGHT_CONF] = "./";
  EXPECT_EQ(aclgrphBuildInitialize(init_options), GRAPH_PARAM_INVALID);
}

TEST_F(GeIrBuildTest, TestDumpGraph) {
  auto graph = GraphFactory::SingeOpGraph2();
  std::string file_path = "dump.bin";
  aclgrphDumpGraph(graph, file_path.c_str(), file_path.length());
}

TEST_F(GeIrBuildTest, TestSetOpAttr) {
  auto graph = GraphFactory::SingeOpGraph2();

  // error attr type
  EXPECT_EQ(aclgrphSetOpAttr(graph, aclgrphAttrType(-1), "./"), GRAPH_FAILED);
  // empty config
  EXPECT_EQ(aclgrphSetOpAttr(graph, ATTR_TYPE_KEEP_DTYPE, nullptr), GRAPH_SUCCESS);

  // TODO config file
}

TEST_F(GeIrBuildTest, TestBuildModelWithShapeRange) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape_range", "[1, 1, -1, -1]"}
  };
  ModelBufferData model_buffer_data{};
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), GRAPH_FAILED);

  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, -1);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), GRAPH_FAILED);

  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithShapeRange_invalid_param1) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape_range", "[1~-1, 1, -1, -1]"}
  };
  ModelBufferData model_buffer_data{};

  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), GRAPH_FAILED);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithShapeRange_invalid_param2) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape_range", "[1~2~3, 1, -1, -1]"}
  };
  ModelBufferData model_buffer_data{};

  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), GRAPH_FAILED);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithShapeRange_invalid_param3) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape_range", "[1]"}
  };
  ModelBufferData model_buffer_data{};

  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), GRAPH_FAILED);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithNamedShapeRange) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  std::string range_str = data_node->GetName() + ":[1, 1, -1, -1]";
  string input_shape_str = data_node->GetName() + ":1,1,244,244";
  std::map<string, string> build_options = {
      {ge::ir_option::INPUT_SHAPE_RANGE, range_str},
      {ge::ir_option::INPUT_SHAPE, input_shape_str}
  };

  ModelBufferData model_buffer_data{};
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestGetIRVersion) {
  int32_t major_version = 0;
  int32_t minor_version = 0;
  int32_t patch_version = 0;
  EXPECT_EQ(aclgrphGetIRVersion(&major_version, &minor_version, &patch_version), SUCCESS);
}

TEST_F(GeIrBuildTest, TestBuildModel_buffer_optimize_invalid) {
  std::map<AscendString, AscendString> init_options = {
      {ge::ir_option::BUFFER_OPTIMIZE, "invalid"}
  };
  EXPECT_NE(aclgrphBuildInitialize(init_options), SUCCESS);
}

// TEST_F(GeIrBuildTest, TestBuildModel_insert_op_invalid) {
//   std::map<AscendString, AscendString> init_options = {
//       {ge::ir_option::INSERT_OP_FILE, "invalid"}
//   };
//   EXPECT_NE(aclgrphBuildInitialize(init_options), SUCCESS);
// }

TEST_F(GeIrBuildTest, TestBuildModel_reuse_memory_invalid) {
  std::map<AscendString, AscendString> init_options = {
      {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, "invalid"}
  };
  EXPECT_NE(aclgrphBuildInitialize(init_options), SUCCESS);
}

TEST_F(GeIrBuildTest, TestBuildModel_single_stream_invalid) {
  std::map<AscendString, AscendString> init_options = {
      {ge::ir_option::ENABLE_SINGLE_STREAM, "invalid"}
  };
  EXPECT_NE(aclgrphBuildInitialize(init_options), SUCCESS);
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicBatch) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:-1,3,16,16"},
      {"ge.dynamicBatchSize", "1,2,4,8,"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicBatch_invalid1) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"ge.dynamicBatchSize", "1,2,4,8"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicBatch_invalid2) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,16,16"},
      {"ge.dynamicBatchSize", "1,2,4,8"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicBatch_invalid3) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,16,16"},
      {"ge.dynamicBatchSize", "a,2,4,8"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicImageSize", "16,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage_invalid1) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,-1,-1"},
      {"ge.dynamicImageSize", "16,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage_invalid2) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,-1,-1, 1"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicImageSize", "16,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage_invalid3) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,16,16"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicImageSize", "16,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage_invalid4) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicImageSize", "a,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicImage_invalid5) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicImageSize", "16,16,16;32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "3,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid1) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "NCHW"},
      {"ge.dynamicDims", "3,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid2) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "3,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid3) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,16,16"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "3,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid4) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", ""}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid5) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "3,16,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid6) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,-1,-1,-1"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "a,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamicDims_invalid7) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicDims", "a,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModelWithDynamic_multi_invalid) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"input_shape", "data1:1,3,16,16"},
      {ge::ir_option::INPUT_FORMAT, "ND"},
      {"ge.dynamicImageSize", "16,16;32,32"},
      {"ge.dynamicDims", "a,16,16;3,32,32"}
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModel_input_format_invalid) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {ge::ir_option::INPUT_FORMAT, "NDND"},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

// TEST_F(GeIrBuildTest, TestBuildModel_param_invalid) {
//   std::map<AscendString, AscendString> init_options;
//   EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

//   auto graph = GraphFactory::SingeOpGraph2();
//   std::map<string, string> build_options = {
//       {ge::ir_option::INPUT_FORMAT, nullptr},
//   };
//   ModelBufferData model_buffer_data{};
//   auto compute_graph = GraphUtils::GetComputeGraph(graph);
//   auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
//   EXPECT_TRUE(data_node != nullptr);
//   auto op_desc = data_node->GetOpDesc();
//   AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
//   EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
//   aclgrphBuildFinalize();
// }

TEST_F(GeIrBuildTest, TestBuildModel_build_mode_invalid) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {BUILD_MODE, "invalid"},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModel_build_step_invalid) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {BUILD_STEP, "invalid"},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModel_build_mode_lead_step) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {BUILD_MODE, BUILD_MODE_TUNING},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, TestBuildModel_not_support_option) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {"invalid", "invalid"},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

// TEST_F(GeIrBuildTest, TestBuildModel_precision_mode_invalid) {
//   std::map<AscendString, AscendString> init_options;
//   EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

//   auto graph = GraphFactory::SingeOpGraph2();
//   std::map<string, string> build_options = {
//       {ge::ir_option::OP_PRECISION_MODE, "invalid"},
//   };
//   ModelBufferData model_buffer_data{};
//   auto compute_graph = GraphUtils::GetComputeGraph(graph);
//   auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
//   EXPECT_TRUE(data_node != nullptr);
//   auto op_desc = data_node->GetOpDesc();
//   AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
//   EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
//   aclgrphBuildFinalize();
// }

TEST_F(GeIrBuildTest, TestBuildModel_shape_generalized_build_mode_invalid) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto graph = GraphFactory::SingeOpGraph2();
  std::map<string, string> build_options = {
      {ge::ir_option::SHAPE_GENERALIZED_BUILD_MODE, "invalid"},
  };
  ModelBufferData model_buffer_data{};
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto data_node = compute_graph->FindFirstNodeMatchType(DATA);
  EXPECT_TRUE(data_node != nullptr);
  auto op_desc = data_node->GetOpDesc();
  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  EXPECT_NE(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

TEST_F(GeIrBuildTest, test_build_and_save_big_model) {
  std::map<AscendString, AscendString> init_options;
  EXPECT_EQ(aclgrphBuildInitialize(init_options), SUCCESS);

  auto sub_data_1 = OP_CFG(DATA).Attr("index", 0)
                                .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 512, 1024, 1024});
  GeTensorDesc data_tensor_desc(GeShape({1, 512, 1024, 1024}), FORMAT_NCHW, DT_FLOAT);
  std::vector<float32_t> data_value_vec1(1 * 512 * 1024 * 1024, 1);
  GeTensorPtr data_tensor1 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec1.data(),
                                                   1 * 512 * 1024 * 1024 * sizeof(DT_FLOAT));
  auto const1 = OP_CFG(CONSTANT).Weight(data_tensor1);
  auto sub_add = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 512, 1024, 1024});
  auto sub_net_output = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 512, 1024, 1024});
  DEF_GRAPH(g1) {
    CHAIN(NODE("sub_data_1", sub_data_1)->EDGE(0, 0)->NODE("sub_add", sub_add));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("sub_add", sub_add));
    CHAIN(NODE("sub_add", sub_add)->EDGE(0, 0)->NODE("sub_net_output", sub_net_output));
  };

  const auto graph = ToGeGraph(g1);
  std::map<string, string> build_options{};
  ModelBufferData model_buffer_data{};
  EXPECT_EQ(aclgrphBuildModel(graph, build_options, model_buffer_data), SUCCESS);
  std::string output_file = "saved_model.om";
  EXPECT_EQ(aclgrphSaveModel(output_file, model_buffer_data), SUCCESS);
  aclgrphBuildFinalize();
}

}  // namespace ge
