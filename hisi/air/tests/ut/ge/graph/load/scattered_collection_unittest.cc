#include <gtest/gtest.h>
#include <vector>
#include <string.h>

#define protected public
#define private public
#include "ut/ge/ffts_plus_proto_tools.h"
#include "graph/def_types.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/zero_copy_offset.h"
#include "graph/load/model_manager/zero_copy_task.h"
#include "graph/load/model_manager/task_info/ffts_plus_proto_transfer.h"
#include "graph/load/model_manager/task_info/fusion_start_task_info.h"
#include "graph/load/model_manager/task_info/fusion_stop_task_info.h"
#include "graph/load/model_manager/task_info/label_switch_by_index_task_info.h"
#include "graph/load/model_manager/task_info/model_exit_task_info.h"
#include "graph/load/model_manager/task_info/stream_active_task_info.h"
#include "graph/load/model_manager/task_info/task_info.h"
#undef protected
#undef private

using namespace std;
using namespace testing;

namespace ge {
class UtestScatteredCollection : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

// zero_copy_offset.cc
TEST_F(UtestScatteredCollection, ZeroCopyOffset_Invalid) {
  ZeroCopyOffset zero_copy_offset;
  int64_t data_size = 0;
  void *virtual_addr = nullptr;

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});

  bool fusion_flag = true;
  auto ret = zero_copy_offset.InitInputDataInfo(data_size, virtual_addr, op_desc, fusion_flag);
  EXPECT_EQ(ret, SUCCESS);

  std::vector<int64_t> input_size_list;
  std::vector<void *> virtual_addr_list;
  size_t idx = 0;
  input_size_list.emplace_back(5);
  virtual_addr_list.emplace_back(virtual_addr);
  ret = zero_copy_offset.InitOutputDataInfo(input_size_list, virtual_addr_list, op_desc, idx, fusion_flag);
  EXPECT_EQ(ret, SUCCESS);

  std::vector<int64_t> fusion_basic_addrs;
  fusion_basic_addrs.emplace_back(10);
  int64_t tensor_addr = 10;
  zero_copy_offset.IsL2Fusion(fusion_basic_addrs, tensor_addr, fusion_flag);

  int64_t output_offset = 0;
  uintptr_t addr_val;
  std::set<const void *> real_virtual_addrs;
  zero_copy_offset.zero_copy_basic_offset_.emplace_back(0);
  zero_copy_offset.zero_copy_relative_offset_.emplace_back(0);
  zero_copy_offset.SetInputOutsideAddrs(output_offset, addr_val, fusion_flag, real_virtual_addrs);

  int64_t input_offset = 0;
  std::vector<const void *> tensor_addrs;
  zero_copy_offset.SetOutputOutsideAddrs(input_offset, fusion_flag, addr_val, tensor_addrs);

  ZeroCopyTask zero_copy_task("task0", 0, 0);
  uintptr_t outside_addr;
  uintptr_t args_base;
  size_t offset = 0;
  zero_copy_offset.valid_relative_offset_ = false;
  zero_copy_offset.SetOutsideAddrsValue(zero_copy_task,  outside_addr, args_base, offset);

  uintptr_t logical_addr;
  uintptr_t device_addr;
  zero_copy_offset.SetLogicalOutsideAddrs(logical_addr, device_addr);
}

// zero_copy_task.cc
TEST_F(UtestScatteredCollection, ZeroCopyTask_SetOriginalArgs) {
  set<size_t> offset_set {0, 1, 2};
  map<uintptr_t, set<size_t>> task_addr_offset;
  task_addr_offset[0] = offset_set;
  std::vector<uint8_t> args_info = {0, 1, 2};

  ZeroCopyTask task0("task0", 0, 0);
  task0.batch_label_ = "abc";
  task0.task_addr_offset_ = task_addr_offset;
  task0.args_info_ = args_info;

  char_t *info = "args_info_";
  task0.SetOriginalArgs(info, strlen(info));
  EXPECT_EQ(task0.args_size_, 0);
}

