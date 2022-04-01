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
#include "itf_handler/itf_handler.h"
#include "fusion_manager/fusion_manager.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class itfhandler_unittest : public testing::Test
{
protected:
    void SetUp()
    {
    }

    void TearDown()
    {
    }
// AUTO GEN PLEASE DO NOT MODIFY IT
};

TEST_F(itfhandler_unittest, initialize_failed1) {
  map<string, string> options;
  Status ret = Initialize(options);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(itfhandler_unittest, initialize_failed2) {
  map<string, string> options;
  options.emplace("ge.socVersion", "Ascend910A");
  Status ret = Initialize(options);
  //EXPECT_EQ(ret, OP_STORE_ADAPTER_MANAGER_INIT_FAILED);
}

TEST_F(itfhandler_unittest, Finalize_success) {
  Status ret = Finalize();
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(itfhandler_unittest, GetOpsKernelInfoStores_suc) {
  map<string, string> options;
  Status ret = Initialize(options);
  EXPECT_EQ(ret, fe::FAILED);
  map<string, OpsKernelInfoStorePtr> op_kern_infos;
  GetOpsKernelInfoStores(op_kern_infos);
}

TEST_F(itfhandler_unittest, get_graph_optimizer_objs_success)
{
  FusionManager& fm = FusionManager::Instance(fe::AI_CORE_NAME);
  fm.op_store_adapter_manager_ = std::make_shared<OpStoreAdapterManager>();
  fm.ops_kernel_info_store_ = make_shared<FEOpsKernelInfoStore>(fm.op_store_adapter_manager_, fe::AI_CORE_NAME);
  fm.graph_opt_ = make_shared<FEGraphOptimizer>(fm.ops_kernel_info_store_, fm.op_store_adapter_manager_, fe::AI_CORE_NAME);
  map<string, GraphOptimizerPtr> graph_optimizers;
  GetGraphOptimizerObjs(graph_optimizers);
  FusionManager& fm2 = FusionManager::Instance(fe::VECTOR_CORE_NAME);
  EXPECT_EQ(fm2.graph_opt_, nullptr);
}
