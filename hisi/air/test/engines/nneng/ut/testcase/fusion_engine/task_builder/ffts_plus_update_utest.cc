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
#include "ffts_plus_update/ffts_plus_update.h"
#include "common/fe_executor/ffts_plus_qos_update.h"
#include "common/aicore_util_attr_define.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "common/util.h"
#include <nlohmann/json.hpp>
#include "../fe_test_utils.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

using FFTSPlusFeUpdatePtr = std::shared_ptr<FFTSPlusFeUpdate>;

class FFTS_plus_update : public testing::Test
{
protected:
void SetUp()
{
  update_ptr_ = std::make_shared<FFTSPlusFeUpdate>();
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
  slice_info_ptr_->thread_mode = true;
  return;
}

public:
	FFTSPlusFeUpdatePtr update_ptr_;
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


// MIXL2
TEST_F(FFTS_plus_update, UpdateMixl2Context)
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
  (void)ge::AttrUtils::SetStr(node->GetOpDesc(), fe::ATTR_NAME_ALIAS_ENGINE_NAME, "ffts_plus");
  Status ret = update_ptr_->UpdateSubTaskAndCache(node, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);

  (void)ge::AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_KERNEL_LIST_FIRST_NAME, "Test");
  ret = update_ptr_->UpdateSubTaskAndCache(node, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::SUCCESS, ret);

  ge::AttrUtils::SetInt(node->GetOpDesc(), "_context_id", 3);
  ret = update_ptr_->UpdateSubTaskAndCache(node, *subtaskFlush_, *fftsPlusTaskInfo_);
  EXPECT_EQ(fe::FAILED, ret);
  vector<optiling::utils::OpRunInfo> op_run_info;
  AutoThreadParam auto_thread_param;
  ret = update_ptr_->GetAutoThreadParam(node, op_run_info, auto_thread_param);
  EXPECT_EQ(fe::SUCCESS, ret);
}