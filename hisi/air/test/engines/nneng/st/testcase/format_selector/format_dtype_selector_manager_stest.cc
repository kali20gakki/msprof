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
#define private public
#define protected public
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_store/ops_kernel_manager.h"

#include "graph/ge_tensor.h"
#include "fusion_manager/fusion_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "common/fe_type_utils.h"
#include "format_selector/builtin/reduce/reduce_format_selector/reduce_process_nz.h"
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
class FormatDtypeSelectorManagerSTest : public testing::Test{
protected:
    static void SetUpTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    static void TearDownTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    // Some expensive resource shared by all tests.
    void SetUp(){
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
        std::map<std::string, std::string> options;
        OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
        fe_ops_kernel_info_store_ptr->Initialize(options);
        cout << "a test Set Up" << endl;
    }
    void TearDown(){
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
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input1_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input2_desc_ptr;
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr;
    shared_ptr<ge::OpDesc> op_desc_ptr;
    const string EQUAL_ORIGIN_FORMAT="NCHW,NCHW,NHWC,NHWC,HWCN,HWCN,CHWN,CHWN,NDHWC,NDHWC,NCDHW,NCDHW,DHWCN,DHWCN,DHWNC,DHWNC,ND,ND";
    const string EQUAL_HEAVY_FORMAT=",NC1HWC0,NC1HWC0,C1HWNCoC0,C1HWNCoC0,FRACTAL_Z,FRACTAL_Z,FRACTAL_NZ,FRACTAL_NZ";
};


bool SelectTbeOpFormatStub(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    op_format_dtype_str = "{\"input0\":{\"name\":\"x\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"},\"output0\":{\"name\":\"y\",\"format\":\"ND,NCHW\", \"dtype\":\"float,float16\"}}";
    return true;
}

bool SelectTbeOpFormatStub1(const te::TbeOpInfo &tbe_op_info, string &op_format_dtype_str)
{
    op_format_dtype_str = "";
    return true;
}

TEST_F(FormatDtypeSelectorManagerSTest, init_format_success) {
    OpDescPtr opdesc_ptr = std::make_shared<OpDesc>("test", "KernelFormatOp1");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_OP_KERNEL, op_kernel_info_ptr->GetOpPattern());
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ("input0.x", inputs[0]->GetUniqueName());
    EXPECT_EQ("output0.y", outputs[0]->GetUniqueName());
    EXPECT_EQ(3, inputs[0]->GetFormat().size());
    EXPECT_EQ(3, outputs[0]->GetFormat().size());

    opdesc_ptr = std::make_shared<OpDesc>("test", "KernelFormatOp2");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_OP_KERNEL, op_kernel_info_ptr->GetOpPattern());

    opdesc_ptr = std::make_shared<OpDesc>("test", "FormatAgnosticOp");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_FORMAT_AGNOSTIC, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    int format_size = 2*(FE_ORIGIN_FORMAT_VECTOR.size() + FE_HEAVY_FORMAT_VECTOR.size());
    EXPECT_EQ(format_size, inputs[0]->GetFormat().size());
    EXPECT_EQ(format_size, outputs[0]->GetFormat().size());

    opdesc_ptr = std::make_shared<OpDesc>("test", "BroadcastOp");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_BROADCAST, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(3, inputs[0]->GetDataType().size());
    EXPECT_EQ(3, outputs[0]->GetDataType().size());

    opdesc_ptr = std::make_shared<OpDesc>("test", "ReduceOp");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_REDUCE, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(2, inputs[0]->GetDataType().size());
    EXPECT_EQ(2, outputs[0]->GetDataType().size());
}

TEST_F(FormatDtypeSelectorManagerSTest, init_customize_format_success) {
    OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithFormat");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_OP_CUSTOMIZE, op_kernel_info_ptr->GetOpPattern());
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, inputs[0]->GetDataType().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetDataType().size());

    op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_EQ(OP_PATTERN_OP_CUSTOMIZE, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, inputs[0]->GetDataType().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetDataType().size());
}

TEST_F(FormatDtypeSelectorManagerSTest, op_kernel_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "KernelFormatOp1");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    map<string,vector<ge::Format>> format_map;
    map<string,vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                               false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);
    vector<ge::Format> format_vec = format_map.at(inputs[0]->GetUniqueName());
    vector<ge::DataType> data_type_vec = data_type_map.at(inputs[0]->GetUniqueName());
    EXPECT_EQ(ge::FORMAT_NCHW, format_vec[0]);
    EXPECT_EQ(ge::FORMAT_NC1HWC0, format_vec[1]);
    EXPECT_EQ(ge::FORMAT_ND, format_vec[2]);
    EXPECT_EQ(ge::DT_FLOAT16, data_type_vec[0]);
    EXPECT_EQ(ge::DT_FLOAT, data_type_vec[1]);
    EXPECT_EQ(ge::DT_FLOAT, data_type_vec[2]);
}

