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
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/fe_graph_optimizer.h"
#include "graph/utils/graph_utils.h"
#include "graph/op_kernel_bin.h"
#include "common/graph_comm.h"
#include "common/fusion_op_comm.h"
#include "common/util/op_info_util.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;
using FusionOpCommPtr = std::shared_ptr<FusionOpComm>;

class UTEST_fusion_op_comm : public testing::Test
{
public:
    FusionOpCommPtr fusion_op_comm_ptr;
protected:
    void SetUp()
    {
        fusion_op_comm_ptr = make_shared<FusionOpComm>();
    }

    void TearDown()
    {
    }

/*
 * batchnorm
 *    |
 *   relu
 */
    static void CreateGraph(ComputeGraphPtr graph) {
      OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
      OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

      // add descriptor
      vector<int64_t> dims = {1,2,3,4};
      GeShape shape(dims);

      GeTensorDesc in_desc1(shape);
      in_desc1.SetFormat(FORMAT_FRACTAL_Z);
      in_desc1.SetDataType(DT_FLOAT16);
      bn_op->AddInputDesc("x", in_desc1);

      GeTensorDesc out_desc1(shape);
      out_desc1.SetFormat(FORMAT_NHWC);
      out_desc1.SetDataType(DT_FLOAT16);
      bn_op->AddOutputDesc("y", out_desc1);


      GeTensorDesc in_desc2(shape);
      in_desc2.SetFormat(FORMAT_NCHW);
      in_desc2.SetDataType(DT_FLOAT16);
      relu_op->AddInputDesc("x", in_desc2);

      GeTensorDesc out_desc2(shape);
      out_desc2.SetFormat(FORMAT_HWCN);
      out_desc2.SetDataType(DT_FLOAT16);
      relu_op->AddOutputDesc("y", out_desc2);


      ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
      ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

      NodePtr bn_node = graph->AddNode(bn_op);
      NodePtr relu_node = graph->AddNode(relu_op);

      GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));

  }
};

TEST_F(UTEST_fusion_op_comm, set_sgt_tbe_fusion_op_suc_0)
{
    ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    CreateGraph(graph);
    vector<ge::NodePtr> fus_nodelist;
    for (auto &node : graph->GetDirectNode()) {
        std::vector<std::string> str_val = {"magic_0", "magic_1"};
        ge::AttrUtils::SetListStr(node->GetOpDesc(), ge::TVM_ATTR_NAME_THREAD_MAGIC, str_val);
        vector<vector<int64_t>> int_i_val(2 ,vector<int64_t>(4, 32));
        ge::AttrUtils::SetListListInt(node->GetOpDesc(), "tbe_op_thread_atomic_output_index", int_i_val);
        vector<int32_t> int_w_val = {3, 6};
        ge::AttrUtils::SetListInt(node->GetOpDesc(), "tbe_op_thread_atomic_workspace_flag", int_w_val);
        vector<ge::Buffer> byte_val;
        ge::AttrUtils::SetListBytes(node->GetOpDesc(), "_thread_tbe_kernel_buffer", byte_val);
        vector<bool> bool_val = {true, false};
        ge::AttrUtils::SetListBool(node->GetOpDesc(), "_thread_is_n_batch_split", bool_val);
        vector<string> kernel_names = {"kernel_names_0", "kernel_names_1"};
        ge::AttrUtils::SetListStr(node->GetOpDesc(), "_thread_kernelname", kernel_names);
        fus_nodelist.push_back(node);
    }
    ge::OpDescPtr fus_opdef = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
    bool flag = false;
    if (fus_opdef == nullptr) {
        EXPECT_EQ(flag, true);
    }
    std::vector<ge::OpKernelBinPtr> list_buffer_vec;
    std::string key = fus_nodelist[0]->GetOpDesc()->GetName();
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr kernel_bin_ptr = std::make_shared<ge::OpKernelBin>(key, move(buffer));
    list_buffer_vec.push_back(kernel_bin_ptr);
    fus_nodelist[0]->GetOpDesc()->SetExtAttr("thread_tbeKernel", list_buffer_vec);
    Configuration::Instance(AI_CORE_NAME).content_map_["dump.originalnodes.enable"] = "true";
    ge::OpDescPtr op_desc = fusion_op_comm_ptr->SetMultiKernelTBEFusionOp(fus_nodelist, fus_opdef, "AIcoreEngine");
    if (op_desc != nullptr) {
        flag = true;
    }
    EXPECT_EQ(flag, true);
}


