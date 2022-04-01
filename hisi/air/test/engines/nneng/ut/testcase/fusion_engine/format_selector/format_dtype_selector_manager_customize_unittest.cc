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

#define private public
#define protected public
#include "common/util/op_info_util.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

#include "graph/ge_tensor.h"
#include "fusion_manager/fusion_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "ops_store/ops_kernel_manager.h"
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
using FEOpsKernelInfoStorePtr = std::shared_ptr<fe::FEOpsKernelInfoStore>;
using OpImplTypeJudgePtr = std::shared_ptr<OpImplTypeJudge>;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using OpStoreAdapterManagerPtr = std::shared_ptr<OpStoreAdapterManager>;
class FormatDtypeSelectorManagerCustomizeUTest : public testing::Test{
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
        fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, AI_CORE_NAME);
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
        OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

        std::map<std::string, std::string> options;
        fe_ops_kernel_info_store_ptr->Initialize(options);
        cout << "a test Set Up" << endl;
    }
    virtual void TearDown(){
        cout << "a test Tear Down" << endl;
        fe_ops_kernel_info_store_ptr->Finalize();

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
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
};
bool SelectTbeOpFormatStub(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    op_format_dtype_str = "{\"input0\":{\"name\":\"x\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"},\"output0\":{\"name\":\"y\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"}}";
    return true;
}

bool SelectTbeOpFormatStub2(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    op_format_dtype_str = "";
    return true;
}

bool SelectTbeOpFormatUnknownStub(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    op_format_dtype_str = "{\"input0\":{\"name\":\"x\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"},\"output0\":{\"name\":\"y\",\"format\":\"ND,NCHW\",\"unknownshape_format\":\"ND,NCHW\",\"dtype\":\"float,float16\"}}";
    return true;
}

bool SelectTbeOpFormatStub1(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    std::vector<te::TbeOpParam> inputs;
    inputs = tbe_op_info.inputs_;
    if (!inputs.empty()) {
        std::vector<te::TbeOpTensor> tensors;
        if (inputs[0].GetTensors(tensors)) {
            if (!tensors.empty()) {
                bool is_first_layer;
                if (tensors[0].GetFirstLayer(is_first_layer) && is_first_layer) {
                    // Check if it's the first layer conv
                    // the first layer conv can use c04 format
                    op_format_dtype_str = "{\"input0\":{\"name\":\"x\",\"format\":\"NC1HWC0_C04,NC1HWC0_C04\", \"dtype\":\"float,float16\"},\"output0\":{\"name\":\"y\",\"format\":\"NC1HWC0_C04,NC1HWC0_C04\", \"dtype\":\"float,float16\"}}";
                    return true;
                }
            }
        }
    }
    op_format_dtype_str = "{\"input0\":{\"name\":\"x\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"},\"output0\":{\"name\":\"y\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"}}";
    return true;
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, op_customize_selector_query_success)
{
const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
vector<int64_t> dim_data = {1, 2, 3, 4};
GeShape shape_data(dim_data);
GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
op_desc_ptr->AddInputDesc("x", data_desc);
op_desc_ptr->AddOutputDesc("y", data_desc);

OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
tbe_adapter_ptr->SelectTbeOpFormat = SelectTbeOpFormatStub;
op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
map<string,vector<ge::Format>> format_map;
map<string,vector<ge::DataType>> data_type_map;
Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr,*(op_desc_ptr.get()),
                                                           false, format_map, data_type_map);
EXPECT_EQ(fe::SUCCESS, result);
vector<ge::Format> format_vec = format_map.at(inputs[0]->GetUniqueName());
vector<ge::DataType> data_type_vec = data_type_map.at(inputs[0]->GetUniqueName());
EXPECT_EQ(ge::FORMAT_ND, format_vec[0]);
EXPECT_EQ(ge::FORMAT_NCHW, format_vec[1]);
EXPECT_EQ(ge::DT_FLOAT, data_type_vec[0]);
EXPECT_EQ(ge::DT_FLOAT16, data_type_vec[1]);