TEST_F(FormatDtypeSelectorManagerSTest, op_formatagnostic_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "FormatAgnosticOp");
    OpKernelInfoPtr op_kernel_info_ptr;
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    map<string,vector<ge::Format>> format_map;
    map<string,vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                               false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);
    vector<ge::Format> format_vec = format_map.at(inputs[0]->GetUniqueName());
    vector<ge::DataType> data_type_vec = data_type_map.at(inputs[0]->GetUniqueName());
    string new_formats_str = GetStrByFormatVec(format_vec);
    string new_data_types_str = GetStrByDataTypeVec(data_type_vec);
    string expect_format_str = "NC1HWC0_C04,NC1HWC0_C04,NC1HWC0,NC1HWC0,C1HWNCoC0,C1HWNCoC0,FRACTAL_Z,FRACTAL_Z,FRACTAL_NZ,FRACTAL_NZ,NDC1HWC0,NDC1HWC0,FRACTAL_Z_3D,FRACTAL_Z_3D,FRACTAL_Z_3D_TRANSPOSE,FRACTAL_Z_3D_TRANSPOSE,NCHW,NCHW,NHWC,NHWC,HWCN,HWCN,CHWN,CHWN,NDHWC,NDHWC,NCDHW,NCDHW,DHWCN,DHWCN,DHWNC,DHWNC,ND,ND";
    EXPECT_EQ(expect_format_str, new_formats_str);
    string expect_data_type_str = "DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT,DT_FLOAT16,DT_FLOAT";
    EXPECT_EQ(expect_data_type_str, new_data_types_str);
    format_vec = format_map.at(outputs[0]->GetUniqueName());
    data_type_vec = data_type_map.at(outputs[0]->GetUniqueName());
    new_formats_str = GetStrByFormatVec(format_vec);
    new_data_types_str = GetStrByDataTypeVec(data_type_vec);
    EXPECT_EQ(expect_format_str, new_formats_str);
    EXPECT_EQ("DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32,DT_INT8,DT_INT32",
              new_data_types_str);
}

TEST_F(FormatDtypeSelectorManagerSTest, op_customize_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    vector<int64_t> dim_data = {1, 2, 3, 4};
    GeShape shape_data(dim_data);
    GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
    op_desc_ptr->AddInputDesc("x", data_desc);
    op_desc_ptr->AddOutputDesc("y", data_desc);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
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

TEST_F(FormatDtypeSelectorManagerSTest, op_customize_selector_query_failed)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    vector<int64_t> dim_data = {1, 2, 3, 4};
    GeShape shape_data(dim_data);
    GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
    op_desc_ptr->AddInputDesc("x", data_desc);
    op_desc_ptr->AddOutputDesc("y", data_desc);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    tbe_adapter_ptr->SelectTbeOpFormat = SelectTbeOpFormatStub1;
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

// grads/x1: 4d, x2: scalar
// grads/x1: 5HD/6D/FZ/NZ, x2: ND
// y1:5HD/6D/FZ/NZ, y2: ND
TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_partscalar_success)
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
TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_5hd_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 8, 8};
    vector<int64_t> dim_data2 = {1, 32, 16, 16};
    op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
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
TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_6d_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 8, 8};
    vector<int64_t> dim_data2 = {16, 32, 16, 16};
    op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());

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
TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_nz_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 16, 16};
    vector<int64_t> dim_data2 = {16,16};
    op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
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

