/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#include <stdio.h>
#include <gtest/gtest.h>
#include "ir_build/option_utils.h"
#include "graph/testcase/ge_graph/graph_builder_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "ge/ge_ir_build.h"
#include "graph/ops_stub.h"
#include "ge/ge_api_types.h"
#define protected public
#define private public
#include "ge/ge_ir_build.h"
#include "ir_build/option_utils.h"
#include "ir_build/attr_options/attr_options.h"
#undef private
#undef protected

const string DATA = "Data";
const string AddNYes = "AddNYes";
const string NETOUTPUT = "NetOutput";

using namespace ge;
class UtestIrCommon : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

class UtestIrBuild : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

static ge::OpDescPtr CreateOpDesc(const std::string &name, const std::string &type) {
  OpDescPtr op_desc = std::make_shared<ge::OpDesc>(name, type);
  ge::GeTensorDesc ge_tensor_desc;
  op_desc->AddInputDesc("input", ge_tensor_desc);
  op_desc->AddOutputDesc("output", ge_tensor_desc);

  return op_desc;
}

static ComputeGraphPtr BuildComputeGraph() {
  auto builder = ut::GraphBuilder("test");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(data2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0,netoutput, 0);

  return builder.GetGraph();
}

static ComputeGraphPtr BuildComputeGraph1() {
  auto builder = ut::GraphBuilder("test");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto node1 = builder.AddNode("addd", "Mul", 2, 1);
  auto node2 = builder.AddNode("ffm", "FrameworkOp", 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(data1, 0, addn1, 0);
  builder.AddDataEdge(data2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0,netoutput, 0);

  return builder.GetGraph();
}

// data not set attr index;
// but becasue of op proto, register attr index. so all data index is zero;
static Graph BuildIrGraph() {
  auto data1 = op::Data("data1");
  auto data2 = op::Data("data2");
  auto data3 = op::Data("data3");
  std::vector<Operator> inputs {data1, data2, data3};
  std::vector<Operator> outputs;

  Graph graph("test_graph");
  graph.SetInputs(inputs).SetOutputs(outputs);
  return graph;
}

// data set attr index, but is not valid
static Graph BuildIrGraph1() {
  auto data1 = op::Data("data1").set_attr_index(0);
  auto data2 = op::Data("data2").set_attr_index(1);
  auto data3 = op::Data("data3");
  auto data4 = op::Data("Test");
  std::vector<Operator> inputs {data1, data2, data3, data4};
  std::vector<Operator> outputs;

  Graph graph("test_graph");
  graph.AddNodeByOp(Operator("gg", "Mul"));
  graph.SetInputs(inputs).SetOutputs(outputs);
  return graph;
}

// data set attr index, but is not valid
static Graph BuildIrGraph2() {
  auto data1 = op::Data("data1").set_attr_index(0);
  auto data2 = op::Data("data2");
  auto data3 = op::Data("data3").set_attr_index(2);
  std::vector<Operator> inputs {data1, data2, data3};
  std::vector<Operator> outputs;

  Graph graph("test_graph");
  graph.SetInputs(inputs).SetOutputs(outputs);
  return graph;
}

// data set attr index
static Graph BuildIrGraph3() {
  auto data1 = op::Data("data1").set_attr_index(0);
  auto data2 = op::Data("data2").set_attr_index(1);
  auto data3 = op::Data("data3").set_attr_index(2);
  std::vector<Operator> inputs {data1, data2, data3};
  std::vector<Operator> outputs;

  Graph graph("test_graph");
  graph.SetInputs(inputs).SetOutputs(outputs);
  return graph;
}

TEST(UtestIrCommon, update_data_op_shape) {
  ge::OpDescPtr op_desc = CreateOpDesc("Data", "Data");
  map<string, vector<int64_t>> shape_map;
  shape_map["Data"] = {{1,2}};

  Status ret = UpdateDataOpShape(op_desc, shape_map);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST(UtestIrCommon, update_data_op_shape_range) {
  ge::OpDescPtr op_desc = CreateOpDesc("Data", "Data");
  std::vector<std::vector<std::pair<int64_t, int64_t>>> index_shape_range_map;

  std::pair<int64_t, int64_t> range_pair(1, 2);
  vector<pair<int64_t, int64_t>> range_pair_tmp = { range_pair };

  index_shape_range_map.push_back(range_pair_tmp);

  AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  Status ret = UpdateDataOpShapeRange(op_desc, index_shape_range_map);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST(UtestIrCommon, update_dynamic_shape_range_success) {
  ComputeGraphPtr graph = BuildComputeGraph();
  std::string input_shape_range = "input1:[1, 2~3, -1];input2:[3~5, 10]";

  Status ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST(UtestIrCommon, update_dynamic_shape_range_failed) {
  ComputeGraphPtr graph = BuildComputeGraph();
  // 1
  std::string input_shape_range = "input1;[1, 2~3, -1]";
  Status ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::PARAM_INVALID);

  // 2
  input_shape_range = "input1:[1, 2~3, -1)";
  ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::PARAM_INVALID);

  //3
  input_shape_range = "input1:[1, 3~2, -1];input2:[3~5, 10]";
  ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::FAILED);

  //4
  input_shape_range = "input1:[1, 2~-3, -1]";
  ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::PARAM_INVALID);

  //5
  input_shape_range = "input:[1, 2~3, -1]";
  ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::PARAM_INVALID);

  //6
  input_shape_range = "addn1:[1, 2~3, -1]";
  ret = UpdateDynamicInputShapeRange(graph, input_shape_range);
  EXPECT_EQ(ret, ge::PARAM_INVALID);
}