// check outputs
vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
format_vec = format_map.at(outputs[0]->GetUniqueName());
data_type_vec = data_type_map.at(outputs[0]->GetUniqueName());
EXPECT_EQ(ge::FORMAT_ND, format_vec[0]);
EXPECT_EQ(ge::FORMAT_NCHW, format_vec[1]);
EXPECT_EQ(ge::DT_FLOAT, data_type_vec[0]);
EXPECT_EQ(ge::DT_FLOAT16, data_type_vec[1]);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, op_customize_selector_query_failed)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    vector<int64_t> dim_data = {1, 2, 3, 4};
    GeShape shape_data(dim_data);
    GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
    op_desc_ptr->AddInputDesc("x", data_desc);
    op_desc_ptr->AddOutputDesc("y", data_desc);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    tbe_adapter_ptr->SelectTbeOpFormat = SelectTbeOpFormatStub2;
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string,vector<ge::Format>> format_map;
    map<string,vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr,*(op_desc_ptr.get()),
                                                                  false, format_map, data_type_map);
    EXPECT_EQ(fe::FAILED, result);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, unknown_shape_op_customize_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    vector<int64_t> dim_data = {1, -1, -1, 4};
    GeShape shape_data(dim_data);
    GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
    op_desc_ptr->AddInputDesc("x", data_desc);
    op_desc_ptr->AddOutputDesc("y", data_desc);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    tbe_adapter_ptr->SelectTbeOpFormat = SelectTbeOpFormatUnknownStub;
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
    map<string,vector<ge::Format>> format_map;
    map<string,vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr,*(op_desc_ptr.get()),
                                                               false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);
    vector<ge::Format> format_vec = format_map.at(inputs[0]->GetUniqueName());
    vector<ge::DataType> data_type_vec = data_type_map.at(inputs[0]->GetUniqueName());
    EXPECT_EQ(ge::FORMAT_ND, format_vec[0]);
    EXPECT_EQ(ge::FORMAT_NCHW, format_vec[1]);
    EXPECT_EQ(ge::DT_FLOAT, data_type_vec[0]);
    EXPECT_EQ(ge::DT_FLOAT16, data_type_vec[1]);

// check outputs
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    format_vec = format_map.at(outputs[0]->GetUniqueName());
    data_type_vec = data_type_map.at(outputs[0]->GetUniqueName());
    EXPECT_EQ(ge::FORMAT_ND, format_vec[0]);
    EXPECT_EQ(ge::FORMAT_NCHW, format_vec[1]);
    EXPECT_EQ(ge::DT_FLOAT, data_type_vec[0]);
    EXPECT_EQ(ge::DT_FLOAT16, data_type_vec[1]);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, ConvertFormatDtype_failed1)
{
  string format_vec_str;
  string data_type_vec_str = "DT_INT16, DT_INT16";
  uint32_t format_size_of_first_input_or_output;
  vector<ge::Format> format_vec;
  vector<ge::DataType> data_type_vec;
  std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  Status status = customize_selector_ptr->ConvertFormatDtype(format_vec_str, data_type_vec_str,
                                                             format_size_of_first_input_or_output, format_vec,
                                                             data_type_vec);
  EXPECT_EQ(status, fe::FAILED);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, ConvertFormatDtype_failed2)
{
  string format_vec_str;
  string data_type_vec_str;
  uint32_t format_size_of_first_input_or_output;
  vector<ge::Format> format_vec;
  vector<ge::DataType> data_type_vec;
  std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  Status status = customize_selector_ptr->ConvertFormatDtype(format_vec_str, data_type_vec_str,
                                                             format_size_of_first_input_or_output, format_vec,
                                                             data_type_vec);
  EXPECT_EQ(status, fe::FAILED);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, UpdateDTypeAndFormat_suc)
{
  set<uint32_t> remain_index_set;
  std::map<std::string, vector<ge::Format>> format_map;
  std::map<std::string, vector<ge::DataType>> data_type_map;

  std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  Status status = customize_selector_ptr->UpdateDTypeAndFormat(remain_index_set, format_map, data_type_map);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_01)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
