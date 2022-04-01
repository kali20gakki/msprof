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
#include "graph/op_kernel_bin.h"
#include "graph/ge_local_context.h"
#include "fusion_manager/fusion_manager.h"
#include "ops_kernel_store/sub_ops_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"

using namespace std;
using namespace testing;
using namespace fe;
using ScopeJsonMap_t = std::map<int64_t, std::string>;

using fe::FEOpsKernelInfoStore;
using fe::SubOpsStore;
using ge::GeTensorDesc;
using ge::GeShape;
using ge::AttrUtils;
using ge::Format;
using ge::DataType;
using ge::ConstGeTensorDescPtr;
using ge::GeTensorDescPtr;
using ge::OpDescPtr;
using ge::OpDesc;
using fe::InputOrOutputInfoPtr ;
using ge::GeAttrValue;
using std::vector;
using std::map;
using namespace ge;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;

enum TestIter {
    TEST_SUCCESS = 0,
    TEST_HAVE_ALL,        // have one "all" type for attr check
    TEST_ATTR_NOT_FOUND,  // can not found attr ATTR_NAME_STR in OpDesc
    TEST_NOT_SUPPORT_DATA_TYPE,  // exit not support ValueType
    TEST_CHECK_FAILED,    // have one not match iter (ATTR_NAME_FLOAT)
    TEST_INT,
    TEST_FLOAT,
    TEST_BOOL,
    TEST_STR,
    TEST_LIST_INT,
    TEST_LIST_FLOAT,
    TEST_LIST_BOOL,
    TEST_LIST_STR,
    TEST_LACK_OF_ATTR_INT
};

static const string ATTR_NAME_INT = "transposX";
static const string ATTR_NAME_FLOAT = "transposY";
static const string ATTR_NAME_STR = "attrStr";
static const string ATTR_NAME_BOOL = "attrBool";
static const string ATTR_NAME_LIST_INT = "attrListInt";
static const string ATTR_NAME_LIST_FLOAT = "attrListFloat";
static const string ATTR_NAME_LIST_STR = "attrListStr";
static const string ATTR_NAME_LIST_BOOL = "attrListBool";
static const string ATTR_NAME_DEFAULT = "attr_name_default";
extern bool teGeneralize(const te::TbeOpInfo &op_info, const te::TE_GENERALIZE_TYPE &general_type,
                         const ge::NodePtr &node);
extern bool checkIsRegistered(const te::TbeOpInfo &op_info, bool &val);
te::LX_QUERY_STATUS GetOpInfoStubTestImplJudgeSt(const te::TbeOpInfo &a, std::string &b) {
  return te::LX_QUERY_SUCC;
};
bool PreBuildTbeOpStubTestImplJudgeSt(te::TbeOpInfo &a, uint64_t b, uint64_t c) {
  return true;
};

bool CheckTbeSupportedReasonRange(te::TbeOpInfo &, te::CheckSupportedResult &, string &reason) {
  reason = "The shape is not support now";
  return true;
}

bool CheckTbeSupportedOtherReason(te::TbeOpInfo &, te::CheckSupportedResult &, string &reason) {
  reason = "other";
  return true;
}

class OptimizeUtilityStubST: public ge::OptimizeUtility {
 public:
  OptimizeUtilityStubST() {}
  virtual ~OptimizeUtilityStubST() override {}

  ge::Status InferShape(ComputeGraph &compute_graph) override{
    return ge::SUCCESS;
  }

  ge::Status InferShape(const ComputeGraphPtr &compute_graph) override {
    return ge::SUCCESS;
  }
};

class STEST_OP_KERNEL_INFO_STORE : public testing::Test{
  protected:
    static void SetUpTestCase() {
        cout << "STEST_OP_KERNEL_INFO_STORE SetUP" << endl;
    }
    static void TearDownTestCase() {
        cout << "STEST_OP_KERNEL_INFO_STORE TearDown" << endl;
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
        tbe_adapter_ptr_ = std::make_shared<TbeOpStoreAdapter>();
        tbe_adapter_ptr_->GetOpInfo = GetOpInfoStubTestImplJudgeSt;
        tbe_adapter_ptr_->PreBuildTbeOp = PreBuildTbeOpStubTestImplJudgeSt;
        op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
                std::make_pair("tbe_op_adapter", tbe_adapter_ptr_));

        sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
        sub_ops_store_ptr->SetSubStoreType("tbe-custom");
        FEOpsStoreInfo tbe_custom {
          2,
          "tbe-custom",
          EN_IMPL_CUSTOM_TBE,
          "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/tbe_custom_opinfo",
          ""};

        sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
        Status stu = sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
        EXPECT_EQ(fe::SUCCESS, stu);
        sub_ops_store_ptr->init_flag_ = true;

        sub_ops_store_ptr_cce = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
        sub_ops_store_ptr_cce->SetSubStoreType("cce-custom");
        FEOpsStoreInfo cce_custom {
        1,
        "cce-custom",
        EN_IMPL_HW_CONSTANT_CCE,
        "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/cce_custom_opinfo",
        ""};
        sub_ops_store_ptr_cce->SetSubStoreInfo(cce_custom);
        stu = sub_ops_store_ptr_cce->InitializeSubStore(fe::AI_CORE_NAME);
        EXPECT_EQ(fe::SUCCESS, stu);
        sub_ops_store_ptr_cce->init_flag_ = true;
        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = {tbe_custom, cce_custom};
        OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

        fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, AI_CORE_NAME);
        fe_ops_kernel_info_store_ptr->Initialize(options);
        fe_ops_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
        fe_ops_store_ptr->init_flag_ = true;
        fe_ops_store_ptr->map_all_sub_store_info_.emplace(std::make_pair("cce-custom", sub_ops_store_ptr_cce));
        fe_ops_store_ptr->map_all_sub_store_info_.emplace(std::make_pair("tbe-custom", sub_ops_store_ptr));
        fe_ops_store_ptr->op_kernel_store_type_ = "FEOpsStore";
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
        op_desc_ptr->AddOutputDesc("z", output0_desc_ptr->Clone());

        format_dtype_querier_ptr_ = std::make_shared<FormatDtypeQuerier>(op_store_adapter_manager_ptr_);
        cout << "A ops kernel info store stest set up" << endl;

    }
    virtual void TearDown(){
        cout << "A ops kernel info store stest is tearing down" << endl;
        sub_ops_store_ptr->FinalizeSubStore();
        sub_ops_store_ptr_cce->FinalizeSubStore();
        fe_ops_store_ptr->Finalize();
        fe_ops_kernel_info_store_ptr->Finalize();
       // c_fe_ops_kernel_info_store_ptr.reset();

    }
    void set_op_desc_default_value (OpDescPtr &op_desc_ptr_t)
    {
        op_desc_ptr_t->SetName("tbe_conv");
        op_desc_ptr_t->SetType("conv");
    }

    void SetOpDescPtrAttrValue(TestIter test_iter, OpDescPtr desc_ptr)
    {
        if (test_iter == TEST_INT) {
            AttrUtils::SetInt(desc_ptr, ATTR_NAME_INT, 10);
        }else if (test_iter != TEST_LACK_OF_ATTR_INT) {
            AttrUtils::SetInt(desc_ptr, ATTR_NAME_INT, 1);
        }
        if (test_iter == TEST_FLOAT) {
            AttrUtils::SetFloat(desc_ptr, ATTR_NAME_FLOAT, 22.0);
        }else{
            AttrUtils::SetFloat(desc_ptr, ATTR_NAME_FLOAT, 2.0);
        }
        if (test_iter == TEST_BOOL) {
            AttrUtils::SetBool(desc_ptr, ATTR_NAME_BOOL, true);
        }else{
            AttrUtils::SetBool(desc_ptr, ATTR_NAME_BOOL, false);
        }
        if (test_iter == TEST_STR) {
            AttrUtils::SetStr(desc_ptr, ATTR_NAME_STR, "not_exist");
        }else{
            AttrUtils::SetStr(desc_ptr, ATTR_NAME_STR, "abc");
        }
        if (test_iter == TEST_LIST_INT) {
            AttrUtils::SetListInt(desc_ptr, ATTR_NAME_LIST_INT, {6,7,8});
        }else{
            AttrUtils::SetListInt(desc_ptr, ATTR_NAME_LIST_INT, {1,2,3});
        }
        if (test_iter == TEST_LIST_FLOAT) {
            AttrUtils::SetListFloat(desc_ptr, ATTR_NAME_LIST_FLOAT, {6.0, 7.0, 8.0});
        }else{
            AttrUtils::SetListFloat(desc_ptr, ATTR_NAME_LIST_FLOAT, {1.0, 2.0, 3.0});
        }
        if (test_iter == TEST_LIST_BOOL) {
            AttrUtils::SetListBool(desc_ptr, ATTR_NAME_LIST_BOOL, {true,false,true});
        }else{
            AttrUtils::SetListBool(desc_ptr, ATTR_NAME_LIST_BOOL, {true,true,true});
        }
        if (test_iter == TEST_LIST_STR) {
            AttrUtils::SetListStr(desc_ptr, ATTR_NAME_LIST_STR, {"aa", "bb", "cc"});
        }else{
            AttrUtils::SetListStr(desc_ptr, ATTR_NAME_LIST_STR, {"a", "b", "c"});
        }

    }
public:
    shared_ptr<fe::SubOpsStore> sub_ops_store_ptr;
    shared_ptr<fe::SubOpsStore>  sub_ops_store_ptr_cce;
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_store_ptr;
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input1_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input2_desc_ptr;
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr;
    shared_ptr<ge::OpDesc> op_desc_ptr;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
    TbeOpStoreAdapterPtr tbe_adapter_ptr_;
    FormatDtypeQuerierPtr format_dtype_querier_ptr_;
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr;
};


