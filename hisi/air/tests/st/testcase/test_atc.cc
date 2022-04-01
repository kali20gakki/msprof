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
#include "ge_running_env/op_reg.h"

#include "utils/model_factory.h"
#include "parser/common/register_tbe.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "framework/omg/parser/parser_inner_ctx.h"
#include "utils/graph_utils.h"
#include "types.h"
#include "init_ge.h"

namespace ge {
class AtcCommonSTest : public AtcTest {};

TEST_F(AtcCommonSTest, pb_model_common_1) {
  std::vector<OpRegistrationData> registrationDatas = domi::OpRegistry::Instance()->registrationDatas;
  for (OpRegistrationData reg_data : registrationDatas) {
    if (reg_data.GetFrameworkType() == domi::TENSORFLOW) {
      (void)ge::OpRegistrationTbe::Instance()->Finalize(reg_data);
      (void)domi::OpRegistry::Instance()->Register(reg_data);
    }
  }

  GetParserContext().default_out_nodes.push_back(std::make_pair("add_test_1", 0));

  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  //"--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_set_out_tensor_names) {
  std::vector<OpRegistrationData> registrationDatas = domi::OpRegistry::Instance()->registrationDatas;
  for (OpRegistrationData reg_data : registrationDatas) {
    if (reg_data.GetFrameworkType() == domi::TENSORFLOW) {
      (void)ge::OpRegistrationTbe::Instance()->Finalize(reg_data);
      (void)domi::OpRegistry::Instance()->Register(reg_data);
    }
  }

  GetParserContext().out_tensor_names.push_back("out_tensor");

  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  //"--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_common_2) {
  unsetenv("ASCEND_OPP_PATH");
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_2");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string keep_dtype = "--keep_dtype=st_run_data/config/keep_dtype.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=add_test_1:0:FP16",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--display_model_info=1",
                  "--precision_mode=force_fp16",
                  "--input_fp16_nodes=Placeholder_1",
                  "--save_original_model=true",
                  const_cast<char *>(keep_dtype.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_keep_dtype_invalid) {
  unsetenv("ASCEND_OPP_PATH");
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_2");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string keep_dtype = "--keep_dtype=invalid";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--display_model_info=1",
                  "--precision_mode=force_fp16",
                  "--input_fp16_nodes=Placeholder_1",
                  "--save_original_model=true",
                  const_cast<char *>(keep_dtype.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_fp16_and_NCIHWC0) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_2");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--input_fp16_nodes=Placeholder_1",
                  "--is_input_adjust_hw_layout=true",
                  "--is_output_adjust_hw_layout=true",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
  CHECK_GRAPH(PreRunBegin) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  };
}

TEST_F(AtcCommonSTest, pb_model_auto_tune_mode) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--auto_tune_mode=RL,GA",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
  CHECK_GRAPH(PreRunBegin) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  };
}

TEST_F(AtcCommonSTest, pb_model_only_precheck) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--mode=3", // FrameworkType
                  "--framework=3", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_with_weight_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_sparse_weight) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend920A",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--sparsity=1",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_weight_compress_both_exist) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string compress_weight_conf = "--compress_weight_conf=st_run_data/config/compress_weight_nodes.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--enable_compress_weight=true",
                  const_cast<char *>(compress_weight_conf.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_weight_compress_conf_invalid) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string compress_weight_conf = "--compress_weight_conf=st_run_data/invalid";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--enable_compress_weight=true",
                  const_cast<char *>(compress_weight_conf.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_weight_compress_enable_invalid) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--enable_compress_weight=invalid",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_out_node_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=invalid:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_out_node_leak_port) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_out_node_port_not_digit) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  std::string op_name_map = "--op_name_map=st_run_data/config/opname_map.cfg";
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:a",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  const_cast<char *>(op_name_map.c_str()),
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, onnx_model_common) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "onnx_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/test.onnx";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=5", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--out_nodes=Conv_0:0;Add_0:0",
                  "--soc_version=Ascend310",
                  "--output_type=Conv_0:0:FP32;Add_0:0:FP16",
                  "--input_shape=x:1,3,640,640",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, onnx_model_common_2) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "onnx_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/test.onnx";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=5", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--out_nodes=Conv_0:0;Add_0:0",
                  "--soc_version=Ascend310",
                  "--input_format=NCHW",
                  "--output_type=Conv_0:0:FP32;Add_0:0:FP16",
                  "--input_shape=x:1,3,640,640",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, onnx_model_input_format_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "onnx_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/test.onnx";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=5", // FrameworkType
                  "--weight=st_run_data/not_exist",
                  "--out_nodes=Conv_0:0;Add_0:0",
                  "--soc_version=Ascend310",
                  "--input_format=NC1HWC0",
                  "--output_type=Conv_0:0:FP32;Add_0:0:FP16",
                  "--input_shape=x:1,3,640,640",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, mindspore_model_common) {
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
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it

  CHECK_GRAPH(PreRunBegin) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 5);
  };
}

