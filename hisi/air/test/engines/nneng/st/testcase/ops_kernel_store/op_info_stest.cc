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
#include <graph/ge_attr_value.h>

#define private public
#define protected public
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

#include "graph/ge_tensor.h"
#include "ops_kernel_store/sub_ops_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "common/configuration.h"
#include "ops_store/sub_op_info_store.h"
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
using fe::SubOpsStore;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

class STEST_OP_KERNEL_INFO : public testing::Test{
protected:
  static void SetUpTestCase() {
      cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
  }
  static void TearDownTestCase() {
      cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
  }
  // Some expensive resource shared by all tests.
  virtual void SetUp(){
      op_desc_ptr = make_shared<ge::OpDesc>();
      input0_desc_ptr = make_shared<ge::GeTensorDesc>();
      input1_desc_ptr = make_shared<ge::GeTensorDesc>();
      input2_desc_ptr = make_shared<ge::GeTensorDesc>();
      output0_desc_ptr = make_shared<ge::GeTensorDesc>();
      std::map<std::string, std::string> options;

      op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
      TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
      op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

      sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
      op_kernel_info_ptr = make_shared<fe::OpKernelInfo>("conv");
      // FEOpsKernelInfoStore initialize;
      FEOpsStoreInfo tbe_custom {
              2,
              "tbe_custom_opinfo",
              EN_IMPL_CUSTOM_TBE,
              "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
              "xx",
              false,
              false};
      sub_ops_store_ptr->SetSubStoreType("tbe_custom_opinfo");
      sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
      sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);

      sub_ops_kernel_ptr = std::make_shared<fe::SubOpInfoStore>(tbe_custom);
      sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);


      op_desc_ptr->SetName("tbe_conv");
      op_desc_ptr->SetType("conv");
      ge::DataType set_dtype = ge::DT_FLOAT16;
      ge::Format set_format = ge::FORMAT_ND;
      std::vector<int64_t> shape_vec{256,256,512};
      ge::GeShape shape_desc = GeShape(shape_vec);

      input0_desc_ptr->SetDataType(set_dtype);
      input0_desc_ptr->SetFormat(set_format);
      input0_desc_ptr->SetShape(shape_desc);
      op_desc_ptr->AddInputDesc("x", input0_desc_ptr->Clone());

      std::vector<int64_t> shape_vec1{256,256,512};
      ge::GeShape shape_desc1 = GeShape(shape_vec1);
      input1_desc_ptr->SetDataType(set_dtype);
      input1_desc_ptr->SetFormat(set_format);
      input1_desc_ptr->SetShape(shape_desc1);
      op_desc_ptr->AddInputDesc("y", input1_desc_ptr->Clone());

      std::vector<int64_t> shape_vec2{256,256,512};
      ge::GeShape shape_desc2 = GeShape(shape_vec2);
      input2_desc_ptr->SetDataType(set_dtype);
      input2_desc_ptr->SetFormat(set_format);
      input2_desc_ptr->SetShape(shape_desc2);
      op_desc_ptr->AddInputDesc("x1", input2_desc_ptr->Clone());

      output0_desc_ptr->SetDataType(set_dtype);
      output0_desc_ptr->SetFormat(set_format);
      output0_desc_ptr->SetShape(shape_desc);
      op_desc_ptr->AddOutputDesc("z", output0_desc_ptr->Clone());

      cout << "a test SetUP" << endl;
  }
  virtual void TearDown(){
      cout << "a test SetUP" << endl;
      sub_ops_store_ptr->FinalizeSubStore();
      sub_ops_store_ptr.reset();
      sub_ops_kernel_ptr->Finalize();
      sub_ops_kernel_ptr.reset();
      Configuration& config = Configuration::Instance(fe::AI_CORE_NAME);
      config.is_init_ = false;
      config.content_map_.clear();
      map<string, string> options;
      string soc_version = "Ascend910A";
      config.Initialize(options, soc_version);

  }
public:
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::SubOpsStore> sub_ops_store_ptr;
  shared_ptr<fe::SubOpInfoStore> sub_ops_kernel_ptr;
  shared_ptr<fe::OpKernelInfo> op_kernel_info_ptr;
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr;
  shared_ptr<ge::GeTensorDesc> input1_desc_ptr;
  shared_ptr<ge::GeTensorDesc> input2_desc_ptr;
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr;
  shared_ptr<ge::OpDesc> op_desc_ptr;
};

