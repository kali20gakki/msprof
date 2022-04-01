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

#define private public
#define protected public
#include "graph/load/model_manager/task_info/ffts_plus_task_info.h"
#include "graph/load/model_manager/tbe_kernel_handle.h"
#include "cce/aicpu_engine_struct.h"
#include "common/properties_manager.h"
#include "framework/common/fmk_error_codes.h"
#include "graph/attr_value.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_manager.h"
#include "ut/ge/ffts_plus_proto_tools.h"

namespace ge {
class UtestFftsPlusTaskInfo : public testing::Test {
protected:
  void SetUp() {}

  void TearDown() {}
};


// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, invalid_ffts_plus_task_info_model_param) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  FftsPlusTaskInfo ffts_plus_task_info;
  // init failed when model without op_desc
  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), PARAM_INVALID);
}

// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_start) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *fftsplusstartctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusstartctx->set_op_index(0);
  fftsplusstartctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_START));
  domi::FftsPlusAtStartCtxDef *startctxdef = fftsplusstartctx->mutable_at_start_ctx();
  InitAtStartCtx(startctxdef);
  startctxdef->add_successor_list(1);
  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), FAILED);
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_software_start) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  davinci_model.runtime_param_.mem_size = 0x4000000;
  davinci_model.runtime_param_.logic_mem_base = 0x10000000;
  davinci_model.runtime_param_.mem_base = 0x0;
  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *fftsplusstartctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusstartctx->set_op_index(0);
  fftsplusstartctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_START));
  domi::FftsPlusAtStartCtxDef *startctxdef = fftsplusstartctx->mutable_at_start_ctx();
  InitAtStartCtx(startctxdef);

  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_software_end) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *fftsplusendctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusendctx->set_op_index(0);
  fftsplusendctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_END));
  domi::FftsPlusAtEndCtxDef *endctxdef = fftsplusendctx->mutable_at_end_ctx();
  InitAtEndCtx(endctxdef);
  endctxdef->add_succ_at_start_slot(1);
  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), FAILED);
}
// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_software_end_label) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *fftsplusendctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusendctx->set_op_index(0);
  fftsplusendctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_END));
  domi::FftsPlusAtEndCtxDef *endctxdef = fftsplusendctx->mutable_at_end_ctx();
  InitAtEndCtx(endctxdef);
  endctxdef->add_succ_out_label_slot(1);
  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_software_end) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  davinci_model.runtime_param_.mem_size = 0x4000000;
  davinci_model.runtime_param_.logic_mem_base = 0x10000000;
  davinci_model.runtime_param_.mem_base = 0x0;
  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *fftsplusendctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusendctx->set_op_index(0);
  fftsplusendctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_END));
  domi::FftsPlusAtEndCtxDef *endctxdef = fftsplusendctx->mutable_at_end_ctx();
  InitAtEndCtx(endctxdef);

  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), SUCCESS);
}
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_software_ctx_lable) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *fftspluslabelctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftspluslabelctx->set_op_index(0);
  fftspluslabelctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_LABEL));
  domi::FftsPlusLabelCtxDef *labelctxdef = fftspluslabelctx->mutable_label_ctx();
  InitLabelCtx(labelctxdef);
  labelctxdef->add_successor_list(1);
  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), FAILED);
}
// test FftsPlusTaskInfo Init software ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_software_ctx) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FftsPlusTaskInfo ffts_plus_task_info;

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);

  davinci_model.runtime_param_.mem_size = 0x4000000;
  davinci_model.runtime_param_.logic_mem_base = 0x10000000;
  davinci_model.runtime_param_.mem_base = 0x0;
  ffts_plus_task_info.io_addrs_ = { 0x12000000, 0x12001000 };

  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *fftsplusstartctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusstartctx->set_op_index(0);
  fftsplusstartctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_START));
  domi::FftsPlusAtStartCtxDef *startctxdef = fftsplusstartctx->mutable_at_start_ctx();
  InitAtStartCtx(startctxdef);

  domi::FftsPlusCtxDef *fftsplusendctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftsplusendctx->set_op_index(0);
  fftsplusendctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AT_END));
  domi::FftsPlusAtEndCtxDef *endctxdef = fftsplusendctx->mutable_at_end_ctx();
  InitAtEndCtx(endctxdef);

  domi::FftsPlusCtxDef *fftspluslabelctx = ffts_plus_task_def->add_ffts_plus_ctx();
  fftspluslabelctx->set_op_index(0);
  fftspluslabelctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_LABEL));

  EXPECT_EQ(ffts_plus_task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_notify_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *notifyctx = ffts_plus_task_def->add_ffts_plus_ctx();
  notifyctx->set_op_index(0);
  notifyctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_NOTIFY_WAIT));

  domi::FftsPlusNotifyCtxDef *notifydef = notifyctx->mutable_notify_ctx();
  InitNotifyCtx(notifydef);
  notifydef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_notify_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *notifyctx = ffts_plus_task_def->add_ffts_plus_ctx();
  notifyctx->set_op_index(0);
  notifyctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_NOTIFY_RECORD));
  domi::FftsPlusNotifyCtxDef *notifydef = notifyctx->mutable_notify_ctx();
  InitNotifyCtx(notifydef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_sdma_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *sdmactx = ffts_plus_task_def->add_ffts_plus_ctx();
  sdmactx->set_op_index(0);
  sdmactx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_SDMA));
  domi::FftsPlusSdmaCtxDef *smdadef = sdmactx->mutable_sdma_ctx();
  InitSdmaCtx(smdadef);
  smdadef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_sdma_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *sdmactx = ffts_plus_task_def->add_ffts_plus_ctx();
  sdmactx->set_op_index(0);
  sdmactx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_SDMA));
  domi::FftsPlusSdmaCtxDef *smdadef = sdmactx->mutable_sdma_ctx();
  InitSdmaCtx(smdadef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_writeval_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *writevalctx = ffts_plus_task_def->add_ffts_plus_ctx();
  writevalctx->set_op_index(0);
  writevalctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_WRITE_VALUE));
  domi::FftsPlusWriteValueCtxDef *writedef = writevalctx->mutable_write_value_ctx();
  InitWriteValueCtx(writedef);
  writedef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_writeval_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *writevalctx = ffts_plus_task_def->add_ffts_plus_ctx();
  writevalctx->set_op_index(0);
  writevalctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_WRITE_VALUE));
  domi::FftsPlusWriteValueCtxDef *writedef = writevalctx->mutable_write_value_ctx();
  InitWriteValueCtx(writedef);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_aicpu_ctx_id) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuFwkCtxCtx(aicpudef);
  aicpudef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_aicpu_fwk_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuFwkCtxCtx(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_aicpu_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuCtxCtx(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_data_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *datactx = ffts_plus_task_def->add_ffts_plus_ctx();
  datactx->set_op_index(0);
  datactx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_FLUSH_DATA));
  domi::FftsPlusDataCtxDef *datadef = datactx->mutable_data_ctx();
  InitDataCtx(datadef);
  datadef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_data_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *datactx = ffts_plus_task_def->add_ffts_plus_ctx();
  datactx->set_op_index(0);
  datactx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_WRITEBACK_DATA));
  domi::FftsPlusDataCtxDef *datadef = datactx->mutable_data_ctx();
  InitDataCtx(datadef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_caseswitch_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *caseswitchctx = ffts_plus_task_def->add_ffts_plus_ctx();
  caseswitchctx->set_op_index(0);
  caseswitchctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_CASE_SWITCH));
  domi::FftsPlusCaseSwitchCtxDef *caseswitchdef = caseswitchctx->mutable_case_switch_ctx();
  InitCaseSwitchCtx(caseswitchdef);
  caseswitchdef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_caseswitch_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *caseswitchctx = ffts_plus_task_def->add_ffts_plus_ctx();
  caseswitchctx->set_op_index(0);
  caseswitchctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_CASE_SWITCH));
  domi::FftsPlusCaseSwitchCtxDef *caseswitchdef = caseswitchctx->mutable_case_switch_ctx();
  InitCaseSwitchCtx(caseswitchdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_candswitch_ctx_param) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *candswitchctx = ffts_plus_task_def->add_ffts_plus_ctx();
  candswitchctx->set_op_index(0);
  candswitchctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_COND_SWITCH));
  domi::FftsPlusCondSwitchCtxDef *candswitchdef = candswitchctx->mutable_cond_switch_ctx();
  InitCondSwitchCtx(candswitchdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), INTERNAL_ERROR);
}
// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_candswitch_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *candswitchctx = ffts_plus_task_def->add_ffts_plus_ctx();

  candswitchctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_COND_SWITCH));
  domi::FftsPlusCondSwitchCtxDef *candswitchdef = candswitchctx->mutable_cond_switch_ctx();
  InitCondSwitchCtx(candswitchdef);

  candswitchdef->add_true_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, sucess_ffts_plus_task_info_hardware_candswitch_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *candswitchctx = ffts_plus_task_def->add_ffts_plus_ctx();
  candswitchctx->set_op_index(0);
  candswitchctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_COND_SWITCH));
  domi::FftsPlusCondSwitchCtxDef *candswitchdef = candswitchctx->mutable_cond_switch_ctx();
  InitCondSwitchCtx(candswitchdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_aicaiv_ctx_1) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aictest"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aivtest"] = {(void*)(0x1235), 2};

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICORE));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  InitAicAivCtx(aicaivdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_aicaiv_ctx_2) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aictest"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aivtest"] = {(void*)(0x1235), 2};

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  InitAicAivCtx(aicaivdef);

  aicaivdef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}