// depends on mindspore success
TEST_F(AtcCommonSTest, om_convert_to_json) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "ms_1");

  // test convert to json
  std::string om_arg = "--om=" + om_path + ".om";
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(om_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--mode=1",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(IsFile((om_path + ".json").c_str()), true);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, om_convert_to_json_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "ms_1");

  // test convert to json
  std::string om_arg = "--om=st_run_data/origin_model/not_exist.om";
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(om_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--mode=1",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, om_display_info) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "ms_1");

  // test convert to json
  std::string om_arg = "--om=" + om_path + ".om";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(om_arg.c_str()),
                                  "--mode=6",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, origin_model_convert_to_json_without_dump_mode) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/origin_model/add.pb";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pb_json_1");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--framework=3", // FrameworkType
                                  "--mode=1",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(IsFile((om_path + ".json").c_str()), true);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, origin_model_convert_to_json_with_dump_mode) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/origin_model/add.pb";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pb_json_1");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--framework=3", // FrameworkType
                                  "--mode=1",
                                  "--dump_mode=1",
                                  "--out_nodes=add_test_1:0",
                                  "--input_shape=Placeholder_1:1,256,256,3",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(IsFile((om_path + ".json").c_str()), true);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, origin_model_convert_to_json_invalid_framework) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/origin_model/add.pb";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pb_json_1");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--framework=10", // FrameworkType
                                  "--mode=1",
                                  "--out_nodes=add_test_1:0",
                                  "--input_shape=Placeholder_1:1,256,256,3",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, origin_model_convert_to_json_invalid_dump_mode) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/origin_model/add.pb";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pb_json_1");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--framework=10", // FrameworkType
                                  "--mode=1",
                                  "--dump_mode=10",
                                  "--out_nodes=add_test_1:0",
                                  "--input_shape=Placeholder_1:1,256,256,3",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pbtxt_convert_to_json) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/origin_model/origin.txt";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pbtxt_json");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--mode=5",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(IsFile((om_path + ".json").c_str()), true);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pbtxt_convert_to_json_file_not_exist) {
  ReInitGe();
  std::string model_arg = "--om=st_run_data/invalid";
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  om_path = PathJoin(om_path.c_str(), "pbtxt_json");
  std::string json_arg = "--json="+om_path+".json";
  char *convert_to_json_argv[] = {"atc",
                                  const_cast<char *>(model_arg.c_str()),
                                  const_cast<char *>(json_arg.c_str()),
                                  "--mode=5",
                                  };
  auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

// TEST_F(AtcCommonSTest, pbtxt_convert_to_json_file_format_invalid) {
//   ReInitGe();
//   std::string model_arg = "--om=st_run_data/config/opname_map.cfg";
//   auto om_path = PathJoin(GetRunPath().c_str(), "temp");
//   om_path = PathJoin(om_path.c_str(), "pbtxt_json");
//   std::string json_arg = "--json="+om_path+".json";
//   char *convert_to_json_argv[] = {"atc",
//                                   const_cast<char *>(model_arg.c_str()),
//                                   const_cast<char *>(json_arg.c_str()),
//                                   "--mode=5",
//                                   };
//   auto ret = main_impl(sizeof(convert_to_json_argv)/sizeof(convert_to_json_argv[0]), convert_to_json_argv);
//   EXPECT_EQ(ret, -1);
//   ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
// }

TEST_F(AtcCommonSTest, pb_model_input_shape_range) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=Placeholder_1:[-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
  CHECK_GRAPH(PreRunBegin) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 3);
  };
}

TEST_F(AtcCommonSTest, pb_model_input_shape_range_node_not_exist) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=invalid:[1~8,256,256,-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_shape_range_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=add_test_1:[1~-1,256,256,-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_shape_range_lead_brace) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=add_test_1:1~8,256,256,-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_shape_range_lead_node) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=:[1~8,256,256,-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_shape_range_more_node) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_common_1");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape_range=a:b:[1~8,256,256,-1]",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}