TEST_F(STEST_OP_KERNEL_INFO, op_input_or_output_finalize_twice)
{
    InputOrOutputInfoPtr InfoPtr = std::make_shared<InputOrOutputInfo>("x");
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.FinalizeInputAndOutput(InfoPtr);

    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO, sub_ops_kernel_init)
{
    Configuration::Instance(fe::AI_CORE_NAME).modify_mixlist_path_ = "./air/test/engines/nneng/config/fe_config/op_info.json";
    sub_ops_kernel_ptr->init_flag_ = false;
    Status ret = sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);

    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO, sub_ops_kernel_init1)
{
    Configuration::Instance(fe::AI_CORE_NAME).modify_mixlist_path_ = "./air/test/engines/nneng/config/fe_config/op_info1.json";
    sub_ops_kernel_ptr->init_flag_ = false;
    Status ret = sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);

    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_init_and_finalize_twice)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("conv");
    op_info_ptr->init_flag_ = true;
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret1 = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
    Status ret2 = op_kernel_info_constructor.FinalizeOpKernelInfo(op_info_ptr);
    Status ret3 = op_kernel_info_constructor.FinalizeOpKernelInfo(op_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret1);
    EXPECT_EQ(fe::SUCCESS, ret2);
    EXPECT_EQ(fe::SUCCESS, ret3);
}

TEST_F(STEST_OP_KERNEL_INFO, op_tensor_desc_init)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret =  op_kernel_info_constructor.ParseInputAndOutputFromOpContent(op_content, op_kernel_info_ptr);
    EXPECT_NE(0, op_kernel_info_ptr->input_infos_.size());
    EXPECT_NE(0, op_kernel_info_ptr->output_infos_.size());
    op_kernel_info_ptr->init_flag_ = true;
    /* Clear memory */
    ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_EQ("", op_kernel_info_ptr->op_type_);
    EXPECT_EQ(true, op_kernel_info_ptr->input_infos_.empty());
    EXPECT_EQ(true, op_kernel_info_ptr->output_infos_.empty());
    EXPECT_EQ(true, op_kernel_info_ptr->attrs_info_.empty());
    EXPECT_EQ(false, op_kernel_info_ptr->init_flag_);
}

TEST_F(STEST_OP_KERNEL_INFO, op_init_attr_value)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitAttrValue(op_content, op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_NE(0, op_kernel_info_ptr->attrs_info_.size());
    EXPECT_EQ(false, op_kernel_info_ptr->attrs_info_[0]->is_support_all_value_);
    EXPECT_EQ("transposX", op_kernel_info_ptr->attrs_info_[0]->attr_name_);
    EXPECT_EQ(true, op_kernel_info_ptr->attrs_info_[0]->is_required_);
    EXPECT_EQ(false, op_kernel_info_ptr->attrs_info_[0]->is_default_value_defined_);
    EXPECT_EQ(ge::GeAttrValue::VT_INT, op_kernel_info_ptr->attrs_info_[0]->dtype_);


    op_kernel_info_ptr->init_flag_ = true;
    /* Clear memory */
    ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_EQ("", op_kernel_info_ptr->op_type_);
    EXPECT_EQ(true, op_kernel_info_ptr->input_infos_.empty());
    EXPECT_EQ(true, op_kernel_info_ptr->output_infos_.empty());
    EXPECT_EQ(true, op_kernel_info_ptr->attrs_info_.empty());
    EXPECT_EQ(false, op_kernel_info_ptr->init_flag_);

}

TEST_F(STEST_OP_KERNEL_INFO, op_init_op_info)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitOpInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    op_kernel_info_ptr->init_flag_ = true;
    /* Clear memory */
    ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_EQ("", op_kernel_info_ptr->op_type_);
    EXPECT_EQ(0, op_kernel_info_ptr->input_infos_.size());
    EXPECT_EQ(0, op_kernel_info_ptr->output_infos_.size());
    EXPECT_EQ(0, op_kernel_info_ptr->attrs_info_.size());
    EXPECT_EQ(false, op_kernel_info_ptr->init_flag_);
}

TEST_F(STEST_OP_KERNEL_INFO, op_info_initialize)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);

    EXPECT_EQ(fe::SUCCESS, ret);
    /* Clear memory */
    ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_EQ("", op_kernel_info_ptr->op_type_);
    EXPECT_EQ(0, op_kernel_info_ptr->input_infos_.size());
    EXPECT_EQ(0, op_kernel_info_ptr->output_infos_.size());
    EXPECT_EQ(0, op_kernel_info_ptr->attrs_info_.size());
    EXPECT_EQ(false, op_kernel_info_ptr->init_flag_);

}

