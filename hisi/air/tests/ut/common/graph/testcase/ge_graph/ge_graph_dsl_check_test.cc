#include "gtest/gtest.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/graph.h"
#include "graph/compute_graph.h"

USING_GE_NS

class GeGraphDslCheckTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};


TEST_F(GeGraphDslCheckTest, test_build_graph_from_optype_with_name) {
  DEF_GRAPH(g1) { CHAIN(NODE("data1", "Data")->NODE("add", "Add")); };

  auto geGraph = ToGeGraph(g1);
  auto computeGraph = ToComputeGraph(g1);

  ASSERT_EQ(geGraph.GetAllNodes().size(), 2);
  ASSERT_EQ(computeGraph->GetAllNodesSize(), 2);
}