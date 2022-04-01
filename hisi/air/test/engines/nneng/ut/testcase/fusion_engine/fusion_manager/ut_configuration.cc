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
#include "common/comm_error_codes.h"

#define protected public
#define private public
#include "common/configuration.h"
#include "common/util/json_util.h"
#include "common/util/constants.h"
#include "common/common_utils.h"
#include "graph/ge_context.h"
#include "ge/ge_api_types.h"
#include "common/aicore_util_constants.h"
#include "fusion_config_manager/fusion_config_parser.h"
#include "graph/ge_local_context.h"
#undef private
#undef protected

using namespace std;
using namespace fe;

class configuration_ut: public testing::Test
{
protected:
    void SetUp()
    {
        map<string, string> options;
        options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
        ge::GetThreadLocalContext().SetGraphOption(options);
        Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
        config.is_init_ = false;
        string soc_version = "Ascend310";
        config.Initialize(options, soc_version);
    }

    void TearDown()
    {
        map<string, string> options;
        options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
        ge::GetThreadLocalContext().SetGraphOption(options);
        Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
        config.is_init_ = false;
        string soc_version = "Ascend310";
        config.Initialize(options, soc_version);
    }
// AUTO GEN PLEASE DO NOT MODIFY IT
};

namespace{
std::string GetConfigFilePathStub(Configuration *This)
{
    std::string config_file_path = "/home/fe_config/fe.ini";
    return config_file_path;
}

ge::graphStatus GetOptionTrue(ge::GEContext *This, const std::string &key, std::string &option)
{
  option = "1";
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetOptionFalse(ge::GEContext *This, const std::string &key, std::string &option)
{
  option = "0";
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetOptionEmpty(ge::GEContext *This, const std::string &key, std::string &option)
{
  option = "";
  return ge::GRAPH_SUCCESS;
}
ge::graphStatus GetOptionFail(ge::GEContext *This, const std::string &key, std::string &option)
{
  option = "0";
  return ge::GRAPH_FAILED;
}
std::string GetBuiltInConfigFilePathStubs(Configuration *This)
{
  return "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_manager/fusion_switch_build_in";
}

}

TEST_F(configuration_ut, init_and_finalize)
{
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  ge::GetThreadLocalContext().SetGraphOption(options);
  string soc_version = "Ascend310";
  Status status = config.Initialize(options, soc_version);
  EXPECT_EQ(status, SUCCESS);
  status = config.Initialize(options, soc_version);
  EXPECT_EQ(config.IsMDCSoc(), false);
  EXPECT_EQ(status, SUCCESS);
  status = config.Finalize();
  EXPECT_EQ(status, SUCCESS);
  status = config.Finalize();
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, get_boolvalue)
{
    bool bool_value = Configuration::Instance(fe::AI_CORE_NAME).GetBoolValue("needl2fusion", false);
    ASSERT_FALSE(bool_value);
}

TEST_F(configuration_ut, get_context_boolvalue)
{
  map<string, string> options;
  options.emplace("aaa", "1");
  options.emplace("bbb", "0");
  options.emplace("ccc", "");
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(Configuration::Instance(fe::AI_CORE_NAME).GetGeContextBoolValue("aaa", false), true);
  EXPECT_EQ(Configuration::Instance(fe::AI_CORE_NAME).GetGeContextBoolValue("bbb", true), false);
  EXPECT_EQ(Configuration::Instance(fe::AI_CORE_NAME).GetGeContextBoolValue("ccc", false), false);
  EXPECT_EQ(Configuration::Instance(fe::AI_CORE_NAME).GetGeContextBoolValue("ddd", true), true);
}

TEST_F(configuration_ut, get_context_string)
{
  map<string, string> options;
  options.emplace("AAA", "111");
  options.emplace("BBB", "000");
  options.emplace("CCC", "");
  ge::GetThreadLocalContext().SetGraphOption(options);
  string str_value;
  Status ret = Configuration::Instance(fe::AI_CORE_NAME).GetGeContextStringValue("AAA", str_value);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(str_value, "111");

  ret = Configuration::Instance(fe::AI_CORE_NAME).GetGeContextStringValue("BBB", str_value);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(str_value, "000");

  ret = Configuration::Instance(fe::AI_CORE_NAME).GetGeContextStringValue("CCC", str_value);
  EXPECT_EQ(ret, SUCCESS);

  ret = Configuration::Instance(fe::AI_CORE_NAME).GetGeContextStringValue("DDD", str_value);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(configuration_ut, get_boolvalue_abnormal)
{
    Configuration::Instance(fe::AI_CORE_NAME).content_map_.emplace("needl2fusion2", "!@##$");
    bool bool_value = Configuration::Instance(fe::AI_CORE_NAME).GetBoolValue("needl2fusion2", false);
    ASSERT_FALSE(bool_value);
}

TEST_F(configuration_ut, loadconfigfile_success)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend310";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    Status status = config.LoadConfigFile();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend310";
    config.Initialize(options, soc_version);
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success1)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Ascend310";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "ascend310";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success2)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Ascend610";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "ascend610";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success3)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Ascend910A";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "ascend910";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success4)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Ascend910B";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "ascend910";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success5)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Hi3796CV300ES";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "hi3796cv300es";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success6)
{
    Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
    config.soc_version_ = "Hi3796CV300CS";
    config.content_map_.clear();
    config.content_map_.emplace("op.store.tbe-builtin", "2|6|op_impl/built-in/ai_core/tbe/config|op_impl/built-in/ai_core/tbe/impl/|true|true");
    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, SUCCESS);
    FEOpsStoreInfo op_store_info;
    config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
    string sub_path = "hi3796cv300cs";
    int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
    EXPECT_EQ(pos, op_store_info.cfg_file_path.length()-sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed1)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.", "1|1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_NAME_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed2)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_VALUE_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed3)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "0|0|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_VALUE_ITEM_SIZE_INCORRECT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed4)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|1|./config/fe_config/tik_custom_opinfo|x|qwer|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_VALUE_ITEM_SIZE_INCORRECT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed5)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|1|./config/fe_config/tik_custom_opinfo||false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_VALUE_ITEM_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed6)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|1||./config/fe_config/tik_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_VALUE_ITEM_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed7)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed8)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "x|1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_PRIORITY_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed9)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|c|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed10)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|15|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed11)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|-1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed12)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "1|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tdk-custom", "2|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_REPEAT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed13)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend610";
    config.Initialize(options, soc_version);
    config.content_map_.clear();
    config.content_map_.emplace("op.store.cce-custom", "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
    config.content_map_.emplace("op.store.tik-custom", "0|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

    Status status = config.AssembleOpsStoreInfoVector();
    EXPECT_EQ(status, OPSTORE_PRIORITY_INVALID);
}

TEST_F(configuration_ut, get_opsstoreinfo)
{
    Configuration::Instance(fe::AI_CORE_NAME).is_init_ = false;
    vector<FEOpsStoreInfo> ops_store_info_vec = Configuration::Instance(fe::AI_CORE_NAME).GetOpsStoreInfo();
    EXPECT_EQ(ops_store_info_vec.size(), 2);

    for (FEOpsStoreInfo &op_store_info : ops_store_info_vec)
    {
        if (op_store_info.op_impl_type == EN_IMPL_HW_TBE)
        {
            EXPECT_EQ(op_store_info.fe_ops_store_name, "tbe-builtin");
        }
        if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE)
        {
            EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-builtin");
            EXPECT_EQ(op_store_info.need_pre_compile, true);
            EXPECT_EQ(op_store_info.need_compile, false);
        }
        if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE)
        {
            EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-builtin");
            EXPECT_EQ(op_store_info.need_pre_compile, true);
            EXPECT_EQ(op_store_info.need_compile, true);
        }
        if (op_store_info.op_impl_type == EN_IMPL_PLUGIN_TBE)
        {
            EXPECT_EQ(op_store_info.need_pre_compile, true);
            EXPECT_EQ(op_store_info.need_compile, false);
        }
    }
}