// test FftsPlusTaskInfo Init cache persistent ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_cache_persistent_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  op_desc->AddInputDesc(tensor);
  op_desc->SetInputOffset({1024});
  const uint32_t persistent_id = 0x00000001U;
  AttrUtils::SetInt(op_desc->MutableInputDesc(0U), "_cache_persist", persistent_id);
  davinci_model.op_list_[0] = op_desc;

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *cachepersistentctx = ffts_plus_task_def->add_ffts_plus_ctx();
  cachepersistentctx->set_op_index(0);
  cachepersistentctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_PERSISTENT_CACHE));
  domi::FftsPlusCachePersistCtxDef *cachepersistentdef = cachepersistentctx->mutable_cache_persist_ctx();
  InitCachePersistentCtx(cachepersistentdef);

  cachepersistentdef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}
// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_aicaiv_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aictest"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aivtest"] = {(void*)(0x1235), 2};

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  InitAicAivCtx(aicaivdef);

  aicaivdef->add_successor_list(1);
  aicaivdef->add_kernel_name("aivtest");
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, sucess_ffts_plus_task_info_hardware_aicaiv_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aictest"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["aivtest"] = {(void*)(0x1235), 2};
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(6);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AIV));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  InitAicAivCtx(aicaivdef);

  aicaivdef->add_successor_list(1);
  aicaivdef->add_kernel_name("aivtest");
  aicaivdef->add_src_slot(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_mixaicaiv_ctx_1) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_a"] = {(void*)(0x1245), 1};
  //davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_b"] = {(void*)(0x1235), 2};
  //davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_a"] = {(void*)(0x12345), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_b"] = {(void*)(0x12435), 2};
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIC));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(mixctxdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_mixaicaiv_ctx_2) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_a"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_b"] = {(void*)(0x1235), 2};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_a"] = {(void*)(0x12345), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_b"] = {(void*)(0x12435), 2};
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(6);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIV));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(mixctxdef);

  mixctxdef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}
// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_hardware_mixaicaiv_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_a"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_b"] = {(void*)(0x1235), 2};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_a"] = {(void*)(0x12345), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_b"] = {(void*)(0x12435), 2};
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIV));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(mixctxdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}

// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, sucess_ffts_plus_task_info_hardware_mixaicaiv_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_a"] = {(void*)(0x1245), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaic_b"] = {(void*)(0x1235), 2};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_a"] = {(void*)(0x12345), 1};
  davinci_model.bin_kernel_handle_.addr_and_pref_cnt_["mixaiv_b"] = {(void*)(0x12435), 2};
  davinci_model.runtime_param_.mem_size = 0x45678;
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(10);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIV));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();
  InitMixAicAivCtx(mixctxdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_hardware_ctx_ex) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *casesdefaultctx = ffts_plus_task_def->add_ffts_plus_ctx();
  casesdefaultctx->set_op_index(0);
  casesdefaultctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_CASE_SWITCH));
  domi::FftsPlusCaseDefaultCtxDef *casesdefaultdef = casesdefaultctx->mutable_case_default_ctx();
  InitCaseDefaultCtx(casesdefaultdef);
  casesdefaultdef->add_successor_list(1);
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), FAILED);
}
// test FftsPlusTaskInfo Init hardware ctx
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_hardware_ctx_ex) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *casesdefaultctx = ffts_plus_task_def->add_ffts_plus_ctx();

  casesdefaultctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_CASE_SWITCH));
  domi::FftsPlusCaseDefaultCtxDef *casesdefaultdef = casesdefaultctx->mutable_case_default_ctx();
  InitCaseDefaultCtx(casesdefaultdef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}
