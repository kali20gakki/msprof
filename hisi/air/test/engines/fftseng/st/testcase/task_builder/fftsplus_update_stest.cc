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

#define protected public
#define private public
#include "executor/ffts_plus_update/ffts_plus_aicore_update.h"
#include "executor/ffts_plus_update/ffts_plus_engine_update.h"
#include "graph/utils/node_utils.h"
#include "graph_builder_utils.h"
#include "graph/utils/op_desc_utils.h"
#include <nlohmann/json.hpp>
#include "../test_utils.h"
#undef private
#undef protected

using namespace std;
using namespace ffts;
using namespace ge;

using FFTSPlusAICoreUpdatePtr = std::shared_ptr<FFTSPlusAICoreUpdate>;

class FFTS_plus_update : public testing::Test
{
protected:
    void SetUp()
    {
      update_ptr_ = std::make_shared<FFTSPlusAICoreUpdate>();
      update_ptr_->input_num_ = 4;
      update_ptr_->input_output_num_ = 8;
      update_ptr_->data_params_ = new DataContextParam[2 * kMaxIndexNum];
      fftsPlusTaskInfo_ = new rtFftsPlusTaskInfo_t;
      memset(fftsPlusTaskInfo_, 0, sizeof(rtFftsPlusTaskInfo_t));
      subtaskFlush_ = new AutoThreadSubTaskFlush;
      slice_info_ptr_ = std::make_shared<ffts::ThreadSliceMap>();
      args_ptr_ = new unsigned long long[20];
      for (int i = 0; i < 20; ++i) {
        args_ptr_[i] = i + 2;
      }
    }
    void TearDown()
    {
      rtFftsPlusComCtx_t* firstPtr = (rtFftsPlusComCtx_t*)fftsPlusTaskInfo_->descBuf;
      if (firstPtr) {
        delete[] firstPtr;
      }
      if (fftsPlusTaskInfo_->fftsPlusSqe) {
        delete fftsPlusTaskInfo_->fftsPlusSqe;
      }
      delete fftsPlusTaskInfo_;
      delete subtaskFlush_;
      delete[] args_ptr_;
    }

  void CreateAutoTaskInfo(rtFftsPlusTaskInfo_t *fftsPlusTaskInfo, bool isAuto)
  {
    if (fftsPlusTaskInfo == nullptr) {
      return;
    }
    memset(fftsPlusTaskInfo, 0, sizeof(rtFftsPlusTaskInfo_t));
    int ctxNum = 36;
    fftsPlusTaskInfo->descBuf = new rtFftsPlusComCtx_t[ctxNum];
    fftsPlusTaskInfo->descBufLen = ctxNum * sizeof(rtFftsPlusComCtx_t);
    fftsPlusTaskInfo->fftsPlusSqe = new rtFftsPlusSqe_t;
    rtFftsPlusSqe_t *aqePtr = const_cast<rtFftsPlusSqe_t *>(fftsPlusTaskInfo->fftsPlusSqe);
    aqePtr->totalContextNum = ctxNum;
    rtFftsPlusComCtx_t *comCtx = (rtFftsPlusComCtx_t*)fftsPlusTaskInfo->descBuf;
    int i = 2;
    for (i = 2; i < 6; i++) {
      rtFftsPlusAicAivCtx_t *ctx = reinterpret_cast<rtFftsPlusAicAivCtx_t*>(comCtx + i);
      ctx->contextType = RT_CTX_TYPE_AICORE;
    }
    for (i = 6; i < 18; i++) {
      rtFftsPlusDataCtx_t *dataCtx = reinterpret_cast<rtFftsPlusDataCtx_t*>(comCtx + i);
      dataCtx->contextType = RT_CTX_TYPE_FLUSH_DATA;
    }
    for (i = 18; i < 30; i++) {
      rtFftsPlusDataCtx_t *dataCtx = reinterpret_cast<rtFftsPlusDataCtx_t*>(comCtx + i);
      dataCtx->contextType = RT_CTX_TYPE_INVALIDATE_DATA;
    }
    rtFftsPlusAtStartCtx_t *startCtx = reinterpret_cast<rtFftsPlusAtStartCtx_t*>(comCtx + 1);
    startCtx->contextType = RT_CTX_TYPE_AT_START;

    rtFftsPlusLabelCtx_t *lableCtx = reinterpret_cast<rtFftsPlusLabelCtx_t*>(comCtx);
    lableCtx->contextType = RT_CTX_TYPE_LABEL;

    lableCtx = reinterpret_cast<rtFftsPlusLabelCtx_t*>(comCtx + 31);
    lableCtx->contextType = RT_CTX_TYPE_LABEL;
  }

