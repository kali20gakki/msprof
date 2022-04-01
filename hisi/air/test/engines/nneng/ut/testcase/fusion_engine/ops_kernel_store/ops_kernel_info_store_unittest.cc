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

#include "common/fe_inner_attr_define.h"
#include "common/util/op_info_util.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/spacesize_calculator/tensor_compute_util.h"
#define private public
#define protected public
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

#include "graph/ge_tensor.h"
#include "graph/ge_local_context.h"
#include "fusion_manager/fusion_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "../../../../graph_constructor/graph_constructor.h"
#include "../../../../graph_constructor/graph_builder_utils.h"
#include "task_builder/task_builder.h"
#include "ops_store/sub_op_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include "common/op_info_common.h"
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
using ScopeJsonMap_t = std::map<int64_t, std::string>;

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
    TEST_LIST_STR
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
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;

te::LX_QUERY_STATUS GetOpInfoStubTestImplJudge(const te::TbeOpInfo &a, std::string &b) {
  return te::LX_QUERY_SUCC;
};
bool PreBuildTbeOpStubTestImplJudge(te::TbeOpInfo &a, uint64_t b, uint64_t c) {
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

class FEOpsKernelInfoStoreTest : public testing::Test{
 protected:
    static void SetUpTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    static void TearDownTestCase() {
        cout << "FEOpsKernelInfoStoreTest SetUP" << endl;
    }
    ge::NodePtr AddNode(ge::ComputeGraphPtr graph, const string &name, const string &type,
                        int32_t out_anchors_num = 1, int32_t in_anchors_num = 1) {
      ge::GeTensorDesc tensor_desc;
      ge::OpDescPtr opdesc = std::make_shared<ge::OpDesc>(name, type);
      for (int32_t i = 0; i < out_anchors_num; i++) {
        opdesc->AddOutputDesc(tensor_desc);
      }
      for (int32_t i = 0; i < in_anchors_num; i++) {
        opdesc->AddInputDesc(tensor_desc);
      }
      ge::NodePtr node = graph->AddNode(opdesc);
      return node;
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
      tbe_adapter_ptr_->GetOpInfo = GetOpInfoStubTestImplJudge;
      tbe_adapter_ptr_->PreBuildTbeOp = PreBuildTbeOpStubTestImplJudge;
      op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr_));
      fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, AI_CORE_NAME);
      FEOpsStoreInfo tbe_custom {
      2,
      "tbe-custom",
      EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      false,
      false};

      vector<FEOpsStoreInfo> store_info;
      store_info.emplace_back(tbe_custom);
      Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);
      OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

      fe_ops_kernel_info_store_ptr->Initialize(options);

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
      cout << "a test Set Up" << endl;
    }
    virtual void TearDown(){
        cout << "a test Tear Down" << endl;
        fe_ops_kernel_info_store_ptr->Finalize();

    }

    OpDescPtr CreateOpDescPtr(TestIter test_iter)
    {
        OpDescPtr desc_ptr = std::make_shared<OpDesc>("test_op_desc", "conv");
        if (test_iter == TEST_INT) {
            AttrUtils::SetInt(desc_ptr, ATTR_NAME_INT, 10);
        }else{
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

        return desc_ptr;
    }
 public:
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr;
    TbeOpStoreAdapterPtr tbe_adapter_ptr_;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input1_desc_ptr;
    shared_ptr<ge::GeTensorDesc> input2_desc_ptr;
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr;
    shared_ptr<ge::OpDesc> op_desc_ptr;
    FormatDtypeQuerierPtr format_dtype_querier_ptr_;
};