TEST(UtestIrCommon, check_dynamic_image_size_fail) {
  map<string, vector<int64_t>> shape_map;
  shape_map["input1"] = {8, 3, -1, -1};
  string input_format = "NCHW";
  string dynamic_image_size = "@64,64;128,128;";

  bool ret = CheckDynamicImagesizeInputShapeValid(shape_map, input_format, dynamic_image_size);
  EXPECT_EQ(ret, false);
}

TEST(UtestIrCommon, check_input_format_failed) {
  std::string format = "invalid";
  Status ret = CheckInputFormat(format);
  EXPECT_EQ(ret, ge::PARAM_INVALID);
}

TEST(UtestIrCommon, check_dynamic_batch_size_input_shape_succ) {
  map<string, vector<int64_t>> shape_map;
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {-1, 2, 3}));
  std::string dynamic_batch_size = "11";

  bool ret = CheckDynamicBatchSizeInputShapeValid(shape_map, dynamic_batch_size);
  EXPECT_EQ(ret, true);
}

TEST(UtestIrCommon, check_dynamic_images_size_input_shape_succ) {
  map<string, vector<int64_t>> shape_map;
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {4, -1, -1, 5}));
  std::string input_format = "NCHW";
  std::string dynamic_image_size = "4,5";

  Status ret = CheckDynamicImagesizeInputShapeValid(shape_map, input_format, dynamic_image_size);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST(UtestIrCommon, check_dynamic_input_param_succ) {
  string dynamic_batch_size = "1";
  string dynamic_image_size;
  string dynamic_dims;
  string input_shape = "data:-1,3,244,244";
  string input_shape_range;
  string input_format = "NCHW";
  bool is_dynamic_input = false;

  Status ret = CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
                                           input_shape, input_shape_range, input_format,is_dynamic_input);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST(UtestIrCommon, check_dynamic_input_param_failed) {
  string dynamic_batch_size = "1";
  string dynamic_image_size;
  string dynamic_dims;
  string input_shape = "data:1,3,244,244";
  string input_shape_range;
  string input_format = "NCHW";
  bool is_dynamic_input = false;

  Status ret = CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
                                           input_shape, input_shape_range, input_format,is_dynamic_input);
  EXPECT_EQ(ret, ge::PARAM_INVALID);
}