  void CreateSubTaskFlush(AutoThreadSubTaskFlush *subtaskFlush)
  {
    if (subtaskFlush == nullptr) {
    return;
    }
    memset(subtaskFlush, 0, sizeof(AutoThreadSubTaskFlush));
    subtaskFlush->device_id = 1;
    slice_info_ptr_->thread_mode = false;
    return;
  }

  void CreateAutoSubTaskFlush(AutoThreadSubTaskFlush *subtaskFlush)
  {
    if (subtaskFlush == nullptr) {
      return;
    }
    subtaskFlush->device_id = 1;
    subtaskFlush->args_base = (void*)args_ptr_;
    tailRunInfo_.SetBlockDim(4);
    tailRunInfo_.AddWorkspace(32);
    tailRunInfo_.AddWorkspace(32);
    tailRunInfo_.AddWorkspace(32);
    nonTailRunInfo_.SetBlockDim(2);
    nonTailRunInfo_.AddWorkspace(20);

    subtaskFlush->op_run_info.push_back(tailRunInfo_);
    subtaskFlush->op_run_info.push_back(nonTailRunInfo_);
    input_addrs_.clear();
    output_addrs_.clear();
    std::vector<std::vector<std::vector<ffts::DimRange>>> input_tensor_slice;
    std::vector<std::vector<std::vector<ffts::DimRange>>> output_tensor_slice;
    int num = 40 / thread_dim_ + 1;
    for (int i = 0; i < thread_dim_; i++) {
    input_addrs_.push_back(0x12345 + i);
    output_addrs_.push_back(0x54321 + i);
    std::vector<std::vector<ffts::DimRange>> tDim1;
    for (int j = 0; j < 4; j++) {
      std::vector<ffts::DimRange> v1;
      ffts::DimRange tDim;
      tDim.higher = num * (i + 1);
      if (tDim.higher > 40) {
      tDim.higher = 40;
      }
      tDim.lower = num * i;
      v1.push_back(tDim);
      tDim.higher = 100;
      tDim.lower = 0;
      v1.push_back(tDim);
      tDim.higher = 100;
      tDim.lower = 0;
      v1.push_back(tDim);
      tDim1.push_back(v1);
    }
    input_tensor_slice.push_back(tDim1);
    }
    output_tensor_slice = input_tensor_slice;
    slice_info_ptr_->input_tensor_slice = input_tensor_slice;
    slice_info_ptr_->output_tensor_slice = output_tensor_slice;
    slice_info_ptr_->slice_instance_num = thread_dim_;
    slice_info_ptr_->parallel_window_size = window_size_;
    slice_info_ptr_->input_tensor_indexes.emplace_back(0);
    slice_info_ptr_->output_tensor_indexes.emplace_back(0);
    slice_info_ptr_->thread_mode = true;
    return;
  }

  void CreateAutoCubeNode()
  {
    FeTestOpDescBuilder builder;
    builder.SetName("test_tvm");
    builder.SetType("conv");
    builder.SetInputs({ 4 });
    builder.SetOutputs({ 4 });
    for (int i = 0; i < 4; i++) {
      builder.AddInputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder.AddOutputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
    }
    auto node = builder.Finish();

    ge::AttrUtils::SetInt(node->GetOpDesc(), "_context_id", 2);
    ge::AttrUtils::SetInt(node->GetOpDesc(), "_default_context_id", 3);
    std::vector<std::vector<int64_t>> ctxVecV;
    std::vector<int64_t> ctxVec;
    int i = 2, num;
    num = 2 + window_size_;
    for (i = 2; i < num; i++) {
      ctxVec.push_back(i);
    }
    (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), kAutoCtxIdList, ctxVec);