TEST_F(AtcCommonSTest, pb_op_precision_mode_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--op_precision_mode=st_run_data/not_exist",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_modify_mixlist_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--modify_mixlist=st_run_data/not_exist",
                  "--precision_mode=force_fp16",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_op_select_implmode_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--op_select_implmode=invalid",
                  "--optypelist_for_implmode=invalid",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_optypelist_for_implmode_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--op_select_implmode=invalid",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_framework_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=10", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_framework_caffe_fail) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=0", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_exceed_max_len) {
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string om_path(4097, 'a');
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_not_file) {
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output=st_run_data/";

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_tensorflow_format_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NC1HWC0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_caffe_format_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=0", // FrameworkType
                  "--input_format=NHWC",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_caffe_model_failed) {
  auto base_path = PathJoin(GetRunPath().c_str(), "temp/");
  Mkdir(base_path.c_str());

  mmSetEnv("ASCEND_OPP_PATH", base_path.c_str(), 1);

  auto test_so_path = PathJoin(GetRunPath().c_str(), "temp/framework");
  Mkdir(test_so_path.c_str());

  test_so_path = PathJoin(test_so_path.c_str(), "built-in");
  Mkdir(test_so_path.c_str());

  test_so_path = PathJoin(test_so_path.c_str(), "caffe");
  Mkdir(test_so_path.c_str());

  auto command = "touch " + test_so_path + "/lib_caffe_parser.so";
  system(command.c_str());

  auto om_path = PathJoin(base_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=0", // FrameworkType
                  "--weight=st_run_data/origin_model/add.pb",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_log_level_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--log=invalid",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, mode_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_abnormal");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--mode=10",
                  "--out_nodes=add_test_1:0",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, single_op) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "singleop");
  std::string single_op = "--singleop=st_run_data/json/single_op/add_op.json";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(single_op.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--soc_version=Ascend310",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, 0);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

// TEST_F(AtcCommonSTest, single_op_output_invalid) {
//   std::string single_op = "--singleop=st_run_data/json/single_op/add_op.json";

//   char *argv[] = {"atc",
//                   const_cast<char *>(single_op.c_str()),
//                   "--soc_version=Ascend310",
//                   };
//   auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
//   EXPECT_EQ(ret, -1);
//   ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
// }

TEST_F(AtcCommonSTest, single_op_op_precision_mode_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "singleop");
  std::string single_op = "--singleop=st_run_data/json/single_op/add_op.json";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(single_op.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--soc_version=Ascend310",
                  "--op_precision_mode=st_run_data/not_exist",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, single_op_modify_mixlist_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "singleop");
  std::string single_op = "--singleop=st_run_data/json/single_op/add_op.json";
  std::string output_arg = "--output="+om_path;

  char *argv[] = {"atc",
                  const_cast<char *>(single_op.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--soc_version=Ascend310",
                  "--modify_mixlist=st_run_data/not_exist",
                  };
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_is_input_adjust_hw_layout_invalid) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--input_fp16_nodes=Placeholder_1",
                  "--is_input_adjust_hw_layout=invald",
                  "--is_output_adjust_hw_layout=true",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_fp16_nodes_not_exist) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--input_fp16_nodes=invalid",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_model_input_fp16_nodes_not_data) {
  ReInitGe();
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--out_nodes=add_test_1:0",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  "--input_fp16_nodes=add_test_1",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_negative) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:-1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_float) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1.1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_node_not_exist) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=invalid:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_content_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=,:",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_type_not_data) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=add_test_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_not_digit) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:a,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_input_shape_exceed) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:2147483648,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_type_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=invalid",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_type_node_not_exist) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=invalid:0:FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_type_content_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=invalid:FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_type_not_digit) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=invalid:a:FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

TEST_F(AtcCommonSTest, pb_output_type_port_invalid) {
  auto om_path = PathJoin(GetRunPath().c_str(), "temp");
  Mkdir(om_path.c_str());
  om_path = PathJoin(om_path.c_str(), "pb_invalid");
  std::string model_arg = "--model=st_run_data/origin_model/add.pb";
  std::string output_arg = "--output="+om_path;
  char *argv[] = {"atc",
                  const_cast<char *>(model_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--framework=3", // FrameworkType
                  "--input_format=NCHW",
                  "--soc_version=Ascend310",
                  "--output_type=invalid:-1:FP32",
                  "--out_nodes=add_test_1:0",
                  "--input_shape=Placeholder_1:1,256,256,3",
                  };
  DUMP_GRAPH_WHEN("PreRunBegin")
  auto ret = main_impl(sizeof(argv)/sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
  ReInitGe(); // the main_impl will call GEFinalize, so re-init after call it
}

}

