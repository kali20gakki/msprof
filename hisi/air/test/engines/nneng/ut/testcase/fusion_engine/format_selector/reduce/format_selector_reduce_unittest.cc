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
#include <string>
#include <memory>
#include <map>
#include <utility>

#include "common/util/op_info_util.h"
#include "common/fe_type_utils.h"
#define private public
#define protected public

#include "graph/ge_tensor.h"
#include "fusion_manager/fusion_manager.h"
#include "ops_store/ops_kernel_manager.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "format_selector/builtin/reduce/reduce_format_selector/reduce_process_nz.h"
using namespace std;
using namespace testing;
using namespace fe;

using fe::FEOpsKernelInfoStore;
using ge::GeTensorDesc;
using ge::GeShape;
using ge::AttrUtils;
using ge::Format;
using ge::DataType;
using ge::ConstGeTensorDescPtr;
using ge::GeTensorDescPtr;
using ge::OpDescPtr;
using ge::OpDesc;
using fe::InputOrOutputInfoPtr;
using ge::GeAttrValue;
using std::vector;
using std::map;
using namespace ge;

using OpImplTypeJudgePtr = std::shared_ptr<OpImplTypeJudge>;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using OpStoreAdapterManagerPtr = std::shared_ptr<OpStoreAdapterManager>;
class FormatDtypeSelectorManagerReduceUTest : public testing::Test{
protected:
    static void SetUpTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    static void TearDownTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    // Some expensive resource shared by all tests.
    virtual void SetUp(){
      op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
      FEOpsStoreInfo tbe_builtin {
              6,
              "tbe-builtin",
              EN_IMPL_HW_TBE,
              "./air/test/engines/nneng/ut/testcase/fusion_engine/format_selector/fe_config/tbe_dynamic_opinfo",
              ""
      };
      vector<FEOpsStoreInfo> store_info;
      store_info.emplace_back(tbe_builtin);
      Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
      std::map<std::string, std::string> options;
      OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

      OpsKernelManager::Instance(AI_CORE_NAME).Initialize();

      cout << "a test Set Up" << endl;
    }
    virtual void TearDown(){
        cout << "a test Tear Down" << endl;

    }

    static Status QueryHighPrioOpImplTypeStub(FEOpsKernelInfoStore *This, const ge::OpDescPtr& op_desc_ptr, OpImplType &impl_type)
    {
        impl_type = EN_IMPL_HW_TBE;
        return fe::SUCCESS;
    }
protected:
    static void CreateOneOpGraph(ComputeGraphPtr graph,  OpDescPtr op_desc_ptr) {
        graph->AddNode(op_desc_ptr);
    }
    static GeTensorDesc CreateTensorDesc(vector<int64_t> dim_data,  ge::Format format, ge::DataType data_type) {
        GeShape shape_data(dim_data);
        GeTensorDesc data_desc(shape_data, format, data_type);
        data_desc.SetOriginFormat(format);
        data_desc.SetOriginShape(shape_data);
        data_desc.SetOriginDataType(data_type);
        return data_desc;
    }
public:
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  const string EQUAL_ORIGIN_FORMAT="NCHW,NCHW,NHWC,NHWC,HWCN,HWCN,CHWN,CHWN,NDHWC,NDHWC,NCDHW,NCDHW,DHWCN,DHWCN,DHWNC,DHWNC,ND,ND";
};

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_success) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 16};
  vector<int64_t> dim_data2 = {16, 8, 1, 1};
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
      "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
      std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
      op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  bool support_flag = false;
  map<string, vector<ge::Format>>::reverse_iterator iter;
  for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
    vector<ge::Format>::iterator ret;
    ret =
        std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, true);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, true);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, true);
  }
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_ubsize_greater) {
  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend910A";

  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 25600, 160, 32};
  vector<int64_t> dim_data2 = {16, 8, 1, 1};
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NHWC, DT_FLOAT16));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NHWC, DT_FLOAT16));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
    "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
    std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, false);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {1, 2});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
    op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  bool support_flag = false;
  map<string, vector<ge::Format>>::reverse_iterator iter;
  for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
    vector<ge::Format>::iterator ret;
    ret =
      std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);
  }
  PlatformInfoManager::Instance().platform_info_map_.clear();
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_ubsize_success) {
  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend910A";

  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 256, 16};
  vector<int64_t> dim_data2 = {16, 8, 1, 1};
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
    "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
    std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, false);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
    op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  bool support_flag = false;
  map<string, vector<ge::Format>>::reverse_iterator iter;
  for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
    vector<ge::Format>::iterator ret;
    ret =
      std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    int pos = iter->first.find("input");
    if (pos != -1) {
      EXPECT_EQ(support_flag, true);
    } else {
      EXPECT_EQ(support_flag, true);
    }

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);
  }
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_failed1) {
  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend910A";

  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 15};
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
      "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
      std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, false);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
      op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::reverse_iterator iter;
  for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
    bool support_flag = false;
    vector<ge::Format>::iterator ret;
    ret =
        std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    int pos = iter->first.find("input");
    if (pos != -1) {
      EXPECT_EQ(support_flag, true);
    } else {
      EXPECT_EQ(support_flag, true);
    }

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);
  }
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_failed2) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 16};
  GeShape shape_data1(dim_data1);
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
      "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
      std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {0, 1});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
      op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::iterator iter;
  for (iter = format_map.begin(); iter != format_map.end(); iter++) {
    bool support_flag = false;
    vector<ge::Format>::iterator ret;
    ret =
        std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    // EXPECT_EQ(support_flag, true);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, true);
  }
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_5hd_6hd_failed3) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32};
  GeShape shape_data1(dim_data1);
  op_desc_ptr->AddInputDesc("x",
                          CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y",
                           CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
      "tbe-builtin", op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
      std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(
      op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::reverse_iterator iter;
  for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
    bool support_flag = false;
    vector<ge::Format>::iterator ret;
    ret =
        std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag = false;
    ret = std::find(iter->second.begin(), iter->second.end(),
                    ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag = true;
    }
    EXPECT_EQ(support_flag, true);
  }
}


TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_6hd_success_NDHWC) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
    vector<int64_t> dim_data1 = {16, 32, 16, 16, 16};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));
    op_desc_ptr->AddOutputDesc(
            "y", CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
    ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    bool support_flag = false;
    map<string, vector<ge::Format>>::reverse_iterator iter;
    for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
        vector<ge::Format>::iterator ret;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_NDC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret =
                std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_C1HWNCoC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_FRACTAL_Z);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);
    }
}

// NCDHW
TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_6hd_success_NCDHW) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
    vector<int64_t> dim_data1 = {16, 16, 32, 16, 16};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NCDHW, DT_FLOAT16));
    op_desc_ptr->AddOutputDesc(
            "y", CreateTensorDesc(dim_data1, FORMAT_NCDHW, DT_FLOAT16));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
    ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    bool support_flag = false;
    map<string, vector<ge::Format>>::reverse_iterator iter;
    for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
        vector<ge::Format>::iterator ret;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_NDC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret =
                std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_C1HWNCoC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_FRACTAL_Z);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);
    }
}

/* shape of two tensor is not 5d */
TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_6hd_failed) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
    vector<int64_t> dim_data1 = {16, 32, 16, 16};
    vector<int64_t> dim_data2 = {16, 8, 1, 1};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));
    op_desc_ptr->AddOutputDesc(
            "y", CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
    ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    bool support_flag;
    map<string, vector<ge::Format>>::reverse_iterator iter;
    for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
        vector<ge::Format>::iterator ret;
        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_NDC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);

        support_flag = false;
        ret =
                std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_C1HWNCoC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_FRACTAL_Z);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, true);
    }
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, op_reduce_selector_6hd_failed2) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("reduce1", "ReduceOp");
    vector<int64_t> dim_data1 = {16, 32, 16, 16, 16};
    vector<int64_t> dim_data2 = {16, 8, 1, 1, 19};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));
    op_desc_ptr->AddOutputDesc(
            "y", CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT16));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin",
            op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
    ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 4});

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    bool support_flag = false;
    map<string, vector<ge::Format>>::reverse_iterator iter;
    uint32_t count = 0;
    for (iter = format_map.rbegin(); iter != format_map.rend(); iter++) {
        vector<ge::Format>::iterator ret;
        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_NDC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);

        support_flag = false;
        ret =
                std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        if (count == 0) {
            EXPECT_EQ(support_flag, false);
        } else {
            EXPECT_EQ(support_flag, false);
        }

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_C1HWNCoC0);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);

        support_flag = false;
        ret = std::find(iter->second.begin(), iter->second.end(),
                        ge::FORMAT_FRACTAL_Z);
        if (ret != iter->second.end()) {
            support_flag = true;
        }
        EXPECT_EQ(support_flag, false);
        count++;
    }
}

TEST_F(FormatDtypeSelectorManagerReduceUTest, check_origin_format_and_shape_test) {
  vector<int64_t> dim = {1};
  ge::GeShape shape(dim);
  size_t dim_min = 2;
  vector<ge::Format> formats;
  vector<ge::GeShape> shapes;
  shapes.emplace_back(shape);
  ReduceProcessNz reduceprocessnz;
  bool ret = reduceprocessnz.CheckOriginFormatAndShape(formats, shapes, dim_min);
  EXPECT_EQ(ret, false);
}