TEST_F(FEOpsKernelInfoStoreTest, initialize_fail){
    map<string, string> options;
    fe_ops_kernel_info_store_ptr = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo tbe_custom {
    2,
    "tbe-custom",
    EN_IMPL_CUSTOM_TBE,
    "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
    ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr->Initialize(options);
    Status ret = fe_ops_kernel_info_store_ptr->Initialize(options);

}

TEST_F(FEOpsKernelInfoStoreTest, initialize_succ){
    shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    map<string, string> options;
    FEOpsStoreInfo tbe_custom {
    2,
    "tbe-custom",
    EN_IMPL_CUSTOM_TBE,
    "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
    ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    Status ret = fe_ops_kernel_info_store_ptr->Initialize(options);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, initialize_read_json_not_exist){
    shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    map<string, string> options;
    FEOpsStoreInfo cce_custom {
    0,
    "cce_custom_opinfo",
    EN_IMPL_CUSTOM_CONSTANT_CCE,
    "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_not_exist",
    ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(cce_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    Status ret = fe_ops_kernel_info_store_ptr->Initialize(options);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, initialize_twice){
    shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    map<string, string> options;
    FEOpsStoreInfo tbe_custom {
    2,
    "tbe-custom",
    EN_IMPL_CUSTOM_TBE,
    "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
    ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    Status ret1 = fe_ops_kernel_info_store_ptr->Initialize(options);
    Status ret2 = fe_ops_kernel_info_store_ptr->Initialize(options);
    EXPECT_EQ(fe::SUCCESS, ret1);
    EXPECT_EQ(fe::SUCCESS, ret2);
}

TEST_F(FEOpsKernelInfoStoreTest, get_all_ops_kernel_info_succ){
    shared_ptr<map<string, ge::OpInfo>> infos = make_shared<map<string, ge::OpInfo>>();
    fe_ops_kernel_info_store_ptr->GetAllOpsKernelInfo(*(infos.get()));
    EXPECT_EQ(false, infos->empty());
    infos.reset();
}

TEST_F(FEOpsKernelInfoStoreTest, get_one_op_kernel_info_ptr)
{
    string op_type = "conv";
    string op_not_exist = "relu";
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", op_type);
    OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", op_not_exist);
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    EXPECT_EQ(op_kernel_info_ptr1, nullptr);
}

TEST_F(FEOpsKernelInfoStoreTest, get_high_prio_op_kernel_info_ptr)
{
  string op_type = "conv";
  string op_not_exist = "relu";

  OpKernelInfoPtr op_kernel_info_ptr;
  OpKernelInfoPtr op_kernel_info_ptr1;
  Status ret = fe_ops_kernel_info_store_ptr->GetHighPrioOpKernelInfoPtr(op_type, op_kernel_info_ptr);
  Status ret1 = fe_ops_kernel_info_store_ptr->GetHighPrioOpKernelInfoPtr(op_not_exist, op_kernel_info_ptr1);

  EXPECT_NE(nullptr, op_kernel_info_ptr);
  if(op_kernel_info_ptr != nullptr){
      EXPECT_EQ("conv", op_kernel_info_ptr->GetOpType());
  }

  EXPECT_EQ(fe::SUCCESS, ret);
  EXPECT_NE(fe::SUCCESS, ret1);
}

TEST_F(FEOpsKernelInfoStoreTest, finalize_succ){
    Status ret = fe_ops_kernel_info_store_ptr->Finalize();
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_supported_succ)
{
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_SUCCESS);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");

    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_supported)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
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

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(nullptr, op_kernel_info_ptr);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_not_exist)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
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

    AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);
    AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);
    AttrUtils::SetListStr(op_desc_ptr_t, "attrListStr", str_vec);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_supported_unkown_attr)
{
    std::map<std::string, std::string> options_t;
    shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_t =
        make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo cce_custom {
    1,
    "cce-custom",
    EN_IMPL_CUSTOM_TBE,
    "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo",
    ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(cce_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    Status stu = fe_ops_kernel_info_store_ptr_t->Initialize(options_t);
    EXPECT_EQ(fe::SUCCESS, stu);

    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    op_desc_ptr_t->SetName("cce_conv");
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
    AttrUtils::SetInt(op_desc_ptr_t, "transposX", int_value);
    AttrUtils::SetFloat(op_desc_ptr_t, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr_t,"attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr_t,"attrStr", str_value);

    // test the default of switch code in CheckAttrSupported;
    AttrUtils::SetListInt(op_desc_ptr_t, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr_t, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr_t, "attrListBool", bool_vec);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_int_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_INT);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_float_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_FLOAT);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}
TEST_F(FEOpsKernelInfoStoreTest, check_attr_str_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_STR);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_bool_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_BOOL);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_supported_list_bool_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_LIST_BOOL);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_supported_list_int_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_LIST_INT);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_list_float_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_LIST_FLOAT);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_attr_list_str_false)
{
    OpDescPtr op_desc_ptr_t = CreateOpDescPtr(TEST_LIST_STR);

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckAttrSupport(test_node, *(op_kernel_info_ptr.get()), reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail1)
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

  ge::DataType set_dtype2 = ge::DT_FLOAT;
  output0_desc_ptr->SetDataType(set_dtype2);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  string un_supported_reason;
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail2)
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
  op_desc_ptr_t->AddInputDesc("h", input2_desc_ptr->Clone());

  string un_supported_reason;
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail3)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> input2_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output1_desc_ptr = make_shared<ge::GeTensorDesc>();
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

  std::vector<int64_t> shape_vec1{256,256,512};
  ge::GeShape shape_desc1 = GeShape(shape_vec1);
  input1_desc_ptr->SetDataType(set_dtype);
  input1_desc_ptr->SetShape(shape_desc1);
  op_desc_ptr_t->AddInputDesc("ccc", input1_desc_ptr->Clone());

  output0_desc_ptr->SetDataType(set_dtype);
  output0_desc_ptr->SetShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  output1_desc_ptr->SetDataType(set_dtype);
  output1_desc_ptr->SetShape(shape_desc);
  op_desc_ptr_t->AddOutputDesc("666", output1_desc_ptr->Clone());

  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  std::string un_supported_reason;
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_supported_fail2)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output1_desc_ptr = make_shared<ge::GeTensorDesc>();
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

    std::vector<int64_t> shape_vec1{256,256,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    op_desc_ptr_t->AddInputDesc("ccc", input1_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    output1_desc_ptr->SetDataType(set_dtype);
    output1_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("666", output1_desc_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_1)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("UnknownShape");

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{256,-1,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_2)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("DynamicShape");

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{256,-1,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
    EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_3)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("DynamicShape");

    ge::DataType set_dtype = ge::DT_INT32;
    std::vector<int64_t> shape_vec{256,-1,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_4)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("DynamicShape");

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{-2};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_5)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("DynamicRank");

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{-2};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_unknown_shape_6)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("DynamicRank");

    ge::DataType set_dtype = ge::DT_INT32;
    std::vector<int64_t> shape_vec{-2};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("y", output0_desc_ptr->Clone());
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_supported_fail3)
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

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
    output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-buildin");
    FEOpsStoreInfo tbe_builtin {
            0,
            "tbe-builtin",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
            ""};
    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_builtin);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);

    sub_ops_store_ptr->SetSubStoreInfo(tbe_builtin);
    bool stu = sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);

    std::shared_ptr<OpKernelInfo> tbe_op_kernel_info_ptr = std::make_shared<OpKernelInfo>("conv");
    std::shared_ptr<InputOrOutputInfo> InputInfoPtr1 = std::make_shared<InputOrOutputInfo>("x");
    std::shared_ptr<InputOrOutputInfo> InputInfoPtr2 = std::make_shared<InputOrOutputInfo>("y");
    std::shared_ptr<InputOrOutputInfo> InputInfoPtr3 = std::make_shared<InputOrOutputInfo>("z");
    InputInfoPtr1->supported_dtypes_ = {ge::DT_FLOAT16};
    InputInfoPtr2->supported_dtypes_ = {ge::DT_FLOAT16};
    InputInfoPtr3->supported_dtypes_ = {ge::DT_FLOAT16};
    std::shared_ptr<InputOrOutputInfo> OutputInfoPtr = std::make_shared<InputOrOutputInfo>("o");
    OutputInfoPtr->supported_dtypes_ = {ge::DT_UNDEFINED};
    tbe_op_kernel_info_ptr->input_infos_ = {InputInfoPtr1, InputInfoPtr2, InputInfoPtr3};
    tbe_op_kernel_info_ptr->output_infos_ = {OutputInfoPtr};
    sub_ops_kernel_ptr->op_kernel_info_map_.emplace(std::make_pair("conv", tbe_op_kernel_info_ptr));

    fe::UnSupportedReason reason;
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret1 = sub_ops_store_ptr->CheckSubStoreSupported(test_node, tbe_op_kernel_info_ptr, reason, CheckSupportMode::DTYPE_FORMAT_MODE, false);
    EXPECT_EQ(false, ret1);
    tbe_op_kernel_info_ptr->input_infos_.clear();
    std::shared_ptr<InputOrOutputInfo> InputInfoPtr4 = std::make_shared<InputOrOutputInfo>("z");
    InputInfoPtr4->supported_dtypes_ = {ge::DT_UNDEFINED};
    tbe_op_kernel_info_ptr->input_infos_ = {InputInfoPtr1, InputInfoPtr2, InputInfoPtr4};
    sub_ops_kernel_ptr->op_kernel_info_map_.clear();
    sub_ops_kernel_ptr->op_kernel_info_map_.emplace(std::make_pair("conv", tbe_op_kernel_info_ptr));
    bool ret2 = sub_ops_store_ptr->CheckSubStoreSupported(test_node, tbe_op_kernel_info_ptr, reason, CheckSupportMode::DTYPE_FORMAT_MODE, false);
    EXPECT_EQ(false, ret2);
}