TEST(UtestIrCommon, check_modify_mixlist_param) {
  std::string precision_mode = "allow_mix_precision";
  std::string modify_mixlist = "/mixlist.json";
  Status ret = CheckModifyMixlistParamValid(precision_mode, modify_mixlist);
  EXPECT_EQ(ret, ge::SUCCESS);

  precision_mode = "";
  ret = CheckModifyMixlistParamValid(precision_mode, modify_mixlist);
  EXPECT_EQ(ret, ge::PARAM_INVALID);
}

TEST(UtestIrCommon, check_compress_weight) {
  std::string enable_compress_weight = "true";
  std::string compress_weight_conf="./";
  Status ret = CheckCompressWeightParamValid(enable_compress_weight, compress_weight_conf);
  EXPECT_EQ(ret, PARAM_INVALID);

  enable_compress_weight = "yes";
  compress_weight_conf = "./";
  ret = CheckCompressWeightParamValid(enable_compress_weight, compress_weight_conf);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST(UtestIrCommon, check_sparsity) {
  EXPECT_EQ(CheckSparseParamValid("-1"), PARAM_INVALID);
  EXPECT_EQ(CheckSparseParamValid("1"), SUCCESS);
}

TEST(UtestIrCommon, check_param_failed) {
  std::string param_invalid = "invalid";

  Status ret = CheckOutputTypeParamValid(param_invalid);
  EXPECT_EQ(ret, PARAM_INVALID);

  ret = CheckBufferOptimizeParamValid(param_invalid);
  EXPECT_EQ(ret, PARAM_INVALID);

  //ret = CheckKeepTypeParamValid(param_invalid);
  //EXPECT_EQ(ret, PARAM_INVALID);

  // ret = CheckInsertOpConfParamValid(param_invalid);
  // EXPECT_EQ(ret, PARAM_INVALID);

  //ret = CheckDisableReuseMemoryParamValid(param_invalid);
  //EXPECT_EQ(ret, PARAM_INVALID);

  ret = CheckEnableSingleStreamParamValid(param_invalid);
  EXPECT_EQ(ret, PARAM_INVALID);

  std::string optypelist_for_implmode;
  std::string op_select_implmode = "1";
  ret = CheckImplmodeParamValid(optypelist_for_implmode, op_select_implmode);
  EXPECT_EQ(ret, PARAM_INVALID);

  ret = CheckLogParamValidAndSetLogLevel(param_invalid);
}

// Get attr index failed, when set input shape range
TEST(UtestIrBuild, check_data_op_attr_index_invalid_0) {
  ComputeGraphPtr compute_graph = BuildComputeGraph();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  const map<string, string> build_options = {
    {"input_shape_range", "[1, 2~3, -1],[4~5, 3~5, 10],[1, 2~3, -1]"}
  };
  ModelBufferData model;
  graphStatus ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

// not set attr index, when set input shape range
TEST(UtestIrBuild, check_data_op_attr_index_invalid_1) {
  Graph graph = BuildIrGraph();
  const map<string, string> build_options = {
    {"input_shape_range", "[1, 2~3, -1],[4~5, 3~5, 10],[1, 2~3, -1]"}
  };
  ModelBufferData model;
  graphStatus ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

// set attr index, but not valid, when set input shape range
TEST(UtestIrBuild, check_data_op_attr_index_invalid_2) {
  Graph graph = BuildIrGraph1();
  const map<string, string> build_options = {
    {"input_shape_range", "[1, 2~3, -1],[4~5, 3~5, 10],[1, 2~3, -1]"}
  };
  ModelBufferData model;
  graphStatus ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, GRAPH_FAILED);

  Graph graph2 = BuildIrGraph2();
  ret = aclgrphBuildModel(graph2, build_options, model);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

// set attr index valid, when set input shape range
// only check data op attr index valid func.
TEST(UtestIrBuild, check_data_op_attr_index_valid) {
  Graph graph = BuildIrGraph3();
  const map<string, string> build_options = {
    {"input_shape_range", "[1, 2~3, -1],[4~5, 3~5, 10],[1, 2~3, -1]"}
  };
  ModelBufferData model;
  graphStatus ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, ge::GE_CLI_GE_NOT_INITIALIZED);
}

// set attr index invalid, when not set input shape range
// only check data op attr index valid func.
TEST(UtestIrBuild, check_data_attr_index_succ_no_input_range) {
  Graph graph = BuildIrGraph1();
  const map<string, string> build_options;
  ModelBufferData model;
  graphStatus ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, ge::GE_CLI_GE_NOT_INITIALIZED);
}

TEST(UtestIrBuild, check_modify_mixlist_param) {
  Graph graph = BuildIrGraph1();
  const std::map<std::string, std::string> build_options = {
    {"ge.exec.modify_mixlist", "/modify.json"}
  };
  ModelBufferData model;

  auto ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, GRAPH_PARAM_INVALID);
}

