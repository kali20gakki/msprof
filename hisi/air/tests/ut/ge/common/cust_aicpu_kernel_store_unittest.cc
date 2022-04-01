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
#define protected public
#define private public
#include "common/cust_aicpu_kernel_store.h"
#include"common/kernel_store.h"
#include "graph/op_kernel_bin.h"
#undef private
#undef protected

namespace ge {
class UtestCustAicpuKernelStore : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestCustAicpuKernelStore, AddCustAICPUKernel_success) {
  CustAICPUKernelStore ai_cpu_kernel_store;
  //CustAICPUKernelPtr cust_aicpu_kernel;
  const char data[] = "1234";
  vector<char> buffer(data, data + strlen(data));
  ge::OpKernelBinPtr cust_aicpu_kernel = std::make_shared<ge::OpKernelBin>(
        "test", std::move(buffer));
  ai_cpu_kernel_store.AddCustAICPUKernel(cust_aicpu_kernel);
  EXPECT_NE(ai_cpu_kernel_store.FindKernel("test"), nullptr);
}

TEST_F(UtestCustAicpuKernelStore, LoadCustAICPUKernelBinToOpDesc) {
  auto opdesc = std::make_shared<OpDesc>("test", "op");
  CustAICPUKernelStore *ai_cpu_kernel_store = new CustAICPUKernelStore();
  const char data[] = "1234";
  vector<char> buffer(data, data + strlen(data));
  ge::OpKernelBinPtr cust_aicpu_kernel = std::make_shared<ge::OpKernelBin>(
        "test", std::move(buffer));
  //opdesc->SetExtAttr("test", cust_aicpu_kernel);
  ai_cpu_kernel_store->AddCustAICPUKernel(cust_aicpu_kernel);
  ai_cpu_kernel_store->LoadCustAICPUKernelBinToOpDesc(opdesc);
  const ge::OpKernelBinPtr BinPtr =opdesc->TryGetExtAttr<ge::OpKernelBinPtr>(ge::OP_EXTATTR_CUSTAICPU_KERNEL, nullptr);
  EXPECT_EQ(BinPtr, cust_aicpu_kernel);
  delete ai_cpu_kernel_store;
  ai_cpu_kernel_store = nullptr;
}

}