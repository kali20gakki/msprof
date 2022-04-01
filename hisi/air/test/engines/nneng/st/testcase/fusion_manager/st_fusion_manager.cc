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
#define protected public
#define private public
#include "fusion_manager/fusion_manager.h"
#include "platform_info.h"
#undef private
#undef protected
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"

using namespace std;
using namespace fe;

class fuison_manager_stest : public testing::Test
{
 protected:
  void SetUp() {}

  void TearDown() {}

};

TEST_F(fuison_manager_stest, dsa_instance)
{
    FusionManager fm = FusionManager::Instance(kDsaCoreName);
    fm.op_store_adapter_manager_ = std::make_shared<OpStoreAdapterManager>();
    fm.ops_kernel_info_store_ = make_shared<FEOpsKernelInfoStore>(fm.op_store_adapter_manager_, fe::kDsaCoreName);
    fm.dsa_graph_opt_ = make_shared<DSAGraphOptimizer>(fm.ops_kernel_info_store_, fm.op_store_adapter_manager_, fe::kDsaCoreName);
    map<string, GraphOptimizerPtr> graph_optimizers;
    std::string AIcoreEngine = "DSAEngine";
    fm.GetGraphOptimizerObjs(graph_optimizers, AIcoreEngine);
    EXPECT_EQ(graph_optimizers.size(), 1);
}