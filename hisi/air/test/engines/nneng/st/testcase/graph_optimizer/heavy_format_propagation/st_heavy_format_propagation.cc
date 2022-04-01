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

#include <memory>

#include "common/util/op_info_util.h"

#define private public
#define protected public
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph_optimizer/heavy_format_propagation/heavy_format_propagation.h"

#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;
using HeavyFormatPropagationPtr = std::shared_ptr<HeavyFormatPropagation>;
class STEST_fusion_engine_heavy_format_distribution : public testing::Test
{
 protected:
  void SetUp()
  {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo heavy_op_info {
        6,
        "tbe-builtin",
        EN_IMPL_HW_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/heavy_opinfo",
        "",
        false,
        false};

    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(heavy_op_info);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);

    reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();
  }

  void TearDown()
  {

  }
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
 protected:

  static void CreateThreeGraph(ComputeGraphPtr graph) {
    /*              Const
     *        /                \
     *  conv (NC1HWC0)         a.m. (NCHW)
     *  After distribution, the input of a.m. will become NC1HWC0*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithScalar(ComputeGraphPtr graph) {
    /*              Const(Scalar)
     *        /                \
     *  conv (NC1HWC0)         a.m. (NCHW)
     *  After distribution, the input of a.m. will still be NCHW*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape());
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithScalar_1(ComputeGraphPtr graph) {
    /*              Const(Scalar, shape {3,4,5,6})
     *        /                     \
     *  conv (NC1HWC0,shape{})         a.m. (NCHW)
     *  After distribution, the input of a.m. will become NC1HWC0*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3,4,5,6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3,4,5,6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithScalar_2(ComputeGraphPtr graph) {
    /*              Const(shape {3,4,5,6})
     *            /                  \
     *     a.m.2 (NCHW,shape 0)       \
     *        /                        \
     *  conv (NC1HWC0,shape{})         a.m.1(NCHW)
     *  After distribution, the input of a.m. will still be NCHW */
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3,4,5,6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3,4,5,6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op2 = std::make_shared<OpDesc>("am2", "ApplyMomentum");
    GeTensorDesc am_tensor_desc2(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc2.SetOriginShape(GeShape());
    am_tensor_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op2->AddOutputDesc(am_tensor_desc2);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op2->AddInputDesc(am_tensor_desc2);
    }
    auto am_node_2 = graph->AddNode(apply_momentum_op2);
    ge::AttrUtils::SetInt(apply_momentum_op2, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node_2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node_2->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithL2LossAndAddN(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)      Conv2D(NC1HWC0)
     *          |                       /
     *        a.m.(NCHW)          L2Loss (NCHW)
     *               \           /
     *                 AddN(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr l2_loss_op = std::make_shared<OpDesc>("l2loss", "L2Loss");
    GeTensorDesc l2_loss_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    l2_loss_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc l2_loss_out_tensor_desc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_out_tensor_desc.SetOriginShape(GeShape({1}));
    l2_loss_out_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    l2_loss_op->AddInputDesc(l2_loss_tensor_desc);
    l2_loss_op->AddOutputDesc(l2_loss_out_tensor_desc);
    auto l2loss_node = graph->AddNode(l2_loss_op);
    ge::AttrUtils::SetInt(l2_loss_op, FE_IMPLY_TYPE, 6);

    OpDescPtr add_no_p = std::make_shared<OpDesc>("addn", "AddN");
    add_no_p->AddInputDesc(l2_loss_out_tensor_desc);
    add_no_p->AddInputDesc(l2_loss_out_tensor_desc);
    add_no_p->AddOutputDesc(l2_loss_out_tensor_desc);
    auto addn_Node = graph->AddNode(add_no_p);
    ge::AttrUtils::SetInt(add_no_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), l2loss_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(l2loss_node->GetOutDataAnchor(0), addn_Node->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), addn_Node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithL2LossAndAddN_1(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)      Conv2D(NC1HWC0)
     *          |                       /
     *        a.m.(NCHW)          L2Loss (NCHW)
     *               \           /
     *                 AddN(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr l2_loss_op = std::make_shared<OpDesc>("l2loss", "L2Loss");
    GeTensorDesc l2_loss_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    l2_loss_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc l2_loss_out_tensor_desc(GeShape(), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_out_tensor_desc.SetOriginShape(GeShape());
    l2_loss_out_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    l2_loss_op->AddInputDesc(l2_loss_tensor_desc);
    l2_loss_op->AddOutputDesc(l2_loss_out_tensor_desc);
    auto l2loss_node = graph->AddNode(l2_loss_op);
    ge::AttrUtils::SetInt(l2_loss_op, FE_IMPLY_TYPE, 6);

    OpDescPtr add_no_p = std::make_shared<OpDesc>("addn", "AddN");
    add_no_p->AddInputDesc(l2_loss_out_tensor_desc);
    add_no_p->AddInputDesc(l2_loss_out_tensor_desc);
    add_no_p->AddOutputDesc(l2_loss_out_tensor_desc);
    auto addn_Node = graph->AddNode(add_no_p);
    ge::AttrUtils::SetInt(add_no_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), l2loss_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(l2loss_node->GetOutDataAnchor(0), addn_Node->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), addn_Node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithBiasAdd(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)
     *          |
     *        a.m.(NCHW)
     *               \
     *                 BiasAdd(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr bias_add_o_p = std::make_shared<OpDesc>("bias_add", "BiasAddGrad");
    bias_add_o_p->AddInputDesc(am_tensor_desc);
    bias_add_o_p->AddOutputDesc(am_tensor_desc);
    auto bias_node = graph->AddNode(bias_add_o_p);
    ge::AttrUtils::SetInt(bias_add_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), bias_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }

  static void CreateThreeGraphWithTransData(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)
     *          |
     *        Transdata(NCHW->Fragz)
     *               \
     *                 BiasAdd(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr trans_op = std::make_shared<OpDesc>("trans", "TransData");
    GeTensorDesc trans_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    trans_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    trans_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    trans_op->AddOutputDesc(conv_tensor_desc_weight);
    trans_op->AddInputDesc(trans_tensor_desc);

    auto trans_node = graph->AddNode(trans_op);
    ge::AttrUtils::SetInt(trans_op, FE_IMPLY_TYPE, 6);

    OpDescPtr bias_add_o_p = std::make_shared<OpDesc>("bias_add", "BiasAddGrad");
    bias_add_o_p->AddInputDesc(trans_tensor_desc);
    bias_add_o_p->AddOutputDesc(trans_tensor_desc);
    auto bias_node = graph->AddNode(bias_add_o_p);
    ge::AttrUtils::SetInt(bias_add_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(trans_node->GetOutDataAnchor(0), bias_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), trans_node->GetInDataAnchor(0));
  }


  static void CreateThreeGraphWithTransData_2(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)
     *          |
     *        Transdata(Fragz->NCHW)
     *               \
     *                 BiasAdd(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr trans_op = std::make_shared<OpDesc>("trans", "TransData");
    GeTensorDesc trans_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    trans_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    trans_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    trans_op->AddOutputDesc(trans_tensor_desc);
    trans_op->AddInputDesc(conv_tensor_desc_weight);

    auto trans_node = graph->AddNode(trans_op);
    ge::AttrUtils::SetInt(trans_op, FE_IMPLY_TYPE, 6);

    OpDescPtr bias_add_o_p = std::make_shared<OpDesc>("bias_add", "BiasAddGrad");
    bias_add_o_p->AddInputDesc(trans_tensor_desc);
    bias_add_o_p->AddOutputDesc(trans_tensor_desc);
    auto bias_node = graph->AddNode(bias_add_o_p);
    ge::AttrUtils::SetInt(bias_add_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(trans_node->GetOutDataAnchor(0), bias_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), trans_node->GetInDataAnchor(0));
  }

  static void CreateThreeGraphWithL2LossAndMul(ComputeGraphPtr graph) {
    /* conv2_d_back_prop_filter(Fragz)
     *          |
     *        a.m.(NCHW)          L2Loss (NCHW)
     *               \           /
     *                 Mul(NCHW)
     *  After distribution, the input and output of a.m. will become Fragz */
    OpDescPtr l2_loss_op = std::make_shared<OpDesc>("l2loss", "L2Loss");
    GeTensorDesc l2_loss_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    l2_loss_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc l2_loss_out_tensor_desc(GeShape({1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    l2_loss_out_tensor_desc.SetOriginShape(GeShape({1}));
    l2_loss_out_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    l2_loss_op->AddInputDesc(l2_loss_tensor_desc);
    l2_loss_op->AddOutputDesc(l2_loss_out_tensor_desc);
    auto l2loss_node = graph->AddNode(l2_loss_op);
    ge::AttrUtils::SetInt(l2_loss_op, FE_IMPLY_TYPE, 6);


    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr mul_o_p = std::make_shared<OpDesc>("mul", "Mul");
    mul_o_p->AddInputDesc(am_tensor_desc);
    mul_o_p->AddInputDesc(l2_loss_out_tensor_desc);
    mul_o_p->AddOutputDesc(am_tensor_desc);
    auto mul_Node = graph->AddNode(mul_o_p);
    ge::AttrUtils::SetInt(mul_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr conv_back_o_p = std::make_shared<OpDesc>("conv2dback", "Conv2DBackpropInput");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_back_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_back_o_p->AddInputDesc(conv_tensor_desc);
    conv_back_o_p->AddOutputDesc(conv_tensor_desc_weight);
    auto conv_back_node = graph->AddNode(conv_back_o_p);
    ge::AttrUtils::SetInt(conv_back_o_p, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(l2loss_node->GetOutDataAnchor(0), mul_Node->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), mul_Node->GetInDataAnchor(0));
    GraphUtils::AddEdge(conv_back_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));
  }
  static void CreateFiveGraph(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)     a.m.3(NCHW)
     *                          |      /
     *                         a.m.2 (NCHW)
     *  After distribution, the input of a.m. will become NC1HWC0*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr data_op = std::make_shared<OpDesc>("data", "Data");
    GeTensorDesc data_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    data_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    data_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    data_op->AddOutputDesc(data_tensor_desc);
    data_op->AddInputDesc(data_tensor_desc);
    auto data_node = graph->AddNode(data_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);


    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am1", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op2 = std::make_shared<OpDesc>("am2", "ApplyMomentum");
    GeTensorDesc am2_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am2_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am2_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op2->AddOutputDesc(am2_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op2->AddInputDesc(am_tensor_desc);
    }
    auto am_node2 = graph->AddNode(apply_momentum_op2);
    ge::AttrUtils::SetInt(apply_momentum_op2, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op3 = std::make_shared<OpDesc>("am3", "ApplyMomentum");
    GeTensorDesc am3_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am3_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am3_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op3->AddOutputDesc(am3_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op3->AddInputDesc(am_tensor_desc);
    }
    auto am_node3 = graph->AddNode(apply_momentum_op3);
    ge::AttrUtils::SetInt(apply_momentum_op3, FE_IMPLY_TYPE, 6);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
  }
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;


  static void CreateSevenGraph(ComputeGraphPtr graph) {
    /*   Data         Const                a.m.4(NCHW)
     *   |     /                \            |
     *  conv (Fz)         a.m. (NCHW)     a.m.3(NCHW)
     *                          |      /
     *                         a.m.2 (NCHW)
     *                          |
     *                         a.m.5(NCHW)
     *  After distribution, the input of a.m. will become NC1HWC0*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr data_op = std::make_shared<OpDesc>("data", "Data");
    GeTensorDesc data_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    data_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    data_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    data_op->AddOutputDesc(data_tensor_desc);
    data_op->AddInputDesc(data_tensor_desc);
    auto data_node = graph->AddNode(data_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);


    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am1", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op2 = std::make_shared<OpDesc>("am2", "ApplyMomentum");
    apply_momentum_op2->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op2->AddInputDesc(am_tensor_desc);
    }
    auto am_node2 = graph->AddNode(apply_momentum_op2);
    ge::AttrUtils::SetInt(apply_momentum_op2, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op3 = std::make_shared<OpDesc>("am3", "ApplyMomentum");
    apply_momentum_op3->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op3->AddInputDesc(am_tensor_desc);
    }
    auto am_node3 = graph->AddNode(apply_momentum_op3);
    ge::AttrUtils::SetInt(apply_momentum_op3, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op4 = std::make_shared<OpDesc>("am4", "ApplyMomentum");
    apply_momentum_op4->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op4->AddInputDesc(am_tensor_desc);
    }
    auto am_node4 = graph->AddNode(apply_momentum_op4);
    ge::AttrUtils::SetInt(apply_momentum_op4, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op5 = std::make_shared<OpDesc>("am5", "ApplyMomentum");
    apply_momentum_op5->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op5->AddInputDesc(am_tensor_desc);
    }
    auto am_node5 = graph->AddNode(apply_momentum_op5);
    ge::AttrUtils::SetInt(apply_momentum_op5, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));

    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node3->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node5->GetInDataAnchor(0));
  }

  static void CreateElevenGraph(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,5,6(NCHW)
     *                          |       ////
     *                         a.m.2 (NCHW)
   *                        /   |   \
   *                    a.m.7  a.m.8 a.m.9
     *  After distribution, the input of a.m. will become NC1HWC0*/
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    GeTensorDesc const_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW,
                                   ge::DT_FLOAT16);
    const_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    const_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    const_op->AddOutputDesc(const_tensor_desc);
    const_op->AddInputDesc(const_tensor_desc);
    auto const_node = graph->AddNode(const_op);

    OpDescPtr data_op = std::make_shared<OpDesc>("data", "Data");
    GeTensorDesc data_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW,
                                  ge::DT_FLOAT16);
    data_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    data_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    data_op->AddOutputDesc(data_tensor_desc);
    data_op->AddInputDesc(data_tensor_desc);
    auto data_node = graph->AddNode(data_op);

    OpDescPtr conv_o_p = std::make_shared<OpDesc>("conv2d", "Conv2D");
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,
                                  ge::DT_FLOAT16);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);


    GeTensorDesc conv_tensor_desc_weight(GeShape({30, 1, 16, 16}),
                                         ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
    conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    conv_o_p->AddInputDesc(conv_tensor_desc);
    conv_o_p->AddInputDesc(conv_tensor_desc_weight);
    conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto conv_node = graph->AddNode(conv_o_p);
    ge::AttrUtils::SetInt(conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op = std::make_shared<OpDesc>("am1", "ApplyMomentum");
    GeTensorDesc am_tensor_desc(GeShape({3, 4, 5, 6}), ge::FORMAT_NCHW,
                                ge::DT_FLOAT16);
    am_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    am_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    apply_momentum_op->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op->AddInputDesc(am_tensor_desc);
    }
    auto am_node = graph->AddNode(apply_momentum_op);
    ge::AttrUtils::SetInt(apply_momentum_op, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op2 = std::make_shared<OpDesc>("am2", "ApplyMomentum");
    apply_momentum_op2->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op2->AddInputDesc(am_tensor_desc);
    }
    auto am_node2 = graph->AddNode(apply_momentum_op2);
    ge::AttrUtils::SetInt(apply_momentum_op2, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op3 = std::make_shared<OpDesc>("am3", "ApplyMomentum");
    apply_momentum_op3->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op3->AddInputDesc(am_tensor_desc);
    }
    auto am_node3 = graph->AddNode(apply_momentum_op3);
    ge::AttrUtils::SetInt(apply_momentum_op3, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op4 = std::make_shared<OpDesc>("am4", "ApplyMomentum");
    apply_momentum_op4->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op4->AddInputDesc(am_tensor_desc);
    }
    auto am_node4 = graph->AddNode(apply_momentum_op4);
    ge::AttrUtils::SetInt(am_tensor_desc, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op5 = std::make_shared<OpDesc>("am5", "ApplyMomentum");
    apply_momentum_op5->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op5->AddInputDesc(am_tensor_desc);
    }
    auto am_node5 = graph->AddNode(apply_momentum_op5);
    ge::AttrUtils::SetInt(apply_momentum_op5, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op6 = std::make_shared<OpDesc>("am6", "ApplyMomentum");
    apply_momentum_op6->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op6->AddInputDesc(am_tensor_desc);
    }
    auto am_node6 = graph->AddNode(apply_momentum_op6);
    ge::AttrUtils::SetInt(apply_momentum_op6, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op7 = std::make_shared<OpDesc>("am7", "ApplyMomentum");
    apply_momentum_op7->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op7->AddInputDesc(am_tensor_desc);
    }
    auto am_node7 = graph->AddNode(apply_momentum_op7);
    ge::AttrUtils::SetInt(apply_momentum_op7, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op8 = std::make_shared<OpDesc>("am8", "ApplyMomentum");
    apply_momentum_op8->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op8->AddInputDesc(am_tensor_desc);
    }
    auto am_node8 = graph->AddNode(apply_momentum_op8);
    ge::AttrUtils::SetInt(apply_momentum_op8, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op9 = std::make_shared<OpDesc>("am9", "ApplyMomentum");
    apply_momentum_op9->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op9->AddInputDesc(am_tensor_desc);
    }
    auto am_node9 = graph->AddNode(apply_momentum_op9);
    ge::AttrUtils::SetInt(apply_momentum_op9, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }
};

TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_01)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    if (node->GetType() == "ApplyMomentumA") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim = {3, 4, 5, 6};
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_02)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateFiveGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim = {30, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_03)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateSevenGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim = {30, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_04)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateElevenGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim = {30, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3" || node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am6") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_05)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateElevenGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim = {30, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3" || node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am6") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am7"|| node->GetName() == "am8"||node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_06)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithScalar(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetType() == "ApplyMomentum") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim_none = {};
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_none, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_06_1)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithScalar_1(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim_none = {};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetType() == "ApplyMomentum") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_none, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_06_2)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithScalar_2(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim_none = {};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetName() == "am") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_none, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_none, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}



TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_07)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithL2LossAndAddN(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> result_dim5_hd_l2_loss = {1,1,1,1,16};
  vector<int64_t> scalar = {1};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetName() == "am") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
    if (node->GetType() == "L2Loss") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(scalar, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
    if (node->GetType() == "AddN") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(scalar, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_08)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithL2LossAndAddN_1(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> result_dim5_hd_l2_loss = {1,1,1,1,16};
  vector<int64_t> scalar = {};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetName() == "am") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
    if (node->GetType() == "L2Loss") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(scalar, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
    if (node->GetType() == "AddN") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> dim_fz_add_n = {1,1,16,16};
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(scalar, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_09)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithL2LossAndMul(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> scalar = {1};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetName() == "am") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetType() == "L2Loss") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(scalar, opdesc->GetOutputDesc(0).GetShape().GetDims());

    }
    if (node->GetType() == "Mul") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(scalar, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_10)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithBiasAdd(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> scalar = {1};
  for(auto node : graph->GetDirectNode()) {
    if (node->GetName() == "am") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetType() == "BiasAddGrad") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_11)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithTransData(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> scalar = {1};
  for(auto node : graph->GetDirectNode()) {

    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "trans") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "BiasAddGrad") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution, heavy_format_distribution_12)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateThreeGraphWithTransData_2(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  vector<int64_t> result_dim = {3, 4, 5, 6};
  vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
  vector<int64_t> result_dim_fz = {30, 1, 16, 16};
  vector<int64_t> scalar = {1};
  for(auto node : graph->GetDirectNode()) {

    if (node->GetType() == "Conv2DBackpropInput") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "trans") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "BiasAddGrad") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
  }

  EXPECT_EQ(fe::SUCCESS, ret);
}