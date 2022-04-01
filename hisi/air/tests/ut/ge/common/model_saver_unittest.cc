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
#define protected public
#define private public
#include "common/model_saver.h"
#include "analyzer/analyzer.h"
#include "graph/op_desc.h"
#include "graph/utils/graph_utils.h"
#undef private
#undef protected
#include "graph/passes/graph_builder_utils.h"

namespace ge
{
class UtestModelSaver : public testing::Test {
protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestModelSaver, SaveJsonToFile_success) {
  Json tmp_json;
  ge::analyzer::OpInfo opinfo;
  opinfo.op_name = "test";
  opinfo.op_name = "test";
  ge::analyzer::GraphInfo graph_info;
  graph_info.op_info.push_back(opinfo);
  Analyzer::GetInstance()->GraphInfoToJson(tmp_json, graph_info);
  EXPECT_EQ(ModelSaver::SaveJsonToFile(nullptr, tmp_json), FAILED);
  EXPECT_EQ(ModelSaver::SaveJsonToFile("./", tmp_json), FAILED);
  EXPECT_EQ(ModelSaver::SaveJsonToFile("./test.pb", tmp_json), SUCCESS);
  system("rm -rf ./test.pb");
}

}