// test FftsPlusTaskInfo UpdateArgs
TEST_F(UtestFftsPlusTaskInfo, failed_ffts_plus_task_info_update_args) {
  DavinciModel davinci_model(0, nullptr);
  FftsPlusTaskInfo task_info;
  task_info.ffts_plus_task_info_.fftsPlusSqe = nullptr;
  task_info.ffts_plus_task_info_.descBuf = nullptr;
  task_info.davinci_model_ = &davinci_model;

  task_info.io_addrs_ = { 0x12000000, 0x12001000 };
  task_info.mode_addr_idx_.emplace(0);
  EXPECT_EQ(task_info.UpdateArgs(), INTERNAL_ERROR);    // args not alloc.
}

// test FftsPlusTaskInfo CalculateArgs
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_calculate_args) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  task_info.davinci_model_ = &(davinci_model);
  task_info.ffts_plus_task_info_.fftsPlusSqe = nullptr;
  task_info.ffts_plus_task_info_.descBuf = nullptr;
  EXPECT_EQ(task_info.CalculateArgs(task_def, &davinci_model), SUCCESS);
}

// test FftsPlusTaskInfo Distribute
TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_distribute) {
  DavinciModel davinci_model(0, nullptr);
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  task_info.stream_ = stream;
  task_info.davinci_model_ = &davinci_model;
  EXPECT_EQ(task_info.Distribute(), SUCCESS);
}

// test FftsPlusTaskInfo FindDumpArgs
TEST_F(UtestFftsPlusTaskInfo, test_get_dump_args) {
  FftsPlusTaskInfo task_info;
  EXPECT_EQ(task_info.FindDumpArgs(std::string("test_op")), 0);
  EXPECT_EQ(task_info.FindDumpArgs(std::string("test")), 0);
}

// test FftsPlusTaskInfo InitDumpArgs
TEST_F(UtestFftsPlusTaskInfo, test_init_dump_args) {
  DavinciModel davinci_model(0, nullptr);
  OpDescPtr op_desc = CreateOpDesc("test", "optp");
  OpDescPtr op_aicpu = CreateOpDesc("test_cput", "optp");
  davinci_model.is_op_debug_reg_ = true;

  FftsPlusTaskInfo task_info;
  task_info.davinci_model_ = &davinci_model;
  task_info.InitDumpArgs(op_desc, 0);
  EXPECT_EQ(task_info.dump_args_offset_.size(), 1);
  task_info.InitDumpArgs(op_aicpu, 0);
  EXPECT_EQ(task_info.dump_args_offset_.size(), 2);
}

