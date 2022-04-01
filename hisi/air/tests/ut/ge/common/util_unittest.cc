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
#include "graph/graph.h"
#include "graph/operator.h"
#include "graph/compute_graph.h"
#include "graph/compute_graph_impl.h"
#include "graph/op_desc.h"
#include "graph/node.h"
#include "graph/utils/graph_utils.h"
#include "external/graph/graph.h"
#include "graph/compute_graph_impl.h"
#include "common/util.h"
#include "common/math_util.h"
#include "graph/passes/graph_builder_utils.h"
#include "external/graph/types.h"
#include "parser/tensorflow/tensorflow_parser.h"

namespace ge {
namespace formats {
class UtestUtilTransfer : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

INT32 mmAccess2(const CHAR *pathName, INT32 mode) {
  return -1;
}

static ComputeGraphPtr BuildSubComputeGraph() {
  ut::GraphBuilder builder = ut::GraphBuilder("subgraph");
  auto data = builder.AddNode("sub_Data", "sub_Data", 0, 1);
  auto netoutput = builder.AddNode("sub_Netoutput", "sub_NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  return graph;
}

// construct graph which contains subgraph
static ComputeGraphPtr BuildComputeGraph() {
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 0, 1);
  auto transdata = builder.AddNode("Transdata", "Transdata", 1, 1);
  transdata->GetOpDesc()->AddSubgraphName("subgraph");
  transdata->GetOpDesc()->SetSubgraphInstanceName(0, "subgraph");
  auto netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, transdata, 0);
  builder.AddDataEdge(transdata, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  // add subgraph
  transdata->SetOwnerComputeGraph(graph);
  ComputeGraphPtr subgraph = BuildSubComputeGraph();
  subgraph->SetParentGraph(graph);
  subgraph->SetParentNode(transdata);
  graph->AddSubgraph("subgraph", subgraph);
  return graph;
}

TEST_F(UtestUtilTransfer, CheckOutputPathValid) {
  EXPECT_EQ(CheckOutputPathValid("", ""), false);
  EXPECT_EQ(CheckOutputPathValid("", "model"), false);

  char max_file_path[14097] = {0};
  memset(max_file_path, 1, 14097);
  EXPECT_EQ(CheckOutputPathValid(max_file_path, "model"), false);

  EXPECT_EQ(CheckOutputPathValid("$#%", ""), false);
  EXPECT_EQ(CheckOutputPathValid("./", ""), true);
  // system("touch test_util");
  // system("chmod 555 test_util");
  // EXPECT_EQ(CheckOutputPathValid("./test_util", ""), false);
  // system("rm -rf test_util");
}

TEST_F(UtestUtilTransfer, CheckInputPathValid) {
  EXPECT_EQ(CheckInputPathValid("", ""), false);
  EXPECT_EQ(CheckInputPathValid("", "model"), false);

  EXPECT_EQ(CheckInputPathValid("$#%", ""), false);

  EXPECT_EQ(CheckInputPathValid("./test_util", ""), false);
}

TEST_F(UtestUtilTransfer, GetFileLength_success) {
  system("rm -rf ./ut_graph1.txt");
  EXPECT_EQ(GetFileLength("./ut_graph1.txt"), -1);
  ComputeGraphPtr cgp = BuildComputeGraph();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(cgp);

  graph.SaveToFile("./ut_graph1.txt");
  EXPECT_NE(GetFileLength("./ut_graph1.txt"), -1);
}

TEST_F(UtestUtilTransfer, ReadBytesFromBinaryFile_success) {
  system("rm -rf ./ut_graph1.txt");
  ComputeGraphPtr cgp = BuildComputeGraph();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(cgp);

  graph.SaveToFile("./ut_graph1.txt");
  char* const_buffer;
  int32_t len;
  EXPECT_EQ(ReadBytesFromBinaryFile("./ut_graph1.txt", &const_buffer, len), true);
}

TEST_F(UtestUtilTransfer, GetCurrentSecondTimestap) {
  EXPECT_NE(GetCurrentSecondTimestap(), FAILED);
  //ComputeGraphPtr cgp = BuildComputeGraph();
 // Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(cgp);
  //EXPECT_NE(GetCurrentSecondTimestap(), FAILED);
}

TEST_F(UtestUtilTransfer, ReadProtoFromText) {
  system("rm -rf ./ut_graph1.txt");
  ComputeGraphPtr cgp = BuildComputeGraph();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(cgp);
  graph.SaveToFile("./ut_graph1.txt");
  domi::tensorflow::GraphDefLibrary message;
  EXPECT_EQ(ReadProtoFromText("./ut_graph1.txt", &message), false);
}

class UtestIntegerChecker : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestIntegerChecker, All) {
#define DEFINE_VALUE(T)                                          \
  static uint64_t T##_max = std::numeric_limits<T>::max();       \
  static uint64_t u##T##_max = std::numeric_limits<u##T>::max(); \
  static int64_t T##_min = std::numeric_limits<T>::min();        \
  static int64_t u##T##_min = std::numeric_limits<u##T>::min();  \
  static uint64_t T##_up_overflow = T##_max + 1;                 \
  static uint64_t u##T##_up_overflow = u##T##_max + 1;           \
  static int64_t T##_lo_overflow = T##_min - 1;                  \
  static int64_t u##T##_lo_overflow = -1;

#define TEST_TYPE(T)                                              \
  EXPECT_TRUE(IntegerChecker<T>::Compat(T##_max));                \
  EXPECT_TRUE(IntegerChecker<T>::Compat(T##_min));                \
  EXPECT_TRUE(IntegerChecker<u##T>::Compat(u##T##_max));          \
  EXPECT_TRUE(IntegerChecker<u##T>::Compat(u##T##_min));          \
  EXPECT_FALSE(IntegerChecker<T>::Compat(T##_up_overflow));       \
  EXPECT_FALSE(IntegerChecker<T>::Compat(T##_lo_overflow));       \
  EXPECT_FALSE(IntegerChecker<u##T>::Compat(u##T##_up_overflow)); \
  EXPECT_FALSE(IntegerChecker<u##T>::Compat(u##T##_lo_overflow));

#define DEFINE_AND_TEST(T) \
  DEFINE_VALUE(T);         \
  TEST_TYPE(T);

  DEFINE_AND_TEST(int8_t);
  DEFINE_AND_TEST(int16_t);
  DEFINE_AND_TEST(int32_t);
  EXPECT_TRUE(IntegerChecker<int64_t>::Compat(std::numeric_limits<int64_t>::max()));
  EXPECT_TRUE(IntegerChecker<int64_t>::Compat(std::numeric_limits<int64_t>::min()));
  EXPECT_FALSE(IntegerChecker<int64_t>::Compat(std::numeric_limits<uint64_t>::max()));
  EXPECT_TRUE(IntegerChecker<uint64_t>::Compat(std::numeric_limits<uint64_t>::min()));
  EXPECT_TRUE(IntegerChecker<uint64_t>::Compat(std::numeric_limits<uint64_t>::max()));
  EXPECT_FALSE(IntegerChecker<uint64_t>::Compat(-1));
}

}  // namespace formats
}  // namespace ge