TEST_F(UtestScatteredCollection, ZeroCopyTask_DistributeParam_Invalid) {
  set<size_t> offset_set {0, 1, 2};
  map<uintptr_t, set<size_t>> task_addr_offset;
  task_addr_offset[0] = offset_set;
  std::vector<uint8_t> args_info = {0, 1, 2};

  ZeroCopyTask task0("task0", 0, 0);
  task0.batch_label_ = "abc";
  task0.task_addr_offset_ = task_addr_offset;
  task0.args_info_ = args_info;

  bool async_mode = true;
  rtStream_t stream;
  auto ret = task0.DistributeParam(async_mode, stream);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestScatteredCollection, ZeroCopyTask_Other_Invalid) {
  ZeroCopyTask task0("task0", 0, 0);
  uintptr_t addr;
  size_t offset = 100;
  //task0.args_size_ = 0;
  auto ret = task0.SetTaskArgsOffset(addr, offset);
  EXPECT_EQ(ret, FAILED);

  task0.is_updated_ = false;
  rtStream_t stream;
  bool async_mode = false;
  ret = task0.DistributeParam(async_mode, stream);
  EXPECT_EQ(ret, SUCCESS);

  task0.is_updated_ = false;
  async_mode = true;
  ret = task0.DistributeParam(async_mode, stream);
  EXPECT_EQ(ret, SUCCESS);
}

// ffts_plus_proto_transfer.cc
TEST_F(UtestScatteredCollection, FftsPlusProtoTransfer_InitManualAicAivCtx_Failed) {
  std::vector<uintptr_t> io_addrs;
  std::vector<void *> ext_args;
  std::set<size_t> mode_addr_idx;
  const RuntimeParam runtime_param;
  FftsPlusProtoTransfer ffpt(0U, io_addrs, runtime_param, ext_args, mode_addr_idx);

  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  task->stream_id_ = 0;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task->mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  // InitFftsPlusTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICORE));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  rtFftsPlusAicAivCtx_t ctx;

  // Test: ctx_def.kernel_name_size() == 0
  EXPECT_EQ(ffpt.InitManualAicAivCtx(*aicaivdef, ctx), FAILED);

  // Test: ctx_def.kernel_name_size() != kAutoMixAicAivCtxPcNum
  aicaivdef->add_kernel_name("aictest1");
  aicaivdef->add_kernel_name("aictest2");
  EXPECT_EQ(ffpt.InitManualAicAivCtx(*aicaivdef, ctx), FAILED);

  // head file
  FftsRunAddrHandle handle1;
  FftsAddrPrefHandle handle2;
  FftsFindNodeHandle handle3;
  FftsSaveCtxArgsHandle handle4;

  ffpt.SetRunAddrHandle(handle1);
  ffpt.SetAddrPrefHandle(handle2);
  ffpt.SetFindNodeHandle(handle3);
  ffpt.SetSaveCtxArgsHandle(handle4);

  // TEST for default function
  const OpDescPtr op_desc;
  STR_FWK_OP_KERNEL fwk_op_kernel;
  const domi::FftsPlusAicpuCtxDef aicpu_ctx_def;
  const domi::aicpuKernelDef aicpu_kernle_def;
  EXPECT_EQ(ffpt.aicpu_get_session_id_(), 0U);
  EXPECT_EQ(ffpt.create_aicpu_session_(fwk_op_kernel), SUCCESS);
  EXPECT_EQ(ffpt.load_cust_aicpu_so_(op_desc, aicpu_ctx_def), SUCCESS);
  EXPECT_EQ(ffpt.save_aicpu_ctx_handle_(op_desc, aicpu_kernle_def), SUCCESS);
}

TEST_F(UtestScatteredCollection, FftsPlusProtoTransfer_InitAutoMixAicAivCtx) {
  std::vector<uintptr_t> io_addrs;
  std::vector<void *> ext_args;
  std::set<size_t> mode_addr_idx;
  const RuntimeParam runtime_param;
  FftsPlusProtoTransfer ffpt(0U, io_addrs, runtime_param, ext_args, mode_addr_idx);

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIC));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();

  rtFftsPlusMixAicAivCtx_t ctx;
  uint32_t start_idx = 0;

  // Test: ctx_def.kernel_name_size() == 0
  EXPECT_EQ(ffpt.InitAutoMixAicAivCtx(*mixctxdef, ctx, start_idx), FAILED);

  InitMixAicAivCtx(mixctxdef, true);
  EXPECT_EQ(ffpt.InitAutoMixAicAivCtx(*mixctxdef, ctx, start_idx), SUCCESS);
}

