#include <gtest/gtest.h>
#include <iostream>
#include <list>

#define private public
#define protected public
#include "cmo/generate_cmo_type_prefetch.h"
#include "common/configuration.h"
#include "common/op_info_common.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/op_desc_utils.h"
#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

using GenerateCMOTypeBasePtr = std::shared_ptr<GenerateCMOTypeBase>;
class GenerateCmoTypePrefetchTest : public testing::Test{
protected:
  static void SetUpTestCase() {
    cout << "GenerateCmoTypePrefetchTest SetUp" << endl;
  }

  static void TearDownTestCase() {
    cout << "GenerateCmoTypePrefetchTest TearDwon" << endl;
  }
  
  virtual void SetUp() {
    cmo_type_base_ = std::make_shared<GenerateCMOTypePrefetch>();
  }

  virtual void TearDown() {
  }
  
public:
  GenerateCMOTypeBasePtr cmo_type_base_;
};

TEST_F(GenerateCmoTypePrefetchTest, GenerateTypeNoWeights) {
  OpDescPtr op_desc_ptr = make_shared<OpDesc>("name", "type");
  vector<int64_t> data_dims={2};
  GeTensorDesc data_tensor_desc(GeShape(data_dims), FORMAT_NCHW, DT_FLOAT);
  op_desc_ptr->AddInputDesc("input1", data_tensor_desc);

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node = graph->AddNode(op_desc_ptr);
  cmo_type_base_->GenerateType(node);

  map <std::string, std::vector<CmoAttr>> cmo;
  cmo = node->GetOpDesc()->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  EXPECT_EQ(cmo.size(), 0);
}

TEST_F(GenerateCmoTypePrefetchTest, GenerateTypeNoParent) {
  OpDescPtr op_desc_ptr = make_shared<OpDesc>("name", "type");
  vector<int64_t> data_dims={2};
  vector<int> dims_value_vec = {2, 3};
  GeTensorDesc data_tensor_desc(GeShape(data_dims), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr dim_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)dims_value_vec.data(), sizeof(dims_value_vec));

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node = graph->AddNode(op_desc_ptr);
  vector<ge::GeTensorPtr> weights{dim_tensor};
  ge::OpDescUtils::SetWeights(node, weights);

  cmo_type_base_->GenerateType(node);
  map <std::string, std::vector<CmoAttr>> cmo;
  cmo = node->GetOpDesc()->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  EXPECT_EQ(cmo.size(), 0);
}

TEST_F(GenerateCmoTypePrefetchTest, GenerateTypePrefetch) {
  OpDescPtr op_desc_ptr1 = make_shared<OpDesc>("name1", "type1");
  OpDescPtr op_desc_ptr2 = make_shared<OpDesc>("name2", "type2");
  vector<int64_t> data_dims={2};
  vector<int> dims_value_vec = {2, 3};
  GeTensorDesc data_tensor_desc(GeShape(data_dims), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc data_tensor_desc1(GeShape(data_dims), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc data_tensor_desc2(GeShape(data_dims), FORMAT_NCHW, DT_FLOAT);
  ge::TensorUtils::SetWeightSize(data_tensor_desc, 8);
  GeTensorPtr dim_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)dims_value_vec.data(), sizeof(dims_value_vec));
  op_desc_ptr1->AddOutputDesc("output1", data_tensor_desc1);
  op_desc_ptr2->AddInputDesc("input1", data_tensor_desc2);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node1 = graph->AddNode(op_desc_ptr1);
  NodePtr node2 = graph->AddNode(op_desc_ptr2);
  vector<ge::GeTensorPtr> weights{dim_tensor};
  ge::OpDescUtils::SetWeights(node2, weights);
  AttrUtils::SetInt(node1->GetOpDesc(), FE_IMPLY_TYPE, 6);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  
  cmo_type_base_->GenerateType(node2);
  map <std::string, std::vector<CmoAttr>> cmo;
  cmo = node1->GetOpDesc()->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  EXPECT_EQ(cmo.size(), 1);
  EXPECT_EQ(cmo[kCmoPrefetch].size(), 1);
}

TEST_F(GenerateCmoTypePrefetchTest, GenerateTypeSamePrefetch) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  OpDescPtr op_desc_ptr1 = make_shared<OpDesc>("name1", "type1");
  OpDescPtr op_desc_ptr2 = make_shared<OpDesc>("name2", "type2");
  OpDescPtr op_desc_ptr3 = make_shared<OpDesc>("name3", "type3");
  OpDescPtr op_const = make_shared<OpDesc>("const", "Const");

  GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), FORMAT_NCHW, DT_FLOAT);
  op_const->AddInputDesc(const_tensor_desc);
  op_const->AddOutputDesc(const_tensor_desc);
  NodePtr node_const = graph->AddNode(op_const);

  GeTensorDesc data_tensor_desc(GeShape({3, 4, 5, 6}), FORMAT_NCHW, DT_FLOAT);
  op_desc_ptr1->AddOutputDesc("output1", data_tensor_desc);
  NodePtr node1 = graph->AddNode(op_desc_ptr1);

  op_desc_ptr2->AddInputDesc("input1", data_tensor_desc);
  op_desc_ptr2->AddInputDesc("input2", data_tensor_desc);
  op_desc_ptr2->AddOutputDesc("output1", data_tensor_desc);
  NodePtr node2 = graph->AddNode(op_desc_ptr2);

  op_desc_ptr3->AddInputDesc("input1", data_tensor_desc);
  op_desc_ptr3->AddInputDesc("input2", data_tensor_desc);
  NodePtr node3 = graph->AddNode(op_desc_ptr3);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node2->GetOutDataAnchor(0), node3->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_const->GetOutDataAnchor(0), node2->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_const->GetOutDataAnchor(0), node3->GetInDataAnchor(1));
  vector<int> dims_value_vec = {2, 3};
  ge::TensorUtils::SetWeightSize(const_tensor_desc, 8);
  GeTensorPtr dim_tensor = std::make_shared<GeTensor>(const_tensor_desc, (uint8_t *)dims_value_vec.data(), sizeof(dims_value_vec));
  vector<ge::GeTensorPtr> weights;
  weights.emplace_back(dim_tensor);
  ge::OpDescUtils::SetWeights(node2, weights);
  ge::OpDescUtils::SetWeights(node3, weights);
  AttrUtils::SetInt(node1->GetOpDesc(), FE_IMPLY_TYPE, 6);
  AttrUtils::SetInt(node2->GetOpDesc(), FE_IMPLY_TYPE, 6);
  AttrUtils::SetInt(node1->GetOpDesc(), ATTR_NAME_OP_READ_WRITE_INDEX, 0);
  AttrUtils::SetInt(node2->GetOpDesc(), ATTR_NAME_OP_READ_WRITE_INDEX, 1);
  AttrUtils::SetInt(node3->GetOpDesc(), ATTR_NAME_OP_READ_WRITE_INDEX, 2);
  cmo_type_base_->GenerateType(node2);
  map <std::string, std::vector<CmoAttr>> cmo;
  cmo = node1->GetOpDesc()->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  EXPECT_EQ(cmo.size(), 1);
  EXPECT_EQ(cmo[kCmoPrefetch].size(), 1);
  cmo_type_base_->GenerateType(node3);
  cmo.clear();
  cmo = node2->GetOpDesc()->TryGetExtAttr(kOpExtattrNameCmo, cmo);
  EXPECT_EQ(cmo.size(), 0);
}