    ctxVec.clear();
    for (i = num; i < num + window_size_ * 3; i++) {
      ctxVec.push_back(i);
    }
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    (void)ge::AttrUtils::SetListListInt(node->GetOpDesc(), "_data_prefetch_ctx_id_list", ctxVecV);
    ctxVec.clear();
    ctxVecV.clear();
    num = num + window_size_ * 3;
    for (i = num; i < num + window_size_ * 3; i++) {
      ctxVec.push_back(i);
    }
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    ctxVecV.push_back(ctxVec);
    (void)ge::AttrUtils::SetListListInt(node->GetOpDesc(), "_invalid_ctx_id_list", ctxVecV);
    ctxVec.clear();
    num = num + window_size_ * 3 + 1;
    for (i = 0; i < 2; i++) {
      ctxVec.push_back(i);
    }
    ctxVec.push_back(num);
    (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), "_all_ctx_id_list", ctxVec);

    update_ptr_->slice_info_ptr_ = slice_info_ptr_;
    (void)ge::AttrUtils::SetInt(node->GetOpDesc(), "qos_label", 5);

    FeTestOpDescBuilder builder1;
    builder1.SetName("test_tvm_in");
    builder1.SetType("conv");
    builder1.SetInputs({ 4 });
    builder1.SetOutputs({ 4 });
    for (int i = 0; i < 4; i++) {
      builder1.AddInputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder1.AddOutputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
    }
    auto node1 = builder1.Finish();

    std::vector<OutDataAnchorPtr> srcs;
    std::vector<InDataAnchorPtr> dsts;
    for (int i = 0; i < 4; ++i) {
      srcs.push_back(node1->GetOutDataAnchor(i));
      dsts.push_back(node->GetInDataAnchor(i));
      ge::AnchorUtils::SetStatus(node->GetOutDataAnchor(i), ge::ANCHOR_DATA);
    }
    // add edges
    for (int i = 0; i < srcs.size(); ++i) {
      GraphUtils::AddEdge(srcs[i], dsts[i]);
    }
    node_ = node;
    return;
  }

  ComputeGraphPtr CreateAutoSubgraph()
  {
    auto sub_builder = ut::ComputeGraphBuilder("test_graph");
    auto node = sub_builder.AddNode("test_tvm", "conv", 4, 4);
    std::vector<int64_t> ctxVec;
    for (int64_t i = 0; i < 2; i++) {
      ctxVec.push_back(i);
    }
    ctxVec.push_back(31);
    (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), "_all_ctx_id_list", ctxVec);
    (void)node->GetOpDesc()->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr_);
    return sub_builder.GetGraph();
  }

  void CreateAutoCubeNodeOver()
  {
    FeTestOpDescBuilder builder;
    builder.SetName("test_tvm");
    builder.SetType("conv");
    builder.SetInputs({ 4 });
    builder.SetOutputs({ 4 });
    for (int i = 0; i < 4; i++) {
      builder.AddInputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder.AddOutputDesc({ 40,100,100 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
    }
    auto node = builder.Finish();
    update_ptr_->slice_info_ptr_ = slice_info_ptr_;
    ge::AttrUtils::SetInt(node->GetOpDesc(), "_context_id", 2);
    ge::AttrUtils::SetInt(node->GetOpDesc(), "_default_context_id", 3);

    std::vector<int32_t> ctxVec;
    for (int i = 100; i < 104; i++) {
      ctxVec.push_back(i);
    }
    (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), kAutoCtxIdList, ctxVec);
    node_ = node;
  }
public:
	FFTSPlusAICoreUpdatePtr update_ptr_;
	ge::NodePtr node_;
	rtFftsPlusTaskInfo_t *fftsPlusTaskInfo_;
	AutoThreadSubTaskFlush *subtaskFlush_;
	vector<int64_t> input_addrs_;
	vector<int64_t> output_addrs_;
	optiling::utils::OpRunInfo nonTailRunInfo_;
	optiling::utils::OpRunInfo tailRunInfo_;
	ffts::ThreadSliceMapPtr slice_info_ptr_;
	unsigned long long* args_ptr_;
  int thread_dim_;
  int window_size_;
};