// x: 4d, y: 2d (the last two axis are not 16 aligned)
// x: null, y:null, z:null
TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_no_heavyformat_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("equal", "Equal");
    vector<int64_t> dim_data1 = {16, 32, 16, 16};
    vector<int64_t> dim_data2 = {16, 8};
    op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("y", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("z", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
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
    EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(inputs[1]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(EQUAL_ORIGIN_FORMAT, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}


TEST_F(FormatDtypeSelectorManagerSTest, op_broadcast_selector_dynamicinput_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("addN", "AddN");
    vector<int64_t> dim_data1 = {16,16,128,64};
    vector<int64_t> dim_data2 = {16,16,128,64};
    vector<int64_t> dim_data3 = {16,16,128,64};
    op_desc_ptr->AddInputDesc("x0", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddInputDesc("x1", CreateTensorDesc(dim_data2, FORMAT_NCHW, DT_FLOAT));
    op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data3, FORMAT_NCHW, DT_FLOAT));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
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

    string x_expect_format = EQUAL_ORIGIN_FORMAT + EQUAL_HEAVY_FORMAT;
    EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(inputs[0]->GetUniqueName())));
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(x_expect_format, GetStrByFormatVec(format_map.at(outputs[0]->GetUniqueName())));
}
TEST_F(FormatDtypeSelectorManagerSTest, op_reduce_selector_5hd_6hd_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("reduce1", "ReduceOp");
    vector<int64_t> dim_data1 = {16, 32, 16, 16};
    vector<int64_t> dim_data2 = {16, 8, 1 ,1};
    op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));
    op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

    ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
    ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

    // check inputs
    map<string,vector<ge::Format>> format_map;
    map<string,vector<ge::DataType>> data_type_map;
    Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                               false, format_map, data_type_map);
    EXPECT_EQ(fe::SUCCESS, result);

    bool support_flag=false;
    map<string, vector<ge::Format>>::reverse_iterator iter;
    for(iter = format_map.rbegin(); iter != format_map.rend(); iter++){
      vector<ge::Format>::iterator ret;
      ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
      if (ret != iter->second.end()) {
        support_flag=true;
      }
      EXPECT_EQ(support_flag, true);

      support_flag=false;
      ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_C1HWNCoC0);
      if (ret != iter->second.end()) {
        support_flag=true;
      }
      EXPECT_EQ(support_flag, true);

      support_flag=false;
      ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_Z);
      if (ret != iter->second.end()) {
        support_flag=true;
      }
      EXPECT_EQ(support_flag, true);
    }
}

TEST_F(FormatDtypeSelectorManagerSTest, op_reduce_selector_nz_success)
{
  const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 16};
  vector<int64_t> dim_data2 = {16, 8, 1 ,1};
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));
  op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT16));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, false);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {0, 1});

  // check inputs
  map<string,vector<ge::Format>> format_map;
  map<string,vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  bool support_flag=false;
  map<string, vector<ge::Format>>::reverse_iterator iter;
  for(iter = format_map.rbegin(); iter != format_map.rend(); iter++){
    vector<ge::Format>::iterator ret;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, true);
  }
}

TEST_F(FormatDtypeSelectorManagerSTest, op_reduce_selector_5hd_6hd_failed1)
{
  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend910A";

  const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 15};
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, false);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string,vector<ge::Format>> format_map;
  map<string,vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::reverse_iterator iter;
  for(iter = format_map.rbegin(); iter != format_map.rend(); iter++){
    bool support_flag=false;
    vector<ge::Format>::iterator ret;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    if (iter->first.find("input") != -1) {
        EXPECT_EQ(support_flag, true);
    } else {
        EXPECT_EQ(support_flag, true);
    }

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);
  }
  PlatformInfoManager::Instance().platform_info_map_.clear();
}

TEST_F(FormatDtypeSelectorManagerSTest, op_reduce_selector_5hd_6hd_failed2)
{
  const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32, 16, 16};
  GeShape shape_data1(dim_data1);
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());

  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {0, 1});

  // check inputs
  map<string,vector<ge::Format>> format_map;
  map<string,vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::iterator iter;
  for(iter = format_map.begin(); iter != format_map.end(); iter++){
    bool support_flag=false;
    vector<ge::Format>::iterator ret;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    // EXPECT_EQ(support_flag, true);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, true);
  }
}

TEST_F(FormatDtypeSelectorManagerSTest, op_reduce_selector_5hd_6hd_failed3)
{
  const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("reduce1", "ReduceOp");
  vector<int64_t> dim_data1 = {16, 32};
  GeShape shape_data1(dim_data1);
  op_desc_ptr->AddInputDesc("x", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));
  op_desc_ptr->AddOutputDesc("y", CreateTensorDesc(dim_data1, FORMAT_NCHW, DT_FLOAT));

  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

  FormatDtypeQuerier format_dtype_querier(op_store_adapter_manager_ptr_);
  vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();

  ge::AttrUtils::SetBool(*(op_desc_ptr.get()), KEEP_DIMS, true);
  ge::AttrUtils::SetListInt(*(op_desc_ptr.get()), AXES_ATTR_NAME, {2, 3});

  // check inputs
  map<string,vector<ge::Format>> format_map;
  map<string,vector<ge::DataType>> data_type_map;
  Status result = format_dtype_querier.GetSupportFormatAndDtype(op_kernel_info_ptr, *(op_desc_ptr.get()),
                                                             false, format_map, data_type_map);
  EXPECT_EQ(fe::SUCCESS, result);

  map<string, vector<ge::Format>>::reverse_iterator iter;
  for(iter = format_map.rbegin(); iter != format_map.rend(); iter++){
    bool support_flag=false;
    vector<ge::Format>::iterator ret;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_NC1HWC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_C1HWNCoC0);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_Z);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, false);

    support_flag=false;
    ret = std::find(iter->second.begin(), iter->second.end(), ge::FORMAT_FRACTAL_NZ);
    if (ret != iter->second.end()) {
      support_flag=true;
    }
    EXPECT_EQ(support_flag, true);
  }
}