TEST_F(UTEST_fusion_op_comm, set_sgt_tbe_fusion_op_suc_1)
{
    ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    CreateGraph(graph);
    vector<ge::NodePtr> fus_nodelist;
    for (auto &node : graph->GetDirectNode()) {
        fus_nodelist.push_back(node);
    }
    ge::OpDescPtr fus_opdef = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
    bool flag = false;
    if (fus_opdef == nullptr) {
        EXPECT_EQ(flag, true);
    }
    std::vector<ge::OpKernelBinPtr> list_buffer_vec;
    std::string key = fus_nodelist[0]->GetOpDesc()->GetName();
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr kernel_bin_ptr = std::make_shared<ge::OpKernelBin>(key, move(buffer));
    list_buffer_vec.push_back(kernel_bin_ptr);
    fus_nodelist[0]->GetOpDesc()->SetExtAttr("thread_tbeKernel", list_buffer_vec);
    ge::OpDescPtr op_desc = fusion_op_comm_ptr->SetMultiKernelTBEFusionOp(fus_nodelist, fus_opdef, "AIcoreEngine");
    if (op_desc != nullptr) {
        flag = true;
    }
    EXPECT_EQ(flag, true);
}

TEST_F(UTEST_fusion_op_comm, set_sgt_tbe_fusion_op_fail)
{
    ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    CreateGraph(graph);
    vector<ge::NodePtr> fus_nodelist;
    ge::OpDescPtr fus_opdef = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
    bool flag = false;
    if (fus_opdef == nullptr) {
        EXPECT_EQ(flag, true);
    }
    ge::OpDescPtr op_desc = fusion_op_comm_ptr->SetMultiKernelTBEFusionOp(fus_nodelist, fus_opdef, "AIcoreEngine");
    if (op_desc != nullptr) {
        flag = true;
    }
    EXPECT_EQ(flag, false);
}

TEST_F(UTEST_fusion_op_comm, set_tbe_fusion_op_suc)
{
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  vector<ge::NodePtr> fus_nodelist;
  for (auto &node : graph->GetDirectNode()) {
    fus_nodelist.push_back(node);
  }
  ge::OpDescPtr fus_opdef = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
  bool flag = false;
  if (fus_opdef == nullptr) {
    EXPECT_EQ(flag, true);
  }
  std::vector<ge::OpKernelBinPtr> list_buffer_vec;
  std::string key = fus_nodelist[0]->GetOpDesc()->GetName();
  const char tbe_bin[] = "tbe_bin";
  vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
  ge::OpKernelBinPtr kernel_bin_ptr = std::make_shared<ge::OpKernelBin>(key, move(buffer));
  list_buffer_vec.push_back(kernel_bin_ptr);
  ge::AttrUtils::SetBool(fus_nodelist[0]->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, true);
  fus_nodelist[0]->GetOpDesc()->SetExtAttr("_mix_aictbeKernel", kernel_bin_ptr);
  fus_nodelist[0]->GetOpDesc()->SetExtAttr("_mix_aivtbeKernel", kernel_bin_ptr);
  fus_nodelist[0]->GetOpDesc()->SetExtAttr("_mix_aivtbe_kernel_name", kernel_bin_ptr);
  fus_nodelist[0]->GetOpDesc()->SetExtAttr("_mix_aictbe_kernel_name", kernel_bin_ptr);

  ge::AttrUtils::SetStr(fus_nodelist[0]->GetOpDesc(), "_mix_aic" + fus_nodelist[0]->GetOpDesc()->GetName() +
                        "_kernelname", "aic_kernel_name");
  ge::AttrUtils::SetStr(fus_nodelist[0]->GetOpDesc(), "_mix_aiv" + fus_nodelist[0]->GetOpDesc()->GetName() +
                        "_kernelname", "aiv_kernel_name");
  ge::AttrUtils::SetStr(fus_nodelist[0]->GetOpDesc(), fe::ATTR_NAME_ALIAS_ENGINE_NAME, "test");
  std::string kernel_name = "te_op_name";
  ge::AttrUtils::SetStr(fus_nodelist[0]->GetOpDesc(), ge::ATTR_NAME_TBE_KERNEL_NAME_FOR_LOAD, kernel_name);
  ge::AttrUtils::SetStr(fus_nodelist[0]->GetOpDesc(), fe::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIC");
  ge::OpDescPtr op_desc = fusion_op_comm_ptr->SetTBEFusionOp(fus_nodelist, fus_opdef, "AIcoreEngine");
  if (op_desc != nullptr) {
    flag = true;
  }
  EXPECT_EQ(flag, true);
}