TEST_F(FEOpsKernelInfoStoreTest, check_supported_fail4)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv");
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.clear();
    string un_supported_reason;
    SubOpsStorePtr sub_ops_kernel_info_store_ptr = nullptr;
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.emplace(std::make_pair("tbe-custom", sub_ops_kernel_info_store_ptr));
    bool ret1 = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret1);

    shared_ptr<ge::OpDesc> op_desc_ptr_t2 = make_shared<ge::OpDesc>();
    op_desc_ptr_t2->SetName("tbe_conv");
    op_desc_ptr_t2->SetType("conv");
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.clear();
    bool ret2 = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t2, un_supported_reason);
    EXPECT_EQ(false, ret2);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_supported_succ)
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


    
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    fe::SupportedFormatAndDtype info(op_kernel_info_ptr, "");
    info.input_index_name_map.emplace(0, "x");
    info.input_index_name_map.emplace(1, "y");
    info.input_index_name_map.emplace(2, "h");
    info.output_index_name_map.emplace(0, "z");
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
            info.op_kernel_info_ptr, *(op_desc_ptr_t.get()), false, info.suppport_formats_map, info
            .support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);

    bool ret = sub_ops_store_ptr->CheckInputSupported(test_node, 3, info);
    bool ret1 = sub_ops_store_ptr->CheckOutputSupported(test_node, 1, info);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(true, ret1);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_supported_fail)
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

    std::vector<int64_t> shape_vec1{256,256,512};
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


    
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    SupportedFormatAndDtype info(op_kernel_info_ptr, "");
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
            info.op_kernel_info_ptr, *(op_desc_ptr_t.get()), false, info.suppport_formats_map, info.support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckInputSupported(test_node, 3, info);
    bool ret1 = sub_ops_store_ptr->CheckOutputSupported(test_node, 1, info);
    EXPECT_EQ(false, ret);
    EXPECT_EQ(false, ret1);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_supported_2)
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

    std::vector<int64_t> shape_vec1{256,256,512};
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

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    SupportedFormatAndDtype info(op_kernel_info_ptr, "");

    info.input_index_name_map.emplace(0, "q");
    info.input_index_name_map.emplace(1, "w");
    info.input_index_name_map.emplace(2, "e");
    info.output_index_name_map.emplace(0, "asdf");
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(info.op_kernel_info_ptr,
        *(op_desc_ptr_t.get()), false, info.suppport_formats_map, info.support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);

    bool ret = sub_ops_store_ptr->CheckInputSupported(test_node, 3, info);
    bool ret1 = sub_ops_store_ptr->CheckOutputSupported(test_node, 1, info);
    EXPECT_EQ(false, ret);
    EXPECT_EQ(false, ret1);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_supported_datetype_fail)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv2");
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

    input0_desc_ptr->SetDataType(ge::DT_UINT8);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    std::vector<int64_t> shape_vec1{256,256,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<int64_t> shape_vec2{256,256,512};
    ge::GeShape shape_desc2 = GeShape(shape_vec2);
    input2_desc_ptr->SetDataType(set_dtype);
    input2_desc_ptr->SetShape(shape_desc2);
    op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(ge::DT_UINT8);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv2");

    SupportedFormatAndDtype info(op_kernel_info_ptr, "");
    info.input_index_name_map.emplace(0, "x");
    info.input_index_name_map.emplace(1, "y");
    info.input_index_name_map.emplace(2, "h");
    info.output_index_name_map.emplace(0, "z");

    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(info.op_kernel_info_ptr,
            *(op_desc_ptr_t.get()), false, info.suppport_formats_map, info.support_data_types_map);

    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    bool ret = sub_ops_store_ptr->CheckInputSupported(test_node, 3, info);
    bool ret1 = sub_ops_store_ptr->CheckOutputSupported(test_node, 1, info);
    EXPECT_EQ(false, ret);
    EXPECT_EQ(false, ret1);
}

TEST_F(FEOpsKernelInfoStoreTest, check_dtype_false)
{
    shared_ptr<ge::GeTensorDesc> input_ptr = make_shared<ge::GeTensorDesc>();
    OpDescPtr test_op_desc_ptr = CreateOpDescPtr(TEST_SUCCESS);
    ge::DataType set_dtype = ge::DT_UINT64;
    ge::Format set_format = ge::FORMAT_ND;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    input_ptr->SetDataType(set_dtype);
    input_ptr->SetFormat(set_format);
    input_ptr->SetShape(shape_desc);
    test_op_desc_ptr->AddInputDesc("x", input_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    InputOrOutputInfoPtr input_info_ptr;
    op_kernel_info_ptr->GetInputInfoByName("x", input_info_ptr);

    map<string, vector<ge::Format>> support_formats;
    map<string, vector<ge::DataType>> support_data_types;
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr,
            *(test_op_desc_ptr.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(test_op_desc_ptr);
    bool ret = sub_ops_store_ptr->CheckDtypeSupported(test_node, input_ptr, input_info_ptr,
            support_data_types.at(input_info_ptr->GetUniqueName()),
            op_kernel_info_ptr);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail4)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc> output1_desc_ptr = make_shared<ge::GeTensorDesc>();
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

    std::vector<int64_t> shape_vec1{256,256,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    op_desc_ptr_t->AddInputDesc("ccc", input1_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(set_dtype);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    output1_desc_ptr->SetDataType(set_dtype);
    output1_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("666", output1_desc_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail5)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv");
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.clear();
    string un_supported_reason;
    SubOpsStorePtr sub_ops_kernel_info_store_ptr = nullptr;
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.emplace(std::make_pair("tbe-custom", sub_ops_kernel_info_store_ptr));
    bool ret1 = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
    EXPECT_EQ(false, ret1);

    shared_ptr<ge::OpDesc> op_desc_ptr_t2 = make_shared<ge::OpDesc>();
    op_desc_ptr_t2->SetName("tbe_conv");
    op_desc_ptr_t2->SetType("conv");
    fe_ops_kernel_info_store_ptr->map_all_sub_store_info_.clear();
    bool ret2 = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t2, un_supported_reason);
    EXPECT_EQ(false, ret2);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail6)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
  op_desc_ptr_t->SetName("tbe_conv");
  op_desc_ptr_t->SetType("conv3");
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
  output0_desc_ptr->SetFormat(ge::FORMAT_NHWC);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NHWC);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  string un_supported_reason;
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv3");;
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail7)
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
  input2_desc_ptr->SetFormat(ge::FORMAT_HWCN);
  input2_desc_ptr->SetOriginFormat(ge::FORMAT_HWCN);
  op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

  ge::DataType set_dtype2 = ge::DT_FLOAT;
  output0_desc_ptr->SetDataType(set_dtype2);
  output0_desc_ptr->SetShape(shape_desc);
  output0_desc_ptr->SetFormat(ge::FORMAT_NCHW);
  output0_desc_ptr->SetOriginFormat(ge::FORMAT_NCHW);
  op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

  string un_supported_reason;
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_accuracy_supported_fail8)
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

  std::vector<int64_t> shape_vec2{256,256,1};
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

  string un_supported_reason;
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
  bool ret = fe_ops_kernel_info_store_ptr->CheckAccuracySupported(op_desc_ptr_t, un_supported_reason);
  // This fuction check input size, but the new version remove checking input size, so changing false to true.
  EXPECT_EQ(true, ret);
}

