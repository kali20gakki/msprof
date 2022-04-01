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

#include "utils/model_factory.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "common/types.h"
#include "graph/model.h"
#include "ge_running_env/path_utils.h"
#include "ge_running_env/tensor_utils.h"
#include "mmpa/mmpa_api.h"
#include "graph/debug/ge_attr_define.h"

#include <unistd.h>

#define RETURN_WHEN_FOUND(name) do { \
      auto iter = model_names_to_path_.find(name); \
      if (iter != model_names_to_path_.end()) { \
        return iter->second; \
      }                                \
    } while(0)

FAKE_NS_BEGIN
namespace {
const std::string &GetModelPath() {
  static std::string path;
  if (!path.empty()) {
    return path;
  }
  path = PathJoin(GetRunPath().c_str(), "models");
  if (!IsDir(path.c_str())) {
    auto ret = mmMkdir(path.c_str(),
                       S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH); // 775
    if (ret != EN_OK) {
      path = "";
    }
  }
  return path;
}

}
std::unordered_map<std::string, std::string> ModelFactory::model_names_to_path_;
const std::string &ge::ModelFactory::GenerateModel_1(bool is_dynamic) {
  std::string name = "ms1_" +std::to_string(is_dynamic) +".pbtxt";
  RETURN_WHEN_FOUND(name);

  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor = GenerateTensor(shape);

  DEF_GRAPH(dynamic_op) {
    ge::OpDescPtr data1;
    if (is_dynamic) {
      data1 = OP_CFG(DATA)
                  .Attr(ATTR_NAME_INDEX, 0)
                  .InCnt(1)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
                  .OutCnt(1)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
                  .Build("data1");

    } else {
      data1 = OP_CFG(DATA)
                  .Attr(ATTR_NAME_INDEX, 0)
                  .InCnt(1)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
                  .OutCnt(1)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
                  .Build("data1");
    }

    auto const1 = OP_CFG(CONSTANT)
                      .Weight(data_tensor)
                      .TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
                      .InCnt(1)
                      .OutCnt(1)
                      .Build("const1");

    auto conv2d1 = OP_CFG(CONV2D).InCnt(2).OutCnt(1).Build("conv2d1");

    auto relu1 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu1");

    auto netoutput1 = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput1");

    CHAIN(NODE(data1)->NODE(conv2d1)->NODE(relu1))->NODE(netoutput1);
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(conv2d1));
  };

  return SaveAsModel(name, ToGeGraph(dynamic_op));
}

const std::string &ge::ModelFactory::GenerateModel_2(bool is_dynamic) {
  std::string name = "ms2_" +std::to_string(is_dynamic) +".pbtxt";
  RETURN_WHEN_FOUND(name);

  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor = GenerateTensor(shape);
  // two input data
  DEF_GRAPH(dynamic_op) {
    ge::OpDescPtr data1;
    if (is_dynamic) {
      data1 = OP_CFG(DATA)
          .Attr(ATTR_NAME_INDEX, 0)
          .InCnt(1)
          .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
          .OutCnt(1)
          .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 3, 16, 16})
          .Build("data1");

    } else {
      data1 = OP_CFG(DATA)
          .Attr(ATTR_NAME_INDEX, 0)
          .InCnt(1)
          .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
          .OutCnt(1)
          .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
          .Build("data1");
    }
    auto data2 = OP_CFG(DATA)
        .Attr(ATTR_NAME_INDEX, 1)
        .InCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
        .OutCnt(1)
        .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 3, 16, 16})
        .Build("data2");
    auto const1 = OP_CFG(CONSTANT)
        .Weight(data_tensor)
        .TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("const1");

    auto conv2d1 = OP_CFG(CONV2D).InCnt(2).OutCnt(1).Build("conv2d1");

    auto relu1 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu1");

    auto relu2 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu2");

    auto netoutput1 = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).Build("netoutput1");

    CHAIN(NODE(data1)->NODE(conv2d1)->NODE(relu1))->NODE(netoutput1);
    CHAIN(NODE(data2)->NODE(relu2)->EDGE(0, 1)->NODE(netoutput1));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(conv2d1));
  };

  return SaveAsModel(name, ToGeGraph(dynamic_op));
}

const std::string &ModelFactory::SaveAsModel(const string &name, const Graph &graph) {
  Model model;
  model.SetName(name);
  model.SetGraph(graph);
  model.SetVersion(1);

  auto &file_dir = GetModelPath();
  auto file_path = PathJoin(file_dir.c_str(), name.c_str());
  if (IsFile(file_path.c_str())) {
    RemoveFile(file_path.c_str());
  }
  model.SaveToFile(file_path);

  return model_names_to_path_[name] = file_path;
}
FAKE_NS_END
