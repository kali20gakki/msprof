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
#include <vector>

#include "runtime/rt.h"

#define protected public
#define private public
#include "hybrid/model/graph_item.h"
#include "hybrid/model/node_item.h"
#include "single_op/single_op.h"
#include "single_op/single_op_manager.h"
#include "single_op/task/build_task_utils.h"
#include "single_op/task/build_task_utils.h"
#include "single_op/task/tbe_task_builder.h"
#include "common/dump/dump_manager.h"
#include "common/tbe_handle_store/tbe_handle_store.h"
#undef private
#undef protected

using namespace std;
using namespace ge;
using namespace hybrid;

class UtestTbeTaskBuilder : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
  ObjectPool<GeTensor> tensor_pool_;
};

TEST_F(UtestTbeTaskBuilder, test_KernelHolder_construct) {
  std::shared_ptr<ge::OpKernelBin> kernel_bin = std::make_shared<ge::OpKernelBin>("bin_name", std::vector<char>());
  void *stub_func = ValueToPtr(1234U);
  KernelHolder holder((const char_t*)stub_func, kernel_bin);
  holder.bin_handle_ = malloc(1);
  EXPECT_EQ(holder.stub_func_, stub_func);
}

TEST_F(UtestTbeTaskBuilder, test_KernelBinRegistry_GetUnique) {
  std::string test = "test";
  const char *out1 = KernelBinRegistry::GetInstance().GetUnique(test.c_str());
  const char *out2 = KernelBinRegistry::GetInstance().GetUnique(test.c_str());
  EXPECT_EQ(out1, out2);
}

TEST_F(UtestTbeTaskBuilder, test_KernelBinRegistry_GetStubFunc) {
  EXPECT_EQ(KernelBinRegistry::GetInstance().GetStubFunc("test"), nullptr);
  std::shared_ptr<ge::OpKernelBin> kernel_bin = std::make_shared<ge::OpKernelBin>("bin_name", std::vector<char>());
  void *stub_func = ValueToPtr(1234U);
  KernelBinRegistry::GetInstance().AddKernel("test",
      std::unique_ptr<KernelHolder>(new KernelHolder((const char_t*)stub_func, kernel_bin)));
  EXPECT_EQ(KernelBinRegistry::GetInstance().GetStubFunc("test"), stub_func);
}

