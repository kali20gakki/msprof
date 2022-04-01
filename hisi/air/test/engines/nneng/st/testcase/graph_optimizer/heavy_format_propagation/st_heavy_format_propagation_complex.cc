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
#include "../../../../graph_constructor/graph_builder_utils.h"
#include "ops_store/ops_kernel_manager.h"
using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;
using HeavyFormatPropagationPtr = std::shared_ptr<HeavyFormatPropagation>;
class STEST_fusion_engine_heavy_format_distribution_complex : public testing::Test {
 protected:

  void SetUp() {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
        std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(
        op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo heavy_op_info{
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

  void TearDown() {

  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
 protected:

  static void Create15NodesGraph(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,5,6(NCHW)  a.m.10(NCHW)
     *                          |         / / / /     \     /
     *                         a.m.2 (NCHW)          d.w.conv1(6d)
     *                        /   |   \                          \
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)    a.m.11(NCHW)
     *                                  \      /
     *                                d.w.conv2(6d)
     *                                    |
     *                                  a.m.13(NCHW) */
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr apply_momentum_op10 = std::make_shared<OpDesc>("am10", "ApplyMomentum");
    apply_momentum_op10->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op10->AddInputDesc(am_tensor_desc);
    }
    auto am_node10 = graph->AddNode(apply_momentum_op10);
    ge::AttrUtils::SetInt(apply_momentum_op10, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op11 = std::make_shared<OpDesc>("am11", "ApplyMomentum");
    apply_momentum_op11->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op11->AddInputDesc(am_tensor_desc);
    }
    auto am_node11 = graph->AddNode(apply_momentum_op11);
    ge::AttrUtils::SetInt(apply_momentum_op11, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node10->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0), am_node11->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }

  static void Create19NodesGraph(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,5,6(NCHW)  a.m.10(NCHW)
     *                          |         / / / /     \     /
     *                         a.m.2 (NCHW)          d.w.conv1(6d)
     *                        /   |   \                          \
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)    a.m.11(NCHW)
     *                                  \      /         a.m.17(Fz)
     *                                d.w.conv2(6d)      /
     *                                    |          a.m.16(Fz)
     *                                    |           /
     *                                    |       a.m.15(Fz)
     *                                    |        /
     *                                    |     a.m.14(Fz)
     *                                    |     /
     *                                  a.m.13(NCHW) */
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr apply_momentum_op10 = std::make_shared<OpDesc>("am10", "ApplyMomentum");
    apply_momentum_op10->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op10->AddInputDesc(am_tensor_desc);
    }
    auto am_node10 = graph->AddNode(apply_momentum_op10);
    ge::AttrUtils::SetInt(apply_momentum_op10, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op11 = std::make_shared<OpDesc>("am11", "ApplyMomentum");
    apply_momentum_op11->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op11->AddInputDesc(am_tensor_desc);
    }
    auto am_node11 = graph->AddNode(apply_momentum_op11);
    ge::AttrUtils::SetInt(apply_momentum_op11, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op14 = std::make_shared<OpDesc>("am14", "ApplyMomentum");
    apply_momentum_op14->AddOutputDesc(conv_tensor_desc_weight);
    apply_momentum_op14->AddInputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 1;  i < 5; i++) {
      apply_momentum_op14->AddInputDesc(am_tensor_desc);
    }
    auto am_node14 = graph->AddNode(apply_momentum_op14);
    ge::AttrUtils::SetInt(apply_momentum_op14, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op15 = std::make_shared<OpDesc>("am15", "ApplyMomentum");
    apply_momentum_op15->AddOutputDesc(conv_tensor_desc_weight);
    apply_momentum_op15->AddInputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 1;  i < 5; i++) {
      apply_momentum_op15->AddInputDesc(am_tensor_desc);
    }
    auto am_node15 = graph->AddNode(apply_momentum_op15);
    ge::AttrUtils::SetInt(apply_momentum_op15, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op16 = std::make_shared<OpDesc>("am16", "ApplyMomentum");
    apply_momentum_op16->AddOutputDesc(conv_tensor_desc_weight);
    apply_momentum_op16->AddInputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 1;  i < 5; i++) {
      apply_momentum_op16->AddInputDesc(am_tensor_desc);
    }
    auto am_node16 = graph->AddNode(apply_momentum_op16);
    ge::AttrUtils::SetInt(apply_momentum_op16, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op17 = std::make_shared<OpDesc>("am17", "ApplyMomentum");
    apply_momentum_op17->AddOutputDesc(conv_tensor_desc_weight);
    apply_momentum_op17->AddInputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 1;  i < 5; i++) {
      apply_momentum_op17->AddInputDesc(am_tensor_desc);
    }
    auto am_node17 = graph->AddNode(apply_momentum_op17);
    ge::AttrUtils::SetInt(apply_momentum_op17, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node10->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0), am_node11->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node14->GetOutDataAnchor(0), am_node13->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node15->GetOutDataAnchor(0), am_node14->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node16->GetOutDataAnchor(0), am_node15->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node17->GetOutDataAnchor(0), am_node16->GetInDataAnchor(0));
  }


  static void Create15NodesGraphWithInvalidOp(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,5,6(NCHW)  a.m.10(NCHW)
     *                          |         / / / /     \     /
     *                          B (NCHW)          d.w.conv1(6d)
     *                        /   |   \                          \
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)    a.m.11(NCHW)
     *                                  \      /
     *                                d.w.conv2(6d)
     *                                    |
     *                                  a.m.13(NCHW)
     *  After distribution, we will stop distribution when we reach op B */
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr apply_momentum_op2 = std::make_shared<OpDesc>("am2", "B");
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

    OpDescPtr apply_momentum_op10 = std::make_shared<OpDesc>("am10", "ApplyMomentum");
    apply_momentum_op10->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op10->AddInputDesc(am_tensor_desc);
    }
    auto am_node10 = graph->AddNode(apply_momentum_op10);
    ge::AttrUtils::SetInt(apply_momentum_op10, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op11 = std::make_shared<OpDesc>("am11", "ApplyMomentum");
    apply_momentum_op11->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op11->AddInputDesc(am_tensor_desc);
    }
    auto am_node11 = graph->AddNode(apply_momentum_op11);
    ge::AttrUtils::SetInt(apply_momentum_op11, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node10->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0), am_node11->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }

  static void Create15NodesGraphWithProtogeneticReshape(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,6(NCHW)         Transdata2(Fz)
     *                          |        ///                       |
     *                        a.m.2 (NCHW)---Transdata1(NCHW)--d.w.conv1(6d)
     *                        /   |   \            |               |
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)     Transdata3(Fz)
     *                                  \      /
     *                                d.w.conv2(6d)
     *                                    |
     *                                  a.m.13(NCHW)
     *  After distribution, we will stop distribution when we reach op Transdata*/
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr apply_momentum_op5 = std::make_shared<OpDesc>("reshape1", "Reshape");
    am_tensor_desc.SetOriginDataType(ge::DT_FLOAT16);
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

    OpDescPtr reshape2 = std::make_shared<OpDesc>("reshape2", "Reshape");
    conv_tensor_desc_weight.SetOriginDataType(ge::DT_FLOAT16);
    reshape2->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      reshape2->AddInputDesc(conv_tensor_desc_weight);
    }
    auto reshape2_node = graph->AddNode(reshape2);
    ge::AttrUtils::SetInt(reshape2, FE_IMPLY_TYPE, 6);

    OpDescPtr reshape3 = std::make_shared<OpDesc>("reshape3", "Reshape");
    conv_tensor_desc_weight.SetOriginDataType(ge::DT_FLOAT16);
    reshape3->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      reshape3->AddInputDesc(conv_tensor_desc_weight);
    }
    auto reshape3_node = graph->AddNode(reshape3);
    ge::AttrUtils::SetInt(reshape3, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node12->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reshape2_node->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0),
                        reshape3_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }

  static void Create15NodesGraphWithFeReshape(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,6(NCHW)         Transdata2(Fz)
     *                          |        ///                       |
     *                        a.m.2 (NCHW)---Transdata1(NCHW)--d.w.conv1(6d)
     *                        /   |   \            |               |
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)     Transdata3(Fz)
     *                                  \      /
     *                                d.w.conv2(6d)
     *                                    |
     *                                  a.m.13(NCHW)
     *  After distribution, we will stop distribution when we reach op Transdata*/
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr reshape1 = std::make_shared<OpDesc>("reshape1", "Reshape");
    reshape1->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      reshape1->AddInputDesc(am_tensor_desc);
    }
    auto reshape1_node = graph->AddNode(reshape1);
    ge::AttrUtils::SetInt(reshape1, FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetBool(reshape1, ge::ATTR_INSERTED_BY_GE, true);

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

    OpDescPtr reshape2 = std::make_shared<OpDesc>("reshape2", "Reshape");
    reshape2->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      reshape2->AddInputDesc(conv_tensor_desc_weight);
    }
    auto reshape2_node = graph->AddNode(reshape2);
    ge::AttrUtils::SetInt(reshape2, FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetBool(reshape2, ge::ATTR_INSERTED_BY_GE, true);

    OpDescPtr reshape3 = std::make_shared<OpDesc>("reshape3", "Reshape");
    reshape3->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      reshape3->AddInputDesc(conv_tensor_desc_weight);
    }
    auto reshape3_node = graph->AddNode(reshape3);
    ge::AttrUtils::SetInt(reshape3, FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetBool(reshape3, ge::ATTR_INSERTED_BY_GE, true);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(reshape1_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(reshape1_node->GetOutDataAnchor(0), am_node12->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(reshape1_node->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(reshape2_node->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0),
                        reshape3_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }

  static void Create15NodesGraphWithTranspose(ComputeGraphPtr graph) {
    /*   Data         Const
     *   |     /                \
     *  conv (Fz)         a.m. (NCHW)  a.m.3,4,6(NCHW)         Transdata2(Fz)
     *                          |        ///                       |
     *                        a.m.2 (NCHW)---Transdata1(NCHW)--d.w.conv1(6d)
     *                        /   |   \            |               |
     *                    a.m.7  a.m.8 a.m.9    a.m.12(NCHW)     Transdata3(Fz)
     *                                  \      /
     *                                d.w.conv2(6d)
     *                                    |
     *                                  a.m.13(NCHW)
     *  After distribution, we will stop distribution when we reach op Transdata*/
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
    GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
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

    OpDescPtr dp_conv_o_p = std::make_shared<OpDesc>("dp_conv2d", "DepthwiseConv2dNative");
    GeTensorDesc dp_conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0,ge::DT_FLOAT16);
    dp_conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
    GeTensorDesc dp_conv_tensor_desc_weight(GeShape({1, 5, 6, 1, 16, 16}), ge::FORMAT_C1HWNCoC0, ge::DT_FLOAT16);
    dp_conv_tensor_desc_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    dp_conv_tensor_desc_weight.SetOriginFormat(ge::FORMAT_NCHW);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node = graph->AddNode(dp_conv_o_p);
    ge::AttrUtils::SetInt(dp_conv_o_p, FE_IMPLY_TYPE, 6);

    OpDescPtr dp_conv_o_p2 = std::make_shared<OpDesc>("dp_conv2d_2", "DepthwiseConv2dNative");
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc);
    dp_conv_o_p2->AddInputDesc(dp_conv_tensor_desc_weight);
    dp_conv_o_p2->AddOutputDesc(conv_tensor_desc);
    auto dp_conv_node2 = graph->AddNode(dp_conv_o_p2);
    ge::AttrUtils::SetInt(dp_conv_o_p2, FE_IMPLY_TYPE, 6);

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

    OpDescPtr apply_momentum_op5 = std::make_shared<OpDesc>("transpose1", "Transpose");
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

    OpDescPtr apply_momentum_op10 = std::make_shared<OpDesc>("transpose2", "Transpose");
    apply_momentum_op10->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op10->AddInputDesc(conv_tensor_desc_weight);
    }
    auto am_node10 = graph->AddNode(apply_momentum_op10);
    ge::AttrUtils::SetInt(apply_momentum_op10, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op11 = std::make_shared<OpDesc>("transpose3", "Transpose");
    apply_momentum_op11->AddOutputDesc(conv_tensor_desc_weight);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op11->AddInputDesc(conv_tensor_desc_weight);
    }
    auto am_node11 = graph->AddNode(apply_momentum_op11);
    ge::AttrUtils::SetInt(apply_momentum_op11, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op12 = std::make_shared<OpDesc>("am12", "ApplyMomentum");
    apply_momentum_op12->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op12->AddInputDesc(am_tensor_desc);
    }
    auto am_node12 = graph->AddNode(apply_momentum_op12);
    ge::AttrUtils::SetInt(apply_momentum_op12, FE_IMPLY_TYPE, 6);

    OpDescPtr apply_momentum_op13 = std::make_shared<OpDesc>("am13", "ApplyMomentum");
    apply_momentum_op13->AddOutputDesc(am_tensor_desc);
    for (uint32_t i = 0;  i < 5; i++) {
      apply_momentum_op13->AddInputDesc(am_tensor_desc);
    }
    auto am_node13 = graph->AddNode(apply_momentum_op13);
    ge::AttrUtils::SetInt(apply_momentum_op13, FE_IMPLY_TYPE, 6);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), am_node->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node->GetOutDataAnchor(0), am_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node3->GetOutDataAnchor(0), am_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(am_node4->GetOutDataAnchor(0), am_node2->GetInDataAnchor(2));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node2->GetInDataAnchor(3));
    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), am_node12->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node6->GetOutDataAnchor(0), am_node2->GetInDataAnchor(4));

    GraphUtils::AddEdge(am_node5->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node10->GetOutDataAnchor(0), dp_conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node->GetOutDataAnchor(0), am_node11->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node9->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node12->GetOutDataAnchor(0), dp_conv_node2->GetInDataAnchor(1));
    GraphUtils::AddEdge(dp_conv_node2->GetOutDataAnchor(0), am_node13->GetInDataAnchor(0));

    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node7->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node8->GetInDataAnchor(0));
    GraphUtils::AddEdge(am_node2->GetOutDataAnchor(0), am_node9->GetInDataAnchor(0));
  }
};


TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_01)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create15NodesGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3" || node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4" || node->GetName() == "am6") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am7" || node->GetName() == "am8" || node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am10" ||node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am11" ||node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }


    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_02)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create19NodesGraph(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3" || node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
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
    if (node->GetName() == "am7" || node->GetName() == "am8") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am10" ||node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am11") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(1).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am14" ||node->GetName() == "am15" || node->GetName() == "am16") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am17") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}



TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_03)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create15NodesGraphWithInvalidOp(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3" || node->GetName() == "am6" || node->GetName() == "am4" ||
        node->GetName() == "am7" || node->GetName() == "am8") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am5") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am10" ||node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am11" ||node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }


    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_04)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create15NodesGraphWithFeReshape(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4" || node->GetName() == "am6" || node->GetName() == "transdata1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am7" || node->GetName() == "am8" || node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "transdata2" || node->GetName() == "transdata3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}


TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_05)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create15NodesGraphWithTranspose(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4" || node->GetName() == "am6" || node->GetName() == "transdata1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am7" || node->GetName() == "am8" || node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "transdata2" || node->GetName() == "transdata3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_heavy_format_distribution_complex, heavy_format_distribution_complex_06)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Create15NodesGraphWithProtogeneticReshape(graph);

  HeavyFormatPropagationPtr heavt_format_propagator = std::make_shared<HeavyFormatPropagation>(AI_CORE_NAME,
                                                                                               op_store_adapter_manager_ptr_,
                                                                                               reflection_builder_ptr_);
  heavt_format_propagator->Initialize();
  Status ret = heavt_format_propagator->PropagateHeavyFormat(*(graph.get()));
  for(auto node : graph->GetDirectNode()) {
    vector<int64_t> result_original_dim = {3, 4, 5, 6};
    vector<int64_t> result_dim_fz = {30, 1, 16, 16};
    vector<int64_t> result_dim5_d = {3, 1, 5, 6, 16};
    vector<int64_t> result_dim6_d = {1, 5, 6, 1, 16, 16};
    if (node->GetName() == "am1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am2") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(1).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(2).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(2).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(2).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(3).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(3).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(3).GetShape().GetDims());
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(4).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(4).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(4).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am4" || node->GetName() == "am6" || node->GetName() == "transdata1") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am7" || node->GetName() == "am8" || node->GetName() == "am9") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "am12") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim6_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetName() == "am13") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }

    if (node->GetName() == "transdata2" || node->GetName() == "transdata3") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetInputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(0).GetShape().GetDims());

      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim_fz, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Const") {
      auto opdesc = node->GetOpDesc();

      EXPECT_EQ(ge::FORMAT_NCHW, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_original_dim, opdesc->GetOutputDesc(0).GetShape().GetDims());
    }
    if (node->GetType() == "Conv2D") {
      auto opdesc = node->GetOpDesc();
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_FRACTAL_Z, opdesc->GetInputDesc(1).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim_fz, opdesc->GetInputDesc(1).GetShape().GetDims());
    }

    if (node->GetType() == "DepthwiseConv2dNative") {
      auto opdesc = node->GetOpDesc();
      vector<int64_t> result_dim5_h_d = {3, 1, 5, 6, 16};
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetOutputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_NC1HWC0, opdesc->GetInputDesc(0).GetFormat());
      EXPECT_EQ(ge::FORMAT_C1HWNCoC0, opdesc->GetInputDesc(1).GetFormat());

      EXPECT_EQ(ge::DT_FLOAT16, opdesc->GetOutputDesc(0).GetDataType());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetOutputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim5_h_d, opdesc->GetInputDesc(0).GetShape().GetDims());
      EXPECT_EQ(result_dim6_d, opdesc->GetInputDesc(1).GetShape().GetDims());
    }
  }
  EXPECT_EQ(fe::SUCCESS, ret);
}
