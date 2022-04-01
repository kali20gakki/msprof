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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "opskernel_manager/ops_kernel_manager.h"
#include "init/gelib.h"
#include "graph/optimize/graph_optimize.h"
#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class SubOpsKernelInfoStore : public OpsKernelInfoStore{
public:
    SubOpsKernelInfoStore(){}
    virtual Status Initialize(const std::map<std::string, std::string> &options);
    virtual Status Finalize();
    virtual void GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const;
    virtual bool CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const;
};

Status SubOpsKernelInfoStore::Initialize(const std::map<std::string, std::string> &options){
    return FAILED;
}

Status SubOpsKernelInfoStore::Finalize(){
    return SUCCESS;
}

void SubOpsKernelInfoStore::GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const{
    infos["opinfo"] = OpInfo();
    return;
}

bool SubOpsKernelInfoStore::CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const{
    return true;
}

class SubGraphOptimizer : public GraphOptimizer{
public:
    SubGraphOptimizer(){final_flag_ = true;}
    virtual Status Initialize(const std::map<std::string, std::string> &options, OptimizeUtility *const optimize_utility);
    virtual Status Finalize();
    virtual Status OptimizeOriginalGraph(ComputeGraph &graph);
    virtual Status OptimizeFusedGraph(ComputeGraph &graph);
    virtual Status OptimizeWholeGraph(ComputeGraph &graph);
    virtual Status GetAttributes(GraphOptimizerAttribute &attrs) const;
    bool final_flag_;
};

Status SubGraphOptimizer::Initialize(const std::map<std::string, std::string> &options, OptimizeUtility *const optimize_utility){
    return SUCCESS;
}

Status SubGraphOptimizer::Finalize(){
	if (final_flag_){
        return SUCCESS;
	}
    return FAILED;
}

Status SubGraphOptimizer::OptimizeOriginalGraph(ComputeGraph &graph){
    return SUCCESS;
}

Status SubGraphOptimizer::OptimizeFusedGraph(ComputeGraph &graph){
    return SUCCESS;
}

Status SubGraphOptimizer::OptimizeWholeGraph(ComputeGraph &graph){
    return SUCCESS;
}

Status SubGraphOptimizer::GetAttributes(GraphOptimizerAttribute &attrs) const{
    return SUCCESS;
}

class UtestOpsKernelManager : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
      OpsKernelManager::GetInstance().ops_kernel_store_.clear();
      OpsKernelManager::GetInstance().graph_optimizers_.clear();
      OpsKernelManager::GetInstance().ops_kernel_info_.clear();
  }
};

TEST_F(UtestOpsKernelManager, Normal) {
    OpsKernelManager &instance = OpsKernelManager::GetInstance();
}

TEST_F(UtestOpsKernelManager, ParsePluginOptions) {
    auto &instance = OpsKernelManager::GetInstance();
    std::map<std::string, std::string> options;
    std::string plugin_name = "plugin";
    bool enable_flag = false;
    options["plugin"] = "name";
    EXPECT_EQ(instance.ParsePluginOptions(options, plugin_name, enable_flag), GE_GRAPH_OPTIONS_INVALID);
    options["plugin"] = "2";
    EXPECT_EQ(instance.ParsePluginOptions(options, plugin_name, enable_flag), GE_GRAPH_OPTIONS_INVALID);
    options["plugin"] = "111111111199";
    EXPECT_EQ(instance.ParsePluginOptions(options, plugin_name, enable_flag), GE_GRAPH_OPTIONS_INVALID);
    options["plugin"] = "0";
    EXPECT_EQ(instance.ParsePluginOptions(options, plugin_name, enable_flag), SUCCESS);
    options["plugin"] = "1";
    EXPECT_EQ(instance.ParsePluginOptions(options, plugin_name, enable_flag), SUCCESS);
}

TEST_F(UtestOpsKernelManager, CheckPluginPtr) {
    auto &instance = OpsKernelManager::GetInstance();
    instance.ops_kernel_store_["kernel"] = nullptr;
    EXPECT_EQ(instance.CheckPluginPtr(), FAILED);
    instance.ops_kernel_store_.clear();
    instance.graph_optimizers_["graph"] = nullptr;
    EXPECT_EQ(instance.CheckPluginPtr(), FAILED);
}

