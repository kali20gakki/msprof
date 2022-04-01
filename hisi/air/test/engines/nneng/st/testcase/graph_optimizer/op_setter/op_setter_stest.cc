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
#include <memory>
#include "common/util/op_info_util.h"

#define private public
#define protected public
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph_optimizer/op_setter/op_setter.h"
#include "graph/debug/ge_attr_define.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

using namespace std;
using namespace ge;
using namespace fe;
using OpSetterPtr = std::shared_ptr<OpSetter>;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
class STEST_OP_SETTER : public testing::Test
{
protected:
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  void SetUp()
  {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
    FEOpsStoreInfo tbe_custom {
            6,
            "tbe-custom",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_slice_op_info/slice_success",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
  }

  void TearDown() {}
  static void CreateOneOpGraph(ComputeGraphPtr graph) {
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
    ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    // add descriptor
    vector<int64_t> dim(4, 1);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    out_desc.SetOriginFormat(FORMAT_NCHW);
    out_desc.SetFormat(FORMAT_NCHW);
    out_desc.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", out_desc);
    relu_op->AddOutputDesc("y", out_desc);

    NodePtr relu_node = graph->AddNode(relu_op);
  }

};

TEST_F(STEST_OP_SETTER, set_op_info_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateOneOpGraph(graph);
  OpSetterPtr op_setter_ptr = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret2 = op_setter_ptr->SetOpInfo(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, ret2);
  string op_slice_info_str;
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();

	op_slice_info_str.clear();
	(void)ge::AttrUtils::GetStr(op_desc_ptr, OP_SLICE_INFO, op_slice_info_str);
	//EXPECT_EQ(op_slice_info_str.empty(), false);

    int imply_type = -1;
    if(!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, imply_type)) {
      continue;
    }
    bool is_continous_input;
    bool is_continous_output;
    ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_CONTINUOUS_INPUT, is_continous_input);
    ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_CONTINUOUS_OUTPUT,is_continous_output);
    if (op_type == "Activation"){
      EXPECT_EQ(is_continous_input, true);
      EXPECT_EQ(is_continous_output, true);
    } else {
      ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_CONTINUOUS_INPUT, is_continous_input);
      ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_CONTINUOUS_OUTPUT,is_continous_output);
      EXPECT_EQ(is_continous_input, false);
      EXPECT_EQ(is_continous_output, false);
    }
  }
}

