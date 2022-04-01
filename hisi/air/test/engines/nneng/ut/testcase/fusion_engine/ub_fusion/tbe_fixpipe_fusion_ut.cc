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

#include <stdio.h>
#include <iostream>
#include <map>
#include <memory>
#include "gtest/gtest.h"
#include "proto/om.pb.h"
#define protected public
#define private public
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "common/aicore_util_types.h"
#include "common/configuration.h"
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/fusion_pass_name.h"
#include "graph_optimizer/op_setter/op_setter.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_fixpipe_fusion_pass.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include "platform_info.h"
#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;
using OpSetterPtr = std::shared_ptr<OpSetter>;
class FIXPIPE_UB_UNITTEST : public testing::Test {
 public:
  using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

 protected:
  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }
  static void TearDownTestCase() { std::
    cout <<"UB fusion TearDown" << std::endl; }

  static void DumpTestGraph(ge::ComputeGraphPtr graph) {
    std::cout << "fixpipe_test : graph_name = " << graph->GetName() << std::endl;
    for (const auto &node : graph->GetAllNodes()) {
      std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType() << std::endl;
      for (const auto &anchor : node->GetAllOutDataAnchors()) {
        for (const auto &peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          if (peer_in_anchor != nullptr && peer_in_anchor->GetOwnerNode() != nullptr) {
            std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType()
                      << " outdatanode name = " << peer_in_anchor->GetOwnerNode()->GetName()
                      << " type = " << peer_in_anchor->GetOwnerNode()->GetType() << std::endl;
          }
        }
      }
      auto out_control_anchor = node->GetOutControlAnchor();
      if (out_control_anchor != nullptr) {
        for (const auto &peer_in_anchor : out_control_anchor->GetPeerInControlAnchors()) {
          if (peer_in_anchor != nullptr && peer_in_anchor->GetOwnerNode() != nullptr) {
            std::cout << "fixpipe_test : node name  = " << node->GetName() << " type = " << node->GetType()
                      << " outcontrolnode name = " << peer_in_anchor->GetOwnerNode()->GetName()
                      << " type = " << peer_in_anchor->GetOwnerNode()->GetType() << std::endl;
          }
        }
      }
    }
  }
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  void SetUp() {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = std::make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
    FEOpsStoreInfo tbe_custom {
            6, "tbe-custom", OpImplType::EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_slice_op_info/slice_success",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
    fe_ops_kernel_info_store_ptr_->Initialize(options);
  }

  virtual void TearDown() {}
  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }

  void SetTvmType(ge::OpDescPtr opdef) {
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
  }

  void BuildGraph(ComputeGraphPtr &graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr data3 = std::make_shared<OpDesc>("DATA3", fe::DATA);
    OpDescPtr data4 = std::make_shared<OpDesc>("DATA4", fe::DATA);
    OpDescPtr data5 = std::make_shared<OpDesc>("DATA5", fe::DATA);
    OpDescPtr data6 = std::make_shared<OpDesc>("DATA6", fe::DATA);
    OpDescPtr data7 = std::make_shared<OpDesc>("DATA7", fe::DATA);
    OpDescPtr data8 = std::make_shared<OpDesc>("DATA8", fe::DATA);
    OpDescPtr data9 = std::make_shared<OpDesc>("DATA9", fe::DATA);

    OpDescPtr transdata1 = std::make_shared<OpDesc>("transdata1", "TransData");
    OpDescPtr transdata2 = std::make_shared<OpDesc>("transdata2", "TransData");
    OpDescPtr conv = std::make_shared<OpDesc>("conv", "Conv2D");
    OpDescPtr fixpipeops1 = std::make_shared<OpDesc>("fixpipe", "Fixpipe");
    OpDescPtr out = std::make_shared<OpDesc>("out", "NetOutput");

    SetPattern(conv, "Convolution");
    SetPattern(fixpipeops1, "fixpipe");
    SetTvmType(transdata1);
    SetTvmType(transdata2);
    SetTvmType(conv);
    SetTvmType(fixpipeops1);
    ge::AttrUtils::SetInt(transdata1, FE_IMPLY_TYPE, static_cast<int>(OpImplType::EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(transdata2, FE_IMPLY_TYPE, static_cast<int>(OpImplType::EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, static_cast<int>(OpImplType::EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(fixpipeops1, FE_IMPLY_TYPE, static_cast<int>(OpImplType::EN_IMPL_HW_TBE));

    ge::GeTensorDesc trans1_inputdesc(GeShape({3, 4, 5, 6}), ge::FORMAT_NHWC, ge::DT_FLOAT);
    trans1_inputdesc.SetOriginShape(GeShape({3, 4, 5, 6}));
    trans1_inputdesc.SetOriginFormat(ge::FORMAT_NHWC);
    ge::GeTensorDesc trans2_outputdesc(GeShape({3, 4, 5, 6, }), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT);
    trans2_outputdesc.SetOriginShape(GeShape({3, 4, 5, 6}));
    trans2_outputdesc.SetOriginFormat(ge::FORMAT_NHWC);
    ge::GeTensorDesc conv_tensor_desc(GeShape({3, 1, 5, 6, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
    conv_tensor_desc.SetOriginShape(GeShape({3, 4, 5, 6}));
    conv_tensor_desc.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data1_weight(GeShape({30, 1, 16, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    data1_weight.SetOriginShape(GeShape({3, 4, 5, 6}));
    data1_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data2_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_UINT64);
    data2_weight.SetOriginShape(GeShape());
    data2_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data3_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
    data3_weight.SetOriginShape(GeShape());
    data3_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data4_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    data4_weight.SetOriginShape(GeShape());
    data4_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data5_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_UINT64);
    data5_weight.SetOriginShape(GeShape());
    data5_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data6_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
    data6_weight.SetOriginShape(GeShape());
    data6_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data7_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    data7_weight.SetOriginShape(GeShape());
    data7_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data8_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    data8_weight.SetOriginShape(GeShape());
    data8_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    ge::GeTensorDesc data9_weight(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_INT8);
    data9_weight.SetOriginShape(GeShape());
    data9_weight.SetOriginFormat(ge::FORMAT_NC1HWC0);

    data->AddOutputDesc(conv_tensor_desc);
    data1->AddOutputDesc(data1_weight);
    data2->AddOutputDesc(data2_weight);
    data3->AddOutputDesc(data3_weight);
    data4->AddOutputDesc(data4_weight);
    data5->AddOutputDesc(data5_weight);
    data6->AddOutputDesc(data6_weight);
    data7->AddOutputDesc(data7_weight);
    data8->AddOutputDesc(data8_weight);
    data9->AddOutputDesc(data9_weight);

    transdata1->AddInputDesc(trans1_inputdesc);
    transdata1->AddOutputDesc(conv_tensor_desc);
    transdata2->AddInputDesc(trans1_inputdesc);
    transdata2->AddOutputDesc(trans2_outputdesc);
    conv->AddInputDesc(conv_tensor_desc);
    conv->AddInputDesc(trans2_outputdesc);
    conv->AddOutputDesc(conv_tensor_desc);

    fixpipeops1->AddInputDesc("x1", conv_tensor_desc);
    fixpipeops1->AddInputDesc("x2", data1_weight);
    fixpipeops1->AddInputDesc("quant_scale_0", data2_weight);
    fixpipeops1->AddInputDesc("relu_weight_0", data3_weight);
    fixpipeops1->AddInputDesc("clip_value_0", data4_weight);
    fixpipeops1->AddInputDesc("quant_scale_1", data5_weight);
    fixpipeops1->AddInputDesc("relu_weight_1", data6_weight);
    fixpipeops1->AddInputDesc("clip_value_1", data7_weight);
    fixpipeops1->AddInputDesc("anti_quant_scale", data8_weight);
    fixpipeops1->AddInputDesc("anti_quant_offset", data9_weight);
    fixpipeops1->AddOutputDesc("y", conv_tensor_desc);

    out->AddInputDesc(conv_tensor_desc);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data_node1 = graph->AddNode(data1);
    NodePtr data_node2 = graph->AddNode(data2);
    NodePtr data_node3 = graph->AddNode(data3);
    NodePtr data_node4 = graph->AddNode(data4);
    NodePtr data_node5 = graph->AddNode(data5);
    NodePtr data_node6 = graph->AddNode(data6);
    NodePtr data_node7 = graph->AddNode(data7);
    NodePtr data_node8 = graph->AddNode(data8);
    NodePtr data_node9 = graph->AddNode(data9);
    NodePtr transdata1_node = graph->AddNode(transdata1);
    NodePtr transdata2_node = graph->AddNode(transdata2);
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr fixpipenode1 = graph->AddNode(fixpipeops1);
    NodePtr outnode = graph->AddNode(out);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
            transdata1_node->GetName(), std::move(buffer));
    transdata1_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),  transdata1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0),  transdata2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(transdata1_node->GetOutDataAnchor(0),  conv_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(transdata2_node->GetOutDataAnchor(0),  conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),  fixpipenode1->GetInDataAnchor(0));
    GraphUtils::AddEdge(data_node1->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(1));
    GraphUtils::AddEdge(data_node2->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(2));
    GraphUtils::AddEdge(data_node3->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(3));
    GraphUtils::AddEdge(data_node4->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(4));
    GraphUtils::AddEdge(data_node5->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(5));
    GraphUtils::AddEdge(data_node6->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(6));
    GraphUtils::AddEdge(data_node7->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(7));
    GraphUtils::AddEdge(data_node8->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(8));
    GraphUtils::AddEdge(data_node9->GetOutDataAnchor(0), fixpipenode1->GetInDataAnchor(9));
    GraphUtils::AddEdge(fixpipenode1->GetOutDataAnchor(0), outnode->GetInDataAnchor(0));
  }
};


TEST_F(FIXPIPE_UB_UNITTEST, pass_case1) {
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend320";
  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  uint32_t init_ret = PlatformInfoManager::Instance().LoadConfigFile(real_path);
  EXPECT_EQ(init_ret, ge::GRAPH_SUCCESS);

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph(graph_out);
  graph_out->TopologicalSorting();
  graph_out->Dump();
  // BufferFusion(graph, graph_out);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(),);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  std::shared_ptr<PassManager> tbe_ub_fusion_pass =
          std::make_shared<PassManager>(fusion_priority_mgr_ptr->GetFusionConfigParserPtr());

  std::cout << "FIXPIPE_UB_ST UB fusion before" << std::endl;
  DumpTestGraph(graph_out);

  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  BufferFusionPassRunner *fixpipeubfusion = new(std::nothrow) BufferFusionPassRunner(
      "TbeFixpipeFusionPass", []() -> BufferFusionPassBase * { return new(std::nothrow) TbeFixPipeFusionPass(); },
      cycle_detector);
  tbe_ub_fusion_pass->AddPass("TbeFixpipeFusionPass", AI_CORE_NAME, fixpipeubfusion, UB_FUSION);
  tbe_ub_fusion_pass->Run(*graph_out);
  std::cout << "fixpipe_test : after fusion " << std::endl;
  DumpTestGraph(graph_out);
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph_out);
  int find = 0;
  std::cout << "fixpipe_test : ub fusion result" << std::endl;
  DumpTestGraph(graph_out);
  for (auto &node : graph_out->GetDirectNode()) {
    if (node->GetType() == "TransData" && node->GetName() == "transdata1transdata2convfixpipe") {
      find = 1;
      break;
    }
  }
  EXPECT_EQ(find, 1);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend310";
}