void CreateConv(ge::NodePtr &node, string op_type) {
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

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_accuracy_supported_succ) {
  ge::NodePtr test_node;
  CreateConv(test_node, "conv");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom",
                                                                                                        "conv");
  SupportedFormatAndDtype info(op_kernel_info_ptr, "");
  Status status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
      op_kernel_info_ptr, *(test_node->GetOpDesc().get()), false,
      info.suppport_formats_map, info.support_data_types_map);
  EXPECT_EQ(fe::SUCCESS, status);
  bool ret = sub_ops_store_ptr->CheckAllTensorsSupportedAccurateMode(test_node, info);
  EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, impl_judge_1) {
  ge::NodePtr test_node;
  CreateConv(test_node, "conv");

  OpImplTypeJudge impl_judge("AiCoreEngine", fe_ops_kernel_info_store_ptr);
  Status result = impl_judge.JudgeByNode(test_node);
  EXPECT_EQ(result, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, impl_judge_2) {
  ge::NodePtr test_node;
  CreateConv(test_node, "conv_dynamic");

  OpImplTypeJudge impl_judge("AiCoreEngine", fe_ops_kernel_info_store_ptr);
  Status result = impl_judge.JudgeByNode(test_node);
  EXPECT_EQ(result, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_accuracy_supported_fail)
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

    std::vector<int64_t> shape_vec1{256,256,512};
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

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    SupportedFormatAndDtype info(op_kernel_info_ptr, "");
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    Status status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr_t.get()), false,
            info.suppport_formats_map, info.support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, status);
    bool ret = sub_ops_store_ptr->CheckAllTensorsSupportedAccurateMode(test_node, info);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_accuracy_supported_2)
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

    std::vector<int64_t> shape_vec1{256,256,512};
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

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    SupportedFormatAndDtype info(op_kernel_info_ptr, "");

    info.input_index_name_map.emplace(0, "q");
    info.input_index_name_map.emplace(1, "w");
    info.input_index_name_map.emplace(2, "e");
    info.output_index_name_map.emplace(0, "asdf");
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    Status status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr_t.get()), false,
            info.suppport_formats_map, info.support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, status);
    bool ret = sub_ops_store_ptr->CheckAllTensorsSupportedAccurateMode(test_node, info);
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, check_input_output_accuracy_supported_datetype_fail)
{
    shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>();
    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  input2_desc_ptr = make_shared<ge::GeTensorDesc>();
    shared_ptr<ge::GeTensorDesc>  output0_desc_ptr = make_shared<ge::GeTensorDesc>();
    op_desc_ptr_t->SetName("tbe_conv");
    op_desc_ptr_t->SetType("conv2");
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

    input0_desc_ptr->SetDataType(ge::DT_UINT8);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddInputDesc("x", input0_desc_ptr->Clone());

    std::vector<int64_t> shape_vec1{256,256,512};
    ge::GeShape shape_desc1 = GeShape(shape_vec1);
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc1);
    op_desc_ptr_t->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<int64_t> shape_vec2{256,256,512};
    ge::GeShape shape_desc2 = GeShape(shape_vec2);
    input2_desc_ptr->SetDataType(set_dtype);
    input2_desc_ptr->SetShape(shape_desc2);
    op_desc_ptr_t->AddInputDesc("x1", input2_desc_ptr->Clone());

    output0_desc_ptr->SetDataType(ge::DT_UINT8);
    output0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr_t->AddOutputDesc("z", output0_desc_ptr->Clone());

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(op_desc_ptr_t);
    
    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);

    OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv2");
    SupportedFormatAndDtype info(op_kernel_info_ptr, "");

    info.input_index_name_map.emplace(0, "x");
    info.input_index_name_map.emplace(1, "y");
    info.input_index_name_map.emplace(2, "h");
    info.output_index_name_map.emplace(0, "z");

    Status status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(
            op_kernel_info_ptr, *(op_desc_ptr_t.get()), false,
            info.suppport_formats_map, info.support_data_types_map);
    EXPECT_EQ(fe::SUCCESS, status);
    bool ret = sub_ops_store_ptr->CheckAllTensorsSupportedAccurateMode(test_node, info);
    EXPECT_EQ(false, ret);
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

    impl_type = EN_IMPL_HW_TBE;
    return fe::SUCCESS;
}

Status CompileOpGetTvmJsonInfoStubTbezz(FEOpsKernelInfoStore* This, ScopeNodeIdMap &fusion_nodes_map, ScopeJsonMap_t &scope_json_map) {
  for (auto &fusion_item : fusion_nodes_map) {
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>("test_tvm", std::move(buffer));
    (fusion_item.second)[0]->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  }
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

static void CreateUnknownOpGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "conv");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "conv");
    // add descriptor
    vector<int64_t> dims = {1,-1,-1,4};
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
    ge::AttrUtils::SetBool(relu_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    ge::AttrUtils::SetBool(bn_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr relu_node = graph->AddNode(relu_op);

    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
}

static void CreateAtomicOpGraph2(ComputeGraphPtr graph) {
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

  std::vector<uint32_t> tmp_output_index(1, 1);
  ge::AttrUtils::SetListInt(bn_op, TBE_OP_ATOMIC_OUTPUT_INDEX,
                                tmp_output_index);
  ge::AttrUtils::SetInt(bn_op, TBE_OP_ATOMIC_WORKSPACE_FLAG, 1);
  ge::AttrUtils::SetInt(bn_op, ATTR_NAME_IS_UNKNOWN_SHAPE_OP, 1);
  std::vector<int64_t> wksp{500,600,800};
  std::vector<int64_t> wkspsize{100,120,200};
  std::vector<int64_t> outputoffset{8500};
  bn_op->SetOutputOffset(outputoffset);
  bn_op->SetWorkspace(wksp);
  bn_op->SetWorkspaceBytes(wkspsize);

  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);

  GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
}