TEST_F(configuration_ut, getstringvalue_success)
{
    string stringvalue;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetStringValue("fusionrulemgr.multireferswitch", stringvalue);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(stringvalue, "yes");
}

TEST_F(configuration_ut, getstringvalue_failed)
{
    string stringvalue;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetStringValue("fusionrulemgr.xxx", stringvalue);
    EXPECT_EQ(status, FAILED);
}

TEST_F(configuration_ut, GetOpStoreInfoByImplType_success)
{
    FEOpsStoreInfo op_store_info;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetOpStoreInfoByImplType(EN_IMPL_CUSTOM_TBE, op_store_info);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(op_store_info.fe_ops_store_name, "tbe-custom");
}

TEST_F(configuration_ut, GetOpStoreInfoByImplType_failed)
{
    FEOpsStoreInfo op_store_info;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetOpStoreInfoByImplType(EN_RESERVED, op_store_info);
    EXPECT_EQ(status, FAILED);
}

TEST_F(configuration_ut, getgraphfilepath)
{
    string graph_file_path;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetGraphFilePath(graph_file_path);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, getcustomfilepath)
{
    string custom_file_path;
    Status status = Configuration::Instance(fe::AI_CORE_NAME).GetCustomFilePath(custom_file_path);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, dsagetgraphfilepath)
{
    string graph_file_path;
    Status status = Configuration::Instance(fe::kDsaCoreName).GetGraphFilePath(graph_file_path);
    EXPECT_EQ(status, FAILED);
    EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, dsagetcustomfilepath)
{
    string custom_file_path;
    Status status = Configuration::Instance(fe::kDsaCoreName).GetCustomFilePath(custom_file_path);
    EXPECT_EQ(status, FAILED);
    EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, dsagetcustompassfilepath)
{
    string graph_file_path;
    Status status = Configuration::Instance(fe::kDsaCoreName).GetCustomPassFilePath(graph_file_path);
    EXPECT_EQ(status, FAILED);
    EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, dsagetbuiltinpassfilepath)
{
    string custom_file_path;
    Status status = Configuration::Instance(fe::kDsaCoreName).GetBuiltinPassFilePath(custom_file_path);
    EXPECT_EQ(status, FAILED);
    EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, get_appendargsmode_1910)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend310";
    
    config.Initialize(options, soc_version);
    AppendArgsMode lm = config.GetAppendArgsMode();
    EXPECT_EQ(lm, AppendArgsMode::L2_BUFFER_ARGS);
}

