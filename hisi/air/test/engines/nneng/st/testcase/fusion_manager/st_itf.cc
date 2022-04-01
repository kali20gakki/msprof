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
#include "itf_handler/itf_handler.h"
#include "fusion_manager/fusion_manager.h"
#undef private
#undef protected

using namespace std;
using namespace fe;

class itfhandler_st : public testing::Test
{
protected:
    void SetUp()
    {
        FEOpsStoreInfo CCE_CUSTOM_OPINFO_STUB {
            0,
            "cce_custom_opinfo",
            EN_IMPL_CUSTOM_CONSTANT_CCE,
            "./llt/cce/optimizer/st/stub/fe_config/cce_custom_opinfo",
            ""
        };
        FEOpsStoreInfo TIK_CUSTOM_OPINFO_STUB  = {
            1,
            "tik_custom_opinfo",
            EN_IMPL_CUSTOM_TIK,
            "./llt/cce/optimizer/st/stub/fe_config/tik_custom_opinfo",
            ""
        };
        FEOpsStoreInfo TBE_CUSTOM_OPINFO_STUB = {
            2,
            "tbe_custom_opinfo",
            EN_IMPL_CUSTOM_TBE,
            "./llt/cce/optimizer/st/stub/fe_config/tbe_custom_opinfo",
            ""
        };
        FEOpsStoreInfo CCE_CONSTANT_OPINFO_STUB = {
            3,
            "cce_constant_opinfo",
            EN_IMPL_HW_CONSTANT_CCE,
            "./llt/cce/optimizer/st/stub/fe_config/cce_constant_opinfo",
            ""
        };
        FEOpsStoreInfo CCE_GENERAL_OPINFO_STUB = {
            4,
            "cce_general_opinfo",
            EN_IMPL_HW_GENERAL_CCE,
            "./llt/cce/optimizer/st/stub/fe_config/cce_general_opinfo",
            ""
        };
        FEOpsStoreInfo TIK_OPINFO_STUB = {
            5,
            "tik_opinfo",
            EN_IMPL_HW_TIK,
            "./llt/cce/optimizer/st/stub/fe_config/tik_opinfo",
            ""
        };
        FEOpsStoreInfo TBE_OPINFO_STUB = {
            6,
            "tbe_opinfo",
            EN_IMPL_HW_TBE,
            "./llt/cce/optimizer/st/stub/fe_config/tbe_opinfo",
            ""
        };
        FEOpsStoreInfo RL_OPINFO_STUB = {
            7,
            "rl_opinfo",
            EN_IMPL_RL,
            "./llt/cce/optimizer/st/stub/fe_config/rl_opinfo",
            ""
        };
        cfg_info_.push_back(CCE_CUSTOM_OPINFO_STUB);
        cfg_info_.push_back(TIK_CUSTOM_OPINFO_STUB);
        cfg_info_.push_back(TBE_CUSTOM_OPINFO_STUB);
        cfg_info_.push_back(CCE_CONSTANT_OPINFO_STUB);
        cfg_info_.push_back(CCE_GENERAL_OPINFO_STUB);
        cfg_info_.push_back(TIK_OPINFO_STUB);
        cfg_info_.push_back(TBE_OPINFO_STUB);
        cfg_info_.push_back(RL_OPINFO_STUB);
    }

    void TearDown()
    {

    }

    std::vector<FEOpsStoreInfo> cfg_info_;

// AUTO GEN PLEASE DO NOT MODIFY IT
};

#if 0   // TODO:
TEST_F(itfhandler_st, initialize_success)
{
    Configuration::Instance().ops_store_info_vector_ = cfg_info_;
    FusionManager fm0 = FusionManager::Instance();
    std:: map<string, string> options;
    options.emplace("ge.socVersion", "Ascend310");
    Status ret = Initialize(options);
    EXPECT_EQ(ret, SUCCESS);
    map<string, OpsKernelInfoStorePtr> op_kern_infos;
    GetOpsKernelInfoStores(op_kern_infos);
    // EXPECT_EQ(op_kern_infos.size(), 8);
    map<string, GraphOptimizerPtr> graph_optimizers;
    GetGraphOptimizerObjs(graph_optimizers);
    EXPECT_EQ(graph_optimizers.size(), 1);
    ret = Finalize();
    EXPECT_EQ(ret, SUCCESS);
}
#endif
