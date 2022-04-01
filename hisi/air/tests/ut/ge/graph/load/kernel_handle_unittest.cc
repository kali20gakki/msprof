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
#include <gmock/gmock.h>

#include "framework/common/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"

#define private public
#define protected public
#include "graph/load/model_manager/tbe_kernel_handle.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;

namespace ge {
class UtestTBEKernelHandle : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};


// test InitTbeHandleWithFfts
TEST_F(UtestTBEKernelHandle, init_tbe_handle_with_ffts) {
  TBEKernelHandle kernel_handle;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  // without tbe_kernel
  EXPECT_EQ(kernel_handle.RegisterHandle(op_desc), INTERNAL_ERROR);

  std::vector<OpKernelBinPtr> tbe_kernel;
  vector<char> buffer;
  string key = op_desc->GetName();
  OpKernelBinPtr tbe_kernel_ptr0 = std::make_shared<ge::OpKernelBin>(key, std::move(buffer));
  OpKernelBinPtr tbe_kernel_ptr1 = std::make_shared<ge::OpKernelBin>(key, std::move(buffer));
  tbe_kernel.push_back(tbe_kernel_ptr0);
  tbe_kernel.push_back(tbe_kernel_ptr1);
  op_desc->SetExtAttr(OP_EXTATTR_NAME_THREAD_TBE_KERNEL, tbe_kernel);
  // without _register_stub_func
  EXPECT_EQ(kernel_handle.RegisterHandle(op_desc), INTERNAL_ERROR);

  vector<string> bin_file_keys;
  bin_file_keys.emplace_back(op_desc->GetName() + "_0");
  bin_file_keys.emplace_back(op_desc->GetName() + "_1");
  AttrUtils::SetListStr(op_desc, "_register_stub_func", bin_file_keys);

  EXPECT_EQ(kernel_handle.RegisterHandle(op_desc), SUCCESS);
  // rtQueryFunctionRegistered(bin_file_key) failed
  EXPECT_EQ(kernel_handle.used_tbe_handle_map_.size(), 0);
}

// test InitBinaryMagic
TEST_F(UtestTBEKernelHandle, init_binary_magic) {
  TBEKernelHandle kernel_handle;
  rtDevBinary_t binary;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  vector<string> json_list;
  AttrUtils::SetListStr(op_desc, TVM_ATTR_NAME_THREAD_MAGIC, json_list);
  // without tvm_magic
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 0, binary, ""), INTERNAL_ERROR);
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF_AICPU");
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF");
  op_desc->DelAttr(TVM_ATTR_NAME_MAGIC);
  AttrUtils::SetListStr(op_desc, TVM_ATTR_NAME_MAGIC, json_list);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 0, binary, ""), SUCCESS);
  EXPECT_EQ(binary.magic, RT_DEV_BINARY_MAGIC_ELF_AICPU);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 1, binary, ""), SUCCESS);
  EXPECT_EQ(binary.magic, RT_DEV_BINARY_MAGIC_ELF);

  json_list.clear();
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF_AICUBE");
  op_desc->DelAttr(TVM_ATTR_NAME_MAGIC);
  AttrUtils::SetListStr(op_desc, TVM_ATTR_NAME_MAGIC, json_list);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 0, binary, ""), SUCCESS);
  EXPECT_EQ(binary.magic, RT_DEV_BINARY_MAGIC_ELF_AIVEC);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 1, binary, ""), SUCCESS);
  EXPECT_EQ(binary.magic, RT_DEV_BINARY_MAGIC_ELF_AICUBE);

  // with invalid json type
  json_list.clear();
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF_INVALID");
  json_list.emplace_back("RT_DEV_BINARY_MAGIC_ELF_INVALID");
  op_desc->DelAttr(TVM_ATTR_NAME_MAGIC);
  AttrUtils::SetListStr(op_desc, TVM_ATTR_NAME_MAGIC, json_list);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, 0, binary, ""), PARAM_INVALID);

  // test unffts
  string json_string = "RT_DEV_BINARY_MAGIC_ELF_AIVEC";
  op_desc->DelAttr(TVM_ATTR_NAME_MAGIC);
  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, json_string);
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, UINT32_MAX, binary, ""), SUCCESS);
  EXPECT_EQ(binary.magic, RT_DEV_BINARY_MAGIC_ELF_AIVEC);
}