TEST_F(FormatDtypeSelectorManagerSTest, filter_c04_format_01)
{
    std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
    std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
    std::map<std::string, vector<ge::Format>> format_map;
    std::map<std::string, vector<ge::DataType>> data_type_map;

    Status status = customize_selector_ptr->FilterC04Format(format_map, data_type_map);
    EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(FormatDtypeSelectorManagerSTest, filter_c04_format_02)
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

TEST_F(FormatDtypeSelectorManagerSTest, filter_c04_format_03)
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

TEST_F(FormatDtypeSelectorManagerSTest, filter_c04_format_04)
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

TEST_F(FormatDtypeSelectorManagerSTest, filter_c04_format_05)
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

TEST_F(FormatDtypeSelectorManagerSTest, converage_01) {
  std::shared_ptr<FormatDtypeSelectorBase> base =
      std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  auto op_desc = std::make_shared<OpDesc>("test", "Test");
  string key = "1";
  vector<ge::Format> result_fm;
  vector<ge::DataType> result_dt;
  EXPECT_EQ(fe::FAILED, base->GetFormatFromOpDescByKey(*op_desc, key, result_fm));
  EXPECT_EQ(fe::FAILED, base->GetDataTypeFromOpDescByKey(*op_desc, key, result_dt));


  map<string, vector<ge::Format>> format_map = {{"2", {ge::FORMAT_ND}}};
  op_desc->SetExtAttr("ext_dynamic_format", format_map);

  map<string, vector<ge::DataType>> dt_map = {{"2", {ge::DT_FLOAT}}};
  op_desc->SetExtAttr("ext_dynamic_datatype", dt_map);

  EXPECT_EQ(fe::FAILED, base->GetFormatFromOpDescByKey(*op_desc, key, result_fm));
  EXPECT_EQ(fe::FAILED, base->GetDataTypeFromOpDescByKey(*op_desc, key, result_dt));

  OpKernelInfoPtr op_kernel_info_ptr = nullptr;
  HeavyFormatInfo heavy_format_info;
  bool is_dynamic_check = false;

  std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
      std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  EXPECT_EQ(fe::FAILED, customize_selector_ptr->SetSupportFormatDtype(op_kernel_info_ptr, heavy_format_info,
                                                                      *op_desc, is_dynamic_check));
}

TEST_F(FormatDtypeSelectorManagerSTest, converage_02) {
  std::shared_ptr<FormatDtypeOpCustomizeSelector> customize_selector_ptr =
      std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr_);
  auto op_desc = std::make_shared<OpDesc>("test", "Test");

  std::map<std::string, vector<ge::Format>> format_map = {
      {"test", {ge::FORMAT_NCHW}}
  };

  std::map<std::string, vector<ge::DataType>> dt_map = {
      {"test", {ge::DT_FLOAT}}
  };

  customize_selector_ptr->SaveDynamicFormatDtype(format_map, dt_map, *op_desc);
}

TEST_F(FormatDtypeSelectorManagerSTest, get_reduce_new_format_dtype_test) {
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

TEST_F(FormatDtypeSelectorManagerSTest, check_part_nonscalar_inputs_test) {
  FormatProccessInputArg input_arg(FORMAT_NCHW, DT_FLOAT, ge::GeShape({1}), ge::FORMAT_FRACTAL_Z, 2);
  BroadcastProcessFractalZ broadcastprocessfractalz;
  Status ret = broadcastprocessfractalz.CheckPartNonScalarInputs(input_arg);
  EXPECT_EQ(ret, false);
}

TEST_F(FormatDtypeSelectorManagerSTest, check_part_nonscalar_inputs_test1) {
  FormatProccessInputArg input_arg(FORMAT_NCHW, DT_FLOAT, ge::GeShape({1}), ge::FORMAT_FRACTAL_Z_3D, 2);
  BroadcastProcessFractalZ3D broadcastprocessfractalz3d;
  Status ret = broadcastprocessfractalz3d.CheckPartNonScalarInputs(input_arg);
  EXPECT_EQ(ret, false);
}

TEST_F(FormatDtypeSelectorManagerSTest, check_origin_format_and_shape_test) {
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