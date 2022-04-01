/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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
#include <iostream>
#include <string>
#include <vector>

#define protected public
#define private public
#include "common/op_info_common.h"
#include "common/configuration.h"
#include "compute_graph.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class STEST_op_info_common_stest : public testing::Test
{
 protected:
  void SetUp()
  {
  }

  void TearDown()
  {
  }

  static void CreateGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BN");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Cast");
    OpDescPtr out_op = std::make_shared<OpDesc>("out", "NetOutput");

    // add descriptor
    vector<int64_t> dims = {1,2,3,4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_FRACTAL_NZ);
    in_desc1.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_FRACTAL_NZ);
    out_desc1.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_NZ);
    in_desc2.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_FRACTAL_NZ);
    out_desc2.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_FRACTAL_NZ);
    in_desc3.SetDataType(DT_FLOAT16);
    out_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_FRACTAL_NZ);
    out_desc3.SetDataType(DT_FLOAT16);
    out_op->AddOutputDesc("y", out_desc3);


    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    NodePtr out_node = graph->AddNode(out_op);

    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));

  }
};

TEST_F(STEST_op_info_common_stest, IsSpecialCast)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = SOC_VERSION_SD3403;
  for (auto &node : graph->GetDirectNode()) {
    if (node->GetType() == "Cast") {
      bool res = IsSpecialCast(node);
      EXPECT_EQ(res, true);
    }
  }
}
