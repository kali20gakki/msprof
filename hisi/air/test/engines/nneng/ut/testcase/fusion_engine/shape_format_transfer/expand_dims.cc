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


#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"

#define protected public
#define private   public
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_cast_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reshape_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transpose_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
#undef protected
#undef private

#include <iostream>

using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

class UTEST_EXPAND_DIMS : public testing::Test {
 protected:
  void SetUp()
  {
    std::map<std::string, std::string> options;
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo tbe_custom {
        2,
        "tbe-custom",
        EN_IMPL_CUSTOM_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
        ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
    Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  }

  void TearDown()
  {
    fe_ops_kernel_info_store_ptr_->Finalize();

  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
 protected:

};


TEST_F(UTEST_EXPAND_DIMS, ExpandDims_01_Nchw) {
  std::vector<int64_t> dim_ori = {23};
  std::string op_type = "";
  ge::Format original_format = ge::FORMAT_NCHW;
  ge::Format final_format = ge::FORMAT_NC1HWC0;
  uint32_t tensor_index = 0;
  string reshape_type = "N";
  std::vector<int64_t> dim_final = {23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "C";
  dim_final = {1,23,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "H";
  dim_final = {1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "W";
  dim_final = {1,1,1,23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NC";
  dim_final = {23,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "NC";
  dim_final = {23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NH";
  dim_final = {23,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NW";
  dim_final = {23,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CH";
  dim_final = {1,23,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CW";
  dim_final = {1,23,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HW";
  dim_final = {1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCH";
  dim_final = {23,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 66};
  reshape_type = "NCH";
  dim_final = {23,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCW";
  dim_final = {23,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHW";
  dim_final = {23,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "CHW";
  dim_final = {1,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);
}


TEST_F(UTEST_EXPAND_DIMS, ExpandDims_02_Nhwc) {
  std::vector<int64_t> dim_ori = {23};
  std::string op_type = "";
  ge::Format original_format = ge::FORMAT_NHWC;
  ge::Format final_format = ge::FORMAT_NC1HWC0;
  uint32_t tensor_index = 0;
  string reshape_type = "N";
  std::vector<int64_t> dim_final = {23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "C";
  dim_final = {1,1,1,23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "H";
  dim_final = {1,23,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "W";
  dim_final = {1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NH";
  dim_final = {23,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "NH";
  dim_final = {23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NW";
  dim_final = {23,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NC";
  dim_final = {23,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HW";
  dim_final = {1,23,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HC";
  dim_final = {1,23,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "WC";
  dim_final = {1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHW";
  dim_final = {23,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 66};
  reshape_type = "NHW";
  dim_final = {23,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHC";
  dim_final = {23,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NWC";
  dim_final = {23,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "HWC";
  dim_final = {1,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {33, 23, 55, 66};
  reshape_type = "WWE";
  dim_final = {33,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {33, 23, 55};
  reshape_type = "NH";
  dim_final = {33,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);
}

TEST_F(UTEST_EXPAND_DIMS, ExpandDims_03_Hwcn) {
  std::vector<int64_t> dim_ori = {23};
  std::string op_type = "";
  ge::Format original_format = ge::FORMAT_HWCN;
  ge::Format final_format = ge::FORMAT_NC1HWC0;
  uint32_t tensor_index = 0;
  string reshape_type = "H";
  std::vector<int64_t> dim_final = {23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "W";
  dim_final = {1,23,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "C";
  dim_final = {1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "N";
  dim_final = {1,1,1,23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "";
  dim_final = {1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "";
  dim_final = {1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HC";
  dim_final = {23,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HN";
  dim_final = {23,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "WC";
  dim_final = {1,23,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "WN";
  dim_final = {1,23,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CN";
  dim_final = {1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "HWC";
  dim_final = {23,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 66};
  reshape_type = "HWC";
  dim_final = {23,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "HWN";
  dim_final = {23,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "HCN";
  dim_final = {23,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "WCN";
  dim_final = {1,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  original_format = ge::FORMAT_ND;
  reshape_type = "CN";
  dim_final = {23,55,66};
  /* ori dimension is larger than the length of CN. We
   * will not expand dimension. */
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  /* we can expand dims only if the reshape_type is CN when the original format is ND */
  dim_ori = {23, 55, 66};
  reshape_type = "WCN";
  dim_final = {23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  /* we can expand dims only if the reshape_type is CN when the original format is ND */
  dim_ori = {23};
  reshape_type = "W";
  dim_final = {23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);
}


TEST_F(UTEST_EXPAND_DIMS, ExpandDims_04_Ndhwc) {
  std::vector<int64_t> dim_ori = {23};
  std::string op_type = "";
  ge::Format original_format = ge::FORMAT_NDHWC;
  ge::Format final_format = ge::FORMAT_NDC1HWC0;
  uint32_t tensor_index = 0;
  string reshape_type = "N";
  std::vector<int64_t> dim_final = {23,1,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "D";
  dim_final = {1,23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "H";
  dim_final = {1,1,23,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "W";
  dim_final = {1,1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "C";
  dim_final = {1,1,1,1,23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "ND";
  dim_final = {23,55,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "ND";
  dim_final = {23,1,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NH";
  dim_final = {23,1,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NW";
  dim_final = {23,1,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NC";
  dim_final = {23,1,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "DH";
  dim_final = {1,23,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "DW";
  dim_final = {1,23,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "DC";
  dim_final = {1,23,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HW";
  dim_final = {1,1,23,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 66};
  reshape_type = "HC";
  dim_final = {1,1,23,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "WC";
  dim_final = {1,1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDH";
  dim_final = {23,55,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDW";
  dim_final = {23,55,1,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDC";
  dim_final = {23,55,1,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHW";
  dim_final = {23,1,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHC";
  dim_final = {23,1,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NWC";
  dim_final = {23,1,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "DHW";
  dim_final = {1,23,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "DHC";
  dim_final = {1,23,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "DWC";
  dim_final = {1,23,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "HWC";
  dim_final = {1,1,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDHW";
  dim_final = {23,55,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NDHW";
  dim_final = {23,55,66, 87, 1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NDHC";
  dim_final = {23,55,66,1,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NDWC";
  dim_final = {23,55,1,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NHWC";
  dim_final = {23,1,55,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "DHWC";
  dim_final = {1,23,55,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);


  dim_ori = {23, 55, 66, 87};
  reshape_type = "DWC";
  dim_final = {23, 55, 66, 87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);
}


TEST_F(UTEST_EXPAND_DIMS, ExpandDims_05_Ncdhw) {
  std::vector<int64_t> dim_ori = {23};
  std::string op_type = "";
  ge::Format original_format = ge::FORMAT_NCDHW;
  ge::Format final_format = ge::FORMAT_NDC1HWC0;
  uint32_t tensor_index = 0;
  string reshape_type = "N";
  std::vector<int64_t> dim_final = {23,1,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "C";
  dim_final = {1,23,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "D";
  dim_final = {1,1,23,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "H";
  dim_final = {1,1,1,23,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "W";
  dim_final = {1,1,1,1,23};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NC";
  dim_final = {23,55,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23};
  reshape_type = "NC";
  dim_final = {23,1,1,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "ND";
  dim_final = {23,1,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NH";
  dim_final = {23,1,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "NW";
  dim_final = {23,1,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CD";
  dim_final = {1,23,55,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CH";
  dim_final = {1,23,1,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "CW";
  dim_final = {1,23,1,1,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "DH";
  dim_final = {1,1,23,55,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 66};
  reshape_type = "DW";
  dim_final = {1,1,23,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55};
  reshape_type = "HW";
  dim_final = {1,1,1,23,55};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCD";
  dim_final = {23,55,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCH";
  dim_final = {23,55,1,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCW";
  dim_final = {23,55,1,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDH";
  dim_final = {23,1,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NDW";
  dim_final = {23,1,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NHW";
  dim_final = {23,1,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "CDH";
  dim_final = {1,23,55,66,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "CDW";
  dim_final = {1,23,55,1,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "CHW";
  dim_final = {1,23,1,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "DHW";
  dim_final = {1,1,23,55,66};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66};
  reshape_type = "NCDH";
  dim_final = {23,55,66,1,1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NCDH";
  dim_final = {23,55,66, 87, 1};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NCDW";
  dim_final = {23,55,66,1,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NCHW";
  dim_final = {23,55,1,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "NDHW";
  dim_final = {23,1,55,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);

  dim_ori = {23, 55, 66, 87};
  reshape_type = "CDHW";
  dim_final = {1,23,55,66,87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);


  dim_ori = {23, 55, 66, 87};
  reshape_type = "NCD";
  dim_final = {23, 55, 66, 87};
  ExpandDimension(dim_ori, op_type, original_format, final_format, tensor_index, reshape_type);
  EXPECT_EQ(dim_ori, dim_final);
}


TEST_F(UTEST_EXPAND_DIMS, GetNzShapeFrom1DNd_01) {
  ge::GeShape new_shape;
  int64_t impl_type = 6;
  vector<int64_t> axis_value = {55,1,1,1,1,16,1,1};
  vector<int64_t> nd_value = {55};
  ShapeTransferAccordingToFormat::GetNzShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);
  vector<int64_t> nz_dim = {1, 4, 16, 16};
  EXPECT_EQ(new_shape.GetDims(), nz_dim);
}


TEST_F(UTEST_EXPAND_DIMS, GetNzRangeFrom1DNd_02) {
  vector<std::pair<int64_t, int64_t>> new_range;
  vector<std::pair<int64_t, int64_t>> range_value =
      {{55, 55}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {16, 16}, {1, 1}, {1, 1}};
  vector<std::pair<int64_t, int64_t>> nd_range_value = {{55, 55}};
  int64_t impl_type = 6;
  RangeTransferAccordingToFormat::GetNzRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);
  vector<std::pair<int64_t, int64_t>> nz_range = {{1, 1}, {4, 4}, {16, 16}, {16, 16}};
  EXPECT_EQ(new_range, nz_range);

}