TEST_F(UtestOpsKernelManager, InitOpKernelInfoStores) {
    auto &instance = OpsKernelManager::GetInstance();
    std::map<std::string, std::string> options;
    instance.ops_kernel_store_["kernel"] = std::make_shared<SubOpsKernelInfoStore>();
    EXPECT_EQ(instance.InitOpKernelInfoStores(options), GE_OPS_KERNEL_STORE_INIT_FAILED);
}

TEST_F(UtestOpsKernelManager, InitOpsKernelInfo) {
    auto &instance = OpsKernelManager::GetInstance();
    instance.ops_kernel_store_["kernel"] = std::make_shared<SubOpsKernelInfoStore>();
    instance.InitOpsKernelInfo();
    std::map<std::string, std::string> options;
    //EXPECT_EQ(GELib::Initialize(options), SUCCESS);
    instance.ops_kernel_info_["empty"] = std::vector<OpInfo>();
    instance.ops_kernel_info_["one"] = std::vector<OpInfo>();
    instance.ops_kernel_info_["one"].push_back(OpInfo());
    instance.InitOpsKernelInfo();
}

TEST_F(UtestOpsKernelManager, InitGraphOptimizers) {
    auto &instance = OpsKernelManager::GetInstance();
    instance.graph_optimizers_["opt"] = std::make_shared<SubGraphOptimizer>();
    std::map<std::string, std::string> options;
    EXPECT_EQ(instance.InitGraphOptimizers(options), SUCCESS);
}

TEST_F(UtestOpsKernelManager, Finalize) {
    auto &instance = OpsKernelManager::GetInstance();
    instance.init_flag_ = false;
    EXPECT_EQ(instance.Finalize(), SUCCESS);
    instance.init_flag_ = true;
    instance.graph_optimizers_["opt"] = std::make_shared<SubGraphOptimizer>();
    EXPECT_EQ(instance.Finalize(), SUCCESS);
    auto p = std::make_shared<SubGraphOptimizer>();
    p->final_flag_ = false;
    instance.graph_optimizers_["fin"] = p;
    instance.init_flag_ = true;
    EXPECT_EQ(instance.Finalize(), FAILED);
}

TEST_F(UtestOpsKernelManager, GetAllOpsKernelInfo) {
    auto &instance = OpsKernelManager::GetInstance();
    EXPECT_EQ(instance.GetAllOpsKernelInfo().size(), 0);
}

TEST_F(UtestOpsKernelManager, GetAllGraphOptimizerObjs) {
    auto &instance = OpsKernelManager::GetInstance();
    EXPECT_EQ(instance.GetAllGraphOptimizerObjs().size(), 0);
}

TEST_F(UtestOpsKernelManager, GetEnableFeFlag) {
    auto &instance = OpsKernelManager::GetInstance();
    EXPECT_EQ(instance.GetEnableFeFlag(), false);
}

TEST_F(UtestOpsKernelManager, GetEnableAICPUFlag) {
    auto &instance = OpsKernelManager::GetInstance();
    EXPECT_EQ(instance.GetEnableAICPUFlag(), false);
}

TEST_F(UtestOpsKernelManager, GetEnablePluginFlag) {
    auto &instance = OpsKernelManager::GetInstance();
    EXPECT_EQ(instance.GetEnablePluginFlag(), false);
}

TEST_F(UtestOpsKernelManager, ClassifyGraphOptimizers) {
    auto &instance = OpsKernelManager::GetInstance();
    instance.composite_engines_["name"] = std::set<std::string>();
    instance.graph_optimizers_["opt"] = std::make_shared<SubGraphOptimizer>();
    instance.ClassifyGraphOptimizers();
}

TEST_F(UtestOpsKernelManager, GetGraphOptimizerByEngine) {
    auto &instance = OpsKernelManager::GetInstance();
    std::string engine_name = "engine";
    std::vector<GraphOptimizerPtr> graph_optimizer;
    instance.composite_engines_["name"] = std::set<std::string>();
    instance.graph_optimizers_["opt"] = std::make_shared<SubGraphOptimizer>();
    instance.GetGraphOptimizerByEngine(engine_name, graph_optimizer);
}


} // namespace ge