TEST_F(FEOpsKernelInfoStoreTest, check_format_nd_success)
{
    shared_ptr<ge::GeTensorDesc> input_ptr = make_shared<ge::GeTensorDesc>();
    OpDescPtr test_op_desc_ptr = CreateOpDescPtr(TEST_SUCCESS);
    ge::DataType set_dtype = ge::DT_UINT64;
    ge::Format set_format = ge::FORMAT_ND;
    std::vector<int64_t> shape_vec{256,256,512};
    ge::GeShape shape_desc = GeShape(shape_vec);

    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
    ge::NodePtr test_node = graph->AddNode(test_op_desc_ptr);
    input_ptr->SetDataType(set_dtype);
    input_ptr->SetOriginFormat(set_format);
    input_ptr->SetFormat(set_format);
    input_ptr->SetShape(shape_desc);
    test_op_desc_ptr->AddInputDesc("x", input_ptr->Clone());

    SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
    OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "conv");
    EXPECT_NE(op_kernel_info_ptr1, nullptr);
    InputOrOutputInfoPtr input_info_ptr1;
    op_kernel_info_ptr1->GetInputInfoByName("x", input_info_ptr1);

    map<string, vector<ge::Format>> support_formats;
    map<string, vector<ge::DataType>> support_data_types;
    Status get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr1,
            *(test_op_desc_ptr.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);

    bool ret1 = sub_ops_store_ptr->CheckFormatSupported(test_node,input_ptr, input_info_ptr1,
            support_formats.at(input_info_ptr1->GetUniqueName()));
    EXPECT_EQ(false, ret1);

    OpKernelInfoPtr op_kernel_info_ptr2 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "K");
    EXPECT_NE(op_kernel_info_ptr2, nullptr);
    InputOrOutputInfoPtr input_info_ptr2;
    op_kernel_info_ptr2->GetInputInfoByName("x", input_info_ptr2);
    get_format_dtype_status = format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr2,
            *(test_op_desc_ptr.get()), false, support_formats, support_data_types);
    EXPECT_EQ(fe::SUCCESS, get_format_dtype_status);
    bool ret2 = sub_ops_store_ptr->CheckFormatSupported(test_node, input_ptr, input_info_ptr2,
            support_formats.at(input_info_ptr2->GetUniqueName()));
    EXPECT_EQ(true, ret2);
}

TEST_F(FEOpsKernelInfoStoreTest, init_formatagnostic_op_fail)
{
    shared_ptr<FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    map<string, string> options;
    FEOpsStoreInfo tbe_custom {
            0,
            "cce_custom_opinfo",
            EN_IMPL_CUSTOM_CONSTANT_CCE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_dynamic_opinfo_fail",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    Status ret = fe_ops_kernel_info_store_ptr->Initialize(options);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_succ)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv4D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));

  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);
  std::string op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);

  FEOpsStoreInfo op_store_info1;
  Configuration::Instance(fe_ops_kernel_info_store_ptr->GetFEOpsKernelInfoStoreName()).
                          GetOpStoreInfoByImplType(EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, op_store_info1);
  OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("non-persistent-tbe-custom", "Conv4D");
  EXPECT_NE(op_kernel_info_ptr1, nullptr);

  std::string op_dsl_file_path1;
  if (op_kernel_info_ptr1 != nullptr &&
      !op_kernel_info_ptr1->GetOpImpPath().empty()) {
    op_dsl_file_path1 = op_kernel_info_ptr1->GetOpImpPath();
  } else {
    op_dsl_file_path1 = op_store_info1.op_impl_file_path;
  }
  std::string path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/op_imply/";
  char resoved_path[260] =  {0x00};
  realpath(path.c_str(), resoved_path);
  path = resoved_path;
  EXPECT_EQ(path, op_dsl_file_path1);

  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv4D");
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  NodePtr node_b = graph->AddNode(op_desc_b);
  ge::AttrUtils::SetBool(node_b->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_b->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_kernel_map_.clear();
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_store_map_.clear();
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);

  FEOpsStoreInfo op_store_info2;
  Configuration::Instance(fe_ops_kernel_info_store_ptr->GetFEOpsKernelInfoStoreName()).
                          GetOpStoreInfoByImplType(EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, op_store_info2);
  OpKernelInfoPtr op_kernel_info_ptr2 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("non-persistent-tbe-custom", "Conv4D");
  EXPECT_NE(op_kernel_info_ptr2, nullptr);
  std::string op_dsl_file_path2;
  if (op_kernel_info_ptr2 != nullptr && !op_kernel_info_ptr2->GetOpImpPath().empty()) {
    op_dsl_file_path2 = op_kernel_info_ptr2->GetOpImpPath();
  } else {
    op_dsl_file_path2 = op_store_info2.op_impl_file_path;
  }
  path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/op_imply/";
  char resoved_path1[260] =  {0x00};
  realpath(path.c_str(), resoved_path1);
  path = resoved_path1;
  EXPECT_EQ(path, op_dsl_file_path2);
}

TEST_F(FEOpsKernelInfoStoreTest, compile_op_get_tvm_json_info_failed){
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
  Status ret = fe_ops_kernel_info_store_ptr->CompileOpGetTvmJsonInfo(fusion_nodes_map, scope_json_map);
  EXPECT_EQ(ret, OP_COMPILER_CHECK_FALSE_FAILED);
}

