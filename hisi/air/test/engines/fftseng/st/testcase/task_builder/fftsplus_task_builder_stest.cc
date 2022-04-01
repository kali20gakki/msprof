/**
*
* @file ffts_plus_task_builder_unittest.cc
*
* @brief
*
* @version 1.0
*
*/
#include <gtest/gtest.h>
#include <iostream>
#include <list>

#define private public
#define protected public
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "task_builder/fftsplus_task_builder.h"
#include "task_builder/thread_ctx/aic_aiv_manual_task_builder.h"
#include "graph/node.h"
#include "graph_builder_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "../test_utils.h"

using namespace std;
using namespace ffts;
using namespace ge;
using AICAIVTaskBuilderPtr = std::shared_ptr<AICAIVTaskBuilder>;

class FFTSPlusTaskBuilderSTest : public testing::Test{
 protected:
  void SetUp(){
    aic_aiv_task_builder_ptr = make_shared<AICAIVTaskBuilder>();
    slice_info_ptr = make_shared<ThreadSliceMap>();
    slice_info_ptr->thread_mode = 0;
  }
  void TearDown(){
  }
 public:
  AICAIVTaskBuilderPtr aic_aiv_task_builder_ptr = nullptr;
  ThreadSliceMapPtr slice_info_ptr  = nullptr;
};

TEST_F(FFTSPlusTaskBuilderSTest, FillCustomersInfo_1)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 0, 4);
  for (int i = 0; i < 4; i++) {
    string node_name = "conv2d";
    node_name += to_string(i);
    auto conv2d = builder.AddNode(node_name, "conv2d", 1, 0);
    ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 1);
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, i);
    builder.AddDataEdge(input, i, conv2d, 0);
  }
  vector<FftsPlusComCtx_t> context_vec;
  ge::AttrUtils::SetInt(input->GetOpDesc(), kThreadScopeId, 1);
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillCustomersInfo(input, sub_ffts_plus_context, context_vec);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusTaskBuilderSTest, FillCustomersInfo_2)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 0, 2);
  auto concat = builder.AddNode(kTypePhonyConcat, kTypePhonyConcat, 1, 3);
  for (int i = 0; i < 3; i++) {
    string node_name = "test";
    node_name += to_string(i);
    auto test_node = builder.AddNode(node_name, "test", 1, 0);
    ge::AttrUtils::SetInt(test_node->GetOpDesc(), kThreadScopeId, i);
    if (i == 2) {
      (void)ge::AttrUtils::SetInt(test_node->GetOpDesc(), kContextId, i);
    }
    builder.AddDataEdge(concat, i, test_node, 0);
  }
  auto conv2d = builder.AddNode("conv2d", "conv2d", 1, 0);
  ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 4);
  (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, 8);
  builder.AddDataEdge(input, 0, concat, 0);
  builder.AddDataEdge(input, 1, conv2d, 0);
  ge::AttrUtils::SetInt(input->GetOpDesc(), kThreadScopeId, 1);
  vector<FftsPlusComCtx_t> context_vec;
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillCustomersInfo(input, sub_ffts_plus_context, context_vec);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusTaskBuilderSTest, FillCustomersInfo_3)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 0, 2);
  auto concat = builder.AddNode(kTypePhonyConcat, kTypePhonyConcat, 1, 3);
  for (int i = 0; i < 3; i++) {
    string node_name = "test";
    node_name += to_string(i);
    auto test_node = builder.AddNode(node_name, "test", 1, 0);
    ge::AttrUtils::SetInt(test_node->GetOpDesc(), kThreadScopeId, 1);
    if (i == 2) {
      (void)ge::AttrUtils::SetInt(test_node->GetOpDesc(), kContextId, i);
    }
    builder.AddDataEdge(concat, i, test_node, 0);
  }
  auto conv2d = builder.AddNode("conv2d", "conv2d", 1, 0);
  ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 4);
  (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, 8);
  builder.AddDataEdge(input, 0, concat, 0);
  builder.AddDataEdge(input, 1, conv2d, 0);
  ge::AttrUtils::SetInt(input->GetOpDesc(), kThreadScopeId, 1);
  ge::AttrUtils::SetInt(concat->GetOpDesc(), kThreadScopeId, 1);
  vector<FftsPlusComCtx_t> context_vec;
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillCustomersInfo(input, sub_ffts_plus_context, context_vec);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusTaskBuilderSTest, FillCustomersInfo_4)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 0, 2);
  auto concat = builder.AddNode(kTypePhonyConcat, kTypePhonyConcat, 1, 3);
  for (int i = 0; i < 3; i++) {
    string node_name = "test";
    node_name += to_string(i);
    auto test_node = builder.AddNode(node_name, "test", 1, 0);
    ge::AttrUtils::SetInt(test_node->GetOpDesc(), kThreadScopeId, 1);
    if (i == 2) {
      (void)ge::AttrUtils::SetInt(test_node->GetOpDesc(), kContextId, i);
    }
    builder.AddDataEdge(concat, i, test_node, 0);
  }
  auto conv2d = builder.AddNode("conv2d", "conv2d", 1, 0);
  ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 4);
  (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, 8);
  builder.AddDataEdge(input, 0, concat, 0);
  builder.AddDataEdge(input, 1, conv2d, 0);
  (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
  ge::AttrUtils::SetInt(concat->GetOpDesc(), kThreadScopeId, 1);
  vector<FftsPlusComCtx_t> context_vec;
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillCustomersInfo(input, sub_ffts_plus_context, context_vec);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusTaskBuilderSTest, FillProducersInfo1)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 2, 0);
  auto concat = builder.AddNode(kTypePhonyConcat, kTypePhonyConcat, 3, 1);
  for (int i = 0; i < 3; i++) {
    string node_name = "test";
    node_name += to_string(i);
    auto test_node = builder.AddNode(node_name, "test", 0, 1);
    if (i == 2) {
      (void)ge::AttrUtils::SetInt(test_node->GetOpDesc(), kContextId, i);
    }
    builder.AddDataEdge(test_node, 0, concat, i);
  }
  auto conv2d = builder.AddNode("conv2d", "conv2d", 0, 1);
  (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, 8);
  builder.AddDataEdge(concat, 0, input, 0);
  builder.AddDataEdge(conv2d, 0, input, 1);
  (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillProducersInfo(input, sub_ffts_plus_context);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusTaskBuilderSTest, FillProducersInfo2)
{
  FftsPlusComCtx_t sub_ffts_plus_context = {0};
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 2, 0);
  auto concat = builder.AddNode(kTypePhonyConcat, kTypePhonyConcat, 3, 1);
  for (int i = 0; i < 3; i++) {
    string node_name = "test";
    node_name += to_string(i);
    auto test_node = builder.AddNode(node_name, "test", 0, 1);
    (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
    if (i == 2) {
      (void)ge::AttrUtils::SetInt(test_node->GetOpDesc(), kContextId, i);
    }
    builder.AddDataEdge(test_node, 0, concat, i);
  }
  auto conv2d = builder.AddNode("conv2d", "conv2d", 0, 1);
  (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kContextId, 8);
  (void)ge::AttrUtils::SetBool(conv2d->GetOpDesc(), kTypeFFTSPlus, true);
  builder.AddDataEdge(concat, 0, input, 0);
  builder.AddDataEdge(conv2d, 0, input, 1);
  (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
  (void)input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  Status ret = aic_aiv_task_builder_ptr->FillProducersInfo(input, sub_ffts_plus_context);
  EXPECT_EQ(ffts::SUCCESS, ret);
}