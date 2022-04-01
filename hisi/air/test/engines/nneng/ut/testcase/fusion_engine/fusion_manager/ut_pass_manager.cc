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
#include <iostream>

#include <list>

#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#define protected public
#define private public
#include "common/configuration.h"
#include "common/pass_manager.h"
//#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/relu_fusion_pass.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class pass_manager_ut: public testing::Test
{
public:


protected:
    void SetUp()
    {
    }

    void TearDown()
    {

    }
// AUTO GEN PLEASE DO NOT MODIFY IT
};

namespace{

}

//TEST_F(pass_manager_ut, run_with_opstore)
//{
//  PassManager fusion_passes(nullptr);
//  fusion_passes.AddPass("ReluFusionPass", AI_CORE_NAME, new(std::nothrow) ReluFusionPass, GRAPH_FUSION);
//  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
//  Status ret = fusion_passes.Run(*(graph.get()), nullptr);
//  EXPECT_EQ(ret, fe::NOT_CHANGED);
//}
//
//TEST_F(pass_manager_ut, run_without_opstore)
//{
//  PassManager fusion_passes(nullptr);
//  fusion_passes.AddPass("ReluFusionPass", AI_CORE_NAME, new(std::nothrow) ReluFusionPass, GRAPH_FUSION);
//  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
//  Status ret = fusion_passes.Run(*(graph.get()));
//  EXPECT_EQ(ret, fe::NOT_CHANGED);
//}