TEST_F(FEOpsKernelInfoStoreTest, CompileAtomicClean_failed){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);
  vector<ge::NodePtr> node_vec = {node_0};
  Status ret = fe_ops_kernel_info_store_ptr->CompileAtomicClean(node_vec);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, pre_compile_and_compile_success)
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
  Status ret = fe_ops_kernel_info_store_ptr->PreCompileAndCompile(node_map, node_0, fusion_node_map, false);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, fuzz_pre_compile_and_compile_success)
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
  Status ret = fe_ops_kernel_info_store_ptr->PreCompileAndCompile(node_map, node_0, fusion_node_map, true);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, compile_single_op_failed)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);
  Status ret = fe_ops_kernel_info_store_ptr->CompileSingleOp(node_0);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_failed)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv5D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));
  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);

  std::string op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);

  node_a->GetOpDesc()->SetType("Conv6D");
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);

  node_a->GetOpDesc()->SetType("Conv7D");
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);

  node_a->GetOpDesc()->SetType("Conv4D");
  op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo1/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);

  op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo1.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_1)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv10D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));
  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);

  std::string op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_2)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv8D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));
  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);

  std::string op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_3)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv9D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));
  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_4)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv7D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));
  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);

  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_dynamic_custom_op_store_info_5)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", "Data");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "Conv4D");
  // add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  GeTensorDesc out_desc1(shape);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc1);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);

  OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  const_op->AddOutputDesc(src_tensor_desc);
  const_op->AddInputDesc(src_tensor_desc);
  auto const_node = graph->AddNode(const_op);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), node_a->GetInDataAnchor(1));

  ge::AttrUtils::SetBool(node_a->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, true);
  std::string op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_a->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_store_map_.clear();
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_kernel_map_.clear();
  Status ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);

  FEOpsStoreInfo op_store_info1;
  Configuration::Instance(fe_ops_kernel_info_store_ptr->GetFEOpsKernelInfoStoreName()).
                          GetOpStoreInfoByImplType(EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, op_store_info1);
  OpKernelInfoPtr op_kernel_info_ptr1 = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("non-persistent-tbe-custom", "Conv4D");
  EXPECT_NE(op_kernel_info_ptr1, nullptr);
  std::string op_dsl_file_path1;
  if (op_kernel_info_ptr1 != nullptr &&
      !op_kernel_info_ptr1->GetOpImpPath().empty()) {
    op_dsl_file_path1 = op_kernel_info_ptr1->GetOpImpPath();
  } else {
    op_dsl_file_path1 = op_store_info1.op_impl_file_path;
  }
  std::string path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/op_imply/";
  char resoved_path[260] =  {0x00};
  realpath(path.c_str(), resoved_path);
  path = resoved_path;
  EXPECT_EQ(path, op_dsl_file_path1);

  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv5D");
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  NodePtr node_b = graph->AddNode(op_desc_b);
  ge::AttrUtils::SetBool(node_b->GetOpDesc(), NON_PERSISTENT_CUSTOM_OP_FLAG, false);
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  op_store_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/non_persistent_tbe_custom_opinfo/non_persistent_tbe_custom_opinfo.json";
  ge::AttrUtils::SetStr(node_b->GetOpDesc(), CUSTOM_OP_IMPL_CONFIG_PATH, op_store_path);
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_kernel_map_.clear();
  OpsKernelManager::Instance(AI_CORE_NAME).sub_ops_store_map_.clear();
  ret = fe_ops_kernel_info_store_ptr->SetDynamicCustomOpStoreInfo(*graph);
  EXPECT_EQ(fe::SUCCESS, ret);

  std::string reason;
  for (auto node : graph->GetDirectNode()) {
    ret = fe_ops_kernel_info_store_ptr->CheckSupported(node->GetOpDesc(), reason);
    if (node->GetType() == "Conv5D") {
      EXPECT_EQ(false, ret);
    }
  }
}