void CreateConvSt(ge::NodePtr &node, string op_type) {
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> input2_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
  op_desc_ptr_t->SetName("tbe_conv");
  op_desc_ptr_t->SetType(op_type);
  int64_t int_value = 1;
  float float_value = 2.0;
  bool bool_value = false;
  string str_value = "abc";
  vector<int64_t> int_vec{1, 2, 3};
  vector<int64_t> rint_vec;
  vector<float> float_vec{4.0, 5.0, 6.0};
  vector<float> rfloat_vec;
  vector<bool> bool_vec{false, true, true};
  vector<bool> rbool_vec;
  std::vector<string> str_vec{"a", "b", "c"};
  AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
  AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
  AttrUtils::SetBool(op_desc_ptr_t, "attrBool", bool_value);
  AttrUtils::SetStr(op_desc_ptr_t, "attrStr", str_value);
  AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
  AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
  AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
  AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

  ge::DataType set_dtype = ge::DT_FLOAT16;
  std::vector<int64_t> shape_vec{256, 256, 512};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  std::vector<int64_t> shape_vec1{256, 256, 512};
  ge::GeShape shape_desc1 = GeShape(shape_vec1);
  input1_desc_ptr->SetDataType(set_dtype);
  input1_desc_ptr->SetShape(shape_desc1);
  input1_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input1_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

  std::vector<int64_t> shape_vec2{256, 256, 512};
  ge::GeShape shape_desc2 = GeShape(shape_vec2);
  input2_desc_ptr->SetDataType(set_dtype);
  input2_desc_ptr->SetShape(shape_desc2);
  input2_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input2_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("h", input2_desc_ptr->Clone());

  ge::DataType set_dtype2 = ge::DT_FLOAT;
  output0_desc_ptr->SetDataType(set_dtype2);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  node = graph->AddNode(op_desc_ptr_t);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, impl_judge_1) {
  ge::NodePtr test_node;
  CreateConvSt(test_node, "conv");

  OpImplTypeJudge impl_judge("AiCoreEngine", fe_ops_store_ptr);
  Status result = impl_judge.JudgeByNode(test_node);
  EXPECT_EQ(result, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, impl_judge_2) {
  ge::NodePtr test_node;
  CreateConvSt(test_node, "conv_dynamic");

  OpImplTypeJudge impl_judge("AiCoreEngine", fe_ops_store_ptr);
  Status result = impl_judge.JudgeByNode(test_node);

  EXPECT_EQ(result, fe::FAILED);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, initialize_succ){
    shared_ptr<SubOpsStore> sub_ops_store_ptr = make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    FEOpsStoreInfo tbe_custom {
          2,
          "tbe-custom",
          EN_IMPL_CUSTOM_TBE,
          "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/tbe_custom_opinfo",
          ""};
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    Status ret = sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, initialize_twice){
    shared_ptr<SubOpsStore> sub_ops_store_ptr = make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    map<string, string> options;
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    FEOpsStoreInfo tbe_custom {
          2,
          "tbe-custom",
          EN_IMPL_CUSTOM_TBE,
          "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/tbe_custom_opinfo",
          ""};
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    Status ret1 = sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    Status ret2 = sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    EXPECT_EQ(fe::SUCCESS, ret1);
    EXPECT_EQ(fe::SUCCESS, ret2);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, get_all_ops_kernel_info_succ){
    shared_ptr<map<string, ge::OpInfo>> infos = make_shared<map<string, ge::OpInfo>>();
    fe_ops_store_ptr->GetAllOpsKernelInfo(*(infos.get()));
    EXPECT_NE(false, infos->size());
    infos.reset();
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, get_one_op_kernel_info_ptr)
{
    string op_type = "conv";
    string op_not_exist = "relu";
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", op_type);
    OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", op_not_exist);
    EXPECT_EQ(nullptr, op_kernel_info_ptr1);
    EXPECT_NE(nullptr, op_kernel_info_ptr);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, get_high_prio_op_kernel_info_ptr)
{
    string op_type = "conv";
    string op_not_exist = "relu";
    OpKernelInfoPtr op_kernel_info_ptr;
    OpKernelInfoPtr op_kernel_info_ptr1;

    Status ret = fe_ops_store_ptr->GetHighPrioOpKernelInfoPtr(op_type, op_kernel_info_ptr);
    Status ret1 = fe_ops_store_ptr->GetHighPrioOpKernelInfoPtr(op_not_exist, op_kernel_info_ptr1);

    EXPECT_NE(nullptr, op_kernel_info_ptr);
    if(op_kernel_info_ptr != nullptr){
        EXPECT_EQ("conv", op_kernel_info_ptr->GetOpType());
    }

    EXPECT_EQ(fe::SUCCESS, ret);
    EXPECT_NE(fe::SUCCESS, ret1);

}

TEST_F(STEST_OP_KERNEL_INFO_STORE, set_lib_type_succ){
    sub_ops_store_ptr->SetSubStoreType(string("tbe-custom"));
    string lib_type;
    sub_ops_store_ptr->GetSubStoreType(lib_type);
    EXPECT_EQ(lib_type, "tbe-custom");
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, get_lib_type_succ){
    string lib_type;
    sub_ops_store_ptr->GetSubStoreType(lib_type);
    EXPECT_EQ(lib_type, "tbe-custom");
}


TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_supported_succ)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    SetOpDescPtrAttrValue(TEST_SUCCESS, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(true, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_supported)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    SetOpDescPtrAttrValue(TEST_SUCCESS, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(true, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_supported_lack_of_attr)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Do not initialize attr int for this op desc */
    SetOpDescPtrAttrValue(TEST_LACK_OF_ATTR_INT, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);

    if(op_kernel_info_ptr != nullptr) {
        std::string reason;
        ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
        ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
        bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
        EXPECT_EQ(false, ret);
    }
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_int_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set int value as 10, which is not supported  */
    SetOpDescPtrAttrValue(TEST_INT, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(op_kernel_info_ptr, nullptr);

    if(op_kernel_info_ptr != nullptr) {
        std::string reason;
        
ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
        ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
        bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
        EXPECT_EQ(false, ret);
    }
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_float_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set float value as 22.0, which is not supported  */
    SetOpDescPtrAttrValue(TEST_FLOAT, op_desc_ptr_t);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    if(op_kernel_info_ptr != nullptr) {
        std::string reason;
        ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
        ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
        bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
        EXPECT_EQ(false, ret);
    }
}
TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_str_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set string value as "not exist", which is not supported  */
    SetOpDescPtrAttrValue(TEST_STR, op_desc_ptr_t);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    if(op_kernel_info_ptr != nullptr) {
        std::string reason;
        ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
        ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
        bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
        EXPECT_EQ(false, ret);
    }
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_bool_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set bool value as true, which is not supported  */
    SetOpDescPtrAttrValue(TEST_BOOL, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_supported_list_bool_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set list bool value as [true, false, true], which is not supported  */
    SetOpDescPtrAttrValue(TEST_LIST_BOOL, op_desc_ptr_t);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_supported_list_int_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set list int value as [6, 7, 8], which is not supported  */
    SetOpDescPtrAttrValue(TEST_LIST_INT, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_list_float_false)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set list float value as [6.0, 7.0, 8.0], which is not supported  */
    SetOpDescPtrAttrValue(TEST_LIST_FLOAT, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_attr_list_str_false)
{

    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set list string value as ["aa", "bb", "cc"], which is not supported  */
    SetOpDescPtrAttrValue(TEST_LIST_STR, op_desc_ptr_t);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}
TEST_F(STEST_OP_KERNEL_INFO_STORE, check_supported_succ)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv");
    int64_t int_value = 1;
    float float_value = 2.0;
    bool bool_value = false;
    string str_value = "abc";
    vector<int64_t> int_vec{1, 2, 3};
    vector<int64_t> rint_vec;
    vector<float> float_vec{4.0, 5.0, 6.0};
    vector<float> rfloat_vec;
    vector<bool> bool_vec{false, true, true};
    vector<bool> rbool_vec;
    std::vector<string> str_vec{"a", "b", "c"};
    AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
    AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);
    AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
    AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    input0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
    input0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    std::vector<int64_t> shape_vec1{256,256,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    input1_desc_ptr->SetFormat(ge::FORMAT_NCHW);
    input1_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
    op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<int64_t> shape_vec2{256,256,512};
    ge::GeShape shape_desc2 = GeShape(shape_vec2);
    input2_desc_ptr->SetDataType(set_dtype);
    input2_desc_ptr->SetShape(shape_desc2);
    input2_desc_ptr->SetFormat(ge::FORMAT_NCHW);
    input2_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
    op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
    output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    string un_supported_reason;
    bool ret2 = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);

    EXPECT_EQ(true, ret);
    EXPECT_EQ(true, ret2);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_supported_shape_fail)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv");
    int64_t int_value = 1;
    float float_value = 2.0;
    bool bool_value = false;
    string str_value = "abc";
    vector<int64_t> int_vec{1, 2, 3};
    vector<int64_t> rint_vec;
    vector<float> float_vec{4.0, 5.0, 6.0};
    vector<float> rfloat_vec;
    vector<bool> bool_vec{false, true, true};
    vector<bool> rbool_vec;
    std::vector<string> str_vec{"a", "b", "c"};
    AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
    AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);
    AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
    AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    std::vector<int64_t> shape_vec1{256,-1,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<int64_t> shape_vec2{256,256,512};
    ge::GeShape shape_desc2 = GeShape(shape_vec2);
    input2_desc_ptr->SetDataType(set_dtype);
    input2_desc_ptr->SetShape(shape_desc2);
    op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);

    string un_supported_reason;
    bool ret1 = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret1);
    un_supported_reason = "";
    bool ret2 = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret2);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_accuracy_supported_succ)
{
  shared_ptr<OpStoreAdapterManager> op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
  shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
  using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
  std::map<std::string, std::string> options;
  op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
  fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
  FEOpsStoreInfo tbe_custom {
      2,
      "tbe-custom",
      EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      ""};

  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(tbe_custom);
  Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
  OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

  fe_ops_kernel_info_store_ptr->Initialize(options);
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
  op_desc_ptr_t->SetName("tbe_conv");
  op_desc_ptr_t->SetType("conv");
  int64_t int_value = 1;
  float float_value = 2.0;
  bool bool_value = false;
  string str_value = "abc";
  vector<int64_t> int_vec{1, 2, 3};
  vector<int64_t> rint_vec;
  vector<float> float_vec{4.0, 5.0, 6.0};
  vector<float> rfloat_vec;
  vector<bool> bool_vec{false, true, true};
  vector<bool> rbool_vec;
  std::vector<string> str_vec{"a", "b", "c"};
  AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
  AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
  AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
  AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);
  AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
  AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
  AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
  AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

  ge::DataType set_dtype = ge::DT_FLOAT16;
  std::vector<int64_t> shape_vec{256,256,512};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  std::vector<int64_t> shape_vec1{256,256,512};
  ge::GeShape shape_desc1 = GeShape(shape_vec1);
  input1_desc_ptr->SetDataType(set_dtype);
  input1_desc_ptr->SetShape(shape_desc1);
  input1_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input1_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

  std::vector<int64_t> shape_vec2{256,256,512};
  ge::GeShape shape_desc2 = GeShape(shape_vec2);
  input2_desc_ptr->SetDataType(set_dtype);
  input2_desc_ptr->SetShape(shape_desc2);
  input2_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input2_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

  ge::DataType set_dtype2 = ge::DT_FLOAT;
  output0_desc_ptr->SetDataType(set_dtype2);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  OpKernelInfoPtr op_kernel_info_ptr;
  SubOpsStorePtr sub_ops_store_ptr = fe_ops_kernel_info_store_ptr->map_all_sub_store_info_["tbe-custom"];
  string un_supported_reason;
  op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
  bool ret1 = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), un_supported_reason);
  bool ret2 = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  int64_t is_check_supported = 1;
  is_check_supported = (is_check_supported << 63);
  ge::AttrUtils::SetInt(op_desc_ptr_t, IS_CHECK_SUPPORTED, is_check_supported);
  bool ret3 = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(true, ret1);
  EXPECT_EQ(true, ret2);
  EXPECT_EQ(false, ret3);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_accuracy_supported_failed1)
{
  shared_ptr<OpStoreAdapterManager> op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
  shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
  using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
  std::map<std::string, std::string> options;
  op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
  fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
  FEOpsStoreInfo tbe_custom {
      2,
      "tbe-custom",
      EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      ""};

  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(tbe_custom);
  Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
  OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

  fe_ops_kernel_info_store_ptr->Initialize(options);
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
  op_desc_ptr_t->SetName("tbe_conv");
  op_desc_ptr_t->SetType("conv");
  int64_t int_value = 1;
  float float_value = 2.0;
  bool bool_value = false;
  string str_value = "abc";
  vector<int64_t> int_vec{1, 2, 3};
  vector<int64_t> rint_vec;
  vector<float> float_vec{4.0, 5.0, 6.0};
  vector<float> rfloat_vec;
  vector<bool> bool_vec{false, true, true};
  vector<bool> rbool_vec;
  std::vector<string> str_vec{"a", "b", "c"};
  AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
  AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
  AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
  AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);
  AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
  AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
  AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
  AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

  ge::DataType set_dtype = ge::DT_FLOAT16;
  std::vector<int64_t> shape_vec{256,256,512};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  std::vector<int64_t> shape_vec1{256,256,512};
  ge::GeShape shape_desc1 = GeShape(shape_vec1);
  input1_desc_ptr->SetDataType(set_dtype);
  input1_desc_ptr->SetShape(shape_desc1);
  input1_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input1_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

  std::vector<int64_t> shape_vec2{256,256,512};
  ge::GeShape shape_desc2 = GeShape(shape_vec2);
  input2_desc_ptr->SetDataType(set_dtype);
  input2_desc_ptr->SetShape(shape_desc2);
  input2_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  input2_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  OpKernelInfoPtr op_kernel_info_ptr;
  string un_supported_reason;

  SubOpsStorePtr sub_ops_store_ptr = fe_ops_kernel_info_store_ptr->map_all_sub_store_info_["tbe-custom"];
  op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
  bool ret1 = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), un_supported_reason);
  bool ret2 = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  int64_t is_check_supported = 1;
  is_check_supported = (is_check_supported << 63);
  ge::AttrUtils::SetInt(op_desc_ptr_t, IS_CHECK_SUPPORTED, is_check_supported);
  bool ret3 = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(true, ret1);
  EXPECT_EQ(false, ret2);
  EXPECT_EQ(false, ret3);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_dtype_false)
{
    shared_ptr<ge::GeTensorDesc> input_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    set_op_desc_default_value(op_desc_ptr_t);
    /* Set list int value as [6, 7, 8], which is not supported  */
    SetOpDescPtrAttrValue(TEST_SUCCESS, op_desc_ptr_t);

    ge::DataType set_dtype = ge::DT_UINT64;
    ge::Format set_format = ge::FORMAT_ND;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input_ptr->SetDataType(set_dtype);
    input_ptr->SetFormat(set_format);
    input_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    InputOrOutputInfoPtr input_info_ptr;
    op_kernel_info_ptr->GetInputInfoByName("x", input_info_ptr);

    map<string, vector<ge::Format>> support_formats;
    map<string, vector<ge::DataType>> support_data_types;
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr,
            *(op_desc_ptr_t.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckDtypeSupported(test_node, input_ptr, input_info_ptr,
                                                   support_data_types.at(input_info_ptr->GetUniqueName()),
                                                   op_kernel_info_ptr);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, finalize_succ){
    Status ret = fe_ops_store_ptr->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

Status GetOpStoreInfoByImplTypeStubzz(Configuration *This, OpImplType op_impl_type, FEOpsStoreInfo& op_store_info)
{
  Status return_status = fe::SUCCESS;
  op_store_info.fe_ops_store_name = "tbe-custom";
  op_store_info.need_pre_compile = true;
  op_store_info.need_compile = true;
  op_store_info.op_impl_file_path = "";
  return return_status;
}

Status CompileOpStubzz(TbeOpStoreAdapter *This, ScopeNodeIdMap &fusion_nodes_map, map<int64_t, std::string>& json_file_map, std::vector<ge::NodePtr> &compile_failed_nodes,
                       const std::vector<ge::NodePtr> &to_del_nodes)
{
  json_file_map.emplace(make_pair(-1, "a.json"));
  return fe::SUCCESS;
}

Status QueryHighPrioOpImplTypeStubTbe1zz(FEOpsKernelInfoStore* This, const ge::OpDescPtr& op_desc_ptr, OpImplType &impl_type) {

  impl_type = EN_IMPL_CUSTOM_TBE;
  return fe::SUCCESS;
}

Status PreCompileOp_Stub(TbeOpStoreAdapter *This, vector<PreCompileNodePara> &compile_para_vec)
{
  return fe::SUCCESS;
}

Status GetOpStoreAdapterStubzz(OpStoreAdapterManager *This, const OpImplType &op_impl_type, OpStoreAdapterPtr &adapter_ptr)
{
  adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  return fe::SUCCESS;
}

bool CheckTbeSupportedStub1(te::TbeOpInfo& opinfo, te::CheckSupportedResult &isSupport, std::string &reason) {
  isSupport = opinfo.IsDynamicImpl() ? te::FULLY_SUPPORTED : te::NOT_SUPPORTED;
  return true;
}

bool CheckTbeSupportedStub2(te::TbeOpInfo& opinfo, te::CheckSupportedResult &isSupport, std::string &reason) {
  isSupport = opinfo.IsDynamicImpl() ? te::NOT_SUPPORTED : te::FULLY_SUPPORTED;
  return true;
}

bool SelectOpFormatStub(const te::TbeOpInfo &tbeOpInfo, std::string &opDtypeFormat) {
  if (tbeOpInfo.IsDynamicImpl()) {
    opDtypeFormat = "{\"input0\":{\"name\":\"x\", \"dtype\":\"float16\", \"format\":\"NCHW\"},"
                    "\"output0\":{\"name\":\"y\", \"dtype\":\"float16\", \"format\":\"NCHW\"}}";
  } else {
    opDtypeFormat = "{\"input0\":{\"name\":\"x\", \"dtype\":\"float16\", \"format\":\"FRACTAL_NZ\"},"
                    "\"output0\":{\"name\":\"y\", \"dtype\":\"float16\", \"format\":\"FRACTAL_NZ\"}}";
  }
  return true;
}

static void CreateSpacesizeTwoOpGraph(ComputeGraphPtr graph) {
  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "conv");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "conv");

  // add descriptor
  vector<int64_t> dims = {1,2,3,4};
  GeShape shape(dims);

  GeTensorDesc in_desc1(shape);
  in_desc1.SetFormat(FORMAT_NCHW);
  in_desc1.SetDataType(DT_FLOAT16);
  relu_op->AddInputDesc("x", in_desc1);

  GeTensorDesc out_desc1(shape);
  out_desc1.SetFormat(FORMAT_HWCN);
  out_desc1.SetDataType(DT_FLOAT16);
  relu_op->AddOutputDesc("y", out_desc1);

  GeTensorDesc in_desc2(shape);
  in_desc2.SetFormat(FORMAT_FRACTAL_Z);
  in_desc2.SetDataType(DT_FLOAT16);
  bn_op->AddInputDesc("x", in_desc2);

  GeTensorDesc out_desc2(shape);
  out_desc2.SetFormat(FORMAT_NHWC);
  out_desc2.SetDataType(DT_FLOAT16);
  bn_op->AddOutputDesc("y", out_desc2);

  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);

  GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_format_nd_success)
{
    shared_ptr<ge::GeTensorDesc> input_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::OpDesc> test_op_desc_ptr = make_shared<ge::OpDesc>();
    SetOpDescPtrAttrValue(TEST_SUCCESS,test_op_desc_ptr);
    ge::DataType set_dtype = ge::DT_UINT64;
    ge::Format set_format = ge::FORMAT_ND;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input_ptr->SetDataType(set_dtype);
    input_ptr->SetOriginFormat(set_format);
    input_ptr->SetFormat(set_format);
    input_ptr->SetShape(shape_desc);
    test_op_desc_ptr->AddInputDesc("x", input_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = fe_ops_store_ptr->map_all_sub_store_info_["tbe-custom"];
    OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    InputOrOutputInfoPtr input_info_ptr1;
    op_kernel_info_ptr1->GetInputInfoByName("x", input_info_ptr1);

    map<string, vector<ge::Format>> support_formats;
    map<string, vector<ge::DataType>> support_data_types;
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr1,
            *(test_op_desc_ptr.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(test_op_desc_ptr);
    bool ret1 = sub_ops_store_ptr->CheckFormatSupported(test_node, input_ptr, input_info_ptr1,
            support_formats.at(input_info_ptr1->GetUniqueName()));
    EXPECT_EQ(false, ret1);


    OpKernelInfoPtr op_kernel_info_ptr2 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "K");
    InputOrOutputInfoPtr input_info_ptr2;
    op_kernel_info_ptr2->GetInputInfoByName("x", input_info_ptr2);
    get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr2,
            *(test_op_desc_ptr.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);

    bool ret2 = sub_ops_store_ptr->CheckFormatSupported(test_node,input_ptr,input_info_ptr2,
            support_formats.at(input_info_ptr2->GetUniqueName()));
    EXPECT_EQ(true, ret2);
}


TEST_F(STEST_OP_KERNEL_INFO_STORE, set_cut_info_01)
{
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<ge::OpDesc>("op", "Relu");
  ge::OpDescPtr op1 = std::make_shared<ge::OpDesc>("op1", "Relu");
  GeShape shape = ge::GeShape({1,2,1,1,1});
  Format format = FORMAT_NHWC;
  DataType dt = DT_FLOAT;
  ge::GeTensorDesc tensor_desc(shape, format, dt);
  op->AddInputDesc(tensor_desc);
  op->AddOutputDesc(tensor_desc);
  op1->AddInputDesc(tensor_desc);
  op1->AddOutputDesc(tensor_desc);

  string relu_slice_info =
      "{\"_op_slice_info\":{\"l1FusionEnable\":0,\"minTbeL1Space\":0,\"reduceMaps\":"
      "[],\"splitMaps\":[{\"inputList\":[{\"axis\":[0],\"headOverLap\":[],\"idx\":0,\"tailOverLap\":"
      "[]}],\"outputList\":[{\"axis\":[0],\"idx\":0}]},{\"inputList\":[{\"axis\":[1],\"headOverLap\":"
      "[],\"idx\":0,\"tailOverLap\":[]}],\"outputList\":[{\"axis\":[1],\"idx\":0}]},{\"inputList\":[{\"axis\":"
      "[2],\"headOverLap\":[],\"idx\":0,\"tailOverLap\":[]}],\"outputList\":[{\"axis\":[2],\"idx\":0}]},"
      "{\"inputList\":[{\"axis\":[3],\"headOverLap\":[],\"idx\":0,\"tailOverLap\":[]}],\"outputList\":[{\"axis\":"
      "[3],\"idx\":0}]},{\"inputList\":[{\"axis\":[4],\"headOverLap\":[],\"idx\":0,\"tailOverLap\":"
      "[]}],\"outputList\":[{\"axis\":[4],\"idx\":0}]}]}}";
  ge::AttrUtils::SetStr(op, "_op_slice_info", relu_slice_info);
  auto node = graph->AddNode(op);
  auto node1 = graph->AddNode(op1);
  fe_ops_kernel_info_store_ptr->SetCutSupportedInfo(node);
  fe_ops_kernel_info_store_ptr->SetCutSupportedInfo(node1);
  auto input = node->GetOpDesc()->MutableInputDesc(0);
  auto input1 = node1->GetOpDesc()->MutableInputDesc(0);

  vector<vector<int64_t>> current_stgy;
  vector<vector<int64_t>> current_stgy1;
  vector<vector<int64_t>> current_stgy_expect = {
      {1, 0, 0, 0, 0},
      {0, 1, 0, 0, 0},
      {0, 0, 1, 0, 0},
      {0, 0, 0, 1, 0},
      {0, 0, 0, 0, 1}
  };
  (void)ge::AttrUtils::GetListListInt(input, "_cut_info", current_stgy);
  (void)ge::AttrUtils::GetListListInt(input1, "_cut_info", current_stgy1);
  ASSERT_EQ(current_stgy.size(), current_stgy_expect.size());
  ASSERT_EQ(current_stgy1.size(), 0);
  EXPECT_EQ(current_stgy[0], current_stgy_expect[0]);
  EXPECT_EQ(current_stgy[1], current_stgy_expect[1]);
  EXPECT_EQ(current_stgy[2], current_stgy_expect[2]);
  EXPECT_EQ(current_stgy[3], current_stgy_expect[3]);
  EXPECT_EQ(current_stgy[4], current_stgy_expect[4]);

  current_stgy = {};
  auto output = node->GetOpDesc()->MutableOutputDesc(0);
  auto output1 = node1->GetOpDesc()->MutableOutputDesc(0);
  (void)ge::AttrUtils::GetListListInt(output, "_cut_info", current_stgy);
  (void)ge::AttrUtils::GetListListInt(output1, "_cut_info", current_stgy1);
  ASSERT_EQ(current_stgy.size(), current_stgy_expect.size());
  ASSERT_EQ(current_stgy1.size(), 0);
  EXPECT_EQ(current_stgy[0], current_stgy_expect[0]);
  EXPECT_EQ(current_stgy[1], current_stgy_expect[1]);
  EXPECT_EQ(current_stgy[2], current_stgy_expect[2]);
  EXPECT_EQ(current_stgy[3], current_stgy_expect[3]);
  EXPECT_EQ(current_stgy[4], current_stgy_expect[4]);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, precompile_01)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("test", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_BOOL;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr =
      OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "DynamicCompileStatic");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  auto node = graph->AddNode(op_desc_ptr_t);
  TbeInfoAssembler t;
  te::TbeOpInfo info("test",
                     "test1",
                     "DynamicCompileStatic",
                     "",
                     fe::AI_CORE_NAME);
  Status ret = t.AssembleTbeInfo(node.get(), op_kernel_info_ptr, info, fe::AI_CORE_NAME);

  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, precompile_02)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("test", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_input_dtype = ge::DT_BOOL;
  ge::DataType set_output_dtype = ge::DT_VARIANT;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_input_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_input_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_output_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_output_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());

  te::TbeOpTensor output_tensor;
  TensorDescAndIndex tensor_info = {output0_desc_ptr, "y", 0, 0, false};

  Status ret = CreateTbeTensor(*op_desc_ptr_t.get(), tensor_info, output_tensor);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_1)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_2)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_ND;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_3)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_INT32;
  ge::Format set_format = ge::FORMAT_ND;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_4)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic1");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedStub1;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_5)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic1");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedStub2;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_6)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic2");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  tbe_adapter_ptr_->SelectTbeOpFormat = SelectOpFormatStub;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_compile_static_7)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic2");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_ND;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  tbe_adapter_ptr_->SelectTbeOpFormat = SelectOpFormatStub;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, reshape_1)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("reshape_1", "Reshape");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("a", input0_desc_ptr->Clone());
  op_desc_ptr_t->AddInputDesc("b", input0_desc_ptr->Clone());
  op_desc_ptr_t->AddInputDesc("c", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  std::string un_supported_reason;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, reshape_2)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("reshape_2", "Reshape");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, 16, 100, 100};
  ge::GeShape shape_desc = GeShape(shape_vec);

  input0_desc_ptr->SetDataType(set_dtype);
  input0_desc_ptr->SetFormat(set_format);
  input0_desc_ptr->SetShape(shape_desc);
  input0_desc_ptr->SetOriginDataType(set_dtype);
  input0_desc_ptr->SetOriginFormat(set_format);
  input0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());
  op_desc_ptr_t->AddInputDesc("z", input0_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetFormat(set_format);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetOriginDataType(set_dtype);
  output0_desc_ptr->SetOriginFormat(set_format);
  output0_desc_ptr->SetOriginShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("d", output0_desc_ptr->Clone());
  op_desc_ptr_t->AddOutputDesc("e", output0_desc_ptr->Clone());

  std::string un_supported_reason;
  bool ret = fe_ops_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, check_dtype_by_allow_fp32_to_fp16_suc)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  shared_ptr<ge::GeTensorDesc> input_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("matmul", "MatMulV10");
  /* Set list int value as [6, 7, 8], which is not supported  */
  SetOpDescPtrAttrValue(TEST_SUCCESS, op_desc_ptr_t);

  ge::DataType set_dtype = ge::DT_FLOAT;
  ge::Format set_format = ge::FORMAT_ND;
  std::vector<int64_t> shape_vec{256,256,512};
  ge::GeShape shape_desc = GeShape(shape_vec);

  vector<int64_t> tensorShape = {1,1,3,1};
  GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, ge::DT_FLOAT);
  input_ptr->SetDataType(set_dtype);
  input_ptr->SetFormat(set_format);
  input_ptr->SetShape(shape_desc);
  output_ptr->SetDataType(set_dtype);
  output_ptr->SetFormat(set_format);
  output_ptr->SetShape(shape_desc);
  op_desc_ptr_t->AddInputDesc("x1", tensor1);
  op_desc_ptr_t->AddOutputDesc("y", tensor1);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("cce-custom", "MatMulV10");
  EXPECT_NE(nullptr, op_kernel_info_ptr);
  InputOrOutputInfoPtr input_info_ptr;
  InputOrOutputInfoPtr output_info_ptr;
  op_kernel_info_ptr->GetInputInfoByName("x1", input_info_ptr);
  op_kernel_info_ptr->GetOutputInfoByName("y", output_info_ptr);

  map<string, vector<ge::Format>> support_formats;
  map<string, vector<ge::DataType>> support_data_types;
  Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr,
          *(op_desc_ptr_t.get()), false, support_formats, support_data_types);
  EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
  for (auto dtype : support_data_types.at(input_info_ptr->GetUniqueName())) {
    std::cout << "current dtype is " << dtype << std::endl;
    std::cout << "input_info_ptr uniqname is " << input_info_ptr->GetUniqueName() << std::endl;
  }
  ge::GeTensorDescPtr input_desc = op_desc_ptr_t->MutableInputDesc(0);
  ge::GeTensorDescPtr output_desc = op_desc_ptr_t->MutableOutputDesc(0);

  bool ret = sub_ops_store_ptr->CheckDtypeSupported(test_node, input_desc, input_info_ptr,
                                                  support_data_types.at(input_info_ptr->GetUniqueName()),
                                                  op_kernel_info_ptr);
  ret = sub_ops_store_ptr->CheckDtypeSupported(test_node, output_desc, output_info_ptr,
                                                  support_data_types.at(output_info_ptr->GetUniqueName()),
                                                  op_kernel_info_ptr);
  EXPECT_EQ(ret, true);

  bool has_need_update_dtype_flag = false;
  has_need_update_dtype_flag = ge::AttrUtils::GetBool(input_desc,
                              NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT, has_need_update_dtype_flag);
  EXPECT_EQ(true, has_need_update_dtype_flag);
  has_need_update_dtype_flag = ge::AttrUtils::GetBool(output_desc,
                              NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT, has_need_update_dtype_flag);
  EXPECT_EQ(true, has_need_update_dtype_flag);
  for (const auto &tensor : op_desc_ptr_t->GetAllInputsDesc()) {
    bool flag = ge::AttrUtils::GetBool(tensor,
                              NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT, flag);
    if (flag) {
      std::cout << "get NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT" << std::endl;
    } else {
      std::cout << "not get NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT" << std::endl;
    }
  }
  std::pair<std::vector<size_t>, std::vector<size_t>> in_out_changed_idx_vec;
  tbe_adapter_ptr_->UpdateTensorByMixPrecisionMode(op_desc_ptr_t, op_kernel_info_ptr, in_out_changed_idx_vec);
  bool all_dtype_fp16_flag = false;
  EXPECT_EQ(ge::DT_FLOAT16, op_desc_ptr_t->MutableInputDesc("x1")->GetDataType());
  EXPECT_EQ(ge::DT_FLOAT16, op_desc_ptr_t->MutableOutputDesc("y")->GetDataType());
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, get_all_atomic_clean_node_suc)
{
  ge::OpDescPtr conv_op = std::make_shared<ge::OpDesc>("Conv", "conv");
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::NodePtr node_ptr = graph->AddNode(conv_op);
  vector<ge::NodePtr> atomic_node_vec;
  Status ret = fe_ops_store_ptr->GetAllAtomicCleanNode(node_ptr, atomic_node_vec);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, compile_op_get_tvm_json_info_failed)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> fusion_nodes;
  fusion_nodes.push_back(node_0.get());
  fusion_nodes_map.emplace(std::make_pair(1, fusion_nodes));
  ScopeJsonMap_t scope_json_map;
  Status ret = fe_ops_store_ptr->CompileOpGetTvmJsonInfo(fusion_nodes_map, scope_json_map);
  EXPECT_EQ(ret, OP_COMPILER_CHECK_FALSE_FAILED);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, pre_compile_and_compile_success)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  std::unordered_map<OpStoreAdapterPtr, vector<PreCompileNodePara>> node_map;
  NodePtr node_0 = graph->AddNode(op_desc_0);
  ScopeNodeIdMap fusion_node_map;
  Status ret = fe_ops_store_ptr->PreCompileAndCompile(node_map, node_0, fusion_node_map, false);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, compile_single_op_failed)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);
  Status ret = fe_ops_store_ptr->CompileSingleOp(node_0);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, update_op_info_store_success)
{
  FEOpsStoreInfo tbe_custom {
  2,
  "tbe-custom",
  EN_IMPL_CUSTOM_TBE,
  "./air/test/engines/nneng/st/testcase/ops_kernel_store/fe_config/tbe_custom_opinfo",
  ""};
  shared_ptr<fe::SubOpInfoStore> sub_ops_kernel_ptr = std::make_shared<fe::SubOpInfoStore>(tbe_custom);
  std::map<std::string, std::uint8_t> update_map;
  std::map<std::string, OpContent> op_content_map;
  OpContent op_content;
  update_map.emplace(std::make_pair("test", 1));
  sub_ops_kernel_ptr->GetOpContentByOpType("conv", op_content);
  sub_ops_kernel_ptr->op_content_map_.emplace(std::make_pair("conv", op_content));
  Status ret = sub_ops_kernel_ptr->UpdateOpInfoStore(update_map);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, finalize_opkernel_info_test) {
  OpKernelInfoPtr op_kernel_info = std::make_shared<OpKernelInfo>("FrameworkOp");
  InputOrOutputInfoPtr input_info_ptr = nullptr;
  op_kernel_info->input_infos_.push_back(input_info_ptr);
  InputOrOutputInfoPtr output_info_ptr = std::make_shared<InputOrOutputInfo>("y");
  op_kernel_info->output_infos_.push_back(output_info_ptr);
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, finalize_opkernel_info_test1) {
  OpKernelInfoPtr op_kernel_info = std::make_shared<OpKernelInfo>("FrameworkOp");
  // InputOrOutputInfoPtr input_info_ptr = std::make_shared<InputOrOutputInfoPtr>("x");
  // op_kernel_info->input_infos_.push_back(input_info_ptr);
  InputOrOutputInfoPtr output_info_ptr = nullptr;
  op_kernel_info->output_infos_.push_back(output_info_ptr);
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, compile_and_set_kernel_name_for_atomic_clean){
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "MatMul");
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape);
    op_desc_0->AddOutputDesc(out_desc);
    ge::NodePtr test_node1 = graph->AddNode(op_desc_0);
    ge::NodePtr test_node2 = graph->AddNode(op_desc_0);
    test_node2->GetOpDesc()->SetExtAttr(ATTR_NAME_ATOMIC_CLEAN_NODE, test_node1);
    test_node2->GetOpDesc()->SetExtAttr(ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    std::string kernel_name = "te_matmul";
    ge::AttrUtils::SetStr(test_node2->GetOpDesc(), test_node2->GetOpDesc()->GetName() + "_kernelname", kernel_name);
    ge::AttrUtils::SetStr(test_node2->GetOpDesc(), "_unregst_oppath", "../../abs.py");
    std::vector<uint32_t> tmp_output_index {1, 1, 2, 1};
    ge::AttrUtils::SetListInt(test_node2->GetOpDesc(), TBE_OP_ATOMIC_OUTPUT_INDEX, tmp_output_index);
    const char tbe_bin[] = "tbe_bin";
    std::vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(test_node2->GetName(), std::move(buffer));
    test_node2->GetOpDesc()->SetExtAttr("tbeKernel", tbe_kernel_ptr);
    std::vector<ge::NodePtr> node_vec;
    std::vector<ge::NodePtr> atomic_clean_nodes;
    node_vec.push_back(test_node2);
    atomic_clean_nodes.push_back(test_node2);
    shared_ptr<FEOpsKernelInfoStore> ops_store = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
    Status ret = ops_store->CompileAndSetKernelNameForAtomicClean(node_vec, atomic_clean_nodes);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, fuzzy_compile_op_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");	
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetOriginShape(shape);
    in_desc1.SetDataType(DT_FLOAT16);
    dy_op->AddInputDesc("x", in_desc1);
    data->AddOutputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginFormat(FORMAT_HWCN);
    out_desc1.SetOriginShape(shape);
    out_desc1.SetDataType(DT_FLOAT16);
    dy_op->AddOutputDesc("z", out_desc1);

    ge::AttrUtils::SetStr(dy_op, ge::ATTR_NAME_UNREGST_OPPATH, "./impl/abc");		
    ge::AttrUtils::SetInt(dy_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
    ge::AttrUtils::SetStr(dy_op, "relu_kernelname", "relu_build");
    ge::AttrUtils::SetBool(dy_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    NodePtr relu_node = graph->AddNode(dy_op);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0)); 

    tbe_adapter_ptr_->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_adapter_ptr_->TeGeneralize = teGeneralize;
    std::vector<ge::NodePtr> node_vec{};
    node_vec.push_back(relu_node);
    Status ret = fe_ops_kernel_info_store_ptr->FuzzCompileOp(node_vec);
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, fuzzy_compile_fusionop_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("DynamicShape", "DynamicShape");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    in_desc1.SetOriginShape(shape);
    dy_op->AddInputDesc("x", in_desc1);
    data->AddInputDesc("x", in_desc1);
    data->AddOutputDesc("y", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    out_desc1.SetOriginShape(shape);
    dy_op->AddOutputDesc("z", out_desc1);

    NodePtr dy_node = graph->AddNode(dy_op);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), dy_node->GetInDataAnchor(0));
    ge::AttrUtils::SetInt(data, ge::ATTR_NAME_PARENT_NODE_INDEX, 0);
    ge::ComputeGraphPtr fusion_graph = std::make_shared<ge::ComputeGraph>("fusionop");
    OpDescPtr test_op = std::make_shared<OpDesc>("test", "test");
    test_op->AddInputDesc("x", in_desc1);
    test_op->AddOutputDesc("y", out_desc1);
    NodePtr test_node = fusion_graph->AddNode(test_op);
    AttrUtils::SetGraph(test_node->GetOpDesc(), "_original_fusion_graph", graph);
    ge::AttrUtils::SetBool(dy_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);

    OptimizeUtilityStubST *optimize_utility_stub = new OptimizeUtilityStubST();
    fe_ops_kernel_info_store_ptr->SetOptimizeUtility(optimize_utility_stub);
    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_adapter_ptr_->TeGeneralize = teGeneralize;
    std::vector<ge::NodePtr> node_vec{};
    node_vec.push_back(test_node);
    Status ret = fe_ops_kernel_info_store_ptr->FuzzCompileOp(node_vec);
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, update_diff_shape_change) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
    vector<int64_t> dims = {-1, -1, -1, 4};
    GeShape shape(dims);
    GeTensorDesc in_desc;
    in_desc.SetShape(shape);
    in_desc.SetOriginShape(shape);
    in_desc.SetFormat(FORMAT_ND_RNN_BIAS);
    in_desc.SetOriginFormat(FORMAT_ND);
    in_desc.SetDataType(DT_FLOAT16);
    in_desc.SetOriginShapeRange({{2,3},{1,3},{4,15},{4,4}});
    relu_op->AddInputDesc("x", in_desc);
    NodePtr relu_node = graph->AddNode(relu_op);
    fe_ops_kernel_info_store_ptr->UpdateNodeShapeAndRange(relu_node);

    vector<int64_t> res_dims = {-1, -1, -1, 64};
    std::vector<std::pair<int64_t, int64_t>> ori_shape_range;
    std::vector<std::pair<int64_t, int64_t>> res_shape_range = {{2, 3}, {1, 3}, {4, 15}, {64, 64}};
    EXPECT_EQ(relu_node->GetOpDesc()->MutableInputDesc(0)->GetShape().GetDims(), res_dims);
    relu_node->GetOpDesc()->MutableInputDesc(0)->GetShapeRange(ori_shape_range);
    EXPECT_EQ(ori_shape_range, res_shape_range);
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dim_shape_check_support_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape");
    vector<int64_t> dims = {-2};
    GeShape shape(dims);
    GeTensorDesc desc;
    desc.SetShape(shape);
    dy_op->AddInputDesc("x", desc);
    dy_op->AddOutputDesc("y", desc);
    NodePtr relu_node = graph->AddNode(dy_op);
    std::map<std::string, std::string> graph_options = GetThreadLocalContext().GetAllGraphOptions();
    graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";
    GetThreadLocalContext().SetGraphOption(graph_options);

    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedReasonRange;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_TRUE(res);
    EXPECT_TRUE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dim_shape_check_support_fail) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape");
    vector<int64_t> dims = {-2};
    GeShape shape(dims);
    GeTensorDesc desc;
    desc.SetShape(shape);
    dy_op->AddInputDesc("x", desc);
    dy_op->AddOutputDesc("y", desc);
    NodePtr relu_node = graph->AddNode(dy_op);
    std::map<std::string, std::string> graph_options = GetThreadLocalContext().GetAllGraphOptions();
    graph_options["ge.shape_generalized_build_mode"] = "shape_generalized";
    GetThreadLocalContext().SetGraphOption(graph_options);

    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedOtherReason;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_FALSE(res);
    EXPECT_FALSE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}

TEST_F(STEST_OP_KERNEL_INFO_STORE, dynamic_shape_check_support_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape");
    vector<int64_t> dims = {-1};
    GeShape shape(dims);
    GeTensorDesc desc;
    desc.SetShape(shape);
    dy_op->AddInputDesc("x", desc);
    dy_op->AddOutputDesc("y", desc);
    NodePtr relu_node = graph->AddNode(dy_op);

    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedReasonRange;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_TRUE(res);
    EXPECT_TRUE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}
