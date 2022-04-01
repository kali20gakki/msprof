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

#include "gtest/gtest.h"

#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/buffer.h"
#include "../../fe_test_utils.h"
#include "securec.h"
#define protected public
#define private public
#include "common/fe_log.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "adapter/adapter_itf/op_store_adapter.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "runtime/kernel.h"
#include "common/configuration.h"
using namespace std;
using namespace testing;
using namespace ge;
using namespace fe;

using OpStoreAdapterManagerPtr = std::shared_ptr<fe::OpStoreAdapterManager>;

FEOpsStoreInfo cce_custom_opinfo_adapter {
      0,
      "cce-custom",
      EN_IMPL_CUSTOM_CONSTANT_CCE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo",
      ""
};
FEOpsStoreInfo tik_custom_opinfo_adapter  {
      1,
      "tik-custom",
      EN_IMPL_CUSTOM_TIK,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tik_custom_opinfo",
      ""
};
FEOpsStoreInfo tbe_custom_opinfo_adapter  {
      2,
      "tbe-custom",
      EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      ""
};
FEOpsStoreInfo cce_constant_opinfo_adapter  {
      3,
      "cce-constant",
      EN_IMPL_HW_CONSTANT_CCE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_constant_opinfo",
      ""
};
FEOpsStoreInfo cce_general_opinfo_adapter  {
      4,
      "cce-general",
      EN_IMPL_HW_GENERAL_CCE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_general_opinfo",
      ""
};
FEOpsStoreInfo tik_opinfo_adapter  {
      5,
      "tik-builtin",
      EN_IMPL_HW_TIK,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tik_opinfo",
      ""
};
FEOpsStoreInfo tbe_opinfo_adapter  {
      6,
      "tbe-builtin",
      EN_IMPL_HW_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
      ""
};
FEOpsStoreInfo rl_opinfo_adapter  {
      7,
      "rl-builtin",
      EN_IMPL_RL,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/rl_opinfo",
      ""
};


std::vector<FEOpsStoreInfo> all_fe_ops_store_info_adapter{
      cce_custom_opinfo_adapter ,
      tik_custom_opinfo_adapter ,
      tbe_custom_opinfo_adapter ,
      cce_constant_opinfo_adapter ,
      cce_general_opinfo_adapter ,
      tik_opinfo_adapter ,
      tbe_opinfo_adapter ,
      rl_opinfo_adapter
};

class UTEST_OpStoreAdapterManage : public testing::Test
{
protected:
    void SetUp()
    {
        OpsAdapterManagePtr = make_shared<OpStoreAdapterManager>();
        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (all_fe_ops_store_info_adapter);
        std:: map<string, string> options;
        OpsAdapterManagePtr->Initialize(options, fe::AI_CORE_NAME);
        cout << OpsAdapterManagePtr->map_all_op_store_adapter_.size() << endl;
        std::cout << "One Test SetUP" << std::endl;
    }

    void TearDown()
    {
        std::cout << "a test SetUP" << std::endl;
        Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
        config.is_init_ = false;
        config.content_map_.clear();

        std::map<string, string> options;
        string soc_version = "Ascend310";
        config.Initialize(options, soc_version);
        OpsAdapterManagePtr->Finalize();

    }

protected:
    OpStoreAdapterManagerPtr OpsAdapterManagePtr;
};

TEST_F(UTEST_OpStoreAdapterManage, get_opstore_adapter_failed)
{
    OpStoreAdapterPtr adapter_ptr = nullptr;
    EXPECT_NE(OpsAdapterManagePtr->GetOpStoreAdapter(EN_RESERVED, adapter_ptr), fe::SUCCESS);
    EXPECT_EQ(adapter_ptr, nullptr);
    EXPECT_NE(OpsAdapterManagePtr->GetOpStoreAdapter(EN_IMPL_RL, adapter_ptr), fe::SUCCESS);
    EXPECT_EQ(adapter_ptr, nullptr);
}