std::map<std::string, vector<ge::DataType>> data_type_map;

Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_02)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
vector<ge::Format> format_vec2 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
vector<ge::Format> format_vec3 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT16};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT16};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT16};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = true;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);
std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
}

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = false;
status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
}
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
}
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_03)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04, FORMAT_NC1HWC0};
vector<ge::Format> format_vec2 = {FORMAT_FRACTAL_Z_C04, FORMAT_FRACTAL_Z};
vector<ge::Format> format_vec3 = {FORMAT_ND, FORMAT_ND};
vector<ge::Format> format_vec4 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
format_map.emplace("o", format_vec4);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec4 = {DT_FLOAT16, DT_FLOAT};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);
data_type_map.emplace("o", date_type_vec4);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = true;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);

std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 1);
if (format_iter->first == "x") {
EXPECT_EQ(format_iter->second[0], FORMAT_NC1HWC0_C04);
}
if (format_iter->first == "y") {
EXPECT_EQ(format_iter->second[0], FORMAT_FRACTAL_Z_C04);
}
if (format_iter->first == "z") {
EXPECT_EQ(format_iter->second[0], FORMAT_ND);
}
if (format_iter->first == "o") {
EXPECT_EQ(format_iter->second[0], FORMAT_NC1HWC0);
}
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 1);
EXPECT_EQ(date_type_iter->second[0], DT_FLOAT16);
}
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_04)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04, FORMAT_NC1HWC0};
vector<ge::Format> format_vec2 = {FORMAT_FRACTAL_Z_C04, FORMAT_FRACTAL_Z};
vector<ge::Format> format_vec3 = {FORMAT_ND, FORMAT_ND};
vector<ge::Format> format_vec4 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
format_map.emplace("o", format_vec4);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec4 = {DT_FLOAT16, DT_FLOAT};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);
data_type_map.emplace("o", date_type_vec4);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = false;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);

std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 1);
if (format_iter->first == "x") {
EXPECT_EQ(format_iter->second[0], FORMAT_NC1HWC0);
}
if (format_iter->first == "y") {
EXPECT_EQ(format_iter->second[0], FORMAT_FRACTAL_Z);
}
if (format_iter->first == "z") {
EXPECT_EQ(format_iter->second[0], FORMAT_ND);
}
if (format_iter->first == "o") {
EXPECT_EQ(format_iter->second[0], FORMAT_NC1HWC0);
}
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 1);
EXPECT_EQ(date_type_iter->second[0], DT_FLOAT);
}
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_05)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
vector<ge::Format> format_vec2 = {FORMAT_FRACTAL_Z, FORMAT_FRACTAL_Z};
vector<ge::Format> format_vec3 = {FORMAT_ND, FORMAT_ND};
vector<ge::Format> format_vec4 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
format_map.emplace("o", format_vec4);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT};
vector<ge::DataType> date_type_vec4 = {DT_FLOAT16, DT_FLOAT};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);
data_type_map.emplace("o", date_type_vec4);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = false;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);

std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
}

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = true;
status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);

for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
}
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
}
}

TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_06)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04,FORMAT_NC1HWC0,FORMAT_NC1HWC0_C04,FORMAT_NC1HWC0};
vector<ge::Format> format_vec2 = {FORMAT_FRACTAL_Z_C04,FORMAT_FRACTAL_Z,FORMAT_FRACTAL_Z_C04,FORMAT_FRACTAL_Z};
vector<ge::Format> format_vec3 = {FORMAT_ND, FORMAT_ND, FORMAT_ND, FORMAT_ND};
vector<ge::Format> format_vec4 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0, FORMAT_NC1HWC0, FORMAT_NC1HWC0};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
format_map.emplace("o", format_vec4);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec4 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);
data_type_map.emplace("o", date_type_vec4);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = false;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);
LogFormatMap(format_map);
LogDataTypeMap(data_type_map);

