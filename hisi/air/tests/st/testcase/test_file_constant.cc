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
class StestFileConstantUtilTransfer : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(StestFileConstantUtilTransfer, GetRealFilePathOK) {
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