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

#include "offline/main_impl.h"
#include "ge_running_env/atc_utils.h"
#include "framework/omg/parser/parser_factory.h"
#include "framework/omg/parser/model_parser.h"
#include "framework/omg/parser/weights_parser.h"
#include "graph/types.h"

#include "ge_running_env/atc_utils.h"
using namespace domi;
using namespace ge;

class UtestMain : public AtcTest {};
namespace {
class AtcFileFactory {
 public:
  static std::string Generatefile1(const std::string &file_type, const std::string &file_name);
  static void RemoveFile(const char *path) {
    remove(path);
  }
};
std::string AtcFileFactory::Generatefile1(const std::string &file_type, const std::string &file_name) {
  std::string pwd = __FILE__;
  std::size_t idx = pwd.find_last_of("/");
  pwd = pwd.substr(0, idx);
  std::string om_file = pwd + "/" + file_name;
  return file_type + om_file;
}

class StubTensorFlowModelParser : public domi::ModelParser {
 public:
  StubTensorFlowModelParser() {}
  ~StubTensorFlowModelParser() {}
  domi::Status Parse(const char *file, ge::Graph &graph) {
    return SUCCESS;
  }
  domi::Status ParseFromMemory(const char *data, uint32_t size, ge::ComputeGraphPtr &graph) {
    return SUCCESS;
  }
  domi::Status ParseFromMemory(const char *data, uint32_t size, ge::Graph &graph) {
    return SUCCESS;
  }
  domi::Status ParseProto(const google::protobuf::Message *proto, ge::ComputeGraphPtr &graph) {
    return SUCCESS;
  }
  domi::Status ParseProtoWithSubgraph(const google::protobuf::Message *proto, GetGraphCallback callback,
                                      ge::ComputeGraphPtr &graph) {
    return SUCCESS;
  }
  ge::DataType ConvertToGeDataType(const uint32_t type) {
    return DT_FLOAT;
  }
  domi::Status ParseAllGraph(const google::protobuf::Message *root_proto, ge::ComputeGraphPtr &root_graph) {
    return SUCCESS;
  }
};

REGISTER_MODEL_PARSER_CREATOR(TENSORFLOW, StubTensorFlowModelParser);
REGISTER_MODEL_PARSER_CREATOR(CAFFE, StubTensorFlowModelParser);
REGISTER_MODEL_PARSER_CREATOR(MINDSPORE, StubTensorFlowModelParser);
REGISTER_MODEL_PARSER_CREATOR(ONNX, StubTensorFlowModelParser);

class StubWeightsParser : public domi::WeightsParser {
 public:
  StubWeightsParser() {}
  ~StubWeightsParser() {}
  domi::Status Parse(const char *file, ge::Graph &graph) {
    return SUCCESS;
  }
  domi::Status ParseFromMemory(const char *input, uint32_t lengt, ge::ComputeGraphPtr &graph) {
    return SUCCESS;
  }
};
REGISTER_WEIGHTS_PARSER_CREATOR(CAFFE, StubWeightsParser);
REGISTER_WEIGHTS_PARSER_CREATOR(ONNX, StubWeightsParser);
REGISTER_WEIGHTS_PARSER_CREATOR(TENSORFLOW, StubWeightsParser);
}  // namespace

TEST_F(UtestMain, MainImplTest_singleop) {
  std::string singleop_arg = AtcFileFactory::Generatefile1("--singleop=", "add_int.json");
  std::string output_arg = AtcFileFactory::Generatefile1("--output=", "./");
  char *argv[] = {"atc", const_cast<char *>(singleop_arg.c_str()), const_cast<char *>(output_arg.c_str()),
                  "--soc_version=\"Ascend310\""};
  //未打桩，st里面覆盖全流程，当前ut只是覆盖此文件相关代码
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_EQ(ret, -1);
}

TEST_F(UtestMain, MainImplTest_check_mem_info) {
  char *argv[] = {"atc", "--auto_tune_mode=\"RA\""};
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
}

TEST_F(UtestMain, MainImplTest_generate_om_model_tf) {
  std::string om_arg = AtcFileFactory::Generatefile1("--model=", "add.pb");
  std::string output_arg = AtcFileFactory::Generatefile1("--output=", "tmp");
  char *argv[] = {"atc",
                  "--mode=0",
                  "--framework=3",
                  const_cast<char *>(om_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--soc_version=\"Ascend310\"",
                  "--input_format=NCHW"};
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.om").c_str());
}

TEST_F(UtestMain, MainImplTest_generate_om_model_caffe) {
  std::string om_arg = AtcFileFactory::Generatefile1("--model=", "add.pb");
  std::string invalid_weight_arg = AtcFileFactory::Generatefile1("--weight=", "add.pb");
  std::string output_arg = AtcFileFactory::Generatefile1("--output=", "tmp");
  char *argv[] = {"atc",
                  "--mode=0",
                  "--framework=0",
                  const_cast<char *>(om_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  const_cast<char *>(invalid_weight_arg.c_str()),
                  "--soc_version=\"Ascend310\"",
                  "--input_format=NCHW"};
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.om").c_str());
}

TEST_F(UtestMain, MainImplTest_generate_om_model_onnx) {
  std::string om_arg = AtcFileFactory::Generatefile1("--model=", "add.pb");
  std::string output_arg = AtcFileFactory::Generatefile1("--output=", "tmp");
  char *argv[] = {"atc",
                  "--mode=0",
                  "--framework=5",
                  const_cast<char *>(om_arg.c_str()),
                  const_cast<char *>(output_arg.c_str()),
                  "--soc_version=\"Ascend310\"",
                  "--input_format=NCHW"};
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.om").c_str());
}

TEST_F(UtestMain, MainImplTest_convert_model_to_json_pb_dump_mode) {
  std::string om_arg = AtcFileFactory::Generatefile1("--om=", "add.pb");
  std::string json_arg = AtcFileFactory::Generatefile1("--json=", "tmp.json");
  char *argv[] = {"atc",
                  "--mode=1",
                  "--framework=3",
                  const_cast<char *>(om_arg.c_str()),
                  const_cast<char *>(json_arg.c_str()),
                  "--dump_mode=1"

  };
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.json").c_str());
}

TEST_F(UtestMain, MainImplTest_convert_model_to_json_pb) {
  std::string om_arg = AtcFileFactory::Generatefile1("--om=", "add.pb");
  std::string json_arg = AtcFileFactory::Generatefile1("--json=", "tmp.json");
  char *argv[] = {
      "atc", "--mode=1", "--framework=3", const_cast<char *>(om_arg.c_str()), const_cast<char *>(json_arg.c_str())

  };
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0);
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.json").c_str());
}

TEST_F(UtestMain, MainImplTest_convert_model_to_json_txt) {
  std::string om_arg = AtcFileFactory::Generatefile1(
      "--om=", "ge_proto_00000261_partition0_rank31_new_sub_graph102_SecondPartitioning.txt");
  std::string json_arg = AtcFileFactory::Generatefile1("--json=", "tmp.json");
  char *argv[] = {"atc", const_cast<char *>(om_arg.c_str()), const_cast<char *>(json_arg.c_str()), "--mode=5"};
  int32_t ret = main_impl(sizeof(argv) / sizeof(argv[0]), argv);
  EXPECT_NE(ret, 0); // txt file not exist
  AtcFileFactory::RemoveFile(AtcFileFactory::Generatefile1("", "tmp.json").c_str());
}