// test FftsPlusTaskInfo Init aicpu ctx
TEST_F(UtestFftsPlusTaskInfo, fail_ffts_plus_task_info_aicpu_fwk_ctx_copy_workspace) {
  DavinciModel davinci_model(0, nullptr);
  davinci_model.runtime_param_.mem_size = 10240;
  davinci_model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(new uint8_t[davinci_model.runtime_param_.mem_size]);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuFwkCtxAndExtInfo(aicpudef);

  // Workspace is empty, SUCCESS for dynamic.
  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
  delete [] reinterpret_cast<uint8_t *>(davinci_model.runtime_param_.mem_base);
  davinci_model.runtime_param_.mem_base = 0U;
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_aicpu_fwk_ctx) {
  DavinciModel davinci_model(0, nullptr);
  davinci_model.runtime_param_.mem_size = 10240;
  davinci_model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(new uint8_t[davinci_model.runtime_param_.mem_size]);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };

  task_def.set_stream_id(0);

  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  davinci_model.op_list_[0] = CreateOpDesc("test", PARTITIONEDCALL);
  davinci_model.op_list_[0]->SetWorkspace({1308});
  davinci_model.op_list_[0]->SetWorkspaceBytes({150});
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuFwkCtxAndExtInfo(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);

  delete [] reinterpret_cast<uint8_t *>(davinci_model.runtime_param_.mem_base);
  davinci_model.runtime_param_.mem_base = 0;
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_aicpu_fwk_ctx_with_io) {
  DavinciModel davinci_model(0, nullptr);
  davinci_model.runtime_param_.mem_size = 10240;
  davinci_model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(new uint8_t[davinci_model.runtime_param_.mem_size]);
  davinci_model.runtime_param_.logic_mem_base = 0x10000000;
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  task_def.set_stream_id(0);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  op_desc->SetInputOffset({1});
  op_desc->SetOutputOffset({100});
  GeTensorDesc input_desc(GeShape({1, 1, 1, 1}), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(input_desc, 4);
  op_desc->AddInputDesc(input_desc);
  GeTensorDesc output_desc(GeShape({1, 1, 1, 1}), FORMAT_NCHW, DT_FLOAT16);
  TensorUtils::SetSize(output_desc, 32);
  op_desc->AddOutputDesc(output_desc);
  op_desc->SetId(0);
  op_desc->SetWorkspace({1308});
  op_desc->SetWorkspaceBytes({150});
  davinci_model.op_list_[0] = op_desc;
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuFwkCtxAndExtInfo(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
  delete [] reinterpret_cast<uint8_t *>(davinci_model.runtime_param_.mem_base);
  davinci_model.runtime_param_.mem_base = 0;
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_aicpu_op_ctx) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  task_def.set_stream_id(0);
  davinci_model.stream_list_ = { stream };
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  auto op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  AttrUtils::SetBool(op_desc, "_AllShape", true);
  op_desc->SetInputOffset({1});
  op_desc->SetOutputOffset({100});
  op_desc->SetId(0);
  op_desc->SetWorkspace({1308});
  op_desc->SetWorkspaceBytes({150});
  davinci_model.op_list_[0] = op_desc;

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(16);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuCtxAndExtInfo(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_custom_aicpu_op_ctx) {
  DavinciModel davinci_model(0, nullptr);
  davinci_model.ge_model_ = MakeShared<GeModel>();
  const auto model_def = MakeShared<domi::ModelTaskDef>();
  davinci_model.ge_model_->SetModelTaskDef(model_def);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  task_def.set_stream_id(0);
  davinci_model.stream_list_ = { stream };
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  auto op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  AttrUtils::SetBool(op_desc, "_AllShape", true);
  op_desc->SetInputOffset({1});
  op_desc->SetOutputOffset({100});
  op_desc->SetId(0);
  op_desc->SetWorkspace({1308});
  op_desc->SetWorkspaceBytes({150});
  davinci_model.op_list_[0] = op_desc;

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(16);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitCustomAicpuCtxAndExtInfo(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestFftsPlusTaskInfo, success_ffts_plus_task_info_aicpu_op_ctx_with_io) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  FftsPlusTaskInfo task_info;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  davinci_model.runtime_param_.mem_size = 0x4000000;
  davinci_model.runtime_param_.logic_mem_base = 0x10000000;
  davinci_model.runtime_param_.mem_base = 0x0;
  task_def.set_stream_id(0);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  op_desc->SetInputOffset({1});
  op_desc->SetOutputOffset({100});

  GeTensorDesc input_desc(GeShape({1, 1, 1, 1}), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(input_desc, 4);
  op_desc->AddInputDesc(input_desc);
  GeTensorDesc output_desc(GeShape({1, 1, 1, 1}), FORMAT_NCHW, DT_FLOAT16);
  TensorUtils::SetSize(output_desc, 32);
  op_desc->AddOutputDesc(output_desc);
  op_desc->SetId(0);
  op_desc->SetWorkspace({1308});
  op_desc->SetWorkspaceBytes({150});
  davinci_model.op_list_[0] = op_desc;

  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(16);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);

  domi::FftsPlusCtxDef *aicpuctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicpuctx->set_op_index(0);
  aicpuctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICPU));
  domi::FftsPlusAicpuCtxDef *aicpudef = aicpuctx->mutable_aicpu_ctx();
  InitAicpuCtxAndExtInfo(aicpudef);

  EXPECT_EQ(task_info.Init(task_def, &davinci_model), SUCCESS);
}
}  // namespace ge
