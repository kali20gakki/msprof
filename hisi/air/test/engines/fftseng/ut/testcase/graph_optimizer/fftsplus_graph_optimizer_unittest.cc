/**
 *
 * @file ffts_plus_graph_optimizer_unittest.cc
 *
 * @brief
 *
 * @version 1.0
 *
 */
#include <gtest/gtest.h>
#include <iostream>

#define private public
#define protected public
#include "engine/engine_manager.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "graph/debug/ge_attr_define.h"
#include "optimizer/graph_optimizer/fftsplus_graph_optimizer.h"
#include "../test_utils.h"

#undef private
#undef protected

using namespace testing;
using namespace std;
using namespace ffts;
using namespace ge;
using FFTSPlusGraphOptimizerPtr = shared_ptr<FFTSPlusGraphOptimizer>;

tune::Status FFTSOptimizerStub1(ge::ComputeGraph &, bool) {
  return tune::FAILED;
}

tune::Status FFTSOptimizerStub2(ge::ComputeGraph &, bool) {
  return tune::SUCCESS;
}

class FftsPlusGraphOptimizerUTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    cout << "FftsPlusGraphOptimizerUTest SetUpTestCase" << endl;
  }
  static void TearDownTestCase() {
    cout << "FftsPlusGraphOptimizerUTest TearDownTestCase" << endl;
  }

  virtual void SetUp() {
    ffts_plus_graph_optimizer_ptr = make_shared<FFTSPlusGraphOptimizer>();
  }

  virtual void TearDown() {
    cout << "a test Tear Down" << endl;
  }

  NodePtr CreateNode() {
    FeTestOpDescBuilder builder;
    builder.SetName("test_tvm");
    builder.SetType("test_tvm");
    builder.AddInputDesc({288,32,16,16}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    builder.AddOutputDesc({288,32,16,16}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    NodePtr node =  builder.Finish();
    ge::AttrUtils::SetListInt(node->GetOpDesc(), ge::TVM_ATTR_NAME_THREAD_BLOCKDIM, {1, 1});
    ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_mode = 0;
    ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ThreadSliceMap>(thread_slice_map);
    std::string str_slice_info;
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), kAttrSgtJsonInfo, str_slice_info);
    return node;
  }

  void CreateGraph(ComputeGraphPtr& graph) {
    NodePtr tile = CreateNode();
    NodePtr tile_node = graph->AddNode(tile);
  }

 public:
  FFTSPlusGraphOptimizerPtr ffts_plus_graph_optimizer_ptr;
};

TEST_F(FftsPlusGraphOptimizerUTest, Initialize_FAILED) {
  map<string, string> options;
  string pre_soc_version = EngineManager::Instance(kFFTSPlusCoreName).GetSocVersion();
  EngineManager::Instance(kFFTSPlusCoreName).soc_version_ = "ascend920a";

  std::string path = "./air/test/engines/fftseng/config/data/platform_config";
  std::string real_path = RealPath(path);
  fe::PlatformInfoManager::Instance().platform_info_map_.clear();
  fe::PlatformInfoManager::Instance().platform_infos_map_.clear();
  uint32_t init_ret = fe::PlatformInfoManager::Instance().LoadConfigFile(real_path);

  Status ret = ffts_plus_graph_optimizer_ptr->Initialize(options, nullptr);
  EngineManager::Instance(kFFTSPlusCoreName).soc_version_ = pre_soc_version;
  fe::PlatformInfoManager::Instance().platform_infos_map_.clear();
}

TEST_F(FftsPlusGraphOptimizerUTest, Initialize_SUCCESS_1) {
  map<string, string> options;
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

  ffts_plus_graph_optimizer_ptr->init_flag_ = true;
  Status ret = ffts_plus_graph_optimizer_ptr->Initialize(options, nullptr);
  EngineManager::Instance(kFFTSPlusCoreName).soc_version_ = pre_soc_version;
  fe::PlatformInfoManager::Instance().platform_infos_map_.clear();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, Initialize_SUCCESS_2) {
  map<string, string> options;
  Status ret = ffts_plus_graph_optimizer_ptr->Initialize(options, nullptr);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, Finalize_SUCCESS_1) {
  Status ret = ffts_plus_graph_optimizer_ptr->Finalize();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, Finalize_SUCCESS_2) {
  ffts_plus_graph_optimizer_ptr->lx_fusion_plugin_ffts_plus_ = std::make_shared<PluginManager>("plugin_path");
  ffts_plus_graph_optimizer_ptr->init_flag_ = true;
  Status ret = ffts_plus_graph_optimizer_ptr->Finalize();
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeOriginalGraph_FAILED) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeOriginalGraph(*graph);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeOriginalGraph_SUCCESS) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  ffts_plus_graph_optimizer_ptr->init_flag_ = true;
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeOriginalGraph(*graph);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeFusedGraph_FAILED_1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  ffts_plus_graph_optimizer_ptr->init_flag_ = false;
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeFusedGraph(*graph);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeFusedGraph_FAILED_2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  ffts_plus_graph_optimizer_ptr->init_flag_ = true;
  ffts_plus_graph_optimizer_ptr->FFTSOptimizer_ = FFTSOptimizerStub1;
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeFusedGraph(*graph);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeFusedGraph_SUCCESS) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  ffts_plus_graph_optimizer_ptr->init_flag_ = true;
  ffts_plus_graph_optimizer_ptr->FFTSOptimizer_ = FFTSOptimizerStub2;
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeFusedGraph(*graph);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, GetAttributes_SUCCESS) {
  ge::GraphOptimizerAttribute attrs;
  Status ret = ffts_plus_graph_optimizer_ptr->GetAttributes(attrs);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeWholeGraph_SUCCESS) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeWholeGraph(*graph);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, CreateFunctionOp_SUCCESS) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  vector<ge::NodePtr> node_vec;
  ge::OpDescPtr op_desc_ptr = ffts_plus_graph_optimizer_ptr->CreateFunctionOp(node_vec);
  bool flag = false;
  if (op_desc_ptr == nullptr) {
    flag = true;
  }
  EXPECT_EQ(true, flag);
}

TEST_F(FftsPlusGraphOptimizerUTest, CreateFunctionOpSubGraph_FAILED) {
  ge::NodePtr function_node;
  std::vector<ge::NodePtr> node_vec;
  vector<fe::FusionDataFlow> input_edge_list;
  vector<fe::FusionDataFlow> output_edge_list;
  Status ret = ffts_plus_graph_optimizer_ptr->CreateFunctionOpSubGraph(function_node, node_vec, input_edge_list,
                                                                       output_edge_list);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, OptimizeGraphBeforeBuild_FAILED) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  Status ret = ffts_plus_graph_optimizer_ptr->OptimizeGraphBeforeBuild(*graph);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, TransSubGraphToFunctionOp_FAILED) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("_sgt_sub_graph");
  CreateGraph(graph);
  for(auto node : graph->GetDirectNode()) {
    (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, 1);
  }
  Status ret = ffts_plus_graph_optimizer_ptr->TransSubGraphToFunctionOp(*graph);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(FftsPlusGraphOptimizerUTest, InitLibPath_SUCCESS) {
  Status ret = ffts_plus_graph_optimizer_ptr->InitLibPath();
  EXPECT_EQ(ffts::SUCCESS, ret);
}