std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
if (format_iter->first == "x") {
vector<ge::Format> x_format_vec = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
EXPECT_EQ(format_iter->second, x_format_vec);
}
if (format_iter->first == "y") {
vector<ge::Format> y_format_vec = {FORMAT_FRACTAL_Z, FORMAT_FRACTAL_Z};
EXPECT_EQ(format_iter->second, y_format_vec);
}
if (format_iter->first == "z") {
vector<ge::Format> z_format_vec = {FORMAT_ND, FORMAT_ND};
EXPECT_EQ(format_iter->second, z_format_vec);
}
if (format_iter->first == "o") {
vector<ge::Format> o_format_vec = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
EXPECT_EQ(format_iter->second, o_format_vec);
}
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
vector<ge::DataType> dt_vec = {DT_FLOAT16, DT_INT8};
EXPECT_EQ(date_type_iter->second, dt_vec);
}
}
TEST_F(FormatDtypeSelectorManagerCustomizeUTest, filter_c04_format_07)
{
std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
        std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
std::map<std::string, vector<ge::Format>> format_map;
vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04,FORMAT_NC1HWC0,FORMAT_NC1HWC0_C04,FORMAT_NC1HWC0};
vector<ge::Format> format_vec2 = {FORMAT_FRACTAL_Z_C04,FORMAT_FRACTAL_Z,FORMAT_FRACTAL_Z_C04,FORMAT_FRACTAL_Z};
vector<ge::Format> format_vec3 = {FORMAT_ND, FORMAT_ND, FORMAT_ND, FORMAT_ND};
vector<ge::Format> format_vec4 = {FORMAT_NC1HWC0, FORMAT_NC1HWC0, FORMAT_NC1HWC0, FORMAT_NC1HWC0};
format_map.emplace("x", format_vec1);
format_map.emplace("y", format_vec2);
format_map.emplace("z", format_vec3);
format_map.emplace("o", format_vec4);
std::map<std::string, vector<ge::DataType>> data_type_map;
vector<ge::DataType> date_type_vec1 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec2 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec3 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
vector<ge::DataType> date_type_vec4 = {DT_FLOAT16, DT_FLOAT16, DT_INT8, DT_INT8};
data_type_map.emplace("x", date_type_vec1);
data_type_map.emplace("y", date_type_vec2);
data_type_map.emplace("z", date_type_vec3);
data_type_map.emplace("o", date_type_vec4);

Configuration::Instance(AI_CORE_NAME).enable_small_channel_ = true;
Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
EXPECT_EQ(status, fe::SUCCESS);
LogFormatMap(format_map);
LogDataTypeMap(data_type_map);

std::map<std::string, vector<ge::Format>>::iterator format_iter;
for (format_iter = format_map.begin(); format_iter != format_map.end();
format_iter++) {
EXPECT_EQ(format_iter->second.size(), 2);
if (format_iter->first == "x") {
vector<ge::Format> x_format_vec = {FORMAT_NC1HWC0_C04, FORMAT_NC1HWC0_C04};
EXPECT_EQ(format_iter->second, x_format_vec);
}
if (format_iter->first == "y") {
vector<ge::Format> y_format_vec = {FORMAT_FRACTAL_Z_C04, FORMAT_FRACTAL_Z_C04};
EXPECT_EQ(format_iter->second, y_format_vec);
}
if (format_iter->first == "z") {
vector<ge::Format> z_format_vec = {FORMAT_ND, FORMAT_ND};
EXPECT_EQ(format_iter->second, z_format_vec);
}
if (format_iter->first == "o") {
vector<ge::Format> o_format_vec = {FORMAT_NC1HWC0, FORMAT_NC1HWC0};
EXPECT_EQ(format_iter->second, o_format_vec);
}
}
std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end();
date_type_iter++) {
EXPECT_EQ(date_type_iter->second.size(), 2);
vector<ge::DataType> dt_vec = {DT_FLOAT16, DT_INT8};
EXPECT_EQ(date_type_iter->second, dt_vec);
}
}