// aic
TEST_F(FFTS_plus_update, UpdateContext1)
{
  thread_dim_ = 4;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  subtaskFlush_->input_addr_base.emplace_back(0x111);
  subtaskFlush_->input_addr_base.emplace_back(0x111);
  subtaskFlush_->output_addr_base.emplace_back(0x111);
  subtaskFlush_->output_addr_base.emplace_back(0x111);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  std::vector<int32_t> cmo_idx = {0};
  (void)ge::AttrUtils::SetListInt(node_->GetOpDesc(), "prefetch_idx", cmo_idx);
  (void)ge::AttrUtils::SetListInt(node_->GetOpDesc(), "write back_idx", cmo_idx);
  Status ret = update_ptr_->UpdateSubTaskAndCache(node_, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
}

// aic test set empty vlable
TEST_F(FFTS_plus_update, UpdateContext2)
{
  thread_dim_ = 2;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  Status ret = update_ptr_->UpdateSubTaskAndCache(node_, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
}

// no need CMO
TEST_F(FFTS_plus_update, UpdateContext5)
{
  thread_dim_ = 6;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kPrefetchEnableBm, 0);
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kInvalidateBm, 0);
  Status ret = update_ptr_->UpdateSubTaskAndCache(node_, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
}

// Has suspend input
TEST_F(FFTS_plus_update, UpdateContext6)
{
  thread_dim_ = 6;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  auto anchor = node_->GetInDataAnchor(2);
  ge::AnchorUtils::SetStatus(anchor, ge::ANCHOR_SUSPEND);
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kPrefetchEnableBm, 15);
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kInvalidateBm, 7);
  Status ret = update_ptr_->UpdateSubTaskAndCache(node_, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
}

// contextIdOverRange
TEST_F(FFTS_plus_update, UpdateContext7)
{
  thread_dim_ = 6;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  auto anchor = node_->GetInDataAnchor(2);
  ge::AnchorUtils::SetStatus(anchor, ge::ANCHOR_SUSPEND);
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kPrefetchEnableBm, 15);
  ge::AttrUtils::SetInt(node_->GetOpDesc(), kInvalidateBm, 7);
  Status ret = update_ptr_->UpdateSubTaskAndCache(node_, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
}

