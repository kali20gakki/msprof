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

#include <gtest/gtest.h>

#define private public
#define protected public

#include "offline/single_op_parser.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/util.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/operator_factory_impl.h"

#define private public
#define protected public

using namespace std;

namespace ge {
class UtestOmg : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestOmg, ParseSingleOpListTest) {
  std::string file;
  std::vector<SingleOpBuildParam> op_list;
  Status ret = SingleOpParser::ParseSingleOpList(file, op_list);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

TEST_F(UtestOmg, SetShapeRangeTest) {
  std::string op_name;
  SingleOpTensorDesc tensor_desc;
  tensor_desc.dims.push_back(1);
  tensor_desc.dims.push_back(-2);
  GeTensorDesc ge_tensor_desc;
  Status ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, PARAM_INVALID);

  tensor_desc.dims.clear();
  tensor_desc.dims.push_back(-2);
  std::vector<int64_t> ranges = {1, 2, 3};
  tensor_desc.dim_ranges.push_back(ranges);
  ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, PARAM_INVALID);

  tensor_desc.dim_ranges.clear();
  ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, SUCCESS);
  
  // dim < 0, range_index = num_shape_ranges
  tensor_desc.dims.clear();
  tensor_desc.dims.push_back(-1);
  ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, PARAM_INVALID);

  // dim < 0, range.size() != 2
  std::vector<int64_t> ranges2 = {1, 2, 3};
  tensor_desc.dim_ranges.push_back(ranges2);
  ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, PARAM_INVALID);

  // success
  tensor_desc.dim_ranges.clear();
  std::vector<int64_t> ranges3 = {1, 2};
  tensor_desc.dim_ranges.push_back(ranges3);
  ret = SingleOpParser::SetShapeRange(op_name, tensor_desc, ge_tensor_desc);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestOmg, ValidateTest) {
  //input_desc:!tensor_desc.GetValidFlag()
  SingleOpDesc op_desc;
  op_desc.op = "test";
  SingleOpTensorDesc input_tensor_desc;
  input_tensor_desc.SetValidFlag(false);
  op_desc.input_desc.push_back(input_tensor_desc);
  bool ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);
  
  //input_desc:tensor_desc.type == DT_UNDEFINED && tensor_desc.format != FORMAT_RESERVED
  op_desc.input_desc.clear();
  input_tensor_desc.SetValidFlag(true);
  input_tensor_desc.type = ge::DT_UNDEFINED;
  input_tensor_desc.format = ge::FORMAT_FRACTAL_Z_G;
  op_desc.input_desc.push_back(input_tensor_desc);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);

  //output_desc:!tensor_desc.GetValidFlag()
  op_desc.input_desc.clear();
  SingleOpTensorDesc output_tensor_desc;
  output_tensor_desc.SetValidFlag(false);
  op_desc.output_desc.push_back(output_tensor_desc);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);

  //output_desc:tensor_desc.type == DT_UNDEFINED
  op_desc.output_desc.clear();
  output_tensor_desc.SetValidFlag(true);
  output_tensor_desc.type = ge::DT_UNDEFINED;
  op_desc.output_desc.push_back(output_tensor_desc);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);

  //output_desc:tensor_desc.format == FORMAT_RESERVED
  op_desc.output_desc.clear();
  output_tensor_desc.SetValidFlag(true);
  output_tensor_desc.type = ge::DT_FLOAT16;
  output_tensor_desc.format = ge::FORMAT_RESERVED;
  op_desc.output_desc.push_back(output_tensor_desc);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);

  //attr.name.empty()
  op_desc.output_desc.clear();
  SingleOpAttr op_attr;
  op_desc.attrs.push_back(op_attr);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);

  //attr.value.IsEmpty()
  op_desc.attrs.clear();
  op_attr.name = "test1";
  op_desc.attrs.push_back(op_attr);
  ret = SingleOpParser::Validate(op_desc);
  EXPECT_EQ(ret, false);
}