TEST_F(configuration_ut, get_auto_tune_mode)
{
    Configuration config(fe::AI_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend310";
    config.Initialize(options, soc_version);
    options.emplace("ge.autoTuneMode", "RL");
    ge::GetThreadLocalContext().SetGraphOption(options);
    AutoTuneMode mode = config.GetAutoTuneMode();
    EXPECT_EQ(mode, AutoTuneMode::TUNE_MODE_RL_TUNE);
}

TEST_F(configuration_ut, get_opsstoreinfo_vectorcore)
{
  Configuration config(fe::VECTOR_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend610";
  config.Initialize(options, soc_version);
  vector<FEOpsStoreInfo> ops_store_info_vec = config.GetOpsStoreInfo();
  EXPECT_EQ(ops_store_info_vec.size(), 2);

  for (FEOpsStoreInfo &op_store_info : ops_store_info_vec)
  {
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE)
    {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-builtin");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, true);
    }
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE)
    {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-custom");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, true);
    }
  }
}

TEST_F(configuration_ut, getgraphfilepath_vectorcore)
{
  string graph_file_path;
  Configuration config(fe::VECTOR_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend610";
  config.Initialize(options, soc_version);
  Status status = config.GetGraphFilePath(graph_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, getcustomfilepath_vectorcore)
{
  string custom_file_path;
  Configuration config(fe::VECTOR_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend610";
  config.Initialize(options, soc_version);
  Status status = config.GetCustomFilePath(custom_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, InitBufferFusionMode_offOptimize2)
{
    Configuration config(AI_CORE_NAME);
    config.is_init_ = false;
    config.content_map_.emplace("l2fusion.enable", "false");
    map<string, string> options;
    options.emplace("ge.bufferOptimize", "off_optimize");
    Status status = config.InitOptions(options);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(config.EnableL1Fusion(), false);
    EXPECT_EQ(config.buffer_optimize_, EN_OFF_OPTIMIZE);

    config.InitBufferFusionMode();
    EXPECT_EQ(config.GetBufferFusionMode(), EN_OPTIMIZE_DISABLE);
}

TEST_F(configuration_ut, InitBufferFusionMode_offOptimize3)
{
    Configuration config(AI_CORE_NAME);
    config.is_init_ = false;
    config.content_map_.emplace("l2fusion.enable", "true");
    map<string, string> options;
    options.emplace("ge.bufferOptimize", "off_optimize");
    Status status = config.InitOptions(options);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(config.EnableL1Fusion(), false);
    EXPECT_EQ(config.buffer_optimize_, EN_OFF_OPTIMIZE);

    config.InitBufferFusionMode();
    EXPECT_EQ(config.GetBufferFusionMode(), EN_OPTIMIZE_DISABLE);
}

TEST_F(configuration_ut, InitBufferFusionMode_else) {
    Configuration config(AI_CORE_NAME);
    config.is_init_ = false;
    config.content_map_.emplace("l2fusion.enable", "true");
    map<string, string> options;
    options.emplace("ge.bufferOptimize", "off_optimize");
    Status status = config.InitOptions(options);
    EXPECT_EQ(status, SUCCESS);
  GetBufferOptimizeString(EN_L1_OPTIMIZE);
  GetBufferFusionModeString(EN_OPTIMIZE_DISABLE);
}

TEST_F(configuration_ut, InitBufferFusionMode_invalid_buffer_optimize)
{
    Configuration config(AI_CORE_NAME);
    config.is_init_ = false;
    map<string, string> options;
    options.emplace("ge.bufferOptimize", "xxx");
    string soc_version = "Ascend310";
    Status status = config.Initialize(options, soc_version);
    EXPECT_EQ(status, FAILED);
    EXPECT_EQ(config.buffer_optimize_, EN_UNKNOWN_OPTIMIZE);
    EXPECT_EQ(config.GetBufferFusionMode(), EN_OPTIMIZE_DISABLE);
    EXPECT_EQ(config.EnableL1Fusion(), false);
}

TEST_F(configuration_ut, init_precision_mode_case1)
{
  map<string, string> options;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Status ret = Configuration::Instance(AI_CORE_NAME).InitPrecisionMode();
  EXPECT_EQ(ret, SUCCESS);

  options[ge::PRECISION_MODE] = "xxxx";
  ge::GetThreadLocalContext().SetGraphOption(options);
  ret = Configuration::Instance(AI_CORE_NAME).InitPrecisionMode();
  EXPECT_EQ(ret, FAILED);
}

TEST_F(configuration_ut, init_op_precision_mode_case1)
{
  Configuration config(AI_CORE_NAME);
  map<string, string> options;
  // unix file format
  options.emplace("ge.exec.op_precision_mode",
                  "./air/test/engines/nneng/config/op_precision_mode_1.ini");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "ccc:high_per,ddd:high_pre");
}

TEST_F(configuration_ut, init_op_precision_mode_case2)
{
  Configuration config(AI_CORE_NAME);
  map<string, string> options;
  // empty file
  options.emplace("ge.exec.op_precision_mode",
                  "./air/test/engines/nneng/config/op_precision_mode_2.ini");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_precision_mode_case3)
{
  Configuration config(AI_CORE_NAME);
  map<string, string> options;
  // dos file format
  options.emplace("ge.exec.op_precision_mode",
                  "./air/test/engines/nneng/config/op_precision_mode_3.ini");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "ccc:high_per,ddd:high_pre");
}

TEST_F(configuration_ut, init_op_precision_mode_case4)
{
  Configuration config(AI_CORE_NAME);
  map<string, string> options;
  options.emplace("ge.exec.op_precision_mode",
                  "./air/test/engines/nneng/config/op_precision_mode_0.ini");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, FAILED);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case1)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision_for_all");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case2)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance_for_all");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case3)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision_for_all");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_precision,Mul:high_precision,Relu:high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case4)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance_for_all");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_performance,Mul:high_performance,Relu:high_performance");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case1)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case2)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case3)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", "");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_performance,Mul:high_precision,Relu:high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case4)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "   ");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_performance,Mul:high_performance,Relu:high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case5)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", "Relu,Add,Mul");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_precision,Mul:high_precision,Relu:high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case6)
{
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = "./air/test/engines/nneng/config/";
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "  Relu ,Add , Mul ");
  Status status = config.InitOpPrecisionMode(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  config.GetOpSelectImplModeStr(str);
  EXPECT_EQ(str, "Add:high_performance,Mul:high_performance,Relu:high_performance");
}