TEST_F(STEST_OP_KERNEL_INFO, input_or_output_info_getter)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    InputOrOutputInfoPtr input0;
    op_kernel_info_ptr->GetInputInfoByName("x", input0);
    input0->GetName();
    input0->GetDataType();
    input0->GetFormat();
    EXPECT_EQ(DYNAMIC, input0->GetParamType());

    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, get_all_input_or_output_info)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    const vector<InputOrOutputInfoPtr> all_input_info = op_kernel_info_ptr->GetAllInputInfo();
    const vector<InputOrOutputInfoPtr> all_output_info = op_kernel_info_ptr->GetAllOutputInfo();
    EXPECT_EQ(3, all_input_info.size());
    EXPECT_EQ(1, all_output_info.size());

    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, get_input_or_output_info_by_name)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    InputOrOutputInfoPtr input_ptr;
    Status ret1 = op_kernel_info_ptr->GetInputInfoByName("x", input_ptr);
    ASSERT_EQ(fe::SUCCESS, ret1);
    EXPECT_EQ("x", input_ptr->GetName());

    InputOrOutputInfoPtr output_ptr;
    Status ret2 = op_kernel_info_ptr->GetOutputInfoByName("z", output_ptr);
    ASSERT_EQ(fe::SUCCESS, ret2);
    EXPECT_EQ("z", output_ptr->GetName());

    InputOrOutputInfoPtr input1_ptr;
    Status ret3 = op_kernel_info_ptr->GetInputInfoByName("x1", input1_ptr);
    EXPECT_NE(fe::SUCCESS, ret3);

    InputOrOutputInfoPtr input2_ptr;
    Status ret4 = op_kernel_info_ptr->GetInputInfoByName("m", input2_ptr);
    EXPECT_NE(fe::SUCCESS, ret4);

    Status ret5 = op_kernel_info_ptr->GetOutputInfoByName("m", input2_ptr);
    EXPECT_NE(fe::SUCCESS, ret5);

    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, get_op_type)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    string op_type = op_kernel_info_ptr->GetOpType();
    EXPECT_NE("abs", op_type);
    EXPECT_EQ("conv", op_type);

    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, get_unknown_shape_format)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    EXPECT_EQ(ret, SUCCESS);
    string op_type = op_kernel_info_ptr->GetOpType();
    for (auto input_info_ptr : op_kernel_info_ptr->GetAllInputInfo()) {
        EXPECT_EQ(input_info_ptr->GetUnknownShapeFormat().size(), 4);
        EXPECT_EQ(input_info_ptr->GetUnknownShapeDataType().size(), 4);
        for (auto dtype : input_info_ptr->GetUnknownShapeDataType()) {
            EXPECT_NE(dtype, ge::DT_FLOAT16);
        }
    }

    for (auto output_info_ptr : op_kernel_info_ptr->GetAllOutputInfo()) {
        EXPECT_EQ(output_info_ptr->GetUnknownShapeFormat().size(), 4);
        EXPECT_EQ(output_info_ptr->GetUnknownShapeDataType().size(), 4);
        for (auto dtype : output_info_ptr->GetUnknownShapeDataType()) {
            EXPECT_NE(dtype, ge::DT_FLOAT);
        }
    }
    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, get_unknown_shape_format_format_agnostic_op)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("formatAgnosticOp", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    EXPECT_EQ(ret, SUCCESS);
    for (auto input_info_ptr : op_kernel_info_ptr->GetAllInputInfo()) {
        EXPECT_GT(input_info_ptr->GetUnknownShapeFormat().size(), 10);
        EXPECT_GT(input_info_ptr->GetUnknownShapeDataType().size(), 10);
    }

    for (auto output_info_ptr : op_kernel_info_ptr->GetAllOutputInfo()) {
        EXPECT_GT(output_info_ptr->GetUnknownShapeFormat().size(), 10);
        EXPECT_GT(output_info_ptr->GetUnknownShapeDataType().size(), 10);
    }
    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, test_finalize)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    Status ret1 = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret1);
}

TEST_F(STEST_OP_KERNEL_INFO, test_get_attr_value)
{
    OpContent op_content;
    sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_NE(0, op_kernel_info_ptr->attrs_info_.size());

    EXPECT_NE(0, op_kernel_info_ptr->attrs_info_[0]->supported_values_.size());
    cout << "====================================the size of attrs_info_ = " << op_kernel_info_ptr->attrs_info_.size() <<endl;
    //vector<ge::GeAttrValue> attr_vec = op_kernel_info_ptr->map_attr_value_.at("attrListInt");
    // vector<ge::GeAttrValue> attr_int_vec = op_kernel_info_ptr->map_attr_value_.at("transposX");
    for (auto el : op_kernel_info_ptr->attrs_info_) {
        cout <<"================================key of attrs_info_  : " << el->attr_name_ << endl;
    }
    // for (auto it:attr_int_vec){
    //     int64_t int_value;
    //     it.GetValue<int64_t>(int_value);
    //     cout <<"the value from op_info "<< int_value << endl;
    // }
    // EXPECT_NE(0, attr_vec.size());
    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}