TEST_F(FEOpsKernelInfoStoreTest, test_modify_mixlist_right) {
  FEOpsStoreInfo ops_store_info;
  SubOpInfoStore sub_op_info_store(ops_store_info);
  std::string json_file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo_modify_mixlist/op_store_right.json";
  std::string real_path = RealPath(json_file_path);
  sub_op_info_store.LoadOpJsonFile(real_path);
  std::string modify_mixlist_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo_modify_mixlist/op_info_right.json";
  real_path = RealPath(modify_mixlist_path);
  sub_op_info_store.LoadCustomJsonFile(real_path);

  auto iter = sub_op_info_store.op_content_map_.find("Yolo");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  auto op_content = iter->second;
  std::string precision_reduce = "";
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag",
                                                                 precision_reduce);
  EXPECT_EQ(precision_reduce, "true");
  EXPECT_EQ(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Matmul");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "false");
  EXPECT_EQ(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Conv2D");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "false");
  EXPECT_EQ(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Cast");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "false");
  EXPECT_EQ(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Add");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  precision_reduce = "";
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "");
  EXPECT_NE(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Abs");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  precision_reduce = "";
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "");
  EXPECT_NE(result, fe::SUCCESS);

  iter = sub_op_info_store.op_content_map_.find("Bias");
  ASSERT_NE(iter, sub_op_info_store.op_content_map_.end());
  op_content = iter->second;
  result = op_kernel_info_constructor.GetStrFromOpContent(op_content, "precision_reduce", "flag", precision_reduce);
  EXPECT_EQ(precision_reduce, "true");
  EXPECT_EQ(result, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, test_es_board_cast) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::GeShape original_shape = GeShape({3, 12, 5, 6});
  GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT, original_shape);

  test.AddOpDesc("Cast", fe::CAST, 1, 1);
  std::string reason;
  ge::NodePtr cast_node;
  test.GetNodeByName("Cast", cast_node);
  fe_ops_kernel_info_store_ptr->CheckSupported(cast_node->GetOpDesc(), reason);
}

TEST_F(FEOpsKernelInfoStoreTest, test_value_depend_case1) {
    OpKernelInfoPtr op_kernel_info_ptr =
            OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType(EN_IMPL_CUSTOM_TBE, "ValueDepend");
    EXPECT_NE(op_kernel_info_ptr, nullptr);
    for (InputOrOutputInfoPtr info_ptr : op_kernel_info_ptr->GetAllInputInfo()) {
        if (info_ptr->GetName() == "a") {
            EXPECT_EQ(info_ptr->GetConstValueDepend(), CONST_REQUIRED);
        }
        if (info_ptr->GetName() == "b") {
            EXPECT_EQ(info_ptr->GetConstValueDepend(), CONST_OPTIONAL);
        }
        if (info_ptr->GetName() == "c") {
            EXPECT_EQ(info_ptr->GetConstValueDepend(), CONST_IGNORE);
        }
        if (info_ptr->GetName() == "d") {
            EXPECT_EQ(info_ptr->GetConstValueDepend(), CONST_IGNORE);
        }
    }
}

TEST_F(FEOpsKernelInfoStoreTest, test_value_depend_case2) {
    OpDescPtr value_depend = std::make_shared<OpDesc>("value_depend", "ValueDepend");

    // add descriptor
    vector<int64_t> dims = {1,2,3,4};
    GeShape shape(dims);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetFormat(FORMAT_NCHW);
    tensor_desc.SetOriginFormat(FORMAT_NCHW);
    tensor_desc.SetDataType(DT_FLOAT);
    tensor_desc.SetOriginDataType(DT_FLOAT);

    value_depend->AddInputDesc("a", tensor_desc);
    value_depend->AddInputDesc("b", tensor_desc);
    value_depend->AddInputDesc("c", tensor_desc);
    value_depend->AddInputDesc("d", tensor_desc);
    value_depend->AddOutputDesc("z", tensor_desc);
    vector<bool> is_input_const = {true, true, false, false};
    value_depend->SetIsInputConst(is_input_const);

    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(value_depend, un_supported_reason);
    cout << un_supported_reason << endl;
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, test_value_depend_case3) {
    OpDescPtr value_depend = std::make_shared<OpDesc>("value_depend", "ValueDepend");

    // add descriptor
    vector<int64_t> dims = {1,2,3,4};
    GeShape shape(dims);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetFormat(FORMAT_NCHW);
    tensor_desc.SetOriginFormat(FORMAT_NCHW);
    tensor_desc.SetDataType(DT_FLOAT);
    tensor_desc.SetOriginDataType(DT_FLOAT);

    value_depend->AddInputDesc("a", tensor_desc);
    value_depend->AddInputDesc("b", tensor_desc);
    value_depend->AddInputDesc("c", tensor_desc);
    value_depend->AddInputDesc("d", tensor_desc);
    value_depend->AddOutputDesc("z", tensor_desc);
    vector<bool> is_input_const = {false, false, true, false};
    value_depend->SetIsInputConst(is_input_const);

    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(value_depend, un_supported_reason);
    cout << un_supported_reason << endl;
    EXPECT_EQ(false, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, test_value_depend_case4) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
    ge::NodePtr value_depend1 = AddNode(graph, "value_depend", fe::CONSTANT);
    ge::NodePtr value_depend2 = AddNode(graph, "value_depend", fe::CONSTANT);
    ge::NodePtr value_depend3 = AddNode(graph, "value_depend", fe::DATA);
    ge::NodePtr value_depend4 = AddNode(graph, "value_depend", fe::DATA);
    ge::NodePtr value_depend5 = AddNode(graph, "value_depend", "ValueDepend", 1, 4);
    ge::NodePtr out_node = AddNode(graph, "out", "Upsample");

    vector<int64_t> dims = {1,2,3,4};
    GeShape shape(dims);

    value_depend1->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
    value_depend1->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend1->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend1->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
    value_depend1->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);
    value_depend2->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
    value_depend2->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend2->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend2->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
    value_depend2->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);
    value_depend3->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
    value_depend3->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend3->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend3->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
    value_depend3->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);
    value_depend4->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
    value_depend4->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend4->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend4->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
    value_depend4->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);

    value_depend5->GetOpDesc()->MutableInputDesc(0)->SetShape(shape);
    value_depend5->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(0)->SetDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(0)->SetOriginDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(1)->SetShape(shape);
    value_depend5->GetOpDesc()->MutableInputDesc(1)->SetFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(1)->SetOriginFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(1)->SetDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(1)->SetOriginDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(2)->SetShape(shape);
    value_depend5->GetOpDesc()->MutableInputDesc(2)->SetFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(2)->SetOriginFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(2)->SetDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(2)->SetOriginDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(3)->SetShape(shape);
    value_depend5->GetOpDesc()->MutableInputDesc(3)->SetFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(3)->SetOriginFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableInputDesc(3)->SetDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableInputDesc(3)->SetOriginDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
    value_depend5->GetOpDesc()->MutableOutputDesc(0)->SetFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableOutputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    value_depend5->GetOpDesc()->MutableOutputDesc(0)->SetDataType(DT_FLOAT);
    value_depend5->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(DT_FLOAT);

    out_node->GetOpDesc()->MutableInputDesc(0)->SetShape(shape);
    out_node->GetOpDesc()->MutableInputDesc(0)->SetFormat(FORMAT_NCHW);
    out_node->GetOpDesc()->MutableInputDesc(0)->SetOriginFormat(FORMAT_NCHW);
    out_node->GetOpDesc()->MutableInputDesc(0)->SetDataType(DT_FLOAT);
    out_node->GetOpDesc()->MutableInputDesc(0)->SetOriginDataType(DT_FLOAT);

    ge::GraphUtils::AddEdge(value_depend1->GetOutDataAnchor(0), value_depend5->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(value_depend2->GetOutDataAnchor(0), value_depend5->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(value_depend3->GetOutDataAnchor(0), value_depend5->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(value_depend4->GetOutDataAnchor(0), value_depend5->GetInDataAnchor(3));
    ge::GraphUtils::AddEdge(value_depend5->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));

    std::string un_supported_reason;
    bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(value_depend5, un_supported_reason);
    cout << un_supported_reason << endl;
    EXPECT_EQ(true, ret);
}

TEST_F(FEOpsKernelInfoStoreTest, set_cut_info_01)
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

TEST_F(FEOpsKernelInfoStoreTest, precompile_01)
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
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
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

TEST_F(FEOpsKernelInfoStoreTest, precompile_02)
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

TEST_F(FEOpsKernelInfoStoreTest, dynamic_compile_static_1)
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
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
}

TEST_F(FEOpsKernelInfoStoreTest, dynamic_compile_static_2)
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
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), false);
}

TEST_F(FEOpsKernelInfoStoreTest, dynamic_compile_static_3)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_1", "DynamicCompileStatic");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, -1, 100, 100};
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
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), false);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), false);
}

TEST_F(FEOpsKernelInfoStoreTest, dynamic_compile_static_4)
{
  shared_ptr<ge::OpDesc> op_desc_ptr_t = make_shared<ge::OpDesc>("DynamicCompileStatic_op_2", "DynamicCompileStatic2");
  shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
  shared_ptr<ge::GeTensorDesc> output0_desc_ptr = make_shared<ge::GeTensorDesc>();

  ge::DataType set_dtype = ge::DT_FLOAT16;
  ge::Format set_format = ge::FORMAT_NCHW;
  std::vector<int64_t> shape_vec{4, -1, 100, 100};
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
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, true);
  EXPECT_EQ(ge::AttrUtils::HasAttr(op_desc_ptr_t, ATTR_NAME_IS_OP_DYNAMIC_IMPL), true);
  EXPECT_EQ(IsOpDynamicImpl(op_desc_ptr_t), true);
}

TEST_F(FEOpsKernelInfoStoreTest, reshape_1)
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
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
}