// MIXL2
TEST_F(FFTS_plus_update, UpdateContext8)
{
  thread_dim_ = 1;
  window_size_ = 1;
  CreateAutoSubTaskFlush(subtaskFlush_);
  memset(fftsPlusTaskInfo_, 0, sizeof(rtFftsPlusTaskInfo_t));
  fftsPlusTaskInfo_->descBuf = new rtFftsPlusComCtx_t[1];
  fftsPlusTaskInfo_->descBufLen = sizeof(rtFftsPlusComCtx_t);
  fftsPlusTaskInfo_->fftsPlusSqe = new rtFftsPlusSqe_t;
  rtFftsPlusSqe_t *aqePtr = const_cast<rtFftsPlusSqe_t *>(fftsPlusTaskInfo_->fftsPlusSqe);
  aqePtr->totalContextNum = 1;
  rtFftsPlusComCtx_t *comCtx = (rtFftsPlusComCtx_t*)fftsPlusTaskInfo_->descBuf;
  rtFftsPlusAicAivCtx_t *ctx = reinterpret_cast<rtFftsPlusAicAivCtx_t*>(comCtx);
  ctx->contextType = RT_CTX_TYPE_MIX_AIC;
  FeTestOpDescBuilder builder;
  builder.SetName("test_tvm");
  builder.SetType("conv");
  builder.SetInputs({ 0 });
  builder.SetOutputs({ 0 });
  auto node = builder.Finish();
  ge::AttrUtils::SetInt(node->GetOpDesc(), "_context_id", 0);
  ge::AttrUtils::SetStr(node->GetOpDesc(), "_mix_aic" + ATTR_NAME_KERNEL_LIST_FIRST_NAME, "test");
  (void)ge::AttrUtils::SetStr(node->GetOpDesc(), ffts::ATTR_NAME_ALIAS_ENGINE_NAME, "ffts_plus");
  Status ret = update_ptr_->UpdateSubTaskAndCache(node, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);
  ge::AttrUtils::SetInt(node->GetOpDesc(), "_context_id", 3);
  ret = update_ptr_->UpdateSubTaskAndCache(node, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FFTS_plus_update, GetAutoThreadParam)
{
  thread_dim_ = 4;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  vector<optiling::utils::OpRunInfo> op_run_info;
  optiling::utils::OpRunInfo tmp_runinfo;
  std::vector<int64_t> workspaces = {22, 23};
  tmp_runinfo.SetWorkspaces(workspaces);
  op_run_info.emplace_back(tmp_runinfo);
  op_run_info.emplace_back(tmp_runinfo);
  AutoThreadParam auto_thread_param;
  CreateAutoCubeNode();
  (void)node_->GetOpDesc()->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr_);
  Status ret = update_ptr_->GetAutoThreadParam(node_, op_run_info, auto_thread_param);
  EXPECT_EQ(fe::SUCCESS, ret);
  optiling::utils::OpRunInfo tmp;
  op_run_info.emplace_back(tmp);
  op_run_info.emplace_back(tmp);
  ret = update_ptr_->GetAutoThreadParam(node_, op_run_info, auto_thread_param);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FFTS_plus_update, FillDataCtxParam)
{
  rtFftsPlusDataCtx_t ctx;
  thread_dim_ = 2;
  CreateAutoSubTaskFlush(subtaskFlush_);
  update_ptr_->slice_info_ptr_ = slice_info_ptr_;
  AutoThreadSubTaskFlush flush_data;
  flush_data.input_addr_base.emplace_back(0x111);
  flush_data.input_addr_base.emplace_back(0x111);
  flush_data.output_addr_base.emplace_back(0x111);
  flush_data.output_addr_base.emplace_back(0x111);
  update_ptr_->input_num_ = 1;
  update_ptr_->data_params_[0].index = 0;
  Status ret = update_ptr_->FillPrefetchCtxParam(&ctx, flush_data, 0);
  EXPECT_EQ(fe::SUCCESS, ret);
  ret = update_ptr_->FillInvAndWriCtxParam(&ctx, flush_data, 0);
  EXPECT_EQ(fe::SUCCESS, ret);
}
TEST_F(FFTS_plus_update, UpdateCmoCtxProc)
{
  thread_dim_ = 4;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  CreateAutoCubeNode();
  update_ptr_->slice_info_ptr_ = slice_info_ptr_;
  AutoThreadSubTaskFlush flush_data;
  flush_data.input_addr_base.emplace_back(0x111);
  flush_data.input_addr_base.emplace_back(0x111);
  flush_data.output_addr_base.emplace_back(0x111);
  flush_data.output_addr_base.emplace_back(0x111);
  update_ptr_->input_num_ = 1;
  update_ptr_->data_params_[0].index = 0;
  Status ret = update_ptr_->UpdateCmoCtxProc(*fftsPlusTaskInfo_, node_, flush_data, RT_CTX_TYPE_FLUSH_DATA, 1);
  EXPECT_EQ(fe::SUCCESS, ret);
  ret = update_ptr_->UpdateCmoCtxProc(*fftsPlusTaskInfo_, node_, flush_data, RT_CTX_TYPE_INVALIDATE_DATA, 1);
  EXPECT_EQ(fe::SUCCESS, ret);
  ret = update_ptr_->UpdateCmoCtxProc(*fftsPlusTaskInfo_, node_, flush_data, RT_CTX_TYPE_WRITEBACK_DATA, 1);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FFTS_plus_update, CalcAutoThreadOutput) {
  thread_dim_ = 4;
  window_size_ = 4;
  CreateAutoCubeNode();
  AutoThreadParam argsPara;
  vector<vector<vector<ffts::DimRange>>> tensor_slice;
  vector<vector<ffts::DimRange>> tmpvv;
  tensor_slice.emplace_back(tmpvv);
  Status ret = update_ptr_->CalcAutoThreadOutput(node_, tensor_slice, argsPara);
  EXPECT_EQ(fe::FAILED, ret);
  tensor_slice.clear();
  vector<ffts::DimRange> tmpv;
  ffts::DimRange dim;
  dim.higher = 10;
  dim.lower = 0;
  tmpv.emplace_back(dim);
  tmpvv.emplace_back(tmpv);
  tensor_slice.emplace_back(tmpvv);
  ret = update_ptr_->CalcAutoThreadOutput(node_, tensor_slice, argsPara);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FFTS_plus_update, UpdateCommonCtx)
{
  thread_dim_ = 4;
  window_size_ = 4;
  CreateAutoSubTaskFlush(subtaskFlush_);
  CreateAutoTaskInfo(fftsPlusTaskInfo_, 1);
  ComputeGraphPtr sub_graph = CreateAutoSubgraph();
  (void)ge::AttrUtils::SetStr(sub_graph, "_ffts_first_op_name", "test_tvm");
  Status ret = FFTSPlusEngineUpdate::UpdateCommonCtx(sub_graph, *fftsPlusTaskInfo_);
  EXPECT_EQ(true, ret);
}