TEST_F(configuration_ut, IsEnableNetworkAnalysis_true)
{
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", true);
  config.Initialize(options, soc_version);
  auto ret = config.IsEnableNetworkAnalysis();
  EXPECT_EQ(ret, true);
}

TEST_F(configuration_ut, IsEnableNetworkAnalysis_false)
{
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "0", true);
  config.Initialize(options, soc_version);
  auto ret = config.IsEnableNetworkAnalysis();
  EXPECT_EQ(ret, false);
}

TEST_F(configuration_ut, init_scope_id_1)
{
  Configuration config(fe::AI_CORE_NAME);
  config.content_map_.emplace("scope.id", "All");
  config.InitScopeId();
  EXPECT_EQ(config.allow_all_scope_id_, true);
}

TEST_F(configuration_ut, init_scope_id_2)
{
  Configuration config(fe::AI_CORE_NAME);
  config.content_map_.emplace("scope.id", "1,2,3");
  config.InitScopeId();
  EXPECT_EQ(config.allow_all_scope_id_, false);
}

TEST_F(configuration_ut, init_buffer_optimize_case1)
{
  map<string, string> options;
  options.emplace(ge::BUFFER_OPTIMIZE, L2_OPTIMIZE);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Configuration config(fe::AI_CORE_NAME);
  Status ret = config.InitBufferOptimize();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(configuration_ut, init_buffer_optimize_case2)
{
  map<string, string> options;
  options.emplace(ge::BUFFER_OPTIMIZE, "123");
  ge::GetThreadLocalContext().SetGraphOption(options);
  Configuration config(fe::AI_CORE_NAME);
  Status ret = config.InitBufferOptimize();
  EXPECT_EQ(ret, FAILED);
}

TEST_F(configuration_ut, init_compress_ratio)
{
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  config.content_map_.clear();
  Status status = config.LoadConfigFile();
  config.InitCompressRatio();
  auto compress_ratios = config.GetCompressRatio();
  for (auto compress_ratio : compress_ratios) {
    cout << "core num:" << compress_ratio.first << ", ratio:" << compress_ratio.second << endl;
  }
  EXPECT_EQ(compress_ratios.size(), 6);
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, init_compress_ratio_1)
{
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  config.content_map_.clear();
  config.compress_ratio_.clear();
  config.content_map_.emplace(std::make_pair("multi_core.compress_ratio", "2:0.8:0.88|%:0.7|10:%%|30:0.8"));
  config.InitCompressRatio();
  auto compress_ratios = config.GetCompressRatio();
  for (auto compress_ratio : compress_ratios) {
    cout << "core num:" << compress_ratio.first << ", ratio:" << compress_ratio.second << endl;
  }
  EXPECT_EQ(compress_ratios.size(), 1);
}