TEST_F(UtestOmg, UpdateDynamicTensorNameTest) {
  SingleOpTensorDesc tensor_desc;
  tensor_desc.dynamic_input_name = "test1";
  std::vector<SingleOpTensorDesc> desc;
  desc.push_back(tensor_desc);
  Status ret = SingleOpParser::UpdateDynamicTensorName(desc);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestOmg, ConvertToBuildParamTest) {
  int32_t index = 0;
  SingleOpDesc single_op_desc;
  single_op_desc.op = "test1";

  SingleOpTensorDesc input_tensor_desc;
  input_tensor_desc.name = "test2";
  input_tensor_desc.dims.push_back(1);
  input_tensor_desc.format = ge::FORMAT_RESERVED;
  input_tensor_desc.type = ge::DT_UNDEFINED;
  input_tensor_desc.is_const = true;
  single_op_desc.input_desc.push_back(input_tensor_desc);

  SingleOpTensorDesc output_tensor_desc;
  output_tensor_desc.name = "test3";
  output_tensor_desc.dims.push_back(2);
  output_tensor_desc.format = ge::FORMAT_RESERVED;
  output_tensor_desc.type = ge::DT_UNDEFINED;
  single_op_desc.output_desc.push_back(output_tensor_desc);
  
  SingleOpAttr op_attr;
  op_attr.name = "attr";
  single_op_desc.attrs.push_back(op_attr);

  SingleOpBuildParam build_param;
  Status ret = SingleOpParser::ConvertToBuildParam(index, single_op_desc, build_param);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestOmg, VerifyOpInputOutputSizeByIrTest) {
  nlohmann::json j;
  SingleOpAttr op_attr;

  //it == kAttrTypeDict.end()
  j["name"] = "test";
  j["type"] = "test";
  j["value"] = "";
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test");
  EXPECT_EQ(op_attr.type, "test");
  
  //VT_BOOL
  j.clear();
  j["name"] = "test2";
  j["type"] = "bool";
  j["value"] = true;
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test2");
  EXPECT_EQ(op_attr.type, "bool");

  //VT_INT
  j.clear();
  j["name"] = "test3";
  j["type"] = "int";
  j["value"] = 1;
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test3");
  EXPECT_EQ(op_attr.type, "int");

  //VT_FLOAT
  j.clear();
  j["name"] = "test4";
  j["type"] = "float";
  j["value"] = 3.14;
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test4");
  EXPECT_EQ(op_attr.type, "float");

  //VT_STRING
  j.clear();
  j["name"] = "test5";
  j["type"] = "string";
  j["value"] = "strval";
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test5");
  EXPECT_EQ(op_attr.type, "string");

  //VT_LIST_BOOL
  j.clear();
  j["name"] = "test6";
  j["type"] = "list_bool";
  j["value"] = {true};
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test6");
  EXPECT_EQ(op_attr.type, "list_bool");

  //VT_LIST_INT
  j.clear();
  j["name"] = "test7";
  j["type"] = "list_int";
  j["value"] = {1};
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test7");
  EXPECT_EQ(op_attr.type, "list_int");

  //VT_LIST_FLOAT
  j.clear();
  j["name"] = "test8";
  j["type"] = "list_float";
  j["value"] = {3.14};
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test8");
  EXPECT_EQ(op_attr.type, "list_float");

  //VT_LIST_STRING
  j.clear();
  j["name"] = "test9";
  j["type"] = "list_string";
  j["value"] = {"strval"};
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test9");
  EXPECT_EQ(op_attr.type, "list_string");

  //VT_LIST_LIST_INT
  j.clear();
  j["name"] = "test10";
  j["type"] = "list_list_int";
  j["value"] = {{1}};
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test10");
  EXPECT_EQ(op_attr.type, "list_list_int");

  //VT_DATA_TYPE
  j.clear();
  j["name"] = "test11";
  j["type"] = "data_type";
  j["value"] = "uint8";
  from_json(j, op_attr);
  EXPECT_EQ(op_attr.name, "test11");
  EXPECT_EQ(op_attr.type, "data_type");
}

TEST_F(UtestOmg, TransConstValueTest) {
  std::string type_str;
  nlohmann::json j;
  SingleOpTensorDesc desc;
  
  //DT_INT8
  desc.type = ge::DT_INT8;
  j["const_value"] = {1, 2};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_INT16
  desc.const_value_size = 0;
  desc.type = ge::DT_INT16;
  j.clear();
  j["const_value"] = {1, 2};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_INT32
  desc.const_value_size = 0;
  desc.type = ge::DT_INT32;
  j.clear();
  j["const_value"] = {1, 2};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_INT64
  desc.const_value_size = 0;
  desc.type = ge::DT_INT64;
  j.clear();
  j["const_value"] = {1, 2};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_FLOAT16
  desc.const_value_size = 0;
  desc.type = ge::DT_FLOAT16;
  j.clear();
  j["const_value"] = {1.1, 2.2};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_FLOAT
  desc.const_value_size = 0;
  desc.type = ge::DT_FLOAT;
  j.clear();
  j["const_value"] = {3.14};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);

  //DT_DOUBLE
  desc.const_value_size = 0;
  desc.type = ge::DT_DOUBLE;
  j.clear();
  j["const_value"] = {2.0};
  TransConstValue(type_str, j, desc);
  EXPECT_NE(desc.const_value_size, 0);
}

} // namespace