TEST_F(UtestScatteredCollection, FftsPlusProtoTransfer_InitManualMixAicAivCtx) {
  std::vector<uintptr_t> io_addrs;
  std::vector<void *> ext_args;
  std::set<size_t> mode_addr_idx;
  const RuntimeParam runtime_param;
  FftsPlusProtoTransfer ffpt(0U, io_addrs, runtime_param, ext_args, mode_addr_idx);

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  InitTaskAdditionalDataInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *mixaicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  mixaicaivctx->set_op_index(0);
  mixaicaivctx->set_context_id(0);
  mixaicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_MIX_AIC));
  domi::FftsPlusMixAicAivCtxDef *mixctxdef = mixaicaivctx->mutable_mix_aic_aiv_ctx();

  rtFftsPlusMixAicAivCtx_t ctx;
  uint32_t start_idx = 0;

  // Test: ctx_def.kernel_name_size() == 0
  EXPECT_EQ(ffpt.InitManualMixAicAivCtx(*mixctxdef, ctx, start_idx), FAILED);

  // Test: ctx_def.kernel_name_size() != kAutoMixAicAivCtxPcNum
  mixctxdef->add_kernel_name("aictest");
  EXPECT_EQ(ffpt.InitManualMixAicAivCtx(*mixctxdef, ctx, start_idx), FAILED);

  mixctxdef->clear_kernel_name();
  InitMixAicAivCtx(mixctxdef, false);
  EXPECT_EQ(ffpt.InitManualMixAicAivCtx(*mixctxdef, ctx, start_idx), SUCCESS);
}

// fusion_start_task_info.cc
TEST_F(UtestScatteredCollection, FusionStartTaskInfo_Invalid) {
  FusionStartTaskInfo fstart;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  auto davinci_model = new DavinciModel(0, nullptr);

  auto ret = fstart.Init(task_def, davinci_model);
  EXPECT_NE(ret, SUCCESS);

  ret = fstart.Distribute();
  EXPECT_EQ(ret, SUCCESS);
}

// fusion_stop_task_info.cc
TEST_F(UtestScatteredCollection, FusionStopTaskInfo_Invalid) {
  FusionStopTaskInfo fstop;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  auto davinci_model = new DavinciModel(0, nullptr);

  auto ret = fstop.Init(task_def, davinci_model);
  EXPECT_NE(ret, SUCCESS);

  ret = fstop.Distribute();
  EXPECT_EQ(ret, SUCCESS);
}

// label_switch_by_index_task_info.cc
TEST_F(UtestScatteredCollection, LabelSwitchByIndexTaskInfo_test) {
  OpDescPtr op_desc = CreateOpDesc("label_switch", LABELSWITCHBYINDEX);
  op_desc->SetId(0);

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  domi::LabelSwitchByIndexDef *label_task_def = task_def.mutable_label_switch_by_index();
  label_task_def->set_op_index(op_desc->GetId());
  label_task_def->set_label_max(2);

  LabelSwitchByIndexTaskInfo task_info;
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_size = 0x40000;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_.push_back(stream);

  // fail for OpDesc not found.
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  // fail for input num
  model.op_list_[op_desc->GetId()] = op_desc;
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);
  EXPECT_EQ(task_info.CalculateArgs(task_def, &model), FAILED);

  // fail for LABEL_SWITCH_LIST
  GeTensorDesc desc;
  op_desc->AddInputDesc(desc);
  op_desc->SetInputOffset({1024});
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  AttrUtils::SetListInt(op_desc, ATTR_NAME_LABEL_SWITCH_LIST, {});
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  AttrUtils::SetListInt(op_desc, ATTR_NAME_LABEL_SWITCH_LIST, {0, 1});
  EXPECT_EQ(task_info.Init(task_def, &model), INTERNAL_ERROR);

  // success for CalculateArgs
  EXPECT_EQ(task_info.CalculateArgs(task_def, &model), SUCCESS);
}

// model_exit_task_info.cc
TEST_F(UtestScatteredCollection, testModelExitTaskInfo) {
  ModelExitTaskInfo meti;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  auto davinci_model = new DavinciModel(0, nullptr);

  auto ret = meti.Init(task_def, davinci_model);
  EXPECT_EQ(ret, FAILED);

  ret = meti.Distribute();
  EXPECT_EQ(ret, SUCCESS);
}

// stream_active_task_info.cc
TEST_F(UtestScatteredCollection, testStreamActiveTaskInfo) {
  DavinciModel model(0, nullptr);

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_.push_back(stream);
  model.op_list_[0] = CreateOpDesc("data", DATA);

  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  StreamActiveTaskInfo sati;
  auto ret = sati.Init(task_def, &model);
  EXPECT_EQ(ret, INTERNAL_ERROR);

  ret = sati.Distribute();
  EXPECT_EQ(ret, SUCCESS);
}
} // end of namespace ge