TEST_F(STEST_OP_KERNEL_INFO, test_dtype_format_special)
{
    OpKernelInfoPtr op_info_ptr = std::make_shared<fe::OpKernelInfo>("conv");
    map<string, string> options;
    // FEOpsKernelInfoStore initialize;
    FEOpsStoreInfo utest_opinfo {
            2,
            "utest_opinfo",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/utest_opinfo",
            "xx",
            false,
            false};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(utest_opinfo);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).is_init_ = false;
    OpsKernelManager::Instance(fe::AI_CORE_NAME).Initialize();
    SubOpInfoStorePtr subOpInfoStorePtr = OpsKernelManager::Instance(fe::AI_CORE_NAME).GetSubOpsKernelByStoreName("utest_opinfo");
    OpContent op_content;
    subOpInfoStorePtr->GetOpContentByOpType("conv", op_content);
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
    op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO, test_get_str_from_op_content)
{
    OpContent op_content;
    op_content.op_type_ = "conv";
    map<string, string> cost{{"cost", "10"}};
    map<string, string> flag{{"flag", "true"}};
    op_content.map_kernel_info_.emplace(make_pair("compute", cost));
    op_content.map_kernel_info_.emplace(make_pair("async", flag));
    string value;
    // std::shared_ptr<fe::OpKernelInfo> info_store_ptr = std::make_shared<fe::OpKernelInfo>();
    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret1 = op_kernel_info_constructor.GetStrFromOpContent(op_content, "partial", "flag", value);
    Status ret2 = op_kernel_info_constructor.GetStrFromOpContent(op_content, "compute", "flag", value);
    EXPECT_NE(fe::SUCCESS, ret1);
    EXPECT_NE(fe::SUCCESS, ret2);
}

TEST_F(STEST_OP_KERNEL_INFO, init_tensor_desc_info_param_type)
{
    map<string, string> map_info{{"name", "x"}, {"paramtype","not_exist"}, {"dtype","float16,float,double,int32"},
                                 {"format","NCHW"}, {"shape", "[256,256,512]"}};

    shared_ptr<InputOrOutputInfo> info_ptr = make_shared<InputOrOutputInfo>("x");
    OpKernelInfoConstructor op_kernel_info_constructor;
    uint32_t temp;
    Status ret = op_kernel_info_constructor.InitializeInputAndOutput(OP_PATTERN_OP_KERNEL, "xOp", map_info, 0, info_ptr,temp, op_kernel_info_ptr);
    EXPECT_NE(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO, test_convert_list_attr_default_value)
{
    OpContent op_content;
    op_content.op_type_ = "otherNode";
    map<std::string, std::string> in_info_map;
    in_info_map.emplace(std::make_pair("defaultValue", "[1,2]"));
    op_content.map_kernel_info_.emplace(std::make_pair("attr_x1", in_info_map));

    string attr_name = "x1";
    std::shared_ptr<AttrInfo> attr_info_ptr = make_shared<AttrInfo>("x1");
    attr_info_ptr->attr_name_ = "x1";
    attr_info_ptr->dtype_ = ge::GeAttrValue::ValueType::VT_LIST_INT;
    attr_info_ptr->is_required_ = false;
    attr_info_ptr->is_support_all_value_ = true;

    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitAttrListTemplate<int64_t>(op_content, attr_name, attr_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO, test_convert_list_attr_default_value_failed)
{
    OpContent op_content;
    op_content.op_type_ = "otherNode";
    map<std::string, std::string> in_info_map;
    op_content.map_kernel_info_.emplace(std::make_pair("attr_x1", in_info_map));

    string attr_name = "x1";
    std::shared_ptr<AttrInfo> attr_info_ptr = make_shared<AttrInfo>("x1");
    attr_info_ptr->attr_name_ = "x1";
    attr_info_ptr->dtype_ = ge::GeAttrValue::ValueType::VT_LIST_INT;
    attr_info_ptr->is_required_ = false;
    attr_info_ptr->is_support_all_value_ = true;

    OpKernelInfoConstructor op_kernel_info_constructor;
    Status ret = op_kernel_info_constructor.InitAttrListTemplate<int64_t>(op_content, attr_name, attr_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_1)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("conv");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "AiCore");
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_2)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("conv3", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("conv3");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "cube");
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_3)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("A", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("A");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "VectorCore");
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_4)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("AA", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("AA");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "");
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_5)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("B", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("B");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "Mix");
}


TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_6)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("BB", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("BB");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "mix");
}

TEST_F(STEST_OP_KERNEL_INFO, op_kernel_info_core_type_test_7)
{
  OpContent op_content;
  sub_ops_kernel_ptr->GetOpContentByOpType("C", op_content);
  OpKernelInfoPtr op_info_ptr = std::make_shared<OpKernelInfo>("C");
  op_info_ptr->init_flag_ = false;
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.InitializeOpKernelInfo(fe::AI_CORE_NAME, op_content, op_info_ptr);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(op_info_ptr->GetCoreType(), "dynamic");
}