// test InitMetaData
TEST_F(UtestTBEKernelHandle, init_meta_data) {
  TBEKernelHandle kernel_handle;
  void *bin_handle = nullptr;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  vector<string> meta_data_list;
  // with empty meta_data
  EXPECT_EQ(kernel_handle.InitMetaData(op_desc, 0, bin_handle, ""), INTERNAL_ERROR);
  meta_data_list.emplace_back("meta_data_0");
  meta_data_list.emplace_back("meta_data_1");
  AttrUtils::SetListStr(op_desc, TVM_ATTR_NAME_METADATA, meta_data_list);
  EXPECT_EQ(kernel_handle.InitMetaData(op_desc, 0, bin_handle, ""), SUCCESS);

  string meta_data = "meta_data";
  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_METADATA, meta_data);
  EXPECT_EQ(kernel_handle.InitMetaData(op_desc, UINT32_MAX, bin_handle, ""), SUCCESS);
}

// test InitKernelName
TEST_F(UtestTBEKernelHandle, init_kernel_name) {
  TBEKernelHandle kernel_handle;
  string kernel_name;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  // failed when name is invalid
  EXPECT_EQ(kernel_handle.InitKernelName(op_desc, 0, kernel_name, ""), INTERNAL_ERROR);
  OpDescPtr op_desc1 = CreateOpDesc("sgt_graph_nodes/loss_scale", SCALE);
  string attr_kernel_name = "_thread_kernelname";
  vector<string> kernel_name_list;
  AttrUtils::SetListStr(op_desc, attr_kernel_name, kernel_name_list);
  // failed without kernel_name
  EXPECT_EQ(kernel_handle.InitKernelName(op_desc, 0, kernel_name, ""), INTERNAL_ERROR);
  kernel_name_list.emplace_back("kernel_name_0");
  kernel_name_list.emplace_back("kernel_name_1");
  AttrUtils::SetListStr(op_desc1, attr_kernel_name, kernel_name_list);
  EXPECT_EQ(kernel_handle.InitKernelName(op_desc1, 0, kernel_name, ""), SUCCESS);

  // without ffts
  auto pos = op_desc->GetName().find("/");
  attr_kernel_name = op_desc->GetName().substr(pos + 1) + "_thread_kernelname";
  kernel_name = "kernel_name";
  AttrUtils::SetStr(op_desc, attr_kernel_name, kernel_name);
  EXPECT_EQ(kernel_handle.InitKernelName(op_desc, UINT32_MAX, kernel_name, ""), SUCCESS);
}

TEST_F(UtestTBEKernelHandle, init_kernel_name_prefix) {
  TBEKernelHandle kernel_handle;
  string kernel_name;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  // failed when name is invalid
  EXPECT_EQ(kernel_handle.FunctionRegister(op_desc, "test", TBEKernelPtr(), "_mix_aic", UINT32_MAX), SUCCESS);

  rtDevBinary_t binary;
  // without tvm_magic
  EXPECT_EQ(kernel_handle.InitBinaryMagic(op_desc, UINT32_MAX, binary, "_mix_aic"), SUCCESS);
}

TEST_F(UtestTBEKernelHandle, KernelRegister_Invalid) {
  TBEKernelHandle kernel_handle;
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  uint32_t thread_index = 0;
  char_t *const bin_file_key = "file key";
  std::string attr_prefix = "attr prefix";

  vector<char> buffer;
  string key = op_desc->GetName();
  OpKernelBinPtr tbe_kernel = std::make_shared<ge::OpKernelBin>(key, std::move(buffer));

  auto ret = kernel_handle.KernelRegister(op_desc, thread_index, bin_file_key, attr_prefix, tbe_kernel);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

}  // namespace ge
