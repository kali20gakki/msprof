/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#define private public
#define protected public
#include "engine/engine_manager.h"
#undef private
#undef protected

using namespace std;
using namespace ffts;
using namespace ge;

class EngineManagerUTEST : public testing::Test {
 protected:
	void SetUp() {
	}

	void TearDown() {
	}
};

TEST_F(EngineManagerUTEST, Initialize_success1) {
  EngineManager em;
  map<string, string> options;
  std::string engine_name;
  std::string soc_version;
  Status ret = em.Initialize(options, engine_name, soc_version);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(EngineManagerUTEST, Initialize_success2) {
  EngineManager em;
  map<string, string> options;
  std::string engine_name;
  std::string soc_version;
  em.inited_ = true;
  Status ret = em.Initialize(options, engine_name, soc_version);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(EngineManagerUTEST, Finalize_success1) {
  EngineManager em;
  Status ret = em.Finalize();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(EngineManagerUTEST, Finalize_success2) {
  EngineManager em;
  em.inited_ = true;
  Status ret = em.Finalize();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(EngineManagerUTEST, GetGraphOptimizerObjs_success1) {
  EngineManager em;
  map<string, GraphOptimizerPtr> graph_optimizers;
  std::string engine_name;
  em.GetGraphOptimizerObjs(graph_optimizers, engine_name);
  EXPECT_EQ(0, graph_optimizers.size());
}

TEST_F(EngineManagerUTEST, GetGraphOptimizerObjs_success2) {
  EngineManager em;
  map<string, GraphOptimizerPtr> graph_optimizers;
  std::string engine_name;
  em.ffts_plus_graph_opt_ = make_shared<FFTSPlusGraphOptimizer>();
  em.GetGraphOptimizerObjs(graph_optimizers, engine_name);
  EXPECT_EQ(1, graph_optimizers.size());
}