TEST(UtestIrBuild, check_op_precision_mode_param) {
  Graph graph = BuildIrGraph1();
  const std::map<std::string, std::string> build_options = {
    {"ge.exec.op_precision_mode", "./op_precision_mode.ini"}
  };
  ModelBufferData model;

  auto ret = aclgrphBuildModel(graph, build_options, model);
  EXPECT_EQ(ret, GRAPH_PARAM_INVALID);
}

TEST(UtestIrBuild, check_build_model_and_build_step) {
  Graph graph_1 = BuildIrGraph1();
  const std::map<std::string, std::string> build_options_1 = {
    {"ge.buildMode", "xxx"}
  };
  ModelBufferData model_1;
  auto ret_1 = aclgrphBuildModel(graph_1, build_options_1, model_1);
  EXPECT_NE(ret_1, GRAPH_SUCCESS);

  Graph graph_2 = BuildIrGraph1();
  const std::map<std::string, std::string> build_options_2 = {
    {"ge.buildStep", "xxx"}
  };
  ModelBufferData model_2;
  auto ret_2 = aclgrphBuildModel(graph_2, build_options_2, model_2);
  EXPECT_NE(ret_2, GRAPH_SUCCESS);

  Graph graph_3 = BuildIrGraph1();
  const std::map<std::string, std::string> build_options_3 = {
    {"ge.buildMode", "tuning"}
  };
  ModelBufferData model_3;
  auto ret_3 = aclgrphBuildModel(graph_3, build_options_3, model_3);
  EXPECT_NE(ret_3, GRAPH_SUCCESS);
}

TEST(UtestIrBuild, atc_cfg_optype_param) {
  ComputeGraphPtr graph = BuildComputeGraph1();
  FILE *fp = fopen("./keep.txt", "w+");
  if (fp) {
    fprintf(fp, "Test\n");
    fprintf(fp, "OpType::Mul\n");
    fprintf(fp, "Optype::Sub\n");
    fclose(fp);
  }
  auto ret = KeepDtypeFunc(graph, "./keep.txt");
  (void)remove("./keep.txt");
  EXPECT_EQ(ret, GRAPH_PARAM_INVALID);
}

TEST(UtestIrBuild, aclgrphGenerateForOp_test) {
  ge::AscendString type("Add");
  ge::TensorDesc tensor_desc;
  int32_t *c = new int32_t[10];
  void *d = static_cast<void *>(c);
  std::unique_ptr<uint8_t[]> data = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t *>(d));
  tensor_desc.SetConstData(std::move(data), sizeof(int));
  ge::Graph graph;
  ge::graphStatus ret = ge::aclgrphGenerateForOp(type, vector<ge::TensorDesc>{tensor_desc},
                                                 vector<ge::TensorDesc>{tensor_desc}, graph);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, aclgrphBuildInitialize_test) {
  std::map<std::string, std::string> global_options;
  global_options[ge::OPTION_EXEC_HCCL_FLAG] = "0";
  ge::graphStatus ret = ge::aclgrphBuildInitialize(global_options);
  ge::aclgrphBuildFinalize();
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);

  std::map<AscendString, AscendString> global_options1;
  global_options1[ge::OPTION_EXEC_HCCL_FLAG] = "0";
  ret = ge::aclgrphBuildInitialize(global_options1);
  ge::aclgrphBuildFinalize();
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, check_dynamic_dims_input_shape_valid_test) {
  std::map<std::string, std::vector<int64_t>> shape_map;
  std::string input_format;
  std::string dynamic_dims;
  std::vector<int64_t> shape = {1, 3, 224,224};
  shape_map.insert(map<std::string, std::vector<int64_t>>::value_type ("NCHW", shape));
  auto ret = ge::CheckDynamicDimsInputShapeValid(shape_map, input_format, dynamic_dims);
  EXPECT_EQ(ret, false);
  input_format = "ND";
  dynamic_dims;
  ret = ge::CheckDynamicDimsInputShapeValid(shape_map, input_format, dynamic_dims);
  EXPECT_EQ(ret, false);
}

