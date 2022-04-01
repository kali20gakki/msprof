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
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

#include "graph/ge_tensor.h"
#include "fusion_manager/fusion_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "ops_store/ops_kernel_manager.h"
#include "format_selector/common/format_dtype_selector_base.h"
#include "format_selector/builtin/broadcast/format_process/broadcast_process_fractal_z.h"
#include "format_selector/builtin/broadcast/format_process/broadcast_process_fractal_z_3d.h"
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
class FormatDtypeSelectorManagerBroadcastUTest : public testing::Test{
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
    const string EQUAL_ORIGIN_FORMAT="NCHW,NCHW,NHWC,NHWC,HWCN,HWCN,CHWN,CHWN,NDHWC,NDHWC,NCDHW,NCDHW,DHWCN,DHWCN,DHWNC,DHWNC,ND,ND";
    const string EQUAL_HEAVY_FORMAT=",NC1HWC0,NC1HWC0,C1HWNCoC0,C1HWNCoC0,FRACTAL_Z,FRACTAL_Z,FRACTAL_NZ,FRACTAL_NZ";
};

// grads/x1: 4d, x2: scalar
// grads/x1: 5HD/6D/FZ/NZ, x2: ND
// y1:5HD/6D/FZ/NZ, y2: ND
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_partscalar_success)
{
const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("minimum", "MinimumGrad");
vector<int64_t> dim_data1 = {16,128,128,64};
vector<int64_t> dim_data2 = {16,128,128,64};
vector<int64_t> dim_data3;
vector<int64_t> dim_data4 = {16,128,128,64};
vector<int64_t> dim_dat5;

op_desc_ptr->AddInputDesc("grads", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddInputDesc("x1", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddInputDesc("x2", CreateTensorDesc(dim_data3, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddOutputDesc("y1", CreateTensorDesc(dim_data4, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddOutputDesc("y2", CreateTensorDesc(dim_dat5, FORMAT_NCHW, DT_FLOAT));

OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
map<string,vector<ge::Format>> format_map;
map<string,vector<ge::DataType>> data_type_map;
Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                           false, format_map, data_type_map);
EXPECT_EQ(fe::SUCCESS, result);

string expect_format = EQUAL_ORIGIN_FORMAT + EQUAL_HEAVY_FORMAT;
string x2_expect_format = EQUAL_ORIGIN_FORMAT+",ND,ND,ND,ND,ND,ND,ND,ND";
EXPECT_EQ(expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
EXPECT_EQ(expect_format, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
EXPECT_EQ(x2_expect_format, GetStrByFormatVec(format_map.at(inputs[2]->GetUniqueName())));

vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
EXPECT_EQ(expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
EXPECT_EQ(x2_expect_format, GetStrByFormatVec(format_map.at(outputs[1]->GetUniqueName())));
}

// x: 4d, y: 4d (the last two axis are not 16 aligned)
// x: 5hd, y:5hd, z:5hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_5hd_success)
{
const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
vector<int64_t> dim_data1 = {16, 32, 8, 8};
vector<int64_t> dim_data2 = {1, 32, 16, 16};
op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
map<string,vector<ge::Format>> format_map;
map<string,vector<ge::DataType>> data_type_map;
Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                           false, format_map, data_type_map);
EXPECT_EQ(fe::SUCCESS, result);

string x_expect_format = EQUAL_ORIGIN_FORMAT+",NC1HWC0,NC1HWC0";
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// x: 4d, y: 4d (the last two axis are not 16 aligned)
// x: 5hd/6d/fz, y:5hd/6d/fz, z:5hd/6d/fz
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6d_success)
{
const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
vector<int64_t> dim_data1 = {16, 32, 8, 8};
vector<int64_t> dim_data2 = {16, 32, 16, 16};
op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
map<string,vector<ge::Format>> format_map;
map<string,vector<ge::DataType>> data_type_map;
Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                           false, format_map, data_type_map);
EXPECT_EQ(fe::SUCCESS, result);

string x_expect_format = EQUAL_ORIGIN_FORMAT+",NC1HWC0,NC1HWC0,C1HWNCoC0,C1HWNCoC0,FRACTAL_Z,FRACTAL_Z";
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// x: 4d, y: 2d(the last two axis are 16 aligned)
// x: NZ, y: NZ, z: NZ
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_nz_success)
{
const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
vector<int64_t> dim_data1 = {16, 32, 16, 16};
vector<int64_t> dim_data2 = {16,16};
op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
EXPECT_NE(op_kernel_info_ptr, nullptr);

TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
map<string,vector<ge::Format>> format_map;
map<string,vector<ge::DataType>> data_type_map;
Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                           false, format_map, data_type_map);
EXPECT_EQ(fe::SUCCESS, result);
string x_expect_format = EQUAL_ORIGIN_FORMAT+",FRACTAL_NZ,FRACTAL_NZ";
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// x: 4d, y: 4d(the second last axis needs broadcast)
// do not support nz
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_nz_not_success) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
  vector<int64_t> dim_data1 = {16, 32, 16, 32};
  vector<int64_t> dim_data2 = {16, 32, 1, 32};
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin",
                                                                                                        op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  // check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);
  string x_expect_format = EQUAL_ORIGIN_FORMAT + ",NC1HWC0,NC1HWC0,C1HWNCoC0,C1HWNCoC0,FRACTAL_Z,FRACTAL_Z";
  EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
  EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
  vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
  EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}


// x: 4d, y: 2d (the last two axis are not 16 aligned)
// x: null, y:null, z:null
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_no_heavyformat_success) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
  vector<int64_t> dim_data1 = {16, 32, 16, 16};
  vector<int64_t> dim_data2 = {16, 8};
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin",
                                                                                                        op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);
  EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
  EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
  vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
  EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}


TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_dynamicinput_success) {
  const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("addN", "AddN");
  vector<int64_t> dim_data1 = {16, 16, 128, 64};
  vector<int64_t> dim_data2 = {16, 16, 128, 64};
  vector<int64_t> dim_data3 = {16, 16, 128, 64};
  op_desc_ptr->AddInputDesc("x0", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddInputDesc("x1", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data3, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin",
                                                                                                        op_desc_ptr->GetType());
  EXPECT_NE(op_kernel_info_ptr, nullptr);

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

// check inputs
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  string x_expect_format = EQUAL_ORIGIN_FORMAT + EQUAL_HEAVY_FORMAT;
  EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
  vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
  EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}


// format are all NDHWC
// original x: 4d, y: 4d z:4d (axis is less than 5)
// they are not support 6hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6hd_success) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 8, 8};
    vector<int64_t> dim_data2 = {1, 32, 16, 16};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y",
                            CreateTensorDesc(dim_data2, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z",
                             CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    string x_expect_format = EQUAL_ORIGIN_FORMAT + ",NC1HWC0,NC1HWC0";
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// format are all NDHWC
// original x: 5d, y: 5d z:5d (C of each tensor is 16 aligned)
// they do not support 6hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6hd_success_1) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 8, 8, 16};
    vector<int64_t> dim_data2 = {1, 32, 16, 16, 32};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y",
                            CreateTensorDesc(dim_data2, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z",
                             CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    string x_expect_format = EQUAL_ORIGIN_FORMAT;
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// format are all NDHWC
// original x: 5d, y: 5d z:5d (axis N and H need to BroadCast)
// they support 6hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6hd_success_2) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {32, 32, 15, 8, 33};
    vector<int64_t> dim_data2 = {16, 32, 8, 8, 33};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y",
                            CreateTensorDesc(dim_data2, FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z",
                             CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    string x_expect_format =
            EQUAL_ORIGIN_FORMAT +
            ",NC1HWC0,NC1HWC0,FRACTAL_NZ,FRACTAL_NZ,NDC1HWC0,NDC1HWC0";
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// format are all NDHWC
// original x: 5d, y: 5d z:5d (axis N\D\H need to BroadCast)
// they support 6hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6hd_success_3) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {1, 16, 1, 8, 32};
    vector<int64_t> dim_data2 = {16, 32, 8, 8, 32};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, ge::FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y",
                            CreateTensorDesc(dim_data2, ge::FORMAT_NDHWC, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z",
                             CreateTensorDesc(dim_data1, FORMAT_NDHWC, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    string x_expect_format =
            EQUAL_ORIGIN_FORMAT +
            ",NC1HWC0,NC1HWC0,FRACTAL_NZ,FRACTAL_NZ,NDC1HWC0,NDC1HWC0";
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}

// format are all NCDHW
// original x: 5d, y: 5d z:5d (axis N\D\H need to BroadCast)
// they support 6hd
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, op_broadcast_selector_6hd_success_4) {
    const OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {1, 32, 16, 1, 8};
    vector<int64_t> dim_data2 = {16, 32, 32, 8, 8};
    op_desc_ptr->AddInputDesc("x",
                            CreateTensorDesc(dim_data1, ge::FORMAT_NCDHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y",
                            CreateTensorDesc(dim_data2, ge::FORMAT_NCDHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z",
                             CreateTensorDesc(dim_data1, ge::FORMAT_NCDHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr;
   op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(
            "tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
            std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    // check inputs
    map<string, vector<ge::Format>> format_map;
    map<string, vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr.get()), false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    string x_expect_format =
            EQUAL_ORIGIN_FORMAT +
            ",NC1HWC0,NC1HWC0,NDC1HWC0,NDC1HWC0";
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format,
              GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}
TEST_F(FormatDtypeSelectorManagerBroadcastUTest, GetDataTypeFromOpDescByKey_fail) {
  using OpStoreAdapterManagerPtr = std::shared_ptr<fe::OpStoreAdapterManager>;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
  using FormatDtypeOpBuiltinSelectorPtr = std::shared_ptr<FormatDtypeOpBuiltinSelector>;
  FormatDtypeOpBuiltinSelectorPtr ptr = std::make_shared<FormatDtypeOpBuiltinSelector>(op_store_adapter_manager_ptr_);
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("equal", "Equal");
  vector<ge::DataType> result;
  Status ret = ptr->GetDataTypeFromOpDescByKey(*op_desc_ptr, "test", result);
  EXPECT_EQ(ret, fe::FAILED);
  std::map<std::string, vector<ge::DataType>> data_type_map = { {"test", {DT_FLOAT16} }};
  (void)op_desc_ptr->SetExtAttr("ext_dynamic_datatype", data_type_map);
  ret = ptr->GetDataTypeFromOpDescByKey(*op_desc_ptr, "test_fail", result);
  EXPECT_EQ(ret, fe::FAILED);
  ret = ptr->GetDataTypeFromOpDescByKey(*op_desc_ptr, "test", result);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FormatDtypeSelectorManagerBroadcastUTest, get_reduce_new_format_dtype_test) {
  using FormatDtypeOpBuiltinSelectorPtr = std::shared_ptr<FormatDtypeOpBuiltinSelector>;
  FormatDtypeOpBuiltinSelectorPtr ptr = std::make_shared<FormatDtypeOpBuiltinSelector>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = std::make_shared<OpKernelInfo>("conv");
  OpPattern op_pattern_;
  op_kernel_info_ptr->op_pattern_ = OpPattern::OP_PATTERN_REDUCE;
  ge::OpDesc op_desc;
  string input_key;
  string output_key;
  map<string, vector<ge::Format>> format_res;
  map<string, vector<ge::DataType>> data_type_res;
  vector<ge::Format> format_vec1 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
  vector<ge::Format> format_vec2 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
  vector<ge::Format> format_vec3 = {FORMAT_NC1HWC0_C04, FORMAT_FRACTAL_Z_C04};
  format_res.emplace("x", format_vec1);
  format_res.emplace("y", format_vec2);
  format_res.emplace("z", format_vec3);
  Status result = ptr->GetReduceNewFormatDType(op_kernel_info_ptr, op_desc, input_key,
                                               output_key, format_res, data_type_res);
  EXPECT_EQ(result, fe::SUCCESS);
}

TEST_F(FormatDtypeSelectorManagerBroadcastUTest, check_part_nonscalar_inputs_test) {
  FormatProccessInputArg input_arg(FORMAT_NCHW, DT_FLOAT, ge::GeShape({1}), ge::FORMAT_FRACTAL_Z, 2);
  BroadcastProcessFractalZ broadcastprocessfractalz;
  Status ret = broadcastprocessfractalz.CheckPartNonScalarInputs(input_arg);
  EXPECT_EQ(ret, false);
}

TEST_F(FormatDtypeSelectorManagerBroadcastUTest, check_part_nonscalar_inputs_test1) {
  FormatProccessInputArg input_arg(FORMAT_NCHW, DT_FLOAT, ge::GeShape({1}), ge::FORMAT_FRACTAL_Z_3D, 2);
  BroadcastProcessFractalZ3D broadcastprocessfractalz3d;
  Status ret = broadcastprocessfractalz3d.CheckPartNonScalarInputs(input_arg);
  EXPECT_EQ(ret, false);
}