TEST_F(FEOpsKernelInfoStoreTest, reshape_2)
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
  OpKernelInfoPtr op_kernel_info_ptr = OpsKernelManager::Instance(AI_CORE_NAME).GetOpKernelInfoByOpType("tbe-custom", "UnknownShape");
  SubOpsStorePtr sub_ops_store_ptr = std::make_shared<SubOpsStore>(op_store_adapter_manager_ptr_);
  std::string un_supported_reason;
  bool ret = fe_ops_kernel_info_store_ptr->CheckSupported(op_desc_ptr_t, un_supported_reason);
  EXPECT_EQ(ret, false);
}

TEST_F(FEOpsKernelInfoStoreTest, update_op_info_store_success)
{
  FEOpsStoreInfo tbe_custom {
  2,
  "tbe-custom",
  EN_IMPL_CUSTOM_TBE,
  "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
  "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
  false,
  false};
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

TEST_F(FEOpsKernelInfoStoreTest, finalize_opkernel_info_test) {
  OpKernelInfoPtr op_kernel_info = std::make_shared<OpKernelInfo>("FrameworkOp");
  InputOrOutputInfoPtr input_info_ptr = nullptr;
  op_kernel_info->input_infos_.push_back(input_info_ptr);
  InputOrOutputInfoPtr output_info_ptr = nullptr;
  op_kernel_info->output_infos_.push_back(output_info_ptr);
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, finalize_opkernel_info_test1) {
  OpKernelInfoPtr op_kernel_info = std::make_shared<OpKernelInfo>("FrameworkOp");
  // InputOrOutputInfoPtr input_info_ptr = std::make_shared<InputOrOutputInfoPtr>("x");
  // op_kernel_info->input_infos_.push_back(input_info_ptr);
  InputOrOutputInfoPtr output_info_ptr = nullptr;
  op_kernel_info->output_infos_.push_back(output_info_ptr);
  OpKernelInfoConstructor op_kernel_info_constructor;
  Status ret = op_kernel_info_constructor.FinalizeOpKernelInfo(op_kernel_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, compile_and_set_kernel_name_for_atomic_clean){
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

TEST_F(FEOpsKernelInfoStoreTest, fuzzy_compile_op_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");	
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "MatMul");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    in_desc1.SetOriginShape(shape);
    relu_op->AddInputDesc("x", in_desc1);
    data->AddOutputDesc("x", in_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_NCHW);
    in_desc2.SetOriginFormat(FORMAT_NCHW);
    in_desc2.SetDataType(DT_FLOAT16);
    in_desc2.SetOriginShape(shape);
    relu_op->AddInputDesc("y", in_desc2);
    data2->AddOutputDesc("y", in_desc2);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    out_desc1.SetOriginShape(shape);
    relu_op->AddOutputDesc("z", out_desc1);

    ge::AttrUtils::SetStr(relu_op, ge::ATTR_NAME_UNREGST_OPPATH, "./impl/abc");		
    ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
    ge::AttrUtils::SetStr(relu_op, "relu_kernelname", "relu_build");
    ge::AttrUtils::SetBool(relu_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    NodePtr relu_node = graph->AddNode(relu_op);
    NodePtr data_node = graph->AddNode(data);
    NodePtr data_node2 = graph->AddNode(data2);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0)); 
    GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), relu_node->GetInDataAnchor(1));
    FEOpsStoreInfo tbe_builtin {
            6,
            "tbe-builtin",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
            ""};
    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-builtin");
    sub_ops_store_ptr->SetSubStoreInfo(tbe_builtin);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_builtin);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_builtin);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-builtin", sub_ops_kernel_ptr);
    tbe_adapter_ptr_->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_adapter_ptr_->TeGeneralize = teGeneralize;
    std::vector<ge::NodePtr> node_vec{};
    node_vec.push_back(relu_node);
    Status ret = fe_ops_kernel_info_store_ptr->FuzzCompileOp(node_vec);
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FEOpsKernelInfoStoreTest, fuzzy_compile_fusionop_success) {
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
    NodePtr test_node = fusion_graph->AddNode(test_op);
    test_op->AddInputDesc("x", in_desc1);
    test_op->AddOutputDesc("y", out_desc1);
    AttrUtils::SetGraph(test_node->GetOpDesc(), "_original_fusion_graph", graph);

    ge::AttrUtils::SetBool(dy_op, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, false);
    FEOpsStoreInfo tbe_custom {
            6,
            "tbe-custom",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo"};
    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_custom);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-custom", sub_ops_kernel_ptr);

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

TEST_F(FEOpsKernelInfoStoreTest, update_diff_shape_change) {
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

TEST_F(FEOpsKernelInfoStoreTest, dim_shape_check_support_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape1");
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

    FEOpsStoreInfo tbe_custom {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo"};
    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_custom);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-custom", sub_ops_kernel_ptr);
    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedReasonRange;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_TRUE(res);
    EXPECT_TRUE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}

TEST_F(FEOpsKernelInfoStoreTest, dim_shape_check_support_fail) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape1");
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

    FEOpsStoreInfo tbe_custom {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo"};
    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_custom);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-custom", sub_ops_kernel_ptr);
    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedOtherReason;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_FALSE(res);
    EXPECT_FALSE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}

TEST_F(FEOpsKernelInfoStoreTest, dynamic_shape_check_support_success) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
    OpDescPtr dy_op = std::make_shared<OpDesc>("dynamicShape", "DynamicShape1");
    vector<int64_t> dims = {-1};
    GeShape shape(dims);
    GeTensorDesc desc;
    desc.SetShape(shape);
    dy_op->AddInputDesc("x", desc);
    dy_op->AddOutputDesc("y", desc);
    NodePtr relu_node = graph->AddNode(dy_op);

    FEOpsStoreInfo tbe_custom {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo"};
    SubOpsStorePtr sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr->SetSubStoreType("tbe-custom");
    sub_ops_store_ptr->SetSubStoreInfo(tbe_custom);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    SubOpInfoStorePtr sub_ops_kernel_ptr = std::make_shared<SubOpInfoStore>(tbe_custom);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-custom", sub_ops_kernel_ptr);
    tbe_adapter_ptr_->engine_name_ = fe::AI_CORE_NAME;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedReasonRange;

    UnSupportedReason reason;
    CheckSupportMode check_mode = CheckSupportMode::DTYPE_FORMAT_MODE;
    OpImplType impl_type = EN_IMPL_HW_TBE;
    bool res = fe_ops_kernel_info_store_ptr->CheckSupportedByOpsStore(relu_node, reason, impl_type, check_mode);
    EXPECT_TRUE(res);
    EXPECT_TRUE(AttrUtils::HasAttr(relu_node->GetOpDesc(), kOpShapeOrRangeUnsupport));
}
