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
#include <memory>
#include <fstream>
#include "file_constant_util.h"
#include "graph/ge_local_context.h"
#include "graph/load/model_manager/davinci_model.h"

namespace ge {
static ge::OpDescPtr CreateOpDesc(string name = "", string type = "") {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({100, 200});
  op_desc->SetOutputOffset({100, 200});
  return op_desc;
}

namespace fileconstant {
class UtestFileConstantUtilTransfer : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestFileConstantUtilTransfer, GetFilePathFromOptionOK) {
  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_buchet_value_bin\", \"value_bin_file\":\"hello.bin\"}]}";
  ge::GetThreadLocalContext().SetGraphOption(options);
  std::map<std::string, std::string> file_id_and_path_map;
  GetFilePathFromOption(file_id_and_path_map);
  EXPECT_EQ(file_id_and_path_map.size(), 1);
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileOK) {
  std::unique_ptr<char[]> buf(new char[2048]);
  string file_name = "no_find_file";
  size_t file_const_size = 100;
  size_t left_size = 0;
  Status ret = CopyOneWeightFromFile((void *)buf.get(), file_name, file_const_size, left_size);
  EXPECT_EQ(ret, GRAPH_FAILED);
  left_size = file_const_size;
  ret = CopyOneWeightFromFile((void *)buf.get(), file_name, file_const_size, left_size);
  EXPECT_EQ(ret, GRAPH_FAILED);

  std::unique_ptr<float[]> float_buf(new float[file_const_size / sizeof(float)]);
  file_name = "test_copy_one_weight.bin";
  std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
  if (out1.is_open()) {
    return;
  }
  out1.write((char *)float_buf.get(), file_const_size);
  out1.close();

  ret = CopyOneWeightFromFile((void *)buf.get(), file_name, file_const_size, left_size);
  EXPECT_EQ(ret, SUCCESS);

  (void)remove("test_copy_one_weight.bin");
}

TEST_F(UtestFileConstantUtilTransfer, GetFilePathOK) {
  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_buchet_value_bin\", \"value_bin_file\":\"hello.bin\"}]}";
  ge::GetThreadLocalContext().SetGraphOption(options);
  std::map<std::string, std::string> file_id_and_path_map;
  GetFilePathFromOption(file_id_and_path_map);
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "vector_search_buchet_value_bin"));
  std::string file_path;
  Status ret = GetFilePath(op_desc, file_id_and_path_map, file_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "hello.bin");
}

TEST_F(UtestFileConstantUtilTransfer, GetRealFilePathOK) {
  std::map<std::string, std::string> file_id_and_path_map;
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  const std::string attr_name_file_id = "file_path";
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, attr_name_file_id, "hello.bin"));
  std::string file_path;
  Status ret = GetFilePath(op_desc, file_id_and_path_map, file_path);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "hello.bin");
}
}
}