TEST(UtestIrBuild, check_and_parse_dynamic_dims_test) {
  const std::map<std::string, std::vector<int64_t>> shape_map;
  int32_t dynamic_dim_num;
  std::string dynamic_dims = "";
  auto ret = ge::CheckAndParseDynamicDims(dynamic_dim_num, dynamic_dims);
  EXPECT_EQ(ret, false);
  dynamic_dims = "1;3;224;224";
  ret = ge::CheckAndParseDynamicDims(dynamic_dim_num, dynamic_dims);
  EXPECT_EQ(ret, false);
}

TEST(UtestIrBuild, parse_input_shape_range_test) {
  std::string shape_range;
  std::vector<std::vector<std::pair<int64_t, int64_t>>> range;
  auto ret = ge::ParseInputShapeRange(shape_range, range);
  ASSERT_NE(ret, SUCCESS);
  shape_range = "1;3;224;224";
  ret = ge::ParseInputShapeRange(shape_range, range);
  ASSERT_EQ(ret, SUCCESS);
}

TEST(UtestIrBuild, parse_check_dynamic_input_param_valid_test) {
  std::string dynamic_batch_size;
  std::string dynamic_image_size;
  std::string dynamic_dims;
  std::string input_shape;
  std::string input_shape_range = "1:3:224:224";
  std::string input_format;
  bool is_dynamic_input;
  auto ret = ge::CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
      input_shape, input_shape_range, input_format, is_dynamic_input);
  ASSERT_NE(ret, SUCCESS);
  dynamic_batch_size = "1";
  dynamic_image_size = "1";
  dynamic_dims = "1:3";
  ret = ge::CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
      input_shape, input_shape_range, input_format, is_dynamic_input);
  ASSERT_NE(ret, SUCCESS);
  dynamic_image_size = "1";
  dynamic_dims = "";
  input_shape = "1";
  ret = ge::CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
      input_shape, input_shape_range, input_format, is_dynamic_input);
  ASSERT_NE(ret, SUCCESS);
  dynamic_image_size = "";
  dynamic_dims = "1";
  input_shape = "1";
  ret = ge::CheckDynamicInputParamValid(dynamic_batch_size, dynamic_image_size, dynamic_dims,
      input_shape, input_shape_range, input_format, is_dynamic_input);
  ASSERT_NE(ret, SUCCESS);
}

TEST(UtestIrBuild, check_log_param_valid_and_set_log_level_test) {
  std::string log = "";
  auto ret = ge::CheckLogParamValidAndSetLogLevel(log);
  ASSERT_EQ(ret, -1);
  log = "debug";
  ret = ge::CheckLogParamValidAndSetLogLevel(log);
  ASSERT_EQ(ret, 0);
  log = "info";
  ret = ge::CheckLogParamValidAndSetLogLevel(log);
  ASSERT_NE(ret, -1);
  log = "warning";
  ret = ge::CheckLogParamValidAndSetLogLevel(log);
  ASSERT_NE(ret, -1);
  log = "error";
  ret = ge::CheckLogParamValidAndSetLogLevel(log);
  ASSERT_NE(ret, -1);
}

