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
#include "format_selector/builtin/process/format_process_registry.h"
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

class FormatDtypeSelectorManagerKernelUTest : public testing::Test{
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

public:
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
 };


TEST_F(FormatDtypeSelectorManagerKernelUTest, init_format_success) {
    OpDescPtr opdesc_ptr = std::make_shared<OpDesc>("test", "KernelFormatOp1");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_OP_KERNEL, op_kernel_info_ptr->GetOpPattern());
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ("input0.x", inputs[0]->GetUniqueName());
    EXPECT_EQ("output0.y", outputs[0]->GetUniqueName());
    EXPECT_EQ(3, inputs[0]->GetFormat().size());
    EXPECT_EQ(3, outputs[0]->GetFormat().size());

    opdesc_ptr = std::make_shared<OpDesc>("test", "KernelFormatOp2");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_OP_KERNEL, op_kernel_info_ptr->GetOpPattern());

    opdesc_ptr = std::make_shared<OpDesc>("test", "FormatAgnosticOp");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
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
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_BROADCAST, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(3, inputs[0]->GetDataType().size());
    EXPECT_EQ(3, outputs[0]->GetDataType().size());

    opdesc_ptr = std::make_shared<OpDesc>("test", "ReduceOp");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", opdesc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_REDUCE, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(2, inputs[0]->GetDataType().size());
    EXPECT_EQ(2, outputs[0]->GetDataType().size());
}

TEST_F(FormatDtypeSelectorManagerKernelUTest, init_customize_format_success) {
    OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithFormat");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_OP_CUSTOMIZE, op_kernel_info_ptr->GetOpPattern());
    vector<InputOrOutputInfoPtr> inputs = op_kernel_info_ptr->GetAllInputInfo();
    vector<InputOrOutputInfoPtr> outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, inputs[0]->GetDataType().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetDataType().size());

    op_desc_ptr= std::make_shared<OpDesc>("test", "DynamicFormatOpWithoutFormat");
    op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(OP_PATTERN_OP_CUSTOMIZE, op_kernel_info_ptr->GetOpPattern());
    inputs = op_kernel_info_ptr->GetAllInputInfo();
    outputs = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(0, inputs[0]->GetFormat().size());
    EXPECT_EQ(0, inputs[0]->GetDataType().size());
    EXPECT_EQ(0, outputs[0]->GetFormat().size());
    EXPECT_EQ(0, outputs[0]->GetDataType().size());
}

TEST_F(FormatDtypeSelectorManagerKernelUTest, op_kernel_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "KernelFormatOp1");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

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

TEST_F(FormatDtypeSelectorManagerKernelUTest, op_formatagnostic_selector_query_success)
{
    const OpDescPtr op_desc_ptr= std::make_shared<OpDesc>("test", "FormatAgnosticOp");
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-builtin", op_desc_ptr->GetType());
    EXPECT_NE(op_kernel_info_ptr, nullptr);

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
TEST_F(FormatDtypeSelectorManagerKernelUTest, FormatProcessExists_fail)
{
  FormatProccessArgs args;
  Status result = FormatProcessExists(args);
  EXPECT_EQ(result, false);
}