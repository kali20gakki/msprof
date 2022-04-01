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
#include "engine/fftsplus_engine.h"
#include "engine/engine_manager.h"
#undef private
#undef protected

using namespace std;
using namespace ffts;
using namespace ge;

class FFTSPlusEngineSTEST : public testing::Test {
 protected:
	void SetUp() {
	}

	void TearDown() {
	}
};

TEST_F(FFTSPlusEngineSTEST, Initialize_failed) {
  map<string, string> options;
  Status ret = Initialize(options);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FFTSPlusEngineSTEST, Initialize_success) {
  map<string, string> options;
  options["ge.socVersion"] = "soc_version_";
  Status ret = Initialize(options);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusEngineSTEST, Finalize_success) {
  Status ret = Finalize();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusEngineSTEST, GetOpsKernelInfoStores_success) {
  map<string, GraphOptimizerPtr> graph_optimizers;
  GetGraphOptimizerObjs(graph_optimizers);
  EXPECT_EQ(1, graph_optimizers.size());
}

TEST_F(FFTSPlusEngineSTEST, GetCompositeEngines_success1) {
  std::map<string, std::set<std::string>> compound_engine_contains;
  std::map<std::string, std::string> compound_engine_2_kernel_lib_name;
  GetCompositeEngines(compound_engine_contains, compound_engine_2_kernel_lib_name);
  EXPECT_EQ(0, compound_engine_2_kernel_lib_name.size());
}

TEST_F(FFTSPlusEngineSTEST, GetCompositeEngines_success2) {
  std::map<string, std::set<std::string>> compound_engine_contains;
  std::map<std::string, std::string> compound_engine_2_kernel_lib_name;

  string pre_soc_version = EngineManager::Instance(kFFTSPlusCoreName).GetSocVersion();
  EngineManager::Instance(kFFTSPlusCoreName).soc_version_ = "ascend920a";

  std::string path = "./air/test/engines/fftseng/config/data/platform_config";
  std::string real_path = RealPath(path);
  fe::PlatformInfoManager::Instance().platform_info_map_.clear();
  fe::PlatformInfoManager::Instance().platform_infos_map_.clear();
  uint32_t init_ret = fe::PlatformInfoManager::Instance().LoadConfigFile(real_path);

  std::map<std::string, std::vector<std::string>> a;
  std::map<std::string, std::vector<std::string>> b;
  a = b;
  fe::PlatFormInfos platform_infos;
  fe::OptionalInfos opti_compilation_infos;
  std::map<std::string, fe::PlatFormInfos> platform_infos_map;
  platform_infos.Init();
  std::map<std::string, std::string> res;
  res[kFFTSMode] = kFFTSPlus;
  platform_infos.SetPlatformRes(kSocInfo, res);
  platform_infos_map["ascend920a"] = platform_infos;
  fe::PlatformInfoManager::Instance().platform_infos_map_ = platform_infos_map;
  std::map<std::string, OpsKernelInfoStorePtr> op_kern_infos;
  GetOpsKernelInfoStores(op_kern_infos);
  GetCompositeEngines(compound_engine_contains, compound_engine_2_kernel_lib_name);
  EngineManager::Instance(kFFTSPlusCoreName).soc_version_ = pre_soc_version;
  fe::PlatformInfoManager::Instance().platform_infos_map_.clear();
  EXPECT_EQ(1, compound_engine_2_kernel_lib_name.size());
}