TEST(UtestIrBuild, check_impl_mode_param_valid_test) {
  std::string optypelist_for_implmode = "mode";
  std::string op_select_implmode = "";
  auto ret = ge::CheckImplmodeParamValid(optypelist_for_implmode, op_select_implmode);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST(UtestIrBuild, update_data_op_shape_range_test) {
  OpDescPtr op = CreateOpDesc("data1", "DATA");
  std::vector<std::vector<std::pair<int64_t, int64_t>>> index_shape_range_map;
  auto ret = ge::UpdateDataOpShapeRange(op, index_shape_range_map);
  EXPECT_EQ(ret, SUCCESS);
  int64_t index = -1;
  AttrUtils::SetInt(op, ATTR_NAME_INDEX, index);
  index = 0;
  AttrUtils::GetInt(op, ATTR_NAME_INDEX, index);
  ret = ge::UpdateDataOpShapeRange(op, index_shape_range_map);
  EXPECT_EQ(ret, SUCCESS);
}

TEST(UtestIrBuild, aclgrphBuildModel_test) {
  Graph graph_1 = BuildIrGraph1();
  ModelBufferData model_1;
  std::map<AscendString, AscendString> global_options1;
  global_options1[ge::OPTION_EXEC_HCCL_FLAG] = "0";
  auto ret = ge::aclgrphBuildModel(graph_1, global_options1, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, aclgrphBuildModel_test1) {
  ModelBufferData model_1;
  std::string output_file = "./test";
  auto ret = ge::aclgrphSaveModel(output_file, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);

  char *output_file1 = nullptr;
  ret = ge::aclgrphSaveModel(output_file1, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
  Graph graph_1 = BuildIrGraph1();
  const std::map<std::string, std::string> build_options_1 = {
    {"ge.buildMode", "xxx"}
  };
  auto ret_1 = aclgrphBuildModel(graph_1, build_options_1, model_1);
  EXPECT_NE(ret_1, GRAPH_SUCCESS);
  ret = ge::aclgrphSaveModel(output_file, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
  ret = ge::aclgrphSaveModel(output_file1, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
  output_file1 = "./test1";
  ret = ge::aclgrphSaveModel(output_file1, model_1);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, aclgrphGetIRVersion_test) {
  int32_t version = 1;
  int32_t *major_version = &version;
  int32_t *minor_version = &version;
  int32_t *patch_version = &version;
  auto ret = ge::aclgrphGetIRVersion(major_version, minor_version, patch_version);
  EXPECT_EQ(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, aclgrphBuildModel_test2) {
  Graph graph_1 = BuildIrGraph1();
  char *file;
  size_t len = 0;
  auto ret = ge::aclgrphDumpGraph(graph_1, file, len);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, aclgrphSetOpAttr_test) {
  Graph graph_1 = BuildIrGraph1();
  aclgrphAttrType attr_type;
  char *cfg_path = "./test";
  auto ret = ge::aclgrphSetOpAttr(graph_1, attr_type, cfg_path);
  EXPECT_NE(ret, ge::GRAPH_SUCCESS);
}

TEST(UtestIrBuild, check_dynamic_imagesize_input_shapevalid_test) {
  OpDescPtr op = CreateOpDesc("data1", "DATA");
  std::map<std::string, std::vector<int64_t>> shape_map;
  std::string input_format = "HHH";
  std::string dynamic_image_size;
  auto ret = ge::CheckDynamicImagesizeInputShapeValid(shape_map, input_format, dynamic_image_size);
  EXPECT_EQ(ret, SUCCESS);
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {1, 3, 224}));
  input_format = "NCHW";
  ret = ge::CheckDynamicImagesizeInputShapeValid(shape_map, input_format, dynamic_image_size);
  EXPECT_EQ(ret, SUCCESS);
  dynamic_image_size = "1,3,224";
  ret = ge::CheckDynamicImagesizeInputShapeValid(shape_map, input_format, dynamic_image_size);
  EXPECT_EQ(ret, SUCCESS);
}

TEST(UtestIrCommon, check_dynamic_batch_size_input_shape_test1) {
  map<string, vector<int64_t>> shape_map;
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {-1, 2, 3}));
  std::string dynamic_batch_size = "11:11";
  auto ret = CheckDynamicBatchSizeInputShapeValid(shape_map, dynamic_batch_size);
  EXPECT_NE(ret, true);
}

TEST(UtestIrCommon, weight_compress_func_test) {
    ComputeGraphPtr graph = BuildComputeGraph1();
    std::string currentDir = "";
    auto ret = ge::WeightCompressFunc(graph, currentDir);
    ASSERT_EQ(ret, SUCCESS);
    currentDir = "./test";
    ret = ge::WeightCompressFunc(graph, currentDir);
    ASSERT_NE(ret, SUCCESS);
    currentDir = __FILE__;
    std::size_t idx = currentDir.find_last_of("/");
    currentDir = currentDir.substr(0, idx) + "/weight_compress.cfg";
    ret = ge::WeightCompressFunc(graph, currentDir);
    ASSERT_EQ(ret, SUCCESS);
}

TEST(UtestIrCommon, is_original_op_find_test) {
    ge::OpDescPtr op_desc = CreateOpDesc("Data", "Data");
    std::vector<std::vector<std::pair<int64_t, int64_t>>> index_shape_range_map;
    std::string op_name = "Data";
    vector<pair<int64_t, int64_t>> range_pair_tmp;
    std::vector<std::string> original_op_names = {"DATA"};
    AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_op_names);
    index_shape_range_map.push_back(range_pair_tmp);

    AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
    auto ret = ge::IsOriginalOpFind(op_desc, op_name);
    ASSERT_EQ(ret, SUCCESS);
    op_name = "TEST";
    ret = ge::IsOriginalOpFind(op_desc, op_name);
    ASSERT_EQ(ret, SUCCESS);
}

TEST(UtestIrCommon, is_op_type_equal_test) {
    auto builder = ut::GraphBuilder("test");
    NodePtr data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
    NodePtr data2 = builder.AddNode("input2", "CASE", 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
    std::string op_type = "DATA1";
    auto ret = ge::IsOpTypeEqual(data1, op_type);
    ASSERT_EQ(ret, SUCCESS);
    data1->SetOrigNode(data2);
    ret = ge::IsOpTypeEqual(data1, op_type);
    ASSERT_EQ(ret, SUCCESS);
}

TEST(UtestIrBuild, aclgrphSaveModelTest) {
  ModelBufferData model_1;
  model_1.length = 1;
  model_1.data = std::make_shared<uint8_t>(1);
  char *output_file = nullptr;
  auto ret = ge::aclgrphSaveModel(output_file, model_1);
  EXPECT_EQ(ret, GRAPH_PARAM_INVALID);

  char *output_file2 = "./graph1.txt";
  ret = ge::aclgrphSaveModel(output_file2, model_1);
  EXPECT_EQ(ret, SUCCESS);
  system("rm ./graph1.txt");

  std::string output_file3 = "./graph2.txt";
  ret = ge::aclgrphSaveModel(output_file3, model_1);
  EXPECT_EQ(ret, SUCCESS);
  system("rm ./graph2.txt");
}

TEST(UtestIrCommon, CheckDynamicBatchSizeInputShapeValidTest) {
  map<string, vector<int64_t>> shape_map;
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {}));
  std::string dynamic_batch_size = "1";

  bool ret = CheckDynamicBatchSizeInputShapeValid(shape_map, dynamic_batch_size);
  EXPECT_EQ(ret, false);
}

TEST(UtestIrCommon, CheckDynamicDimsInputShapeValidTest) {
  map<string, vector<int64_t>> shape_map;
  shape_map.insert(std::pair<string, vector<int64_t>>("data", {}));
  std::string input_format = "ND";
  std::string dynamic_dims;

  bool ret = CheckDynamicDimsInputShapeValid(shape_map, input_format, dynamic_dims);
  EXPECT_EQ(ret, false);
}
