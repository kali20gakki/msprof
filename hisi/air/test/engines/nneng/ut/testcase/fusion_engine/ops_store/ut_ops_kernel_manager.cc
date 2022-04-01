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
#include <iostream>

#include <list>
#define protected public
#define private public
#include "ops_store/ops_kernel_manager.h"
#include "common/configuration.h"
#include "common/aicore_util_constants.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class ops_kernel_manager_unit_test : public testing::Test
{
protected:
    void SetUp()
    {
      FEOpsStoreInfo tbe_custom {
              2,
              "tbe-custom",
              EN_IMPL_CUSTOM_TBE,
              "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
              "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
              false,
              false};

      vector<FEOpsStoreInfo> store_info;
      store_info.emplace_back(tbe_custom);
      Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    }

    void TearDown()
    {

    }

// AUTO GEN PLEASE DO NOT MODIFY IT
};

TEST_F(ops_kernel_manager_unit_test, initialize_success)
{
  OpsKernelManager opsKernelManager(AI_CORE_NAME);
  Status ret = opsKernelManager.Initialize();
  EXPECT_EQ(ret, fe::SUCCESS);

  EXPECT_EQ(opsKernelManager.sub_ops_store_map_.size(), 1);
  EXPECT_EQ(opsKernelManager.sub_ops_kernel_map_.size(), 1);

  EXPECT_NE(opsKernelManager.GetSubOpsKernelByStoreName("tbe-custom"), nullptr);
  EXPECT_EQ(opsKernelManager.GetSubOpsKernelByStoreName("cce-custom"), nullptr);

  EXPECT_NE(opsKernelManager.GetSubOpsKernelByImplType(EN_IMPL_CUSTOM_TBE), nullptr);
  EXPECT_EQ(opsKernelManager.GetSubOpsKernelByImplType(EN_IMPL_HW_TBE), nullptr);

  EXPECT_NE(opsKernelManager.GetOpKernelInfoByOpType("tbe-custom", "conv2"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType("tbe-custom", "conv123"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType("cce-custom", "conv2"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType("cce-custom", "conv123"), nullptr);

  EXPECT_NE(opsKernelManager.GetOpKernelInfoByOpType(EN_IMPL_CUSTOM_TBE, "conv2"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType(EN_IMPL_CUSTOM_TBE, "conv123"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType(EN_IMPL_HW_TBE, "conv2"), nullptr);
  EXPECT_EQ(opsKernelManager.GetOpKernelInfoByOpType(EN_IMPL_HW_TBE, "conv123"), nullptr);

  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetOriginDataType(DT_FLOAT);
  ge::OpDescPtr op_desc_ptr = std::make_shared<ge::OpDesc>("test_op_desc", "conv2");
  op_desc_ptr->AddInputDesc(tensor_desc);
  op_desc_ptr->AddOutputDesc(tensor_desc);

  ge::AttrUtils::SetInt(op_desc_ptr, "_fe_imply_type", 2);
  OpKernelInfoPtr opKernelInfoPtr = opsKernelManager.GetOpKernelInfoByOpDesc(op_desc_ptr);
  bool flag = opKernelInfoPtr == nullptr;
  EXPECT_EQ(flag, false);

  op_desc_ptr->SetType("conv345");
  opKernelInfoPtr = opsKernelManager.GetOpKernelInfoByOpDesc(op_desc_ptr);
  flag = opKernelInfoPtr == nullptr;
  EXPECT_EQ(flag, true);

  ge::AttrUtils::SetInt(op_desc_ptr, "_fe_imply_type", 4);
  opKernelInfoPtr = opsKernelManager.GetOpKernelInfoByOpDesc(op_desc_ptr);
  flag = opKernelInfoPtr == nullptr;
  EXPECT_EQ(flag, true);

  op_desc_ptr->SetType("conv2");
  opKernelInfoPtr = opsKernelManager.GetOpKernelInfoByOpDesc(op_desc_ptr);
  flag = opKernelInfoPtr == nullptr;
  EXPECT_EQ(flag, true);
}