TEST_F(UtestTbeTaskBuilder, test_TbeTaskBuilder) {
  std::string model_name = "test_model";

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  ge::NodePtr node = graph->AddNode(op_desc);

  domi::TaskDef task_def;
  task_def.mutable_kernel()->set_args_size(sizeof(void *));
  TbeTaskBuilder builder(model_name, node, task_def);

  (void)AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", "test_kernel_name");
  (void)AttrUtils::SetStr(op_desc, op_desc->GetName() + "_atomic_kernelname", "test_kernel_name");
  std::string out;
  builder.GetKernelName(op_desc, out);
  EXPECT_EQ(out, "test_kernel_name");

  AttrUtils::SetStr(op_desc, builder.GetKeyForTvmMetaData(), "meta_data_test");

  EXPECT_EQ(builder.DoRegisterMeta(reinterpret_cast<void *>(98)), SUCCESS);
  EXPECT_EQ(builder.DoRegisterMeta(reinterpret_cast<void *>(99)), FAILED);

  std::string test = "test";
  EXPECT_EQ(builder.DoRegisterFunction(reinterpret_cast<void *>(98), test.c_str(), test.c_str()), SUCCESS);
  EXPECT_NE(builder.DoRegisterFunction(reinterpret_cast<void *>(99), test.c_str(), test.c_str()), SUCCESS);

  std::shared_ptr<ge::OpKernelBin> kernel_bin = std::make_shared<ge::OpKernelBin>("bin_name", std::vector<char>());
  void *bin_handle = nullptr;
  SingleOpModelParam param;
  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  EXPECT_EQ(builder.DoRegisterKernel(*kernel_bin, test.c_str(), &bin_handle, param), SUCCESS);

  TbeOpTask task;
  builder.stub_name_ = "test1";
  EXPECT_EQ(builder.DoRegisterFunction(reinterpret_cast<void *>(98), test.c_str(), test.c_str()), SUCCESS);
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_TBE_KERNEL, kernel_bin);
  op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, kernel_bin);
  EXPECT_EQ(builder.RegisterKernel(task, param), SUCCESS);

  EXPECT_EQ(builder.RegisterKernelWithHandle(param), SUCCESS);

  EXPECT_EQ(builder.GetKeyForOpParamSize(), "op_para_size");
  EXPECT_EQ(builder.GetKeyForTvmMetaData(), "tvm_metadata");
  EXPECT_EQ(builder.InitKernelArgs(reinterpret_cast<void *>(98), 10, param), SUCCESS);

  uint32_t magic;
  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  EXPECT_EQ(builder.GetMagic(magic), SUCCESS);
  EXPECT_EQ(magic, RT_DEV_BINARY_MAGIC_ELF_AIVEC);

  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AICUBE");
  EXPECT_EQ(builder.GetMagic(magic), SUCCESS);
  EXPECT_EQ(magic, RT_DEV_BINARY_MAGIC_ELF_AICUBE);

  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "invalid");
  EXPECT_EQ(builder.GetMagic(magic), PARAM_INVALID);

  (void)AttrUtils::SetInt(op_desc, builder.GetKeyForOpParamSize(), -1);
  EXPECT_EQ(builder.InitTilingInfo(task), ACL_ERROR_GE_PARAM_INVALID);

  EXPECT_NE(builder.BuildTask(task, param), SUCCESS);  // ??

  task.need_tiling_ = true;
  task.args_ = std::unique_ptr<uint8_t[]>(new uint8_t[24]());
  task.arg_size_ = 24;
  task.max_tiling_size_ = 0;
  param.graph_is_dynamic = true;
  int64_t buffer = 4;
  task.overflow_addr_ =  &buffer;

  EXPECT_NE(builder.SetKernelArgs(task, param, op_desc), SUCCESS);  // ??

  (void)AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", "ZZZZ");     // ??
  EXPECT_EQ(builder.RegisterKernelWithHandle(param), ACL_ERROR_GE_INTERNAL_ERROR);
}

TEST_F(UtestTbeTaskBuilder, test_updatetilingargs) {
  std::string model_name = "test_model";

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  ge::NodePtr node = graph->AddNode(op_desc);

  TbeOpTask task;
  domi::TaskDef task_def;
  task_def.mutable_kernel()->set_args_size(sizeof(void *));
  TbeTaskBuilder builder(model_name, node, task_def);
  task.need_tiling_ = true;
  size_t kernel_def_size = 256U;
  size_t index1 = 1;
  size_t index2 = 2;
  EXPECT_EQ(builder.UpdateTilingArgs(task, index1, kernel_def_size), SUCCESS);
  EXPECT_EQ(builder.UpdateTilingArgs(task, index2, kernel_def_size), SUCCESS);
}

TEST_F(UtestTbeTaskBuilder, test_AtomicAddrCleanTaskBuilder) {
  std::string model_name = "test_model";

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  ge::NodePtr node = graph->AddNode(op_desc);

  domi::TaskDef task_def;
  task_def.mutable_kernel()->set_args_size(sizeof(void *));
  AtomicAddrCleanTaskBuilder builder(model_name, node, task_def);

  SingleOpModelParam param;
  EXPECT_EQ(builder.InitKernelArgs(nullptr, 10, param), SUCCESS);
  EXPECT_EQ(builder.GetKeyForOpParamSize(), "atomic_op_para_size");
  EXPECT_EQ(builder.GetKeyForTvmMetaData(), "_atomic_tvm_metadata");

  EXPECT_EQ(builder.GetKeyForTvmMetaData(), "_atomic_tvm_metadata");

  (void)AttrUtils::SetStr(op_desc, op_desc->GetName() + "_atomic_kernelname", "test_kernel_name");
  std::string out;
  builder.GetKernelName(op_desc, out);
  EXPECT_EQ(out, "test_kernel_name");

  std::shared_ptr<ge::OpKernelBin> kernel_bin = std::make_shared<ge::OpKernelBin>("bin_name", std::vector<char>());
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_TBE_KERNEL, kernel_bin);
  EXPECT_EQ(builder.GetTbeKernel(op_desc)->GetName(), kernel_bin->GetName());
}
